#include "BackType.h"
#include "BackType_Enums.h"
#include "BackType_RasterLogic.h"
#include "BackType_Strings.h"
#include "BackType_TextLogic.h"

#include "AEConfig.h"

#include "entry.h"
#include "AE_Effect.h"
#include "AE_EffectCB.h"
#include "AE_Macros.h"
#include "AE_PluginData.h"
#include "Param_Utils.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <new>

static_assert(BACKTYPE_AE_EFFECT_VERSION ==
                  PF_VERSION(BACKTYPE_VERSION_MAJOR,
                             BACKTYPE_VERSION_MINOR,
                             BACKTYPE_VERSION_PATCH,
                             PF_Stage_DEVELOP,
                             BACKTYPE_VERSION_BUILD),
              "BackType encoded AE version must match PF_VERSION.");

enum BackTypeParam {
    BACKTYPE_INPUT = 0,
    BACKTYPE_PROGRESS,
    BACKTYPE_POSITION_X,
    BACKTYPE_POSITION_Y,
    BACKTYPE_ANCHOR_MODE,
    BACKTYPE_BACKWARD_MOTION,
    BACKTYPE_DIRECTION,
    BACKTYPE_REVEAL_MODE,
    BACKTYPE_CURSOR_ENABLED,
    BACKTYPE_CURSOR_STYLE,
    BACKTYPE_CURSOR_BLINK_SPEED,
    BACKTYPE_CURSOR_OFFSET,
    BACKTYPE_CHARACTER_FADE,
    BACKTYPE_RENDER_PADDING,
    BACKTYPE_OPACITY,
    BACKTYPE_NUM_PARAMS
};

enum BackTypeDiskId {
    BACKTYPE_PROGRESS_DISK_ID = 3,
    BACKTYPE_POSITION_X_DISK_ID,
    BACKTYPE_POSITION_Y_DISK_ID,
    BACKTYPE_ANCHOR_MODE_DISK_ID,
    BACKTYPE_BACKWARD_MOTION_DISK_ID,
    BACKTYPE_DIRECTION_DISK_ID,
    BACKTYPE_REVEAL_MODE_DISK_ID,
    BACKTYPE_CURSOR_ENABLED_DISK_ID,
    BACKTYPE_CURSOR_BLINK_SPEED_DISK_ID,
    BACKTYPE_CURSOR_OFFSET_DISK_ID,
    BACKTYPE_OPACITY_DISK_ID = 18,
    BACKTYPE_CURSOR_STYLE_DISK_ID,
    BACKTYPE_CHARACTER_FADE_DISK_ID,
    BACKTYPE_RENDER_PADDING_DISK_ID
};

