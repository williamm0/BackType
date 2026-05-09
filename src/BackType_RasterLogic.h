#pragma once

#include "BackType_Enums.h"
#include "BackType_TextLogic.h"
#include "TextRenderer.h"

#include <cstddef>
#include <cstdint>
#include <string>

namespace backtype {

struct RasterBounds {
    bool found = false;
    int min_x = 0;
    int min_y = 0;
    int max_x = 0;
    int max_y = 0;

    int width() const;
    int height() const;
};

struct RasterRevealState {
    std::size_t visible_bytes = 0;
    std::size_t previous_visible_bytes = 0;
    double visible_fraction = 0.0;
    double previous_fraction = 0.0;
    double newest_opacity = 1.0;
};

struct CursorDrawRequest {
    CursorStyle style = CursorStyle::Line;
    DrawPosition position;
    double line_height = 0.0;
    double thickness = 2.0;
    Color color;
    double opacity = 1.0;
};

RasterRevealState compute_raster_reveal(const std::string &text,
                                        double progress,
                                        RevealMode mode,
                                        double character_fade_percent);

RasterBounds find_alpha_bounds(const PixelBuffer &source);
Color average_alpha_color(const PixelBuffer &source, const RasterBounds &bounds);

TextBounds visible_raster_bounds(const RasterBounds &source_bounds,
                                 const RasterRevealState &reveal,
                                 Direction direction);

void copy_revealed_raster(const PixelBuffer &source,
                          const PixelBuffer &target,
                          const RasterBounds &source_bounds,
                          DrawPosition target_position,
                          const RasterRevealState &reveal,
                          Direction direction,
                          double render_padding,
                          double opacity);

DrawPosition cursor_position_for_reveal(DrawPosition target_position,
                                        const RasterBounds &source_bounds,
                                        const RasterRevealState &reveal,
                                        Direction direction,
                                        double cursor_offset);

void draw_cursor(const PixelBuffer &target, const CursorDrawRequest &request);

} // namespace backtype
