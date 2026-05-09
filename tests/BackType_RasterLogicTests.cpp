#include "../src/BackType_RasterLogic.h"
#include "../src/BackType_TextLogic.h"
#include "../src/TextRenderer.h"

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

namespace {

void fail(const std::string &message) {
    std::cerr << "FAIL: " << message << "\n";
    std::exit(1);
}

void expect_true(bool condition, const std::string &message) {
    if (!condition) {
        fail(message);
    }
}

void expect_eq(std::size_t actual, std::size_t expected, const std::string &message) {
    if (actual != expected) {
        fail(message + " expected " + std::to_string(expected) + " got " + std::to_string(actual));
    }
}

void expect_near(double actual, double expected, double tolerance, const std::string &message) {
    if (std::fabs(actual - expected) > tolerance) {
        fail(message + " expected " + std::to_string(expected) + " got " + std::to_string(actual));
    }
}

void set_pixel(std::vector<std::uint8_t> &pixels, int rowbytes, int x, int y,
               std::uint8_t a, std::uint8_t r, std::uint8_t g, std::uint8_t b) {
    auto *pixel = pixels.data() + y * rowbytes + x * 4;
    pixel[0] = a;
    pixel[1] = r;
    pixel[2] = g;
    pixel[3] = b;
}

} // namespace

int main() {
    using namespace backtype;

    const RasterRevealState half = compute_raster_reveal("hello", 50.0, RevealMode::Character, 0.0);
    expect_eq(half.visible_bytes, 2, "character reveal uses floored visible character count");
    expect_true(half.visible_fraction > 0.4 && half.visible_fraction < 0.55,
                "visible fraction tracks approximate character advances");
    expect_near(half.newest_opacity, 1.0, 0.0001, "fade disabled keeps newest character opaque");

    const RasterRevealState spaced = compute_raster_reveal("hi, you", 50.0, RevealMode::Character, 100.0);
    expect_eq(spaced.visible_bytes, 3, "spaces and punctuation count deterministically");
    expect_true(spaced.previous_fraction < spaced.visible_fraction, "newest character has its own reveal span");
    expect_true(spaced.newest_opacity >= 0.0 && spaced.newest_opacity <= 1.0,
                "newest opacity is clamped");

    expect_near(push_easing_multiplier(0.5, PushEasing::Linear), 0.5, 0.0001, "linear easing is unchanged");
    expect_true(push_easing_multiplier(0.5, PushEasing::EaseOut) > 0.5, "ease out advances push earlier");
    expect_near(push_easing_multiplier(1.0, PushEasing::EaseInOut), 1.0, 0.0001, "easing ends at one");

    constexpr int width = 12;
    constexpr int height = 8;
    constexpr int rowbytes = width * 4;
    std::vector<std::uint8_t> pixels(static_cast<std::size_t>(height * rowbytes), 0);
    set_pixel(pixels, rowbytes, 3, 2, 255, 200, 210, 220);
    set_pixel(pixels, rowbytes, 8, 5, 128, 100, 110, 120);

    PixelBuffer source{pixels.data(), width, height, rowbytes, PixelFormat::Argb8};
    const RasterBounds bounds = find_alpha_bounds(source);
    expect_true(bounds.found, "alpha bounds should find source text pixels");
    expect_eq(static_cast<std::size_t>(bounds.min_x), 3, "bounds min x");
    expect_eq(static_cast<std::size_t>(bounds.max_x), 8, "bounds max x");
    expect_eq(static_cast<std::size_t>(bounds.min_y), 2, "bounds min y");
    expect_eq(static_cast<std::size_t>(bounds.max_y), 5, "bounds max y");

    const Color average = average_alpha_color(source, bounds);
    expect_true(average.r > 0.0 && average.g > 0.0 && average.b > 0.0, "average text color is sampled");

    std::vector<std::uint8_t> out_pixels(static_cast<std::size_t>(height * rowbytes), 0);
    PixelBuffer target{out_pixels.data(), width, height, rowbytes, PixelFormat::Argb8};
    copy_revealed_raster(source,
                         target,
                         bounds,
                         {2.0, 1.0},
                         half,
                         1.0);
    const RasterBounds copied = find_alpha_bounds(target);
    expect_true(copied.found, "revealed raster copy should write pixels");
    expect_true(copied.min_y == 1 || copied.min_y == 4,
                "copy keeps vertical placement independent of visible glyph bounds");

    draw_cursor(target,
                {CursorStyle::Line,
                 {6.0, 1.0},
                 10.0,
                 4.0,
                 average,
                 1.0});
    const RasterBounds with_cursor = find_alpha_bounds(target);
    expect_true(with_cursor.max_x >= copied.max_x, "cursor draws at the active x position");

    const std::string descender_cases[] = {
        "hello",
        "yoyo",
        "gpgpgp",
        "jumps quickly",
        "WHY gym?",
        "pqygj"};
    for (const auto &sample : descender_cases) {
        double reference_y = 0.0;
        bool have_reference = false;
        for (double progress = 10.0; progress <= 100.0; progress += 10.0) {
            const RasterRevealState state = compute_raster_reveal(sample, progress, RevealMode::Character, 0.0);
            const DrawPosition position = compute_draw_position(
                {AnchorMode::CenterLocked, Direction::MoveLeft, 100.0, 80.0, 100.0},
                {200.0 * state.visible_fraction, 40.0},
                {200.0, 40.0});
            if (!have_reference) {
                reference_y = position.y;
                have_reference = true;
            } else {
                expect_near(position.y, reference_y, 0.0001,
                            "descender strings keep a stable vertical draw position: " + sample);
            }
        }
    }

    std::cout << "BackType_RasterLogicTests passed\n";
    return 0;
}