namespace {

PF_Err about(PF_OutData *out_data) {
    if (!out_data) {
        return PF_Err_BAD_CALLBACK_PARAM;
    }

    std::strncpy(out_data->return_msg,
                 backtype_strings::kAbout,
                 sizeof(out_data->return_msg) - 1);
    out_data->return_msg[sizeof(out_data->return_msg) - 1] = '\0';
    return PF_Err_NONE;
}

PF_Err global_setup(PF_OutData *out_data) {
    if (!out_data) {
        return PF_Err_BAD_CALLBACK_PARAM;
    }

    out_data->my_version = BACKTYPE_AE_EFFECT_VERSION;
    out_data->out_flags = PF_OutFlag_DEEP_COLOR_AWARE | PF_OutFlag_I_EXPAND_BUFFER;
    out_data->out_flags2 = PF_OutFlag2_SUPPORTS_THREADED_RENDERING;
    return PF_Err_NONE;
}

PF_Err params_setup(PF_InData *in_data, PF_OutData *out_data) {
    if (!in_data || !out_data) {
        return PF_Err_BAD_CALLBACK_PARAM;
    }

    PF_Err err = PF_Err_NONE;
    PF_ParamDef def;

    AEFX_CLR_STRUCT(def);
    PF_ADD_FLOAT_SLIDERX(backtype_strings::kProgress,
                         0.0,
                         100.0,
                         0.0,
                         100.0,
                         100.0,
                         PF_Precision_INTEGER,
                         0,
                         0,
                         BACKTYPE_PROGRESS_DISK_ID);

    AEFX_CLR_STRUCT(def);
    PF_ADD_FLOAT_SLIDERX(backtype_strings::kPositionX,
                         -100.0,
                         200.0,
                         0.0,
                         100.0,
                         50.0,
                         PF_Precision_INTEGER,
                         0,
                         0,
                         BACKTYPE_POSITION_X_DISK_ID);

    AEFX_CLR_STRUCT(def);
    PF_ADD_FLOAT_SLIDERX(backtype_strings::kPositionY,
                         -100.0,
                         200.0,
                         0.0,
                         100.0,
                         50.0,
                         PF_Precision_INTEGER,
                         0,
                         0,
                         BACKTYPE_POSITION_Y_DISK_ID);

    AEFX_CLR_STRUCT(def);
    PF_ADD_POPUP(backtype_strings::kAnchorMode,
                 4,
                 3,
                 "First Character Locked|Newest Character Locked|Center Locked|Last Character Locked",
                 BACKTYPE_ANCHOR_MODE_DISK_ID);

    AEFX_CLR_STRUCT(def);
    PF_ADD_FLOAT_SLIDERX(backtype_strings::kBackwardMotion,
                         0.0,
                         300.0,
                         0.0,
                         300.0,
                         100.0,
                         PF_Precision_INTEGER,
                         0,
                         0,
                         BACKTYPE_BACKWARD_MOTION_DISK_ID);

    AEFX_CLR_STRUCT(def);
    PF_ADD_POPUP(backtype_strings::kDirection,
                 4,
                 1,
                 "Move Left|Move Right|Move Up|Move Down",
                 BACKTYPE_DIRECTION_DISK_ID);

    AEFX_CLR_STRUCT(def);
    PF_ADD_POPUP(backtype_strings::kRevealMode,
                 2,
                 1,
                 "Character|Word",
                 BACKTYPE_REVEAL_MODE_DISK_ID);

    AEFX_CLR_STRUCT(def);
    PF_ADD_CHECKBOXX(backtype_strings::kCursorEnabled,
                     TRUE,
                     0,
                     BACKTYPE_CURSOR_ENABLED_DISK_ID);

    AEFX_CLR_STRUCT(def);
    PF_ADD_POPUP(backtype_strings::kCursorStyle,
                 3,
                 1,
                 "Line|Block|Underscore",
                 BACKTYPE_CURSOR_STYLE_DISK_ID);

    AEFX_CLR_STRUCT(def);
    PF_ADD_FLOAT_SLIDERX(backtype_strings::kCursorBlinkSpeed,
                         0.0,
                         10.0,
                         0.0,
                         10.0,
                         2.0,
                         PF_Precision_INTEGER,
                         0,
                         0,
                         BACKTYPE_CURSOR_BLINK_SPEED_DISK_ID);

    AEFX_CLR_STRUCT(def);
    PF_ADD_FLOAT_SLIDERX(backtype_strings::kCursorOffset,
                         -100.0,
                         100.0,
                         -100.0,
                         100.0,
                         8.0,
                         PF_Precision_INTEGER,
                         0,
                         0,
                         BACKTYPE_CURSOR_OFFSET_DISK_ID);

    AEFX_CLR_STRUCT(def);
    PF_ADD_FLOAT_SLIDERX(backtype_strings::kCharacterFade,
                         0.0,
                         100.0,
                         0.0,
                         100.0,
                         0.0,
                         PF_Precision_INTEGER,
                         0,
                         0,
                         BACKTYPE_CHARACTER_FADE_DISK_ID);

    AEFX_CLR_STRUCT(def);
    PF_ADD_FLOAT_SLIDERX(backtype_strings::kRenderPadding,
                         0.0,
                         200.0,
                         0.0,
                         200.0,
                         8.0,
                         PF_Precision_INTEGER,
                         0,
                         0,
                         BACKTYPE_RENDER_PADDING_DISK_ID);

    AEFX_CLR_STRUCT(def);
    PF_ADD_FLOAT_SLIDERX(backtype_strings::kOpacity,
                         0.0,
                         100.0,
                         0.0,
                         100.0,
                         100.0,
                         PF_Precision_INTEGER,
                         0,
                         0,
                         BACKTYPE_OPACITY_DISK_ID);

    out_data->num_params = BACKTYPE_NUM_PARAMS;
    return err;
}

double slider_value(PF_ParamDef *param, double fallback) {
    if (!param) {
        return fallback;
    }
    const double value = static_cast<double>(param->u.fs_d.value);
    return std::isfinite(value) ? value : fallback;
}

int popup_value(PF_ParamDef *param, int fallback) {
    return param ? static_cast<int>(param->u.pd.value) : fallback;
}

bool checkbox_value(PF_ParamDef *param, bool fallback) {
    return param ? param->u.bd.value != 0 : fallback;
}

PF_Err frame_setup(PF_InData *in_data, PF_OutData *out_data, PF_ParamDef *params[]) {
    if (!in_data || !out_data || !params || !params[BACKTYPE_INPUT]) {
        return PF_Err_BAD_CALLBACK_PARAM;
    }

    const double requested_padding = std::clamp(
        slider_value(params[BACKTYPE_RENDER_PADDING], 8.0), 0.0, 200.0);
    const double scale_x = in_data->downsample_x.den != 0
                               ? static_cast<double>(in_data->downsample_x.num) /
                                     static_cast<double>(in_data->downsample_x.den)
                               : 1.0;
    const double scale_y = in_data->downsample_y.den != 0
                               ? static_cast<double>(in_data->downsample_y.num) /
                                     static_cast<double>(in_data->downsample_y.den)
                               : 1.0;
    const A_long padding_x = static_cast<A_long>(std::ceil(requested_padding * scale_x));
    const A_long padding_y = static_cast<A_long>(std::ceil(requested_padding * scale_y));
    const PF_LayerDef &input = params[BACKTYPE_INPUT]->u.ld;
    out_data->width = input.width + 2 * padding_x;
    out_data->height = input.height + 2 * padding_y;
    out_data->origin.h = static_cast<A_short>(padding_x);
    out_data->origin.v = static_cast<A_short>(padding_y);
    return PF_Err_NONE;
}

PF_Err render(PF_InData *in_data, PF_ParamDef *params[], PF_LayerDef *output) {
    if (!in_data || !params || !output || !output->data) {
        return PF_Err_BAD_CALLBACK_PARAM;
    }

    PF_LayerDef *input = params[BACKTYPE_INPUT] ? &params[BACKTYPE_INPUT]->u.ld : nullptr;
    if (!input || !input->data) {
        return PF_Err_BAD_CALLBACK_PARAM;
    }

    const auto deep_flag = static_cast<PF_WorldFlags>(PF_WorldFlag_DEEP);
    const bool input_is_deep = (input->world_flags & deep_flag) != 0;
    const bool output_is_deep = (output->world_flags & deep_flag) != 0;
    if (input_is_deep != output_is_deep) {
        return PF_Err_BAD_CALLBACK_PARAM;
    }
    const backtype::PixelFormat pixel_format = output_is_deep
                                                   ? backtype::PixelFormat::Argb16
                                                   : backtype::PixelFormat::Argb8;
    backtype::PixelBuffer target;
    target.data = output->data;
    target.width = output->width;
    target.height = output->height;
    target.rowbytes = output->rowbytes;
    target.format = pixel_format;

    backtype::PixelBuffer source;
    source.data = input->data;
    source.width = input->width;
    source.height = input->height;
    source.rowbytes = input->rowbytes;
    source.format = pixel_format;
    if (!backtype::valid_pixel_buffer(source) || !backtype::valid_pixel_buffer(target)) {
        return PF_Err_BAD_CALLBACK_PARAM;
    }

    backtype::clear_target(target);

    const backtype::RasterBounds source_bounds = backtype::find_alpha_bounds(source);
    if (!source_bounds.found) {
        return PF_Err_NONE;
    }

    const double position_x = (slider_value(params[BACKTYPE_POSITION_X], 50.0) / 100.0) * input->width +
                              static_cast<double>(in_data->output_origin_x);
    const double position_y = (slider_value(params[BACKTYPE_POSITION_Y], 50.0) / 100.0) * input->height +
                              static_cast<double>(in_data->output_origin_y);
    const double backward_motion = std::clamp(slider_value(params[BACKTYPE_BACKWARD_MOTION], 100.0), 0.0, 300.0);
    const double cursor_offset = std::clamp(slider_value(params[BACKTYPE_CURSOR_OFFSET], 8.0), -100.0, 100.0);
    const double opacity = backtype::clamp_percent(slider_value(params[BACKTYPE_OPACITY], 100.0)) / 100.0;
    const double character_fade = backtype::clamp_percent(slider_value(params[BACKTYPE_CHARACTER_FADE], 0.0));
    const double progress = backtype::clamp_progress(slider_value(params[BACKTYPE_PROGRESS], 100.0));
    const auto reveal_mode = static_cast<backtype::RevealMode>(
        std::clamp(popup_value(params[BACKTYPE_REVEAL_MODE], 1), 1, 2));
    const auto direction = static_cast<backtype::Direction>(
        std::clamp(popup_value(params[BACKTYPE_DIRECTION], 1), 1, 4));
    const backtype::RasterRevealState reveal = backtype::compute_raster_reveal(
        source, source_bounds, progress, reveal_mode, direction, character_fade);
    const backtype::TextBounds visible_bounds = backtype::visible_raster_bounds(source_bounds, reveal, direction);
    const double stable_height = static_cast<double>(source_bounds.height());

    const backtype::LayoutInput layout{
        static_cast<backtype::AnchorMode>(std::clamp(popup_value(params[BACKTYPE_ANCHOR_MODE], 3), 1, 4)),
        direction,
        position_x,
        position_y,
        backward_motion};

    const backtype::DrawPosition draw_position = backtype::compute_draw_position(
        layout,
        visible_bounds,
        {static_cast<double>(source_bounds.width()), stable_height});

    const double comp_seconds = in_data->time_scale != 0
                                    ? static_cast<double>(in_data->current_time) / static_cast<double>(in_data->time_scale)
                                    : 0.0;

    backtype::copy_revealed_raster(source,
                                   target,
                                   source_bounds,
                                   draw_position,
                                   reveal,
                                   direction,
                                   opacity);

    const bool draw_cursor = checkbox_value(params[BACKTYPE_CURSOR_ENABLED], true) &&
                             backtype::cursor_visible(comp_seconds, slider_value(params[BACKTYPE_CURSOR_BLINK_SPEED], 2.0));
    if (draw_cursor) {
        const backtype::Color cursor_color = backtype::average_alpha_color(source, source_bounds);
        const auto cursor_style = static_cast<backtype::CursorStyle>(
            std::clamp(popup_value(params[BACKTYPE_CURSOR_STYLE], 1), 1, 3));
        const backtype::DrawPosition cursor_position = backtype::cursor_position_for_reveal(
            draw_position,
            source_bounds,
            reveal,
            direction,
            cursor_offset);
        backtype::draw_cursor(target,
                              {cursor_style,
                               direction,
                               cursor_position,
                               static_cast<double>(source_bounds.width()),
                               stable_height,
                               std::max(1.0, stable_height * 0.06),
                               cursor_color,
                               opacity});
    }

    return PF_Err_NONE;
}

} // namespace

