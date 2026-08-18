#pragma once

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>

namespace backtype {

enum class PixelFormat {
    Argb8,
    Argb16,
    ArgbFloat
};

struct Color {
    double r = 1.0;
    double g = 1.0;
    double b = 1.0;
    double a = 1.0;
};

struct PixelBuffer {
    void *data = nullptr;
    int width = 0;
    int height = 0;
    int rowbytes = 0;
    PixelFormat format = PixelFormat::Argb8;
};

constexpr double kArgb16ChannelMax = 32768.0;

inline double clamp_unit(double value) noexcept {
    if (!std::isfinite(value)) {
        return 0.0;
    }
    return std::clamp(value, 0.0, 1.0);
}

inline std::size_t bytes_per_pixel(PixelFormat format) noexcept {
    switch (format) {
        case PixelFormat::Argb16:
            return sizeof(std::uint16_t) * 4U;
        case PixelFormat::ArgbFloat:
            return sizeof(float) * 4U;
        case PixelFormat::Argb8:
        default:
            return sizeof(std::uint8_t) * 4U;
    }
}

inline bool valid_pixel_buffer(const PixelBuffer &buffer) noexcept {
    if (!buffer.data || buffer.width <= 0 || buffer.height <= 0 || buffer.rowbytes == 0) {
        return false;
    }
    const auto absolute_rowbytes = static_cast<std::uint64_t>(
        buffer.rowbytes < 0 ? -static_cast<std::int64_t>(buffer.rowbytes) : buffer.rowbytes);
    const auto required = static_cast<std::uint64_t>(buffer.width) * bytes_per_pixel(buffer.format);
    return absolute_rowbytes >= required;
}

inline std::uint8_t *pixel_row(const PixelBuffer &buffer, int y) noexcept {
    return static_cast<std::uint8_t *>(buffer.data) +
           static_cast<std::ptrdiff_t>(y) * static_cast<std::ptrdiff_t>(buffer.rowbytes);
}

inline void clear_target(const PixelBuffer &target) noexcept {
    if (!valid_pixel_buffer(target)) {
        return;
    }
    const std::size_t active_bytes = static_cast<std::size_t>(target.width) * bytes_per_pixel(target.format);
    for (int y = 0; y < target.height; ++y) {
        std::fill_n(pixel_row(target, y), active_bytes, std::uint8_t{0});
    }
}

inline std::uint8_t unit_to_u8(double value) noexcept {
    return static_cast<std::uint8_t>(std::lround(clamp_unit(value) * 255.0));
}

inline std::uint16_t unit_to_ae_u16(double value) noexcept {
    return static_cast<std::uint16_t>(std::lround(clamp_unit(value) * kArgb16ChannelMax));
}

} // namespace backtype
