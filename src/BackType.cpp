#include "BackType.h"
#include "BackType_Enums.h"
#include "BackType_Strings.h"
#include "BackType_TextLogic.h"
#include "TextRenderer.h"

#include "AEConfig.h"

#ifdef AE_OS_WIN
#include <Windows.h>
#endif

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
#include <string>
#include <vector>

enum BackTypeParam {
    BACKTYPE_INPUT = 0,
    BACKTYPE_TEXT,
    BACKTYPE_PROGRESS,
    BACKTYPE_FONT_SIZE,
    BACKTYPE_TEXT_COLOR,
    BACKTYPE_POSITION_X,
    BACKTYPE_POSITION_Y,
    BACKTYPE_ANCHOR_MODE,
    BACKTYPE_BACKWARD_MOTION,
    BACKTYPE_DIRECTION,
    BACKTYPE_REVEAL_MODE,
    BACKTYPE_CURSOR_ENABLED,
    BACKTYPE_CURSOR_CHARACTER,
    BACKTYPE_CURSOR_BLINK_SPEED,
    BACKTYPE_CURSOR_OFFSET,
    BACKTYPE_POP_AMOUNT,
    BACKTYPE_JITTER_AMOUNT,
    BACKTYPE_OPACITY,
    BACKTYPE_NUM_PARAMS
};

enum BackTypeDiskId {
    BACKTYPE_TEXT_DISK_ID = 1,
    BACKTYPE_PROGRESS_DISK_ID,
    BACKTYPE_FONT_SIZE_DISK_ID,
    BACKTYPE_TEXT_COLOR_DISK_ID,
    BACKTYPE_POSITION_X_DISK_ID,
    BACKTYPE_POSITION_Y_DISK_ID,
    BACKTYPE_ANCHOR_MODE_DISK_ID,
    BACKTYPE_BACKWARD_MOTION_DISK_ID,
    BACKTYPE_DIRECTION_DISK_ID,
    BACKTYPE_REVEAL_MODE_DISK_ID,
    BACKTYPE_CURSOR_ENABLED_DISK_ID,
    BACKTYPE_CURSOR_CHARACTER_DISK_ID,
    BACKTYPE_CURSOR_BLINK_SPEED_DISK_ID,
    BACKTYPE_CURSOR_OFFSET_DISK_ID,
    BACKTYPE_POP_AMOUNT_DISK_ID,
    BACKTYPE_JITTER_AMOUNT_DISK_ID,
    BACKTYPE_OPACITY_DISK_ID
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

    out_data->my_version = PF_VERSION(BACKTYPE_VERSION_MAJOR,
                                      BACKTYPE_VERSION_MINOR,
                                      BACKTYPE_VERSION_PATCH,
                                      PF_Stage_DEVELOP,
                                      BACKTYPE_VERSION_BUILD);
    out_data->out_flags = PF_OutFlag_PIX_INDEPENDENT;
    out_data->out_flags2 = 0;
    return PF_Err_NONE;
}

