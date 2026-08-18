#include "BackType_TextLogic.h"

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>

namespace {

void expect_true(bool condition, const char *message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(1);
    }
}

void expect_near(double actual, double expected, double tolerance, const char *message) {
    if (std::fabs(actual - expected) > tolerance) {
        std::cerr << "FAIL: " << message << " expected " << expected << " got " << actual << '\n';
        std::exit(1);
    }
}

} // namespace

int main() {
    using namespace backtype;

    expect_near(clamp_progress(-10.0), 0.0, 0.0, "progress clamps below zero");
    expect_near(clamp_progress(140.0), 100.0, 0.0, "progress clamps above 100");
    expect_near(clamp_progress(std::numeric_limits<double>::quiet_NaN()), 0.0, 0.0,
                "non-finite progress is safe");
    expect_near(clamp_percent(std::numeric_limits<double>::infinity()), 0.0, 0.0,
                "non-finite percentages are safe");

    const TextBounds visible_bounds{120.0, 20.0};
    const TextBounds full_bounds{300.0, 40.0};
    const LayoutInput default_layout{};
    expect_true(default_layout.anchor_mode == AnchorMode::CenterLocked, "default anchor is center locked");

    const DrawPosition newest_left = compute_draw_position(
        {AnchorMode::NewestCharacterLocked, Direction::MoveLeft, 200.0, 100.0, 100.0},
        visible_bounds,
        full_bounds);
    expect_near(newest_left.x, 80.0, 0.001, "newest locked move-left shifts by visible width");
    expect_near(newest_left.y, 80.0, 0.001, "horizontal layouts keep a stable vertical center");

    const DrawPosition center_left = compute_draw_position(
        {AnchorMode::CenterLocked, Direction::MoveLeft, 200.0, 100.0, 100.0},
        visible_bounds,
        full_bounds);
    expect_near(center_left.x, 140.0, 0.001, "horizontal center lock uses visible width");
    expect_near(center_left.y, 80.0, 0.001, "horizontal center lock uses full line height");

    const DrawPosition center_up = compute_draw_position(
        {AnchorMode::CenterLocked, Direction::MoveUp, 200.0, 100.0, 100.0},
        visible_bounds,
        full_bounds);
    expect_near(center_up.x, 50.0, 0.001, "vertical center lock centers the full text width");
    expect_near(center_up.y, 90.0, 0.001, "vertical center lock centers the visible height");

    const DrawPosition first_right = compute_draw_position(
        {AnchorMode::FirstCharacterLocked, Direction::MoveRight, 200.0, 100.0, 300.0},
        visible_bounds,
        full_bounds);
    expect_near(first_right.x, 200.0, 0.001, "first-character lock ignores backward amount");

    const DrawPosition last_left = compute_draw_position(
        {AnchorMode::LastCharacterLocked, Direction::MoveLeft, 200.0, 100.0, 100.0},
        visible_bounds,
        full_bounds);
    expect_near(last_left.x, -100.0, 0.001, "last-character lock uses full text width");

    expect_true(cursor_visible(0.25, 2.0), "cursor is visible in the first half of its cycle");
    expect_true(!cursor_visible(0.75, 2.0), "cursor is hidden in the second half of its cycle");
    expect_true(cursor_visible(10.0, 0.0), "zero blink speed keeps the cursor visible");
    expect_true(cursor_visible(10.0, std::numeric_limits<double>::quiet_NaN()),
                "invalid blink speed keeps the cursor visible");

    std::cout << "BackType_TextLogicTests passed\n";
    return 0;
}
