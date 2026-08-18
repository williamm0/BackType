#include "BackType_TextLogic.h"

#include <algorithm>
#include <cmath>

namespace backtype {

double clamp_progress(double progress) noexcept {
    if (!std::isfinite(progress)) {
        return 0.0;
    }
    return std::clamp(progress, 0.0, 100.0);
}

double clamp_percent(double value) noexcept {
    if (!std::isfinite(value)) {
        return 0.0;
    }
    return std::clamp(value, 0.0, 100.0);
}

DrawPosition compute_draw_position(const LayoutInput &input,
                                   const TextBounds &visible_bounds,
                                   const TextBounds &full_bounds) {
    const bool vertical_direction = input.direction == Direction::MoveUp ||
                                    input.direction == Direction::MoveDown;
    DrawPosition position{input.position_x, input.position_y - full_bounds.height * 0.5};
    const double strength = std::clamp(input.backward_motion, 0.0, 300.0) / 100.0;

    switch (input.anchor_mode) {
        case AnchorMode::FirstCharacterLocked:
            return position;

        case AnchorMode::CenterLocked:
            if (vertical_direction) {
                position.x -= full_bounds.width * 0.5;
                position.y = input.position_y - visible_bounds.height * 0.5;
            } else {
                position.x -= visible_bounds.width * 0.5;
            }
            return position;

        case AnchorMode::LastCharacterLocked:
            if (input.direction == Direction::MoveLeft) {
                position.x -= full_bounds.width * strength;
            } else if (input.direction == Direction::MoveRight) {
                position.x += full_bounds.width * strength;
            } else if (input.direction == Direction::MoveUp) {
                position.y -= full_bounds.height * strength;
            } else {
                position.y += full_bounds.height * strength;
            }
            return position;

        case AnchorMode::NewestCharacterLocked:
        default:
            if (input.direction == Direction::MoveLeft) {
                position.x -= visible_bounds.width * strength;
            } else if (input.direction == Direction::MoveRight) {
                position.x += visible_bounds.width * strength;
            } else if (input.direction == Direction::MoveUp) {
                position.y -= visible_bounds.height * strength;
            } else {
                position.y += visible_bounds.height * strength;
            }
            return position;
    }
}

bool cursor_visible(double comp_time_seconds, double blink_speed) noexcept {
    if (blink_speed <= 0.0 || !std::isfinite(blink_speed)) {
        return true;
    }

    const double phase = std::fmod(std::max(0.0, comp_time_seconds) * blink_speed, 2.0);
    return phase < 1.0;
}

} // namespace backtype
