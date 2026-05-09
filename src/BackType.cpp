#include "BackType.h"
#include "BackType_Enums.h"
#include "BackType_RasterLogic.h"
#include "BackType_Strings.h"
#include "BackType_TextLogic.h"

#include "AEConfig.h"

#ifdef AE_OS_WIN
#include <Windows.h>
#endif

#include "entry.h"
#include "AE_Effect.h"
#include "AE_EffectCB.h"
#include "AE_GeneralPlug.h"
#include "AE_Macros.h"
#include "AE_PluginData.h"
#include "Param_Utils.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <new>
#include <string>

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

AEGP_PluginID s_aegp_plugin_id = 0;

template <typename SuiteT>
class SuiteLease {
public:
    SuiteLease(SPBasicSuite *basic, const char *name, int version)
        : basic_(basic), name_(name), version_(version) {
        if (basic_) {
            const void *suite = nullptr;
            if (!basic_->AcquireSuite(name_, version_, &suite)) {
                suite_ = reinterpret_cast<SuiteT *>(const_cast<void *>(suite));
            }
        }
    }

    ~SuiteLease() {
        if (basic_ && suite_) {
            basic_->ReleaseSuite(name_, version_);
        }
    }

    SuiteT *operator->() const {
        return suite_;
    }

    explicit operator bool() const {
        return suite_ != nullptr;
    }

private:
    SPBasicSuite *basic_ = nullptr;
    const char *name_ = nullptr;
    int version_ = 0;
    SuiteT *suite_ = nullptr;
};

PF_Err ensure_aegp_registration(PF_InData *in_data) {
    if (s_aegp_plugin_id || !in_data || !in_data->pica_basicP) {
        return PF_Err_NONE;
    }

    SuiteLease<AEGP_UtilitySuite6> utility(in_data->pica_basicP, kAEGPUtilitySuite, kAEGPUtilitySuiteVersion6);
    if (!utility) {
        return PF_Err_NONE;
    }

    AEGP_PluginID plugin_id = 0;
    if (!utility->AEGP_RegisterWithAEGP(nullptr, BACKTYPE_NAME, &plugin_id)) {
        s_aegp_plugin_id = plugin_id;
    }
    return PF_Err_NONE;
}

std::string utf16_to_utf8(const A_u_short *text, std::size_t length) {
    std::string result;
    result.reserve(length);

    for (std::size_t i = 0; i < length; ++i) {
        std::uint32_t codepoint = text[i];
        if (codepoint == 0) {
            break;
        }

        if (codepoint >= 0xD800 && codepoint <= 0xDBFF && i + 1 < length) {
            const std::uint32_t low = text[i + 1];
            if (low >= 0xDC00 && low <= 0xDFFF) {
                codepoint = 0x10000 + ((codepoint - 0xD800) << 10) + (low - 0xDC00);
                ++i;
            }
        }

        if (codepoint <= 0x7F) {
            result.push_back(static_cast<char>(codepoint));
        } else if (codepoint <= 0x7FF) {
            result.push_back(static_cast<char>(0xC0 | (codepoint >> 6)));
            result.push_back(static_cast<char>(0x80 | (codepoint & 0x3F)));
        } else if (codepoint <= 0xFFFF) {
            result.push_back(static_cast<char>(0xE0 | (codepoint >> 12)));
            result.push_back(static_cast<char>(0x80 | ((codepoint >> 6) & 0x3F)));
            result.push_back(static_cast<char>(0x80 | (codepoint & 0x3F)));
        } else {
            result.push_back(static_cast<char>(0xF0 | (codepoint >> 18)));
            result.push_back(static_cast<char>(0x80 | ((codepoint >> 12) & 0x3F)));
            result.push_back(static_cast<char>(0x80 | ((codepoint >> 6) & 0x3F)));
            result.push_back(static_cast<char>(0x80 | (codepoint & 0x3F)));
        }
    }

    return result;
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

    out_data->my_version = BACKTYPE_AE_EFFECT_VERSION;
    out_data->out_flags = 0;
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
    return param ? static_cast<double>(param->u.fs_d.value) : fallback;
}

int popup_value(PF_ParamDef *param, int fallback) {
    return param ? static_cast<int>(param->u.pd.value) : fallback;
}

bool checkbox_value(PF_ParamDef *param, bool fallback) {
    return param ? param->u.bd.value != 0 : fallback;
}

