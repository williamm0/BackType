#pragma once

#include "BackType_Enums.h"

#include <cstddef>
#include <string>

namespace backtype {

struct TextBounds {
    double width = 0.0;
    double height = 0.0;
};

struct LayoutInput {
    AnchorMode anchor_mode = AnchorMode::CenterLocked;
    Direction direction = Direction::MoveLeft;
    double position_x = 0.0;
    double position_y = 0.0;
    double backward_motion = 100.0;
};

struct DrawPosition {
    double x = 0.0;
    double y = 0.0;
};

double clamp_progress(double progress);
double clamp_percent(double value);

std::size_t utf8_codepoint_count(const std::string &text);
std::size_t byte_index_for_codepoint_count(const std::string &text, std::size_t codepoint_count);

std::size_t visible_character_count(const std::string &text, double progress);
std::size_t visible_word_character_count(const std::string &text, double progress);
std::size_t visible_text_byte_count(const std::string &text, double progress, RevealMode mode);
std::string visible_text(const std::string &text, double progress, RevealMode mode);

DrawPosition compute_draw_position(const LayoutInput &input,
                                   const TextBounds &visible_bounds,
                                   const TextBounds &full_bounds);

bool cursor_visible(double comp_time_seconds, double blink_speed);
double deterministic_jitter(std::size_t character_index, long frame_index, double amount);

} // namespace backtype
