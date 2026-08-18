#include "BackType_RasterLogic.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <vector>

namespace backtype {

namespace {

struct Pixel {
    double a = 0.0;
    double r = 0.0;
    double g = 0.0;
    double b = 0.0;
};

struct AxisRun {
    int start = 0;
    int end = 0;
};

bool direction_is_horizontal(Direction direction) noexcept {
    return direction == Direction::MoveLeft || direction == Direction::MoveRight;
}

bool direction_is_forward(Direction direction) noexcept {
    return direction == Direction::MoveLeft || direction == Direction::MoveUp;
}

double finite_nonnegative(double value) noexcept {
    return std::isfinite(value) ? std::max(0.0, value) : 0.0;
}

Pixel read_pixel(const PixelBuffer &buffer, int x, int y) noexcept {
    if (!valid_pixel_buffer(buffer) || x < 0 || y < 0 || x >= buffer.width || y >= buffer.height) {
        return {};
    }

    const std::size_t offset = static_cast<std::size_t>(x) * bytes_per_pixel(buffer.format);
    const auto *address = pixel_row(buffer, y) + offset;
    switch (buffer.format) {
        case PixelFormat::Argb16: {
            const auto *pixel = reinterpret_cast<const std::uint16_t *>(address);
            return {clamp_unit(static_cast<double>(pixel[0]) / kArgb16ChannelMax),
                    finite_nonnegative(static_cast<double>(pixel[1]) / kArgb16ChannelMax),
                    finite_nonnegative(static_cast<double>(pixel[2]) / kArgb16ChannelMax),
                    finite_nonnegative(static_cast<double>(pixel[3]) / kArgb16ChannelMax)};
        }
        case PixelFormat::ArgbFloat: {
            const auto *pixel = reinterpret_cast<const float *>(address);
            return {clamp_unit(pixel[0]),
                    finite_nonnegative(pixel[1]),
                    finite_nonnegative(pixel[2]),
                    finite_nonnegative(pixel[3])};
        }
        case PixelFormat::Argb8:
        default:
            return {static_cast<double>(address[0]) / 255.0,
                    static_cast<double>(address[1]) / 255.0,
                    static_cast<double>(address[2]) / 255.0,
                    static_cast<double>(address[3]) / 255.0};
    }
}

void write_pixel(const PixelBuffer &buffer, int x, int y, const Pixel &pixel) noexcept {
    if (!valid_pixel_buffer(buffer) || x < 0 || y < 0 || x >= buffer.width || y >= buffer.height) {
        return;
    }

    const std::size_t offset = static_cast<std::size_t>(x) * bytes_per_pixel(buffer.format);
    auto *address = pixel_row(buffer, y) + offset;
    switch (buffer.format) {
        case PixelFormat::Argb16: {
            auto *output = reinterpret_cast<std::uint16_t *>(address);
            output[0] = unit_to_ae_u16(pixel.a);
            output[1] = unit_to_ae_u16(pixel.r);
            output[2] = unit_to_ae_u16(pixel.g);
            output[3] = unit_to_ae_u16(pixel.b);
            break;
        }
        case PixelFormat::ArgbFloat: {
            auto *output = reinterpret_cast<float *>(address);
            output[0] = static_cast<float>(clamp_unit(pixel.a));
            output[1] = static_cast<float>(finite_nonnegative(pixel.r));
            output[2] = static_cast<float>(finite_nonnegative(pixel.g));
            output[3] = static_cast<float>(finite_nonnegative(pixel.b));
            break;
        }
        case PixelFormat::Argb8:
        default:
            address[0] = unit_to_u8(pixel.a);
            address[1] = unit_to_u8(pixel.r);
            address[2] = unit_to_u8(pixel.g);
            address[3] = unit_to_u8(pixel.b);
            break;
    }
}

void composite_pixel(const PixelBuffer &target, int x, int y, const Pixel &source, double opacity) noexcept {
    const double applied_opacity = clamp_unit(opacity);
    const double source_alpha = clamp_unit(source.a * applied_opacity);
    if (source_alpha <= 0.0) {
        return;
    }

    const Pixel destination = read_pixel(target, x, y);
    const double inverse_alpha = 1.0 - source_alpha;
    write_pixel(target,
                x,
                y,
                {source_alpha + destination.a * inverse_alpha,
                 source.r * applied_opacity + destination.r * inverse_alpha,
                 source.g * applied_opacity + destination.g * inverse_alpha,
                 source.b * applied_opacity + destination.b * inverse_alpha});
}

std::vector<AxisRun> occupied_axis_runs(const PixelBuffer &source,
                                        const RasterBounds &bounds,
                                        Direction direction) {
    const bool horizontal = direction_is_horizontal(direction);
    const int axis_length = horizontal ? bounds.width() : bounds.height();
    const int cross_length = horizontal ? bounds.height() : bounds.width();
    std::vector<bool> occupied(static_cast<std::size_t>(axis_length), false);

    for (int axis = 0; axis < axis_length; ++axis) {
        for (int cross = 0; cross < cross_length; ++cross) {
            const int x = horizontal ? bounds.min_x + axis : bounds.min_x + cross;
            const int y = horizontal ? bounds.min_y + cross : bounds.min_y + axis;
            if (read_pixel(source, x, y).a > 0.0) {
                occupied[static_cast<std::size_t>(axis)] = true;
                break;
            }
        }
    }

    std::vector<AxisRun> runs;
    for (int axis = 0; axis < axis_length;) {
        while (axis < axis_length && !occupied[static_cast<std::size_t>(axis)]) {
            ++axis;
        }
        if (axis >= axis_length) {
            break;
        }
        const int start = axis;
        while (axis + 1 < axis_length && occupied[static_cast<std::size_t>(axis + 1)]) {
            ++axis;
        }
        runs.push_back({start, axis});
        ++axis;
    }
    return runs;
}

std::vector<AxisRun> group_word_runs(const std::vector<AxisRun> &character_runs, int cross_length) {
    if (character_runs.empty()) {
        return {};
    }

    const int word_gap = std::max(3, static_cast<int>(std::lround(static_cast<double>(cross_length) * 0.18)));
    std::vector<AxisRun> words;
    AxisRun current = character_runs.front();
    for (std::size_t index = 1; index < character_runs.size(); ++index) {
        const AxisRun next = character_runs[index];
        const int gap = next.start - current.end - 1;
        if (gap >= word_gap) {
            words.push_back(current);
            current = next;
        } else {
            current.end = next.end;
        }
    }
    words.push_back(current);
    return words;
}

std::vector<double> reveal_fractions(const PixelBuffer &source,
                                     const RasterBounds &bounds,
                                     RevealMode mode,
                                     Direction direction) {
    const bool horizontal = direction_is_horizontal(direction);
    const int axis_length = horizontal ? bounds.width() : bounds.height();
    const int cross_length = horizontal ? bounds.height() : bounds.width();
    std::vector<AxisRun> runs = occupied_axis_runs(source, bounds, direction);
    if (mode == RevealMode::Word) {
        runs = group_word_runs(runs, cross_length);
    }

    std::vector<double> fractions;
    fractions.reserve(runs.size() + 1U);
    fractions.push_back(0.0);
    if (direction_is_forward(direction)) {
        for (const AxisRun &run : runs) {
            fractions.push_back(static_cast<double>(run.end + 1) / static_cast<double>(axis_length));
        }
    } else {
        for (auto run = runs.rbegin(); run != runs.rend(); ++run) {
            fractions.push_back(static_cast<double>(axis_length - run->start) / static_cast<double>(axis_length));
        }
    }
    if (fractions.size() > 1U) {
        fractions.back() = 1.0;
    }
    return fractions;
}

void fill_rect(const PixelBuffer &target,
               int left,
               int top,
               int width,
               int height,
               const Color &color,
               double opacity) noexcept {
    if (!valid_pixel_buffer(target) || width <= 0 || height <= 0 || opacity <= 0.0) {
        return;
    }

    const double alpha = clamp_unit(color.a);
    const Pixel pixel{alpha,
                      finite_nonnegative(color.r) * alpha,
                      finite_nonnegative(color.g) * alpha,
                      finite_nonnegative(color.b) * alpha};
    const int right = std::min(target.width, left + width);
    const int bottom = std::min(target.height, top + height);
    for (int y = std::max(0, top); y < bottom; ++y) {
        for (int x = std::max(0, left); x < right; ++x) {
            composite_pixel(target, x, y, pixel, opacity);
        }
    }
}

} // namespace

int RasterBounds::width() const noexcept {
    return found ? max_x - min_x + 1 : 0;
}

int RasterBounds::height() const noexcept {
    return found ? max_y - min_y + 1 : 0;
}

RasterRevealState compute_raster_reveal(const PixelBuffer &source,
                                        const RasterBounds &source_bounds,
                                        double progress,
                                        RevealMode mode,
                                        Direction direction,
                                        double character_fade_percent) {
    RasterRevealState state;
    if (!valid_pixel_buffer(source) || !source_bounds.found) {
        return state;
    }

    const std::vector<double> fractions = reveal_fractions(source, source_bounds, mode, direction);
    const std::size_t revealable_count = fractions.empty() ? 0U : fractions.size() - 1U;
    if (revealable_count == 0U) {
        return state;
    }

    const double clamped_progress = clamp_progress(progress);
    const double raw_index = static_cast<double>(revealable_count) * (clamped_progress / 100.0);
    const bool fade_enabled = mode == RevealMode::Character && character_fade_percent > 0.0;
    std::size_t visible_index = 0U;
    if (mode == RevealMode::Word && clamped_progress > 0.0) {
        visible_index = static_cast<std::size_t>(std::ceil(raw_index));
    } else if (fade_enabled && raw_index > 0.0) {
        visible_index = static_cast<std::size_t>(std::ceil(raw_index));
    } else {
        visible_index = static_cast<std::size_t>(std::floor(raw_index));
    }
    visible_index = std::min(visible_index, revealable_count);

    state.visible_fraction = fractions[visible_index];
    state.previous_fraction = fractions[visible_index > 0U ? visible_index - 1U : 0U];
    if (!fade_enabled || visible_index == 0U || clamped_progress >= 100.0) {
        return state;
    }

    double phase = raw_index - std::floor(raw_index);
    if (phase <= 0.0) {
        phase = 1.0;
    }
    const double fade_window = std::clamp(character_fade_percent / 100.0, 0.01, 1.0);
    state.newest_opacity = std::clamp(phase / fade_window, 0.0, 1.0);
    return state;
}

RasterBounds find_alpha_bounds(const PixelBuffer &source) {
    RasterBounds bounds;
    if (!valid_pixel_buffer(source)) {
        return bounds;
    }

    for (int y = 0; y < source.height; ++y) {
        for (int x = 0; x < source.width; ++x) {
            if (read_pixel(source, x, y).a <= 0.0) {
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

Color average_alpha_color(const PixelBuffer &source, const RasterBounds &bounds) {
    if (!valid_pixel_buffer(source) || !bounds.found) {
        return {};
    }

    double total_alpha = 0.0;
    double red = 0.0;
    double green = 0.0;
    double blue = 0.0;
    for (int y = bounds.min_y; y <= bounds.max_y; ++y) {
        for (int x = bounds.min_x; x <= bounds.max_x; ++x) {
            const Pixel pixel = read_pixel(source, x, y);
            if (pixel.a <= 0.0) {
                continue;
            }
            total_alpha += pixel.a;
            red += pixel.r;
            green += pixel.g;
            blue += pixel.b;
        }
    }
    if (total_alpha <= 0.0) {
        return {};
    }
    return {red / total_alpha, green / total_alpha, blue / total_alpha, 1.0};
}

TextBounds visible_raster_bounds(const RasterBounds &source_bounds,
                                 const RasterRevealState &reveal,
                                 Direction direction) {
    if (!source_bounds.found) {
        return {};
    }
    const double width = static_cast<double>(source_bounds.width());
    const double height = static_cast<double>(source_bounds.height());
    return direction_is_horizontal(direction)
               ? TextBounds{width * reveal.visible_fraction, height}
               : TextBounds{width, height * reveal.visible_fraction};
}

void copy_revealed_raster(const PixelBuffer &source,
                          const PixelBuffer &target,
                          const RasterBounds &source_bounds,
                          DrawPosition target_position,
                          const RasterRevealState &reveal,
                          Direction direction,
                          double opacity) {
    if (!valid_pixel_buffer(source) || !valid_pixel_buffer(target) ||
        source.format != target.format || !source_bounds.found ||
        reveal.visible_fraction <= 0.0 || opacity <= 0.0) {
        return;
    }

    const bool horizontal = direction_is_horizontal(direction);
    const bool forward = direction_is_forward(direction);
    const int axis_length = horizontal ? source_bounds.width() : source_bounds.height();
    const int visible_extent = std::clamp(
        static_cast<int>(std::ceil(static_cast<double>(axis_length) * reveal.visible_fraction)), 0, axis_length);
    const int previous_extent = std::clamp(
        static_cast<int>(std::floor(static_cast<double>(axis_length) * reveal.previous_fraction)), 0, visible_extent);
    const int target_left = static_cast<int>(std::lround(target_position.x));
    const int target_top = static_cast<int>(std::lround(target_position.y));
    const int source_axis_start = forward ? 0 : axis_length - visible_extent;

    for (int source_y = source_bounds.min_y; source_y <= source_bounds.max_y; ++source_y) {
        for (int source_x = source_bounds.min_x; source_x <= source_bounds.max_x; ++source_x) {
            const int rel_x = source_x - source_bounds.min_x;
            const int rel_y = source_y - source_bounds.min_y;
            const int axis = horizontal ? rel_x : rel_y;
            const bool revealed = forward ? axis < visible_extent : axis >= axis_length - visible_extent;
            if (!revealed) {
                continue;
            }
            const bool newest = forward ? axis >= previous_extent : axis < axis_length - previous_extent;
            const double pixel_opacity = newest ? opacity * reveal.newest_opacity : opacity;
            const int target_x = target_left + rel_x - (!forward && horizontal ? source_axis_start : 0);
            const int target_y = target_top + rel_y - (!forward && !horizontal ? source_axis_start : 0);
            composite_pixel(target,
                            target_x,
                            target_y,
                            read_pixel(source, source_x, source_y),
                            pixel_opacity);
        }
    }
}

DrawPosition cursor_position_for_reveal(DrawPosition target_position,
                                        const RasterBounds &source_bounds,
                                        const RasterRevealState &reveal,
                                        Direction direction,
                                        double cursor_offset) {
    const double content_width = static_cast<double>(source_bounds.width());
    const double content_height = static_cast<double>(source_bounds.height());
    const double visible_width = content_width * reveal.visible_fraction;
    const double visible_height = content_height * reveal.visible_fraction;
    switch (direction) {
        case Direction::MoveRight:
            return {target_position.x - cursor_offset, target_position.y};
        case Direction::MoveUp:
            return {target_position.x, target_position.y + visible_height + cursor_offset};
        case Direction::MoveDown:
            return {target_position.x, target_position.y - cursor_offset};
        case Direction::MoveLeft:
        default:
            return {target_position.x + visible_width + cursor_offset, target_position.y};
    }
}

void draw_cursor(const PixelBuffer &target, const CursorDrawRequest &request) {
    if (!valid_pixel_buffer(target) || request.content_width <= 0.0 ||
        request.content_height <= 0.0 || request.opacity <= 0.0) {
        return;
    }

    const int thickness = std::max(1, static_cast<int>(std::lround(request.thickness)));
    const int x = static_cast<int>(std::lround(request.position.x));
    const int y = static_cast<int>(std::lround(request.position.y));
    const int width = std::max(1, static_cast<int>(std::lround(request.content_width)));
    const int height = std::max(1, static_cast<int>(std::lround(request.content_height)));

    if (!direction_is_horizontal(request.direction)) {
        const int block_height = std::max(thickness * 3, height / 3);
        const int cursor_height = request.style == CursorStyle::Block ? block_height : thickness;
        const double cursor_opacity = request.style == CursorStyle::Block ? request.opacity * 0.55 : request.opacity;
        fill_rect(target, x, y, width, cursor_height, request.color, cursor_opacity);
        return;
    }

    switch (request.style) {
        case CursorStyle::Block:
            fill_rect(target, x, y, std::max(thickness * 3, height / 3), height,
                      request.color, request.opacity * 0.55);
            break;
        case CursorStyle::Underscore:
            fill_rect(target, x, y + std::max(0, height - thickness),
                      std::max(thickness * 4, height / 3), thickness, request.color, request.opacity);
            break;
        case CursorStyle::Line:
        default:
            fill_rect(target, x, y, thickness, height, request.color, request.opacity);
            break;
    }
}

} // namespace backtype