std::string read_layer_source_text(PF_InData *in_data) {
    if (!in_data || !in_data->effect_ref || !in_data->pica_basicP) {
        return "";
    }

    if (ensure_aegp_registration(in_data) || !s_aegp_plugin_id) {
        return "";
    }

    SuiteLease<AEGP_PFInterfaceSuite1> pf_interface(in_data->pica_basicP,
                                                    kAEGPPFInterfaceSuite,
                                                    kAEGPPFInterfaceSuiteVersion1);
    SuiteLease<AEGP_StreamSuite6> stream_suite(in_data->pica_basicP,
                                               kAEGPStreamSuite,
                                               kAEGPStreamSuiteVersion6);
    SuiteLease<AEGP_TextDocumentSuite1> text_suite(in_data->pica_basicP,
                                                   kAEGPTextDocumentSuite,
                                                   kAEGPTextDocumentSuiteVersion1);
    SuiteLease<AEGP_MemorySuite1> memory_suite(in_data->pica_basicP,
                                               kAEGPMemorySuite,
                                               kAEGPMemorySuiteVersion1);
    if (!pf_interface || !stream_suite || !text_suite || !memory_suite) {
        return "";
    }

    AEGP_LayerH layer = nullptr;
    AEGP_StreamRefH text_stream = nullptr;
    AEGP_StreamValue2 stream_value;
    AEFX_CLR_STRUCT(stream_value);
    AEGP_MemHandle unicode_handle = nullptr;
    bool have_stream_value = false;
    std::string text;

    const A_Time layer_time{in_data->current_time, in_data->time_scale != 0 ? in_data->time_scale : 1};

    if (pf_interface->AEGP_GetEffectLayer(in_data->effect_ref, &layer) || !layer) {
        return "";
    }
    if (stream_suite->AEGP_GetNewLayerStream(s_aegp_plugin_id, layer, AEGP_LayerStream_SOURCE_TEXT, &text_stream) ||
        !text_stream) {
        return "";
    }

    AEGP_StreamType stream_type = AEGP_StreamType_NO_DATA;
    if (!stream_suite->AEGP_GetStreamType(text_stream, &stream_type) &&
        stream_type == AEGP_StreamType_TEXT_DOCUMENT &&
        !stream_suite->AEGP_GetNewStreamValue(s_aegp_plugin_id,
                                              text_stream,
                                              AEGP_LTimeMode_LayerTime,
                                              &layer_time,
                                              FALSE,
                                              &stream_value)) {
        have_stream_value = true;
    }

    if (have_stream_value &&
        stream_value.val.text_documentH &&
        !text_suite->AEGP_GetNewText(s_aegp_plugin_id, stream_value.val.text_documentH, &unicode_handle) &&
        unicode_handle) {
        AEGP_MemSize bytes = 0;
        A_u_short *unicode = nullptr;
        if (!memory_suite->AEGP_GetMemHandleSize(unicode_handle, &bytes) &&
            !memory_suite->AEGP_LockMemHandle(unicode_handle, reinterpret_cast<void **>(&unicode)) &&
            unicode) {
            const std::size_t max_units = static_cast<std::size_t>(bytes / sizeof(A_u_short));
            std::size_t length = 0;
            while (length < max_units && unicode[length] != 0) {
                ++length;
            }
            text = utf16_to_utf8(unicode, length);
            memory_suite->AEGP_UnlockMemHandle(unicode_handle);
        }
    }

    if (unicode_handle) {
        memory_suite->AEGP_FreeMemHandle(unicode_handle);
    }
    if (have_stream_value) {
        stream_suite->AEGP_DisposeStreamValue(&stream_value);
    }
    if (text_stream) {
        stream_suite->AEGP_DisposeStream(text_stream);
    }
    return text;
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

    PF_LayerDef *input = params[BACKTYPE_INPUT] ? &params[BACKTYPE_INPUT]->u.ld : nullptr;
    if (!input || !input->data || input->width <= 0 || input->height <= 0 ||
        input->rowbytes < input->width * 4 || output->rowbytes < output->width * 4 ||
        PF_WORLD_IS_DEEP(input) || PF_WORLD_IS_DEEP(output)) {
        return PF_Err_NONE;
    }

    const std::string text = read_layer_source_text(in_data);
    const double progress = backtype::clamp_progress(slider_value(params[BACKTYPE_PROGRESS], 100.0));
    if (text.empty()) {
        return PF_Err_NONE;
    }

    const auto reveal_mode = static_cast<backtype::RevealMode>(
        std::clamp(popup_value(params[BACKTYPE_REVEAL_MODE], 1), 1, 2));
    backtype::PixelBuffer source;
    source.data = input->data;
    source.width = input->width;
    source.height = input->height;
    source.rowbytes = input->rowbytes;
    source.format = backtype::PixelFormat::Argb8;

    const backtype::RasterBounds source_bounds = backtype::find_alpha_bounds(source);
    if (!source_bounds.found) {
        return PF_Err_NONE;
    }

    const double position_x = (slider_value(params[BACKTYPE_POSITION_X], 50.0) / 100.0) * output->width;
    const double position_y = (slider_value(params[BACKTYPE_POSITION_Y], 50.0) / 100.0) * output->height;
    const double backward_motion = std::clamp(slider_value(params[BACKTYPE_BACKWARD_MOTION], 100.0), 0.0, 300.0);
    const double cursor_offset = std::clamp(slider_value(params[BACKTYPE_CURSOR_OFFSET], 8.0), -100.0, 100.0);
    const double render_padding = std::clamp(slider_value(params[BACKTYPE_RENDER_PADDING], 8.0), 0.0, 200.0);
    const double opacity = backtype::clamp_percent(slider_value(params[BACKTYPE_OPACITY], 100.0)) / 100.0;
    const double character_fade = backtype::clamp_percent(slider_value(params[BACKTYPE_CHARACTER_FADE], 0.0));
    const backtype::RasterRevealState reveal = backtype::compute_raster_reveal(text, progress, reveal_mode, character_fade);
    const auto direction = static_cast<backtype::Direction>(
        std::clamp(popup_value(params[BACKTYPE_DIRECTION], 1), 1, 4));
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
                                   render_padding,
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
                               cursor_position,
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
            if (!err) {
                err = ensure_aegp_registration(in_data);
            }
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
