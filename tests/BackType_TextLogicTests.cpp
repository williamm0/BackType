#include "../src/BackType_TextLogic.h"

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <string>

namespace {

void expect_true(bool condition, const char *message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << "\n";
        std::exit(1);
    }
}

void expect_eq(std::size_t actual, std::size_t expected, const char *message) {
    if (actual != expected) {
        std::cerr << "FAIL: " << message << " expected " << expected << " got " << actual << "\n";
        std::exit(1);
    }
}

void expect_near(double actual, double expected, double tolerance, const char *message) {
    if (std::fabs(actual - expected) > tolerance) {
        std::cerr << "FAIL: " << message << " expected " << expected << " got " << actual << "\n";
        std::exit(1);
    }
}

} // namespace

int main() {
    using namespace backtype;

    expect_eq(visible_character_count("hello", 0.0), 0, "0 percent reveals no characters");
    expect_eq(visible_character_count("hello", 10.0), 0, "10 percent floors visible characters");
    expect_eq(visible_character_count("hello", 25.0), 1, "25 percent reveals one character");
    expect_eq(visible_character_count("hello", 50.0), 2, "50 percent reveals two characters");
    expect_eq(visible_character_count("hello", 100.0), 5, "100 percent reveals all characters");
    expect_eq(visible_character_count("hello", 140.0), 5, "progress clamps above 100");

    expect_eq(visible_word_character_count("hello brave world", 1.0), 5, "word mode reveals first word once progress starts");
    expect_eq(visible_word_character_count("hello brave world", 50.0), 11, "word mode reveals complete words");
    expect_eq(visible_word_character_count("hello brave world", 100.0), 17, "word mode reveals all words");

    const TextBounds bounds{120.0, 40.0};
    const LayoutInput newest_left{AnchorMode::NewestCharacterLocked, Direction::MoveLeft, 200.0, 100.0, 100.0};
    const DrawPosition newest_left_pos = compute_draw_position(newest_left, bounds, bounds);
    expect_near(newest_left_pos.x, 80.0, 0.001, "newest locked move-left shifts by visible width");
    expect_near(newest_left_pos.y, 80.0, 0.001, "move-left keeps visual center on the Y anchor");

    const LayoutInput center_locked{AnchorMode::CenterLocked, Direction::MoveLeft, 200.0, 100.0, 100.0};
    const DrawPosition center_pos = compute_draw_position(center_locked, bounds, bounds);
    expect_near(center_pos.x, 140.0, 0.001, "center locked centers visible text");
    expect_near(center_pos.y, 80.0, 0.001, "center locked keeps the visual center on the Y anchor");

    const LayoutInput first_right{AnchorMode::FirstCharacterLocked, Direction::MoveRight, 200.0, 100.0, 300.0};
    const DrawPosition first_right_pos = compute_draw_position(first_right, bounds, bounds);
    expect_near(first_right_pos.x, 200.0, 0.001, "first character locked ignores backward amount");
    expect_near(first_right_pos.y, 80.0, 0.001, "first character locked still centers vertically");

    const TextBounds full_bounds{300.0, 40.0};
    const LayoutInput last_left{AnchorMode::LastCharacterLocked, Direction::MoveLeft, 200.0, 100.0, 100.0};
    const DrawPosition last_left_pos = compute_draw_position(last_left, bounds, full_bounds);
    expect_near(last_left_pos.x, -100.0, 0.001, "last character locked uses full text width");
    expect_near(last_left_pos.y, 80.0, 0.001, "last character locked still centers vertically");

    expect_true(cursor_visible(0.25, 2.0), "cursor visible in first half of blink cycle");
    expect_true(!cursor_visible(0.75, 2.0), "cursor hidden in second half of blink cycle");
    expect_true(cursor_visible(10.0, 0.0), "cursor stays visible when blink speed is zero");

    const double jitter_a = deterministic_jitter(3, 0, 15.0);
    const double jitter_b = deterministic_jitter(3, 0, 15.0);
    const double jitter_c = deterministic_jitter(4, 0, 15.0);
    expect_near(jitter_a, jitter_b, 0.000001, "jitter is stable for same character and frame");
    expect_true(std::fabs(jitter_a - jitter_c) > 0.000001, "jitter differs by character index");

    std::cout << "BackType_TextLogicTests passed\n";
    return 0;
}
