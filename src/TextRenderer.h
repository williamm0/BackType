#pragma once

#include <algorithm>
#include <cmath>
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

inline double clamp_unit(double value) {
    if (!std::isfinite(value)) {
        return 0.0;
    }
    return std::clamp(value, 0.0, 1.0);
}

inline void clear_target(const PixelBuffer &target) {
    if (!target.data || target.width <= 0 || target.height <= 0 || target.rowbytes <= 0) {
        return;
    }

    for (int y = 0; y < target.height; ++y) {
        auto *row = static_cast<std::uint8_t *>(target.data) + y * target.rowbytes;
        std::fill(row, row + target.rowbytes, 0);
    }
}

inline void alpha_over(double src_r,
                       double src_g,
                       double src_b,
                       double src_a,
                       double &dst_r,
                       double &dst_g,
                       double &dst_b,
                       double &dst_a) {
    src_a = clamp_unit(src_a);
    const double out_a = src_a + dst_a * (1.0 - src_a);
    if (out_a <= 0.0) {
        dst_r = 0.0;
        dst_g = 0.0;
        dst_b = 0.0;
        dst_a = 0.0;
        return;
    }

    dst_r = (src_r * src_a + dst_r * dst_a * (1.0 - src_a)) / out_a;
    dst_g = (src_g * src_a + dst_g * dst_a * (1.0 - src_a)) / out_a;
    dst_b = (src_b * src_a + dst_b * dst_a * (1.0 - src_a)) / out_a;
    dst_a = out_a;
}

inline std::uint8_t unit_to_u8(double value) {
    return static_cast<std::uint8_t>(std::lround(clamp_unit(value) * 255.0));
}

inline std::uint16_t unit_to_u16(double value) {
    return static_cast<std::uint16_t>(std::lround(clamp_unit(value) * 65535.0));
}

inline void composite_rgba8_pixel(const PixelBuffer &target,
                                  int x,
                                  int y,
                                  std::uint8_t premul_r,
                                  std::uint8_t premul_g,
                                  std::uint8_t premul_b,
                                  std::uint8_t alpha) {
    if (!target.data || target.rowbytes <= 0 || x < 0 || y < 0 ||
        x >= target.width || y >= target.height || alpha == 0) {
        return;
    }

    const double src_a = static_cast<double>(alpha) / 255.0;
    const double src_r = src_a > 0.0 ? (static_cast<double>(premul_r) / 255.0) / src_a : 0.0;
    const double src_g = src_a > 0.0 ? (static_cast<double>(premul_g) / 255.0) / src_a : 0.0;
    const double src_b = src_a > 0.0 ? (static_cast<double>(premul_b) / 255.0) / src_a : 0.0;

    auto *row = static_cast<std::uint8_t *>(target.data) + y * target.rowbytes;

    if (target.format == PixelFormat::Argb8) {
        if ((x + 1) * 4 > target.rowbytes) {
            return;
        }
        auto *pixel = row + x * 4;
        double dst_a = static_cast<double>(pixel[0]) / 255.0;
        double dst_r = static_cast<double>(pixel[1]) / 255.0;
        double dst_g = static_cast<double>(pixel[2]) / 255.0;
        double dst_b = static_cast<double>(pixel[3]) / 255.0;
        alpha_over(src_r, src_g, src_b, src_a, dst_r, dst_g, dst_b, dst_a);
        pixel[0] = unit_to_u8(dst_a);
        pixel[1] = unit_to_u8(dst_r);
        pixel[2] = unit_to_u8(dst_g);
        pixel[3] = unit_to_u8(dst_b);
    } else if (target.format == PixelFormat::Argb16) {
        if ((x + 1) * 8 > target.rowbytes) {
            return;
        }
        auto *pixel = reinterpret_cast<std::uint16_t *>(row + x * 8);
        double dst_a = static_cast<double>(pixel[0]) / 65535.0;
        double dst_r = static_cast<double>(pixel[1]) / 65535.0;
        double dst_g = static_cast<double>(pixel[2]) / 65535.0;
        double dst_b = static_cast<double>(pixel[3]) / 65535.0;
        alpha_over(src_r, src_g, src_b, src_a, dst_r, dst_g, dst_b, dst_a);
        pixel[0] = unit_to_u16(dst_a);
        pixel[1] = unit_to_u16(dst_r);
        pixel[2] = unit_to_u16(dst_g);
        pixel[3] = unit_to_u16(dst_b);
    } else {
        if ((x + 1) * 16 > target.rowbytes) {
            return;
        }
        auto *pixel = reinterpret_cast<float *>(row + x * 16);
        double dst_a = pixel[0];
        double dst_r = pixel[1];
        double dst_g = pixel[2];
        double dst_b = pixel[3];
        alpha_over(src_r, src_g, src_b, src_a, dst_r, dst_g, dst_b, dst_a);
        pixel[0] = static_cast<float>(clamp_unit(dst_a));
        pixel[1] = static_cast<float>(clamp_unit(dst_r));
        pixel[2] = static_cast<float>(clamp_unit(dst_g));
        pixel[3] = static_cast<float>(clamp_unit(dst_b));
    }
}

} // namespace backtype
