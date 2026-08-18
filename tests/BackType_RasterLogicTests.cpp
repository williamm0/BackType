#include "BackType_RasterLogic.h"

#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <string>
#include <thread>
#include <vector>

namespace {

void fail(const std::string &message) {
    std::cerr << "FAIL: " << message << '\n';
    std::exit(1);
}

void expect_true(bool condition, const std::string &message) {
    if (!condition) {
        fail(message);
    }
}

void expect_near(double actual, double expected, double tolerance, const std::string &message) {
    if (std::fabs(actual - expected) > tolerance) {
        fail(message + " expected " + std::to_string(expected) + " got " + std::to_string(actual));
    }
}

std::size_t byte_offset(int rowbytes, int x, int y, std::size_t bytes_per_pixel) {
    return static_cast<std::size_t>(y * rowbytes) + static_cast<std::size_t>(x) * bytes_per_pixel;
}

void set_argb8(std::vector<std::uint8_t> &pixels,
               int rowbytes,
               int x,
               int y,
               std::uint8_t a,
               std::uint8_t r,
               std::uint8_t g,
               std::uint8_t b) {
    const std::size_t offset = byte_offset(rowbytes, x, y, 4U);
    pixels[offset] = a;
    pixels[offset + 1U] = r;
    pixels[offset + 2U] = g;
    pixels[offset + 3U] = b;
}

std::uint8_t argb8_channel(const std::vector<std::uint8_t> &pixels,
                           int rowbytes,
                           int x,
                           int y,
                           int channel) {
    return pixels[byte_offset(rowbytes, x, y, 4U) + static_cast<std::size_t>(channel)];
}

void set_argb16(std::vector<std::uint16_t> &pixels,
                int width,
                int x,
                int y,
                std::uint16_t a,
                std::uint16_t r,
                std::uint16_t g,
                std::uint16_t b) {
    const std::size_t offset = static_cast<std::size_t>((y * width + x) * 4);
    pixels[offset] = a;
    pixels[offset + 1U] = r;
    pixels[offset + 2U] = g;
    pixels[offset + 3U] = b;
}

} // namespace

