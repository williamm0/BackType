#include "../src/BackType.h"
#include "../src/BackType_Enums.h"
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

struct AlphaBounds {
    bool found = false;
    int min_x = 0;
    int min_y = 0;
    int max_x = 0;
    int max_y = 0;
};

AlphaBounds find_alpha_bounds(const std::vector<std::uint8_t> &pixels, int width, int height, int rowbytes) {
    AlphaBounds bounds;

    for (int y = 0; y < height; ++y) {
        const auto *row = pixels.data() + y * rowbytes;
        for (int x = 0; x < width; ++x) {
            if (row[x * 4] == 0) {
                continue;
            }

            if (!bounds.found) {
                bounds = {true, x, y, x, y};
            } else {
                bounds.min_x = std::min(bounds.min_x, x);
                bounds.min_y = std::min(bounds.min_y, y);
                bounds.max_x = std::max(bounds.max_x, x);
                bounds.max_y = std::max(bounds.max_y, y);
            }
        }
    }

    return bounds;
}

} // namespace

int main() {
    constexpr int kWidth = 640;
    constexpr int kHeight = 360;
    constexpr int kRowbytes = kWidth * 4;
    constexpr double kFontSize = 72.0;
    constexpr double kAnchorX = kWidth * 0.5;
    constexpr double kAnchorY = kHeight * 0.5;

    std::vector<std::uint8_t> pixels(static_cast<std::size_t>(kHeight * kRowbytes), 0);
    backtype::PixelBuffer target{pixels.data(), kWidth, kHeight, kRowbytes, backtype::PixelFormat::Argb8};
    backtype::clear_target(target);

    const std::string text = BACKTYPE_DEFAULT_TEXT;
    const backtype::TextMetrics visible_metrics = backtype::measure_text(text, kFontSize);
    expect_true(visible_metrics.width > 0.0, "text metrics width should be positive");
    expect_true(visible_metrics.height > 0.0, "text metrics height should be positive");

    const backtype::DrawPosition draw_position = backtype::compute_draw_position(
        {backtype::AnchorMode::NewestCharacterLocked,
         backtype::Direction::MoveLeft,
         kAnchorX,
         kAnchorY,
         100.0},
        {visible_metrics.width, visible_metrics.height},
        {visible_metrics.width, visible_metrics.height});

    const bool rendered = backtype::render_text(target,
                                                {text,
                                                 draw_position.x,
                                                 draw_position.y,
                                                 kFontSize,
                                                 {1.0, 1.0, 1.0, 1.0},
                                                 1.0});
    expect_true(rendered, "text render call should succeed");

    const AlphaBounds alpha_bounds = find_alpha_bounds(pixels, kWidth, kHeight, kRowbytes);
    expect_true(alpha_bounds.found, "render should produce visible alpha");
    expect_true(alpha_bounds.min_x > 0, "text should not clip the left edge");
    expect_true(alpha_bounds.max_x < kWidth - 1, "text should not clip the right edge");
    expect_true(alpha_bounds.min_y > 0, "text should not clip the top edge");
    expect_true(alpha_bounds.max_y < kHeight - 1, "text should not clip the bottom edge");

    const double rendered_center_y = (alpha_bounds.min_y + alpha_bounds.max_y) * 0.5;
    expect_true(std::fabs(rendered_center_y - kAnchorY) < 24.0,
                "text should stay near the requested vertical center"
                " (minY=" + std::to_string(alpha_bounds.min_y) +
                ", maxY=" + std::to_string(alpha_bounds.max_y) +
                ", centerY=" + std::to_string(rendered_center_y) +
                ", anchorY=" + std::to_string(kAnchorY) +
                ", drawY=" + std::to_string(draw_position.y) +
                ", metricsHeight=" + std::to_string(visible_metrics.height) +
                ", ascent=" + std::to_string(visible_metrics.ascent) +
                ", descent=" + std::to_string(visible_metrics.descent) + ")");
    expect_true(std::fabs(alpha_bounds.max_x - kAnchorX) < 48.0,
                "newest character lock should keep the right edge near the X anchor"
                " (minX=" + std::to_string(alpha_bounds.min_x) +
                ", maxX=" + std::to_string(alpha_bounds.max_x) +
                ", anchorX=" + std::to_string(kAnchorX) +
                ", drawX=" + std::to_string(draw_position.x) +
                ", metricsWidth=" + std::to_string(visible_metrics.width) + ")");

    std::cout << "BackType_RenderSmoke passed\n";
    return 0;
}
