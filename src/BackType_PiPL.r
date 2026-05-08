#include "AEConfig.h"
#include "AE_Effect.h"
#include "AE_EffectVers.h"
#include "PiPL.r"

#define BACKTYPE_VERSION_MAJOR 0
#define BACKTYPE_VERSION_MINOR 1
#define BACKTYPE_VERSION_PATCH 0
#define BACKTYPE_VERSION_BUILD 1

resource 'PiPL' (16000) {
    {
        Kind { AEEffect },
        Name { "BackType" },
        Category { "jx plugins" },

#ifdef AE_OS_WIN
        CodeWin64X86 { "EntryPointFunc" },
#else
        CodeMacIntel64 { "EntryPointFunc" },
        CodeMacARM64 { "EntryPointFunc" },
#endif

        AE_PiPL_Version { 2, 0 },
        AE_Effect_Spec_Version { PF_PLUG_IN_VERSION, PF_PLUG_IN_SUBVERS },
        AE_Effect_Version {
            (BACKTYPE_VERSION_MAJOR << 19) |
            (BACKTYPE_VERSION_MINOR << 15) |
            (BACKTYPE_VERSION_PATCH << 4) |
            BACKTYPE_VERSION_BUILD
        },
        AE_Effect_Info_Flags { 0 },
        AE_Effect_Global_OutFlags {
            PF_OutFlag_PIX_INDEPENDENT |
            PF_OutFlag_USE_OUTPUT_EXTENT
        },
        AE_Effect_Global_OutFlags_2 {
            PF_OutFlag2_SUPPORTS_THREADED_RENDERING
        },
        AE_Effect_Match_Name { "jx.BackType" },
        AE_Reserved_Info { 0 }
    }
};
