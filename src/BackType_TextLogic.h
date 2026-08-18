#pragma once

#include "BackType_Enums.h"

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

double clamp_progress(double progress) noexcept;
double clamp_percent(double value) noexcept;

DrawPosition compute_draw_position(const LayoutInput &input,
                                   const TextBounds &visible_bounds,
                                   const TextBounds &full_bounds);

bool cursor_visible(double comp_time_seconds, double blink_speed) noexcept;

} // namespace backtype
