#include "BackType_TextLogic.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <vector>

namespace backtype {

namespace {

bool is_utf8_continuation(unsigned char value) {
    return (value & 0xC0U) == 0x80U;
}

bool is_word_character(unsigned char value) {
    return value > ' ';
}

std::vector<std::size_t> word_end_byte_offsets(const std::string &text) {
    std::vector<std::size_t> ends;
    bool in_word = false;

    for (std::size_t i = 0; i < text.size(); ++i) {
        const auto ch = static_cast<unsigned char>(text[i]);
        const bool word_char = is_word_character(ch);

        if (word_char && !in_word) {
            in_word = true;
        } else if (!word_char && in_word) {
            ends.push_back(i);
            in_word = false;
        }
    }

    if (in_word) {
        ends.push_back(text.size());
    }

    return ends;
}

std::uint32_t hash_jitter_seed(std::size_t character_index, long frame_index) {
    std::uint32_t value = 2166136261u;
    value ^= static_cast<std::uint32_t>(character_index + 1u);
    value *= 16777619u;
    value ^= static_cast<std::uint32_t>(frame_index + 0x9E3779B9u);
    value *= 16777619u;
    value ^= value >> 16;
    return value;
}

} // namespace

double clamp_progress(double progress) {
    if (!std::isfinite(progress)) {
        return 0.0;
    }
    return std::clamp(progress, 0.0, 100.0);
}

double clamp_percent(double value) {
    if (!std::isfinite(value)) {
        return 0.0;
    }
    return std::clamp(value, 0.0, 100.0);
}

std::size_t utf8_codepoint_count(const std::string &text) {
    std::size_t count = 0;
    for (const auto ch : text) {
        if (!is_utf8_continuation(static_cast<unsigned char>(ch))) {
            ++count;
        }
    }
    return count;
}

std::size_t byte_index_for_codepoint_count(const std::string &text, std::size_t codepoint_count) {
    if (codepoint_count == 0) {
        return 0;
    }

    std::size_t seen = 0;
    for (std::size_t i = 0; i < text.size(); ++i) {
        if (!is_utf8_continuation(static_cast<unsigned char>(text[i]))) {
            ++seen;
            if (seen > codepoint_count) {
                return i;
            }
        }
    }

    return text.size();
}

std::size_t visible_character_count(const std::string &text, double progress) {
    const auto total = utf8_codepoint_count(text);
    const auto visible = static_cast<std::size_t>(std::floor(total * (clamp_progress(progress) / 100.0)));
    return byte_index_for_codepoint_count(text, visible);
}

std::size_t visible_word_character_count(const std::string &text, double progress) {
    const double clamped = clamp_progress(progress);
    if (clamped <= 0.0 || text.empty()) {
        return 0;
    }

    const auto ends = word_end_byte_offsets(text);
    if (ends.empty()) {
        return 0;
    }

    const auto words_to_reveal = std::max<std::size_t>(
        1,
        static_cast<std::size_t>(std::ceil(ends.size() * (clamped / 100.0))));
    const auto index = std::min(words_to_reveal, ends.size()) - 1;
    return ends[index];
}

std::size_t visible_text_byte_count(const std::string &text, double progress, RevealMode mode) {
    return mode == RevealMode::Word
               ? visible_word_character_count(text, progress)
               : visible_character_count(text, progress);
}

std::string visible_text(const std::string &text, double progress, RevealMode mode) {
    return text.substr(0, visible_text_byte_count(text, progress, mode));
}

DrawPosition compute_draw_position(const LayoutInput &input,
                                   const TextBounds &visible_bounds,
                                   const TextBounds &full_bounds) {
    DrawPosition position{input.position_x, input.position_y - visible_bounds.height * 0.5};
    const double strength = std::clamp(input.backward_motion, 0.0, 300.0) / 100.0;

    switch (input.anchor_mode) {
        case AnchorMode::FirstCharacterLocked:
            return position;

        case AnchorMode::CenterLocked:
            position.x -= visible_bounds.width * 0.5;
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

bool cursor_visible(double comp_time_seconds, double blink_speed) {
    if (blink_speed <= 0.0 || !std::isfinite(blink_speed)) {
        return true;
    }

    const double phase = std::fmod(std::max(0.0, comp_time_seconds) * blink_speed, 2.0);
    return phase < 1.0;
}

double deterministic_jitter(std::size_t character_index, long frame_index, double amount) {
    if (amount <= 0.0 || !std::isfinite(amount)) {
        return 0.0;
    }

    const std::uint32_t hash = hash_jitter_seed(character_index, frame_index);
    const double normalized = static_cast<double>(hash & 0xFFFFu) / 65535.0;
    return (normalized * 2.0 - 1.0) * amount;
}

} // namespace backtype