int main() {
    using namespace backtype;

    constexpr int width = 18;
    constexpr int height = 10;
    constexpr int rowbytes = width * 4;
    std::vector<std::uint8_t> source_pixels(static_cast<std::size_t>(height * rowbytes), 0U);

    for (int y = 2; y <= 6; ++y) {
        for (int x = 2; x <= 4; ++x) {
            set_argb8(source_pixels, rowbytes, x, y, 255U, 200U, 100U, 50U);
        }
        for (int x = 7; x <= 9; ++x) {
            set_argb8(source_pixels, rowbytes, x, y, 255U, 100U, 200U, 50U);
        }
        for (int x = 13; x <= 15; ++x) {
            set_argb8(source_pixels, rowbytes, x, y, 255U, 50U, 100U, 200U);
        }
    }
    set_argb8(source_pixels, rowbytes, 2, 2, 128U, 100U, 50U, 25U);

    PixelBuffer source{source_pixels.data(), width, height, rowbytes, PixelFormat::Argb8};
    const RasterBounds bounds = find_alpha_bounds(source);
    expect_true(bounds.found && bounds.min_x == 2 && bounds.max_x == 15 &&
                    bounds.min_y == 2 && bounds.max_y == 6,
                "alpha bounds include every antialiased edge pixel");

    const RasterRevealState half = compute_raster_reveal(
        source, bounds, 50.0, RevealMode::Character, Direction::MoveLeft, 0.0);
    expect_true(half.visible_fraction > 0.2 && half.visible_fraction < 0.3,
                "character reveal snaps to a complete glyph run");

    std::vector<std::uint8_t> left_pixels(static_cast<std::size_t>(height * rowbytes), 0U);
    PixelBuffer left_target{left_pixels.data(), width, height, rowbytes, PixelFormat::Argb8};
    copy_revealed_raster(source, left_target, bounds, {2.0, 2.0}, half, Direction::MoveLeft, 1.0);
    expect_true(argb8_channel(left_pixels, rowbytes, 4, 4, 0) == 255U,
                "the first glyph is copied through its final column");
    expect_true(argb8_channel(left_pixels, rowbytes, 7, 4, 0) == 0U,
                "the next glyph is not cut in half or revealed early");

    const RasterRevealState right_half = compute_raster_reveal(
        source, bounds, 50.0, RevealMode::Character, Direction::MoveRight, 0.0);
    std::vector<std::uint8_t> right_pixels(static_cast<std::size_t>(height * rowbytes), 0U);
    PixelBuffer right_target{right_pixels.data(), width, height, rowbytes, PixelFormat::Argb8};
    copy_revealed_raster(source, right_target, bounds, {2.0, 2.0}, right_half, Direction::MoveRight, 1.0);
    expect_true(argb8_channel(right_pixels, rowbytes, 4, 4, 0) == 255U,
                "reverse reveal places the complete rightmost glyph at the requested draw position");
    expect_true(argb8_channel(right_pixels, rowbytes, 7, 4, 0) == 0U,
                "reverse reveal keeps the preceding glyph hidden");

    const RasterRevealState first_word = compute_raster_reveal(
        source, bounds, 1.0, RevealMode::Word, Direction::MoveLeft, 0.0);
    expect_true(first_word.visible_fraction > half.visible_fraction,
                "word mode groups nearby glyph runs and reveals the first word immediately");
    expect_true(first_word.visible_fraction < 1.0, "word mode recognizes a larger inter-word gap");

    const RasterRevealState fading = compute_raster_reveal(
        source, bounds, 45.0, RevealMode::Character, Direction::MoveLeft, 100.0);
    expect_true(fading.newest_opacity > 0.0 && fading.newest_opacity < 1.0,
                "character fade produces a bounded partial opacity");

    const Color average = average_alpha_color(source, bounds);
    expect_true(average.r > 0.0 && average.g > 0.0 && average.b > 0.0,
                "average color unpremultiplies source samples");

    std::vector<std::uint8_t> overlap_pixels(static_cast<std::size_t>(height * rowbytes), 0U);
    set_argb8(overlap_pixels, rowbytes, 5, 3, 128U, 128U, 0U, 0U);
    PixelBuffer overlap_target{overlap_pixels.data(), width, height, rowbytes, PixelFormat::Argb8};
    draw_cursor(overlap_target,
                {CursorStyle::Block, Direction::MoveLeft, {5.0, 3.0}, 12.0, 4.0, 1.0,
                 Color{0.0, 0.0, 1.0, 1.0}, 1.0});
    expect_true(argb8_channel(overlap_pixels, rowbytes, 5, 3, 0) > 180U,
                "cursor overlap increases premultiplied alpha");
    expect_true(argb8_channel(overlap_pixels, rowbytes, 5, 3, 1) < 80U &&
                    argb8_channel(overlap_pixels, rowbytes, 5, 3, 3) > 120U,
                "premultiplied source-over blending preserves correct edge colors");

    constexpr int deep_width = 4;
    constexpr int deep_height = 3;
    constexpr int deep_rowbytes = deep_width * 8;
    std::vector<std::uint16_t> deep_source_pixels(
        static_cast<std::size_t>(deep_width * deep_height * 4), 0U);
    set_argb16(deep_source_pixels, deep_width, 1, 1, 32768U, 32768U, 16384U, 8192U);
    PixelBuffer deep_source{deep_source_pixels.data(), deep_width, deep_height, deep_rowbytes, PixelFormat::Argb16};
    const RasterBounds deep_bounds = find_alpha_bounds(deep_source);
    expect_true(deep_bounds.found && deep_bounds.min_x == 1 && deep_bounds.min_y == 1,
                "16-bpc alpha bounds use AE's 32768 channel maximum");
    const RasterRevealState deep_full = compute_raster_reveal(
        deep_source, deep_bounds, 100.0, RevealMode::Character, Direction::MoveLeft, 0.0);
    std::vector<std::uint16_t> deep_target_pixels(
        static_cast<std::size_t>(deep_width * deep_height * 4), 0U);
    PixelBuffer deep_target{deep_target_pixels.data(), deep_width, deep_height, deep_rowbytes, PixelFormat::Argb16};
    copy_revealed_raster(deep_source, deep_target, deep_bounds, {1.0, 1.0}, deep_full,
                         Direction::MoveLeft, 1.0);
    const std::size_t deep_offset = static_cast<std::size_t>((1 * deep_width + 1) * 4);
    expect_true(deep_target_pixels[deep_offset] == 32768U &&
                    deep_target_pixels[deep_offset + 2U] == 16384U,
                "16-bpc copy preserves full and fractional channel values");

    const DrawPosition right_cursor = cursor_position_for_reveal(
        {10.0, 20.0}, bounds, right_half, Direction::MoveRight, 3.0);
    expect_near(right_cursor.x, 7.0, 0.001, "right-to-left cursor follows the active reveal edge");
    const DrawPosition down_cursor = cursor_position_for_reveal(
        {10.0, 20.0}, bounds, right_half, Direction::MoveDown, 3.0);
    expect_near(down_cursor.y, 17.0, 0.001, "vertical cursor follows the active Y edge");

    constexpr int thread_count = 8;
    std::vector<std::vector<std::uint8_t>> threaded_outputs(
        thread_count, std::vector<std::uint8_t>(static_cast<std::size_t>(height * rowbytes), 0U));
    std::vector<std::thread> threads;
    threads.reserve(thread_count);
    for (int thread_index = 0; thread_index < thread_count; ++thread_index) {
        threads.emplace_back([&, thread_index] {
            PixelBuffer output{threaded_outputs[static_cast<std::size_t>(thread_index)].data(),
                               width,
                               height,
                               rowbytes,
                               PixelFormat::Argb8};
            for (int iteration = 0; iteration < 100; ++iteration) {
                clear_target(output);
                const RasterRevealState state = compute_raster_reveal(
                    source, bounds, 67.0, RevealMode::Character, Direction::MoveLeft, 35.0);
                copy_revealed_raster(source, output, bounds, {2.0, 2.0}, state,
                                     Direction::MoveLeft, 0.8);
            }
        });
    }
    for (std::thread &thread : threads) {
        thread.join();
    }
    for (int thread_index = 1; thread_index < thread_count; ++thread_index) {
        expect_true(threaded_outputs[static_cast<std::size_t>(thread_index)] == threaded_outputs.front(),
                    "parallel frames produce deterministic isolated output");
    }

    std::cout << "BackType_RasterLogicTests passed\n";
    return 0;
}