PF_Err params_setup(PF_InData *in_data, PF_OutData *out_data) {
    if (!in_data || !out_data) {
        return PF_Err_BAD_CALLBACK_PARAM;
    }

    PF_Err err = PF_Err_NONE;
    PF_ParamDef def;

    AEFX_CLR_STRUCT(def);
    // AE's public effect-param API in SDK 25.6 does not expose a native text
    // field. Keep this as a normal popup so applying the effect never registers
    // unsupported no-data controls in the standard Effect Controls panel.
    PF_ADD_POPUP(backtype_strings::kText, 1, 1, BACKTYPE_DEFAULT_TEXT, BACKTYPE_TEXT_DISK_ID);

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
    PF_ADD_FLOAT_SLIDERX(backtype_strings::kFontSize,
                         8.0,
                         300.0,
                         8.0,
                         300.0,
                         72.0,
                         PF_Precision_INTEGER,
                         0,
                         0,
                         BACKTYPE_FONT_SIZE_DISK_ID);

    AEFX_CLR_STRUCT(def);
    PF_ADD_COLOR(backtype_strings::kTextColor, 255, 255, 255, BACKTYPE_TEXT_COLOR_DISK_ID);

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
                 2,
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
    PF_ADD_POPUP(backtype_strings::kCursorCharacter,
                 1,
                 1,
                 "Vertical Bar",
                 BACKTYPE_CURSOR_CHARACTER_DISK_ID);

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
    PF_ADD_FLOAT_SLIDERX(backtype_strings::kPopAmount,
                         0.0,
                         100.0,
                         0.0,
                         100.0,
                         0.0,
                         PF_Precision_INTEGER,
                         0,
                         0,
                         BACKTYPE_POP_AMOUNT_DISK_ID);

    AEFX_CLR_STRUCT(def);
    PF_ADD_FLOAT_SLIDERX(backtype_strings::kJitterAmount,
                         0.0,
                         50.0,
                         0.0,
                         50.0,
                         0.0,
                         PF_Precision_INTEGER,
                         0,
                         0,
                         BACKTYPE_JITTER_AMOUNT_DISK_ID);

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
    return param ? static_cast<double>(param->u.fs_d.value) : fallback;
}

int popup_value(PF_ParamDef *param, int fallback) {
    return param ? static_cast<int>(param->u.pd.value) : fallback;
}

bool checkbox_value(PF_ParamDef *param, bool fallback) {
    return param ? param->u.bd.value != 0 : fallback;
}

std::string string_value(PF_ParamDef *param, const char *fallback) {
    (void)param;
    return fallback;
}

std::vector<std::string> utf8_characters(const std::string &text) {
    std::vector<std::string> chars;
    for (std::size_t i = 0; i < text.size();) {
        const auto ch = static_cast<unsigned char>(text[i]);
        std::size_t length = 1;
        if ((ch & 0xE0U) == 0xC0U) {
            length = 2;
        } else if ((ch & 0xF0U) == 0xE0U) {
            length = 3;
        } else if ((ch & 0xF8U) == 0xF0U) {
            length = 4;
        }
        length = std::min(length, text.size() - i);
        chars.push_back(text.substr(i, length));
        i += length;
    }
    return chars;
}

void render_visible_text(const backtype::PixelBuffer &target,
                         const std::string &visible,
                         double x,
                         double y,
                         double font_size,
                         const backtype::Color &color,
                         double opacity,
                         double jitter_amount,
                         long frame_index) {
    if (visible.empty()) {
        return;
    }

    if (jitter_amount <= 0.0) {
        backtype::render_text(target, {visible, x, y, font_size, color, opacity});
        return;
    }

    double advance = 0.0;
    const auto characters = utf8_characters(visible);
    for (std::size_t i = 0; i < characters.size(); ++i) {
        const double jitter_x = backtype::deterministic_jitter(i, frame_index, jitter_amount);
        const double jitter_y = backtype::deterministic_jitter(i + 8192, frame_index, jitter_amount);
        backtype::render_text(target, {characters[i], x + advance + jitter_x, y + jitter_y, font_size, color, opacity});
        advance += backtype::measure_text(characters[i], font_size).width;
    }
}