extern "C" DllExport PF_Err PluginDataEntryFunction2(PF_PluginDataPtr inPtr,
                                                      PF_PluginDataCB2 inPluginDataCallBackPtr,
                                                      SPBasicSuite *inSPBasicSuitePtr,
                                                      const char *inHostName,
                                                      const char *inHostVersion) {
    (void)inSPBasicSuitePtr;
    (void)inHostName;
    (void)inHostVersion;

    PF_Err result = PF_Err_INVALID_CALLBACK;
    PF_REGISTER_EFFECT_EXT2(inPtr,
                            inPluginDataCallBackPtr,
                            BACKTYPE_NAME,
                            BACKTYPE_MATCH_NAME,
                            BACKTYPE_CATEGORY,
                            AE_RESERVED_INFO,
                            "EffectMain",
                            "https://github.com/williamm0/BackType");
    return result;
}

extern "C" DllExport PF_Err EffectMain(PF_Cmd cmd,
                                       PF_InData *in_data,
                                       PF_OutData *out_data,
                                       PF_ParamDef *params[],
                                       PF_LayerDef *output,
                                       void *extra) {
    PF_Err err = PF_Err_NONE;

    switch (cmd) {
        case PF_Cmd_ABOUT:
            err = about(out_data);
            break;
        case PF_Cmd_GLOBAL_SETUP:
            err = global_setup(out_data);
            break;
        case PF_Cmd_PARAMS_SETUP:
            err = params_setup(in_data, out_data);
            break;
        case PF_Cmd_FRAME_SETUP:
            err = frame_setup(in_data, out_data, params);
            break;
        case PF_Cmd_RENDER:
            try {
                err = render(in_data, params, output);
            } catch (const std::bad_alloc &) {
                err = PF_Err_OUT_OF_MEMORY;
            } catch (...) {
                err = PF_Err_INTERNAL_STRUCT_DAMAGED;
            }
            break;
        default:
            break;
    }

    (void)extra;
    return err;
}
