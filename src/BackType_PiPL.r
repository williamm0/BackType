#include "AE_EffectVers.h"

#ifndef AE_OS_WIN
    #ifndef AE_OS_MAC
        #define AE_OS_MAC 1
    #endif
    #include <AE_General.r>
#endif

#ifndef PF_OutFlag_PIX_INDEPENDENT
    #define PF_OutFlag_PIX_INDEPENDENT 0x00000400
#endif

#ifndef PF_OutFlag_CUSTOM_UI
    #define PF_OutFlag_CUSTOM_UI 0x00008000
#endif

#define BACKTYPE_VERSION_MAJOR 0
#define BACKTYPE_VERSION_MINOR 1
#define BACKTYPE_VERSION_PATCH 1
#define BACKTYPE_VERSION_BUILD 1

resource 'PiPL' (16000) {
    {
        Kind { AEEffect },
        Name { "BackType" },
        Category { "jx plugins" },

#ifdef AE_OS_WIN
        CodeWin64X86 { "EffectMain" },
#else
        CodeMacIntel64 { "EffectMain" },
        CodeMacARM64 { "EffectMain" },
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
            PF_OutFlag_PIX_INDEPENDENT | PF_OutFlag_CUSTOM_UI
        },
        AE_Effect_Global_OutFlags_2 {
            0
        },
        AE_Effect_Match_Name { "jx.BackType" },
        AE_Reserved_Info { 0 },
        AE_Effect_Support_URL { "https://github.com/williamm0/BackType" }
    }
};