PF_Err render(PF_InData *in_data, PF_ParamDef *params[], PF_LayerDef *output) {
    if (!in_data || !params || !output || !output->data) {
        return PF_Err_NONE;
    }

    backtype::PixelBuffer target;
    target.data = output->data;
    target.width = output->width;
    target.height = output->height;
    target.rowbytes = output->rowbytes;
    target.format = backtype::PixelFormat::Argb8;
    backtype::clear_target(target);

    const std::string text = string_value(params[BACKTYPE_TEXT], BACKTYPE_DEFAULT_TEXT);
    const double progress = backtype::clamp_progress(slider_value(params[BACKTYPE_PROGRESS], 100.0));
    if (text.empty() || progress <= 0.0) {
        return PF_Err_NONE;
    }

    const auto reveal_mode = static_cast<backtype::RevealMode>(
        std::clamp(popup_value(params[BACKTYPE_REVEAL_MODE], 1), 1, 2));
    const std::string visible = backtype::visible_text(text, progress, reveal_mode);
    if (visible.empty()) {
        return PF_Err_NONE;
    }

    const double font_size = std::clamp(slider_value(params[BACKTYPE_FONT_SIZE], 72.0), 8.0, 300.0);
    const double position_x = (slider_value(params[BACKTYPE_POSITION_X], 50.0) / 100.0) * output->width;
    const double position_y = (slider_value(params[BACKTYPE_POSITION_Y], 50.0) / 100.0) * output->height;
    const double backward_motion = std::clamp(slider_value(params[BACKTYPE_BACKWARD_MOTION], 100.0), 0.0, 300.0);
    const double cursor_offset = std::clamp(slider_value(params[BACKTYPE_CURSOR_OFFSET], 8.0), -100.0, 100.0);
    const double jitter_amount = std::clamp(slider_value(params[BACKTYPE_JITTER_AMOUNT], 0.0), 0.0, 50.0);
    const double opacity = backtype::clamp_percent(slider_value(params[BACKTYPE_OPACITY], 100.0)) / 100.0;
    const double pop_amount = std::clamp(slider_value(params[BACKTYPE_POP_AMOUNT], 0.0), 0.0, 100.0);
    (void)pop_amount; // TODO(v0.2): scale the newest character around its center as it appears.

    PF_Pixel color_param;
    color_param.red = 255;
    color_param.green = 255;
    color_param.blue = 255;
    color_param.alpha = 255;
    if (params[BACKTYPE_TEXT_COLOR]) {
        color_param = params[BACKTYPE_TEXT_COLOR]->u.cd.value;
    }
    backtype::Color color;
    color.r = static_cast<double>(color_param.red) / 255.0;
    color.g = static_cast<double>(color_param.green) / 255.0;
    color.b = static_cast<double>(color_param.blue) / 255.0;
    color.a = 1.0;

    const backtype::TextMetrics visible_metrics = backtype::measure_text(visible, font_size);
    const backtype::TextMetrics full_metrics = backtype::measure_text(text, font_size);

    const backtype::LayoutInput layout{
        static_cast<backtype::AnchorMode>(std::clamp(popup_value(params[BACKTYPE_ANCHOR_MODE], 2), 1, 4)),
        static_cast<backtype::Direction>(std::clamp(popup_value(params[BACKTYPE_DIRECTION], 1), 1, 4)),
        position_x,
        position_y,
        backward_motion};

    const backtype::DrawPosition draw_position = backtype::compute_draw_position(
        layout,
        {visible_metrics.width, visible_metrics.height},
        {full_metrics.width, full_metrics.height});

    const double comp_seconds = in_data->time_scale != 0
                                    ? static_cast<double>(in_data->current_time) / static_cast<double>(in_data->time_scale)
                                    : 0.0;
    const long frame_index = in_data->time_step != 0 ? static_cast<long>(in_data->current_time / in_data->time_step) : 0;

    render_visible_text(target,
                        visible,
                        draw_position.x,
                        draw_position.y,
                        font_size,
                        color,
                        opacity,
                        jitter_amount,
                        frame_index);

    const bool draw_cursor = checkbox_value(params[BACKTYPE_CURSOR_ENABLED], true) &&
                             backtype::cursor_visible(comp_seconds, slider_value(params[BACKTYPE_CURSOR_BLINK_SPEED], 2.0));
    if (draw_cursor) {
        const std::string cursor = string_value(params[BACKTYPE_CURSOR_CHARACTER], BACKTYPE_DEFAULT_CURSOR);
        if (!cursor.empty()) {
            backtype::render_text(target,
                                  {cursor,
                                   draw_position.x + visible_metrics.width + cursor_offset,
                                   draw_position.y,
                                   font_size,
                                   color,
                                   opacity});
        }
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
