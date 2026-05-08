#include "BackType.h"
#include "BackType_Enums.h"
#include "BackType_Strings.h"
#include "BackType_TextData.h"
#include "BackType_TextLogic.h"
#include "TextRenderer.h"

#include "AEConfig.h"

#ifdef AE_OS_WIN
#include <Windows.h>
#endif

#include "entry.h"
#include "AE_Effect.h"
#include "AE_EffectCB.h"
#include "AE_EffectUI.h"
#include "AEFX_SuiteHelper.h"
#include "AE_Macros.h"
#include "AE_PluginData.h"
#include "Param_Utils.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <new>
#include <string>
#include <vector>

enum BackTypeParam {
    BACKTYPE_INPUT = 0,
    BACKTYPE_TEXT,
    BACKTYPE_TEXT_SOURCE,
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
    BACKTYPE_TEXT_SOURCE_DISK_ID,
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

constexpr int kTextControlWidth = 260;
constexpr int kTextControlHeight = 24;
void *const kTextArbitraryRefcon = reinterpret_cast<void *>(static_cast<uintptr_t>(0x42545458));

PF_ArbitraryH create_text_handle(PF_InData *in_data, const std::string &text) {
    if (!in_data || !in_data->utils) {
        return nullptr;
    }

    PF_ArbitraryH handle = PF_NEW_HANDLE(sizeof(BackTypeTextData));
    if (!handle) {
        return nullptr;
    }

    auto *data = reinterpret_cast<BackTypeTextData *>(PF_LOCK_HANDLE(handle));
    if (!data) {
        PF_DISPOSE_HANDLE(handle);
        return nullptr;
    }

    backtype::set_text_data(data, text);
    PF_UNLOCK_HANDLE(handle);
    return handle;
}

std::string text_from_handle(PF_InData *in_data, PF_ArbitraryH handle, const char *fallback) {
    if (!in_data || !handle) {
        return fallback ? fallback : "";
    }

    auto *data = reinterpret_cast<const BackTypeTextData *>(PF_LOCK_HANDLE(handle));
    if (!data) {
        return fallback ? fallback : "";
    }

    const std::string text = backtype::text_from_data(data);
    PF_UNLOCK_HANDLE(handle);
    return text;
}

PF_Err set_text_handle(PF_InData *in_data, PF_ParamDef *param, const std::string &text) {
    if (!in_data || !param || !param->u.arb_d.value) {
        return PF_Err_BAD_CALLBACK_PARAM;
    }

    auto *data = reinterpret_cast<BackTypeTextData *>(PF_LOCK_HANDLE(param->u.arb_d.value));
    if (!data) {
        return PF_Err_OUT_OF_MEMORY;
    }

    backtype::set_text_data(data, text);
    PF_UNLOCK_HANDLE(param->u.arb_d.value);
    param->uu.change_flags = PF_ChangeFlag_CHANGED_VALUE;
    return PF_Err_NONE;
}

std::vector<DRAWBOT_UTF16Char> utf8_to_drawbot_utf16(const std::string &text) {
    std::vector<DRAWBOT_UTF16Char> output;
    output.reserve(text.size() + 1);

    for (std::size_t i = 0; i < text.size();) {
        unsigned int cp = static_cast<unsigned char>(text[i]);
        std::size_t length = 1;

        if ((cp & 0xE0U) == 0xC0U && i + 1 < text.size()) {
            cp = ((cp & 0x1FU) << 6U) | (static_cast<unsigned char>(text[i + 1]) & 0x3FU);
            length = 2;
        } else if ((cp & 0xF0U) == 0xE0U && i + 2 < text.size()) {
            cp = ((cp & 0x0FU) << 12U) |
                 ((static_cast<unsigned char>(text[i + 1]) & 0x3FU) << 6U) |
                 (static_cast<unsigned char>(text[i + 2]) & 0x3FU);
            length = 3;
        } else if ((cp & 0xF8U) == 0xF0U && i + 3 < text.size()) {
            cp = ((cp & 0x07U) << 18U) |
                 ((static_cast<unsigned char>(text[i + 1]) & 0x3FU) << 12U) |
                 ((static_cast<unsigned char>(text[i + 2]) & 0x3FU) << 6U) |
                 (static_cast<unsigned char>(text[i + 3]) & 0x3FU);
            length = 4;
        }

        if (cp <= 0xFFFFU) {
            output.push_back(static_cast<DRAWBOT_UTF16Char>(cp));
        } else {
            cp -= 0x10000U;
            output.push_back(static_cast<DRAWBOT_UTF16Char>(0xD800U + (cp >> 10U)));
            output.push_back(static_cast<DRAWBOT_UTF16Char>(0xDC00U + (cp & 0x3FFU)));
        }

        i += length;
    }

    output.push_back(0);
    return output;
}

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
    out_data->out_flags = PF_OutFlag_PIX_INDEPENDENT | PF_OutFlag_CUSTOM_UI;
    out_data->out_flags2 = 0;
    return PF_Err_NONE;
}

