#include "TextRenderer.h"

#if defined(_WIN32)

#include <windows.h>
#include <gdiplus.h>

#include <mutex>
#include <vector>

#pragma comment(lib, "gdiplus.lib")

namespace backtype {

namespace {

std::wstring utf8_to_utf16(const std::string &text) {
    if (text.empty()) {
        return {};
    }

    const int count = MultiByteToWideChar(CP_UTF8, 0, text.c_str(), -1, nullptr, 0);
    if (count <= 0) {
        return {};
    }

    std::wstring result(static_cast<std::size_t>(count - 1), L'\0');
    MultiByteToWideChar(CP_UTF8, 0, text.c_str(), -1, result.data(), count);
    return result;
}

bool ensure_gdiplus() {
    static std::once_flag once;
    static bool ok = false;
    static ULONG_PTR token = 0;

    std::call_once(once, [] {
        Gdiplus::GdiplusStartupInput input;
        ok = Gdiplus::GdiplusStartup(&token, &input, nullptr) == Gdiplus::Ok;
    });

    return ok;
}

Gdiplus::Color to_gdiplus_color(const Color &color, double opacity) {
    return Gdiplus::Color(
        static_cast<BYTE>(unit_to_u8(color.a * opacity)),
        static_cast<BYTE>(unit_to_u8(color.r)),
        static_cast<BYTE>(unit_to_u8(color.g)),
        static_cast<BYTE>(unit_to_u8(color.b)));
}

} // namespace

TextMetrics measure_text(const std::string &text, double font_size) {
    if (!ensure_gdiplus() || text.empty() || font_size <= 0.0) {
        return {};
    }

    const std::wstring wide_text = utf8_to_utf16(text);
    if (wide_text.empty()) {
        return {};
    }

    Gdiplus::Bitmap bitmap(1, 1, PixelFormat32bppPARGB);
    Gdiplus::Graphics graphics(&bitmap);
    graphics.SetTextRenderingHint(Gdiplus::TextRenderingHintAntiAliasGridFit);

    Gdiplus::Font font(L"Arial", static_cast<Gdiplus::REAL>(font_size), Gdiplus::FontStyleRegular, Gdiplus::UnitPixel);
    Gdiplus::StringFormat format(Gdiplus::StringFormat::GenericTypographic());
    Gdiplus::RectF bounds;
    graphics.MeasureString(wide_text.c_str(), -1, &font, Gdiplus::PointF(0.0f, 0.0f), &format, &bounds);

    TextMetrics metrics;
    metrics.width = std::max(0.0f, bounds.Width);
    metrics.height = std::max(1.0f, bounds.Height);
    metrics.ascent = metrics.height * 0.8;
    metrics.descent = metrics.height - metrics.ascent;
    return metrics;
}

bool render_text(const PixelBuffer &target, const TextRenderRequest &request) {
    if (!ensure_gdiplus() || !target.data || target.width <= 0 || target.height <= 0 || request.text.empty()) {
        return false;
    }

    const std::wstring wide_text = utf8_to_utf16(request.text);
    if (wide_text.empty()) {
        return false;
    }

    const TextMetrics metrics = measure_text(request.text, request.font_size);
    const int scratch_width = std::max(1, static_cast<int>(std::ceil(metrics.width + 4.0)));
    const int scratch_height = std::max(1, static_cast<int>(std::ceil(metrics.height + 4.0)));
    const int scratch_rowbytes = scratch_width * 4;
    std::vector<std::uint8_t> scratch(static_cast<std::size_t>(scratch_rowbytes * scratch_height), 0);

    Gdiplus::Bitmap bitmap(scratch_width,
                           scratch_height,
                           scratch_rowbytes,
                           PixelFormat32bppPARGB,
                           scratch.data());
    Gdiplus::Graphics graphics(&bitmap);
    graphics.Clear(Gdiplus::Color(0, 0, 0, 0));
    graphics.SetTextRenderingHint(Gdiplus::TextRenderingHintAntiAliasGridFit);

    Gdiplus::Font font(L"Arial",
                       static_cast<Gdiplus::REAL>(request.font_size),
                       Gdiplus::FontStyleRegular,
                       Gdiplus::UnitPixel);
    Gdiplus::StringFormat format(Gdiplus::StringFormat::GenericTypographic());
    Gdiplus::SolidBrush brush(to_gdiplus_color(request.color, request.opacity));
    graphics.DrawString(wide_text.c_str(), -1, &font, Gdiplus::PointF(2.0f, 2.0f), &format, &brush);

    const int start_x = static_cast<int>(std::floor(request.x - 2.0));
    const int start_y = static_cast<int>(std::floor(request.y - 2.0));

    for (int y = 0; y < scratch_height; ++y) {
        const auto *src = scratch.data() + y * scratch_rowbytes;
        for (int x = 0; x < scratch_width; ++x) {
            const auto *pixel = src + x * 4;
            composite_rgba8_pixel(target,
                                  start_x + x,
                                  start_y + y,
                                  pixel[2],
                                  pixel[1],
                                  pixel[0],
                                  pixel[3]);
        }
    }

    return true;
}

} // namespace backtype

#endif
