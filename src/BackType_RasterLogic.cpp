#include "BackType_RasterLogic.h"

#include <algorithm>
#include <cmath>
#include <mutex>
#include <unordered_map>
#include <vector>

namespace backtype {

namespace {

struct Pixel {
    std::uint8_t a = 0;
    std::uint8_t r = 0;
    std::uint8_t g = 0;
    std::uint8_t b = 0;
};

struct CachedRevealBoundaries {
    std::vector<std::size_t> bytes;
    std::vector<double> fractions;
};

struct RevealCacheKey {
    std::string text;
    RevealMode mode = RevealMode::Character;

    bool operator==(const RevealCacheKey &other) const {
        return mode == other.mode && text == other.text;
    }
};

struct RevealCacheKeyHash {
    std::size_t operator()(const RevealCacheKey &key) const {
        return std::hash<std::string>{}(key.text) ^
               (static_cast<std::size_t>(key.mode) + 0x9e3779b97f4a7c15ULL +
                (std::hash<std::string>{}(key.text) << 6) +
                (std::hash<std::string>{}(key.text) >> 2));
    }
};

bool supports_argb8(const PixelBuffer &buffer) {
    return buffer.data && buffer.width > 0 && buffer.height > 0 &&
           buffer.rowbytes >= buffer.width * 4 && buffer.format == PixelFormat::Argb8;
}

Pixel read_pixel(const PixelBuffer &buffer, int x, int y) {
    if (!supports_argb8(buffer) || x < 0 || y < 0 || x >= buffer.width || y >= buffer.height) {
        return {};
    }

    const auto *pixel = static_cast<const std::uint8_t *>(buffer.data) + y * buffer.rowbytes + x * 4;
    return {pixel[0], pixel[1], pixel[2], pixel[3]};
}

void composite_pixel(const PixelBuffer &target, int x, int y, Pixel pixel, double opacity) {
    if (pixel.a == 0 || opacity <= 0.0) {
        return;
    }

    const double alpha = clamp_unit(static_cast<double>(pixel.a) / 255.0 * opacity);
    composite_rgba8_pixel(target,
                          x,
                          y,
                          unit_to_u8(static_cast<double>(pixel.r) / 255.0 * alpha),
                          unit_to_u8(static_cast<double>(pixel.g) / 255.0 * alpha),
                          unit_to_u8(static_cast<double>(pixel.b) / 255.0 * alpha),
                          unit_to_u8(alpha));
}

bool is_word_character(unsigned char value) {
    return value > ' ';
}

std::size_t utf8_codepoint_length(const std::string &text, std::size_t offset) {
    if (offset >= text.size()) {
        return 0;
    }
    const auto ch = static_cast<unsigned char>(text[offset]);
    if ((ch & 0xE0U) == 0xC0U) {
        return std::min<std::size_t>(2, text.size() - offset);
    }
    if ((ch & 0xF0U) == 0xE0U) {
        return std::min<std::size_t>(3, text.size() - offset);
    }
    if ((ch & 0xF8U) == 0xF0U) {
        return std::min<std::size_t>(4, text.size() - offset);
    }
    return 1;
}

double approximate_advance_weight(const std::string &text, std::size_t offset) {
    const unsigned char ch = offset < text.size() ? static_cast<unsigned char>(text[offset]) : 0;
    if (ch <= ' ') {
        return 0.45;
    }
    if (ch == '.' || ch == ',' || ch == ':' || ch == ';' || ch == '!' || ch == '|' || ch == '\'' || ch == '`') {
        return 0.35;
    }
    if (ch == 'i' || ch == 'l' || ch == 'I' || ch == '[' || ch == ']' || ch == '(' || ch == ')') {
        return 0.55;
    }
    if (ch == 'm' || ch == 'w' || ch == 'M' || ch == 'W' || ch == '@') {
        return 1.35;
    }
    if (ch >= 0x80) {
        return 1.0;
    }
    return 1.0;
}

CachedRevealBoundaries build_reveal_boundaries(const std::string &text, RevealMode mode) {
    CachedRevealBoundaries boundaries;
    boundaries.bytes.push_back(0);
    boundaries.fractions.push_back(0.0);

    std::vector<std::size_t> character_ends;
    std::vector<double> cumulative_advances;
    double total_advance = 0.0;

    for (std::size_t offset = 0; offset < text.size();) {
        const std::size_t length = std::max<std::size_t>(1, utf8_codepoint_length(text, offset));
        total_advance += approximate_advance_weight(text, offset);
        offset = std::min(text.size(), offset + length);
        character_ends.push_back(offset);
        cumulative_advances.push_back(total_advance);
    }

    if (mode == RevealMode::Word) {
        bool in_word = false;
        for (std::size_t i = 0; i < text.size(); ++i) {
            const bool word_char = is_word_character(static_cast<unsigned char>(text[i]));
            if (word_char && !in_word) {
                in_word = true;
            } else if (!word_char && in_word) {
                boundaries.bytes.push_back(i);
                const auto found = std::lower_bound(character_ends.begin(), character_ends.end(), i);
                const std::size_t advance_index = static_cast<std::size_t>(std::distance(character_ends.begin(), found));
                boundaries.fractions.push_back(total_advance > 0.0 && advance_index < cumulative_advances.size()
                                                   ? cumulative_advances[advance_index] / total_advance
                                                   : 0.0);
                in_word = false;
            }
        }
        if (in_word) {
            boundaries.bytes.push_back(text.size());
            boundaries.fractions.push_back(1.0);
        }
        return boundaries;
    }

    boundaries.bytes.reserve(character_ends.size() + 1);
    boundaries.fractions.reserve(character_ends.size() + 1);
    for (std::size_t i = 0; i < character_ends.size(); ++i) {
        boundaries.bytes.push_back(character_ends[i]);
        boundaries.fractions.push_back(total_advance > 0.0 ? cumulative_advances[i] / total_advance : 0.0);
    }
    return boundaries;
}

CachedRevealBoundaries cached_reveal_boundaries(const std::string &text, RevealMode mode) {
    static std::mutex cache_mutex;
    static std::unordered_map<RevealCacheKey, CachedRevealBoundaries, RevealCacheKeyHash> cache;
    static constexpr std::size_t kMaxEntries = 64;

    const RevealCacheKey key{text, mode};
    {
        std::lock_guard<std::mutex> lock(cache_mutex);
        const auto found = cache.find(key);
        if (found != cache.end()) {
            return found->second;
        }
    }

    CachedRevealBoundaries built = build_reveal_boundaries(text, mode);

    std::lock_guard<std::mutex> lock(cache_mutex);
    if (cache.size() >= kMaxEntries) {
        cache.clear();
    }
    cache.emplace(key, built);
    return built;
}

void fill_rect(const PixelBuffer &target,
               int left,
               int top,
               int width,
               int height,
               const Color &color,
               double opacity) {
    if (!supports_argb8(target) || width <= 0 || height <= 0 || opacity <= 0.0) {
        return;
    }

    const std::uint8_t alpha = unit_to_u8(color.a * opacity);
    const std::uint8_t red = unit_to_u8(color.r * color.a * opacity);
    const std::uint8_t green = unit_to_u8(color.g * color.a * opacity);
    const std::uint8_t blue = unit_to_u8(color.b * color.a * opacity);

    for (int y = std::max(0, top); y < std::min(target.height, top + height); ++y) {
        for (int x = std::max(0, left); x < std::min(target.width, left + width); ++x) {
            composite_rgba8_pixel(target, x, y, red, green, blue, alpha);
        }
    }
}

} // namespace

int RasterBounds::width() const {
    return found ? max_x - min_x + 1 : 0;
}

int RasterBounds::height() const {
    return found ? max_y - min_y + 1 : 0;
}

RasterRevealState compute_raster_reveal(const std::string &text,
                                        double progress,
                                        RevealMode mode,
                                        double character_fade_percent) {
    RasterRevealState state;
    const CachedRevealBoundaries boundaries = cached_reveal_boundaries(text, mode);
    if (boundaries.bytes.empty()) {
        return state;
    }

    const double clamped = clamp_progress(progress);
    const std::size_t revealable_count = boundaries.bytes.size() - 1;
    std::size_t visible_index = 0;
    if (mode == RevealMode::Word) {
        if (clamped > 0.0 && revealable_count > 0) {
            visible_index = std::max<std::size_t>(
                1,
                static_cast<std::size_t>(std::ceil(static_cast<double>(revealable_count) * (clamped / 100.0))));
        }
    } else {
        visible_index = static_cast<std::size_t>(std::floor(static_cast<double>(revealable_count) * (clamped / 100.0)));
    }
    visible_index = std::min(visible_index, revealable_count);

    state.visible_bytes = boundaries.bytes[visible_index];
    state.previous_visible_bytes = boundaries.bytes[visible_index > 0 ? visible_index - 1 : 0];
    state.visible_fraction = boundaries.fractions[visible_index];
    state.previous_fraction = boundaries.fractions[visible_index > 0 ? visible_index - 1 : 0];

    if (mode != RevealMode::Character || character_fade_percent <= 0.0 ||
        state.visible_bytes == 0 || clamp_progress(progress) >= 100.0) {
        state.newest_opacity = 1.0;
        return state;
    }

    const double total = static_cast<double>(revealable_count);
    if (total <= 0.0) {
        state.newest_opacity = 0.0;
        return state;
    }

    const double raw = total * (clamp_progress(progress) / 100.0);
    const double visible = std::floor(raw);
    const double phase = std::clamp(raw - visible, 0.0, 1.0);
    const double fade_window = std::clamp(character_fade_percent / 100.0, 0.01, 1.0);
    state.newest_opacity = std::clamp(phase / fade_window, 0.0, 1.0);
    return state;
}

double push_easing_multiplier(double progress_unit, PushEasing easing) {
    const double t = std::clamp(progress_unit, 0.0, 1.0);
    switch (easing) {
        case PushEasing::EaseOut:
            return 1.0 - (1.0 - t) * (1.0 - t);
        case PushEasing::EaseInOut:
            return t < 0.5 ? 2.0 * t * t : 1.0 - std::pow(-2.0 * t + 2.0, 2.0) * 0.5;
        case PushEasing::Linear:
        default:
            return t;
    }
}

RasterBounds find_alpha_bounds(const PixelBuffer &source) {
    RasterBounds bounds;
    if (!supports_argb8(source)) {
        return bounds;
    }

    for (int y = 0; y < source.height; ++y) {
        for (int x = 0; x < source.width; ++x) {
            if (read_pixel(source, x, y).a == 0) {
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
    if (!supports_argb8(source) || !bounds.found) {
        return {};
    }

    double total_alpha = 0.0;
    double red = 0.0;
    double green = 0.0;
    double blue = 0.0;

    for (int y = bounds.min_y; y <= bounds.max_y; ++y) {
        for (int x = bounds.min_x; x <= bounds.max_x; ++x) {
            const Pixel pixel = read_pixel(source, x, y);
            if (pixel.a == 0) {
                continue;
            }

            const double alpha = static_cast<double>(pixel.a) / 255.0;
            total_alpha += alpha;
            red += static_cast<double>(pixel.r) / 255.0 * alpha;
            green += static_cast<double>(pixel.g) / 255.0 * alpha;
            blue += static_cast<double>(pixel.b) / 255.0 * alpha;
        }
    }

    if (total_alpha <= 0.0) {
        return {};
    }

    return {red / total_alpha, green / total_alpha, blue / total_alpha, 1.0};
}

void copy_revealed_raster(const PixelBuffer &source,
                          const PixelBuffer &target,
                          const RasterBounds &source_bounds,
                          DrawPosition target_position,
                          const RasterRevealState &reveal,
                          double opacity) {
    if (!supports_argb8(source) || !supports_argb8(target) || !source_bounds.found ||
        reveal.visible_fraction <= 0.0 || opacity <= 0.0) {
        return;
    }

    const int content_width = source_bounds.width();
    const int content_height = source_bounds.height();
    const int visible_width = std::clamp(static_cast<int>(std::ceil(content_width * reveal.visible_fraction)), 0, content_width);
    const int previous_width = std::clamp(static_cast<int>(std::floor(content_width * reveal.previous_fraction)), 0, visible_width);
    const int target_left = static_cast<int>(std::lround(target_position.x));
    const int target_top = static_cast<int>(std::lround(target_position.y));

    for (int y = 0; y < content_height; ++y) {
        for (int x = 0; x < visible_width; ++x) {
            double pixel_opacity = opacity;
            if (x >= previous_width && reveal.newest_opacity < 1.0) {
                pixel_opacity *= reveal.newest_opacity;
            }

            composite_pixel(target,
                            target_left + x,
                            target_top + y,
                            read_pixel(source, source_bounds.min_x + x, source_bounds.min_y + y),
                            pixel_opacity);
        }
    }
}

void draw_cursor(const PixelBuffer &target, const CursorDrawRequest &request) {
    if (!supports_argb8(target) || request.line_height <= 0.0 || request.opacity <= 0.0) {
        return;
    }

    const int thickness = std::max(1, static_cast<int>(std::lround(request.thickness)));
    const int x = static_cast<int>(std::lround(request.position.x));
    const int top = static_cast<int>(std::lround(request.position.y));
    const int height = std::max(1, static_cast<int>(std::lround(request.line_height)));

    switch (request.style) {
        case CursorStyle::Block:
            fill_rect(target, x, top, std::max(thickness * 3, height / 3), height, request.color, request.opacity * 0.55);
            break;
        case CursorStyle::Underscore:
            fill_rect(target, x, top + std::max(0, height - thickness), std::max(thickness * 4, height / 3), thickness, request.color, request.opacity);
            break;
        case CursorStyle::Line:
        default:
            fill_rect(target, x, top, thickness, height, request.color, request.opacity);
            break;
    }
}

} // namespace backtype