PF_Err params_setup(PF_InData *in_data, PF_OutData *out_data) {
    if (!in_data || !out_data) {
        return PF_Err_BAD_CALLBACK_PARAM;
    }

    PF_Err err = PF_Err_NONE;
    PF_ParamDef def;

    PF_ArbitraryH default_text = create_text_handle(in_data, BACKTYPE_DEFAULT_TEXT);
    if (!default_text) {
        return PF_Err_OUT_OF_MEMORY;
    }

    AEFX_CLR_STRUCT(def);
    PF_ADD_ARBITRARY2(backtype_strings::kText,
                      kTextControlWidth,
                      kTextControlHeight,
                      PF_ParamFlag_SUPERVISE,
                      PF_PUI_CONTROL,
                      default_text,
                      BACKTYPE_TEXT_DISK_ID,
                      kTextArbitraryRefcon);

    AEFX_CLR_STRUCT(def);
    PF_ADD_POPUP(backtype_strings::kTextSource,
                 2,
                 1,
                 "Plugin Text|Layer Text",
                 BACKTYPE_TEXT_SOURCE_DISK_ID);

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

    PF_CustomUIInfo custom_ui;
    AEFX_CLR_STRUCT(custom_ui);
    custom_ui.events = PF_CustomEFlag_EFFECT;
    err = (*(in_data->inter.register_ui))(in_data->effect_ref, &custom_ui);

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

std::string string_value(PF_InData *in_data, PF_ParamDef *param, const char *fallback) {
    if (!param) {
        return fallback ? fallback : "";
    }

    if (param->param_type == PF_Param_ARBITRARY_DATA) {
        return text_from_handle(in_data, param->u.arb_d.value, fallback);
    }

    return fallback ? fallback : "";
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

    const auto text_source = static_cast<backtype::TextSource>(
        std::clamp(popup_value(params[BACKTYPE_TEXT_SOURCE], 1), 1, 2));
    (void)text_source; // Layer text is exposed for UX, but render uses Plugin Text until AE source-text access is added.
    const std::string text = string_value(in_data, params[BACKTYPE_TEXT], BACKTYPE_DEFAULT_TEXT);
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
        static_cast<backtype::AnchorMode>(std::clamp(popup_value(params[BACKTYPE_ANCHOR_MODE], 3), 1, 4)),
        static_cast<backtype::Direction>(std::clamp(popup_value(params[BACKTYPE_DIRECTION], 1), 1, 4)),
        position_x,
        position_y,
        backward_motion};

    const backtype::DrawPosition draw_position = backtype::compute_draw_position(
        layout,
        {visible_metrics.width, full_metrics.height},
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
            const std::string cursor = BACKTYPE_DEFAULT_CURSOR;
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

PF_Err draw_text_param_ui(PF_InData *in_data,
                          PF_OutData *out_data,
                          PF_ParamDef *params[],
                          PF_EventExtra *event_extra) {
    if (!in_data || !out_data || !params || !event_extra ||
        event_extra->effect_win.index != BACKTYPE_TEXT ||
        event_extra->effect_win.area != PF_EA_CONTROL) {
        return PF_Err_NONE;
    }

    PF_Err err = PF_Err_NONE;
    PF_Err err2 = PF_Err_NONE;
    DRAWBOT_Suites suites;
    AEFX_CLR_STRUCT(suites);

    DRAWBOT_DrawRef drawing_ref = nullptr;
    DRAWBOT_SurfaceRef surface_ref = nullptr;
    DRAWBOT_SupplierRef supplier_ref = nullptr;
    DRAWBOT_BrushRef background_brush = nullptr;
    DRAWBOT_PenRef border_pen = nullptr;
    DRAWBOT_BrushRef text_brush = nullptr;
    DRAWBOT_PathRef path_ref = nullptr;
    DRAWBOT_FontRef font_ref = nullptr;

    ERR(AEFX_AcquireDrawbotSuites(in_data, out_data, &suites));

    PF_EffectCustomUISuite1 *effect_ui = nullptr;
    ERR(AEFX_AcquireSuite(in_data,
                          out_data,
                          kPFEffectCustomUISuite,
                          kPFEffectCustomUISuiteVersion1,
                          nullptr,
                          reinterpret_cast<void **>(&effect_ui)));
    if (!err && effect_ui) {
        ERR((*effect_ui->PF_GetDrawingReference)(event_extra->contextH, &drawing_ref));
        AEFX_ReleaseSuite(in_data, out_data, kPFEffectCustomUISuite, kPFEffectCustomUISuiteVersion1, nullptr);
    }

    ERR(suites.drawbot_suiteP->GetSupplier(drawing_ref, &supplier_ref));
    ERR(suites.drawbot_suiteP->GetSurface(drawing_ref, &surface_ref));
    ERR(suites.supplier_suiteP->NewPath(supplier_ref, &path_ref));

    DRAWBOT_RectF32 rect;
    rect.left = static_cast<float>(event_extra->effect_win.current_frame.left) + 0.5f;
    rect.top = static_cast<float>(event_extra->effect_win.current_frame.top) + 0.5f;
    rect.width = static_cast<float>(event_extra->effect_win.current_frame.right -
                                    event_extra->effect_win.current_frame.left) -
                 1.0f;
    rect.height = static_cast<float>(event_extra->effect_win.current_frame.bottom -
                                     event_extra->effect_win.current_frame.top) -
                  1.0f;
    ERR(suites.path_suiteP->AddRect(path_ref, &rect));

    DRAWBOT_ColorRGBA color;
    color.red = 0.16f;
    color.green = 0.16f;
    color.blue = 0.16f;
    color.alpha = 1.0f;
    ERR(suites.supplier_suiteP->NewBrush(supplier_ref, &color, &background_brush));
    ERR(suites.surface_suiteP->FillPath(surface_ref, background_brush, path_ref, kDRAWBOT_FillType_Default));

    color.red = 0.34f;
    color.green = 0.34f;
    color.blue = 0.34f;
    ERR(suites.supplier_suiteP->NewPen(supplier_ref, &color, 1.0f, &border_pen));
    ERR(suites.surface_suiteP->StrokePath(surface_ref, border_pen, path_ref));

    float default_font_size = 0.0f;
    ERR(suites.supplier_suiteP->GetDefaultFontSize(supplier_ref, &default_font_size));
    ERR(suites.supplier_suiteP->NewDefaultFont(supplier_ref, default_font_size, &font_ref));

    color.red = 0.92f;
    color.green = 0.92f;
    color.blue = 0.92f;
    color.alpha = 1.0f;
    ERR(suites.supplier_suiteP->NewBrush(supplier_ref, &color, &text_brush));

    const std::string text = string_value(in_data, params[BACKTYPE_TEXT], BACKTYPE_DEFAULT_TEXT);
    const auto utf16 = utf8_to_drawbot_utf16(text.empty() ? std::string(" ") : text);

    DRAWBOT_PointF32 origin;
    origin.x = rect.left + 6.0f;
    origin.y = rect.top + rect.height - 7.0f;
    ERR(suites.surface_suiteP->DrawString(surface_ref,
                                          text_brush,
                                          font_ref,
                                          utf16.data(),
                                          &origin,
                                          kDRAWBOT_TextAlignment_Default,
                                          kDRAWBOT_TextTruncation_EndEllipsis,
                                          rect.width - 12.0f));

    if (text_brush) {
        ERR2(suites.supplier_suiteP->ReleaseObject(reinterpret_cast<DRAWBOT_ObjectRef>(text_brush)));
    }
    if (font_ref) {
        ERR2(suites.supplier_suiteP->ReleaseObject(reinterpret_cast<DRAWBOT_ObjectRef>(font_ref)));
    }
    if (border_pen) {
        ERR2(suites.supplier_suiteP->ReleaseObject(reinterpret_cast<DRAWBOT_ObjectRef>(border_pen)));
    }
    if (background_brush) {
        ERR2(suites.supplier_suiteP->ReleaseObject(reinterpret_cast<DRAWBOT_ObjectRef>(background_brush)));
    }
    if (path_ref) {
        ERR2(suites.supplier_suiteP->ReleaseObject(reinterpret_cast<DRAWBOT_ObjectRef>(path_ref)));
    }
    ERR2(AEFX_ReleaseDrawbotSuites(in_data, out_data));

    if (!err) {
        event_extra->evt_out_flags = PF_EO_HANDLED_EVENT;
    }
    return err;
}

PF_Err handle_text_param_key(PF_InData *in_data,
                             PF_ParamDef *params[],
                             PF_EventExtra *event_extra) {
    if (!in_data || !params || !event_extra ||
        event_extra->effect_win.index != BACKTYPE_TEXT ||
        event_extra->effect_win.area != PF_EA_CONTROL) {
        return PF_Err_NONE;
    }

    const PF_KeyCode keycode = event_extra->u.key_down.keycode;
    std::string text = string_value(in_data, params[BACKTYPE_TEXT], BACKTYPE_DEFAULT_TEXT);
    bool changed = false;

    if (PF_KEYCODE_IS_PRINTABLE(keycode)) {
        const unsigned int cp = static_cast<unsigned int>(PF_KEYCODE_GET_SHORTCUT_CHARACTER(keycode));
        if (cp >= 32U) {
            text = backtype::append_utf8_character(text, cp);
            changed = true;
        }
    } else {
        const auto control = static_cast<PF_ControlCode>(PF_KEYCODE_GET_CONTROL_CODE(keycode));
        if (control == PF_ControlCode_Space) {
            text.push_back(' ');
            changed = true;
        } else if (control == PF_ControlCode_Backspace || control == PF_ControlCode_Delete) {
            text = backtype::erase_last_utf8_character(text);
            changed = true;
        }
    }

    if (!changed) {
        return PF_Err_NONE;
    }

    PF_Err err = set_text_handle(in_data, params[BACKTYPE_TEXT], text);
    if (!err) {
        event_extra->evt_out_flags = PF_EO_HANDLED_EVENT | PF_EO_UPDATE_NOW;
    }
    return err;
}

PF_Err handle_event(PF_InData *in_data,
                    PF_OutData *out_data,
                    PF_ParamDef *params[],
                    PF_EventExtra *event_extra) {
    if (!event_extra) {
        return PF_Err_BAD_CALLBACK_PARAM;
    }

    switch (event_extra->e_type) {
        case PF_Event_DRAW:
            return draw_text_param_ui(in_data, out_data, params, event_extra);
        case PF_Event_KEYDOWN:
            return handle_text_param_key(in_data, params, event_extra);
        case PF_Event_DO_CLICK:
            if (event_extra->effect_win.index == BACKTYPE_TEXT &&
                event_extra->effect_win.area == PF_EA_CONTROL) {
                event_extra->evt_out_flags = PF_EO_HANDLED_EVENT;
            }
            return PF_Err_NONE;
        case PF_Event_ADJUST_CURSOR:
            if (event_extra->effect_win.index == BACKTYPE_TEXT &&
                event_extra->effect_win.area == PF_EA_CONTROL) {
                event_extra->u.adjust_cursor.set_cursor = PF_Cursor_HORZ_I_BEAM;
                event_extra->evt_out_flags = PF_EO_HANDLED_EVENT;
            }
            return PF_Err_NONE;
        default:
            return PF_Err_NONE;
    }
}

PF_Err handle_arbitrary(PF_InData *in_data, PF_ArbParamsExtra *extra) {
    if (!in_data || !extra) {
        return PF_Err_BAD_CALLBACK_PARAM;
    }

    PF_Err err = PF_Err_NONE;
    switch (extra->which_function) {
        case PF_Arbitrary_NEW_FUNC:
            if (extra->u.new_func_params.refconPV == kTextArbitraryRefcon) {
                *extra->u.new_func_params.arbPH = create_text_handle(in_data, BACKTYPE_DEFAULT_TEXT);
                err = *extra->u.new_func_params.arbPH ? PF_Err_NONE : PF_Err_OUT_OF_MEMORY;
            }
            break;

        case PF_Arbitrary_DISPOSE_FUNC:
            if (extra->u.dispose_func_params.arbH) {
                PF_DISPOSE_HANDLE(extra->u.dispose_func_params.arbH);
            }
            break;

        case PF_Arbitrary_COPY_FUNC:
            if (extra->u.copy_func_params.src_arbH && extra->u.copy_func_params.dst_arbPH) {
                const std::string text = text_from_handle(in_data, extra->u.copy_func_params.src_arbH, "");
                *extra->u.copy_func_params.dst_arbPH = create_text_handle(in_data, text);
                err = *extra->u.copy_func_params.dst_arbPH ? PF_Err_NONE : PF_Err_OUT_OF_MEMORY;
            }
            break;

        case PF_Arbitrary_FLAT_SIZE_FUNC:
            if (extra->u.flat_size_func_params.flat_data_sizePLu) {
                *extra->u.flat_size_func_params.flat_data_sizePLu = sizeof(BackTypeTextData);
            }
            break;

        case PF_Arbitrary_FLATTEN_FUNC:
            if (extra->u.flatten_func_params.arbH &&
                extra->u.flatten_func_params.flat_dataPV &&
                extra->u.flatten_func_params.buf_sizeLu >= sizeof(BackTypeTextData)) {
                auto *src = reinterpret_cast<const BackTypeTextData *>(PF_LOCK_HANDLE(extra->u.flatten_func_params.arbH));
                if (src) {
                    std::memcpy(extra->u.flatten_func_params.flat_dataPV, src, sizeof(BackTypeTextData));
                    PF_UNLOCK_HANDLE(extra->u.flatten_func_params.arbH);
                } else {
                    err = PF_Err_OUT_OF_MEMORY;
                }
            }
            break;

        case PF_Arbitrary_UNFLATTEN_FUNC:
            if (extra->u.unflatten_func_params.flat_dataPV &&
                extra->u.unflatten_func_params.arbPH &&
                extra->u.unflatten_func_params.buf_sizeLu >= sizeof(BackTypeTextData)) {
                PF_ArbitraryH handle = create_text_handle(in_data, "");
                if (!handle) {
                    err = PF_Err_OUT_OF_MEMORY;
                } else {
                    auto *dst = reinterpret_cast<BackTypeTextData *>(PF_LOCK_HANDLE(handle));
                    if (dst) {
                        std::memcpy(dst, extra->u.unflatten_func_params.flat_dataPV, sizeof(BackTypeTextData));
                        dst->text[kBackTypeTextMaxBytes - 1] = '\0';
                        PF_UNLOCK_HANDLE(handle);
                        *extra->u.unflatten_func_params.arbPH = handle;
                    } else {
                        PF_DISPOSE_HANDLE(handle);
                        err = PF_Err_OUT_OF_MEMORY;
                    }
                }
            }
            break;

        case PF_Arbitrary_INTERP_FUNC:
            if (extra->u.interp_func_params.left_arbH && extra->u.interp_func_params.right_arbH) {
                const std::string left = text_from_handle(in_data, extra->u.interp_func_params.left_arbH, "");
                const std::string right = text_from_handle(in_data, extra->u.interp_func_params.right_arbH, "");
                const std::string chosen = extra->u.interp_func_params.tF < 0.5 ? left : right;
                *extra->u.interp_func_params.interpPH = create_text_handle(in_data, chosen);
                err = *extra->u.interp_func_params.interpPH ? PF_Err_NONE : PF_Err_OUT_OF_MEMORY;
            }
            break;

        case PF_Arbitrary_COMPARE_FUNC:
            if (extra->u.compare_func_params.compareP) {
                const std::string a = text_from_handle(in_data, extra->u.compare_func_params.a_arbH, "");
                const std::string b = text_from_handle(in_data, extra->u.compare_func_params.b_arbH, "");
                *extra->u.compare_func_params.compareP = (a == b) ? PF_ArbCompare_EQUAL : PF_ArbCompare_NOT_EQUAL;
            }
            break;

        case PF_Arbitrary_PRINT_SIZE_FUNC:
            if (extra->u.print_size_func_params.print_sizePLu) {
                *extra->u.print_size_func_params.print_sizePLu = kBackTypeTextMaxBytes;
            }
            break;

        case PF_Arbitrary_PRINT_FUNC:
            if (extra->u.print_func_params.print_bufferPC && extra->u.print_func_params.print_sizeLu > 0) {
                const std::string text = text_from_handle(in_data, extra->u.print_func_params.arbH, "");
                const auto max_copy = static_cast<std::size_t>(extra->u.print_func_params.print_sizeLu - 1);
                const auto copy_size = std::min(text.size(), max_copy);
                std::memcpy(extra->u.print_func_params.print_bufferPC, text.data(), copy_size);
                extra->u.print_func_params.print_bufferPC[copy_size] = '\0';
            }
            break;

        case PF_Arbitrary_SCAN_FUNC:
            if (extra->u.scan_func_params.arbPH && extra->u.scan_func_params.bufPC) {
                const std::string text(extra->u.scan_func_params.bufPC,
                                       extra->u.scan_func_params.bytes_to_scanLu);
                *extra->u.scan_func_params.arbPH = create_text_handle(in_data, text);
                err = *extra->u.scan_func_params.arbPH ? PF_Err_NONE : PF_Err_OUT_OF_MEMORY;
            }
            break;

        default:
            break;
    }

    return err;
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
        case PF_Cmd_EVENT:
            err = handle_event(in_data, out_data, params, reinterpret_cast<PF_EventExtra *>(extra));
            break;
        case PF_Cmd_ARBITRARY_CALLBACK:
            err = handle_arbitrary(in_data, reinterpret_cast<PF_ArbParamsExtra *>(extra));
            break;
        default:
            break;
    }

    (void)extra;
    return err;
}
