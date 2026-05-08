#include "TextRenderer.h"

#if defined(__APPLE__)

#include <CoreFoundation/CoreFoundation.h>
#include <CoreGraphics/CoreGraphics.h>
#include <CoreText/CoreText.h>

#include <vector>

namespace backtype {

namespace {

constexpr const char *kDefaultFontName = "Helvetica Neue";

struct CFReleaseGuard {
    explicit CFReleaseGuard(CFTypeRef value) : value(value) {}
    ~CFReleaseGuard() {
        if (value) {
            CFRelease(value);
        }
    }
    CFTypeRef value = nullptr;
};

CFStringRef make_cf_string(const std::string &text) {
    return CFStringCreateWithCString(kCFAllocatorDefault, text.c_str(), kCFStringEncodingUTF8);
}

CTFontRef make_font(double font_size) {
    CFStringRef name = CFStringCreateWithCString(kCFAllocatorDefault, kDefaultFontName, kCFStringEncodingUTF8);
    CFReleaseGuard name_guard(name);
    return CTFontCreateWithName(name, static_cast<CGFloat>(font_size), nullptr);
}

CTLineRef make_line(const std::string &text, double font_size, const Color *color = nullptr, double opacity = 1.0) {
    CFStringRef string = make_cf_string(text);
    if (!string) {
        return nullptr;
    }
    CFReleaseGuard string_guard(string);

    CTFontRef font = make_font(font_size);
    if (!font) {
        return nullptr;
    }
    CFReleaseGuard font_guard(font);

    const void *keys[2] = {kCTFontAttributeName, kCTForegroundColorAttributeName};
    CFTypeRef values[2] = {font, nullptr};
    CGColorRef cg_color = nullptr;

    if (color) {
        const CGFloat components[4] = {
            static_cast<CGFloat>(clamp_unit(color->r)),
            static_cast<CGFloat>(clamp_unit(color->g)),
            static_cast<CGFloat>(clamp_unit(color->b)),
            static_cast<CGFloat>(clamp_unit(color->a * opacity))
        };
        CGColorSpaceRef color_space = CGColorSpaceCreateDeviceRGB();
        cg_color = CGColorCreate(color_space, components);
        CGColorSpaceRelease(color_space);
        values[1] = cg_color;
    }

    CFDictionaryRef attributes = CFDictionaryCreate(kCFAllocatorDefault,
                                                    keys,
                                                    values,
                                                    color ? 2 : 1,
                                                    &kCFTypeDictionaryKeyCallBacks,
                                                    &kCFTypeDictionaryValueCallBacks);
    if (cg_color) {
        CGColorRelease(cg_color);
    }
    if (!attributes) {
        return nullptr;
    }
    CFReleaseGuard attributes_guard(attributes);

    CFAttributedStringRef attributed =
        CFAttributedStringCreate(kCFAllocatorDefault, string, attributes);
    if (!attributed) {
        return nullptr;
    }
    CFReleaseGuard attributed_guard(attributed);

    return CTLineCreateWithAttributedString(attributed);
}

} // namespace

TextMetrics measure_text(const std::string &text, double font_size) {
    if (text.empty() || font_size <= 0.0) {
        return {};
    }

    CTLineRef line = make_line(text, font_size);
    if (!line) {
        return {};
    }
    CFReleaseGuard line_guard(line);

    CGFloat ascent = 0.0;
    CGFloat descent = 0.0;
    CGFloat leading = 0.0;
    const double width = CTLineGetTypographicBounds(line, &ascent, &descent, &leading);

    TextMetrics metrics;
    metrics.width = std::max(0.0, width);
    metrics.ascent = std::max(0.0, static_cast<double>(ascent));
    metrics.descent = std::max(0.0, static_cast<double>(descent));
    metrics.height = std::max(1.0, metrics.ascent + metrics.descent + static_cast<double>(leading));
    return metrics;
}

bool render_text(const PixelBuffer &target, const TextRenderRequest &request) {
    if (!target.data || target.width <= 0 || target.height <= 0 || request.text.empty()) {
        return false;
    }

    const TextMetrics metrics = measure_text(request.text, request.font_size);
    const int scratch_width = std::max(1, static_cast<int>(std::ceil(metrics.width + 4.0)));
    const int scratch_height = std::max(1, static_cast<int>(std::ceil(metrics.height + 4.0)));
    const int scratch_rowbytes = scratch_width * 4;
    std::vector<std::uint8_t> scratch(static_cast<std::size_t>(scratch_rowbytes * scratch_height), 0);

    CGColorSpaceRef color_space = CGColorSpaceCreateDeviceRGB();
    CGContextRef context = CGBitmapContextCreate(scratch.data(),
                                                 scratch_width,
                                                 scratch_height,
                                                 8,
                                                 scratch_rowbytes,
                                                 color_space,
                                                 kCGImageAlphaPremultipliedLast | kCGBitmapByteOrder32Big);
    CGColorSpaceRelease(color_space);
    if (!context) {
        return false;
    }

    CGContextSetAllowsAntialiasing(context, true);
    CGContextSetShouldAntialias(context, true);
    CGContextTranslateCTM(context, 0.0, static_cast<CGFloat>(scratch_height));
    CGContextScaleCTM(context, 1.0, -1.0);

    CTLineRef line = make_line(request.text, request.font_size, &request.color, request.opacity);
    if (!line) {
        CGContextRelease(context);
        return false;
    }

    CGContextSetTextPosition(context, 2.0, static_cast<CGFloat>(metrics.ascent + 2.0));
    CTLineDraw(line, context);

    CFRelease(line);
    CGContextRelease(context);

    const int start_x = static_cast<int>(std::floor(request.x - 2.0));
    const int start_y = static_cast<int>(std::floor(request.y - 2.0));

    for (int y = 0; y < scratch_height; ++y) {
        const auto *src = scratch.data() + y * scratch_rowbytes;
        for (int x = 0; x < scratch_width; ++x) {
            const auto *pixel = src + x * 4;
            composite_rgba8_pixel(target,
                                  start_x + x,
                                  start_y + y,
                                  pixel[0],
                                  pixel[1],
                                  pixel[2],
                                  pixel[3]);
        }
    }

    return true;
}

} // namespace backtype

#endif
