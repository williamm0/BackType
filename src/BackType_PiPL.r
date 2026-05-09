#include "AE_EffectVers.h"
#include "BackType.h"

#ifndef AE_OS_WIN
    #ifndef AE_OS_MAC
        #define AE_OS_MAC 1
    #endif
    #include <AE_General.r>
#endif

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
        AE_Effect_Version { BACKTYPE_AE_EFFECT_VERSION },
        AE_Effect_Info_Flags { 0 },
        AE_Effect_Global_OutFlags {
            0
        },
        AE_Effect_Global_OutFlags_2 {
            0
        },
        AE_Effect_Match_Name { "jx.BackType" },
        AE_Reserved_Info { 0 },
        AE_Effect_Support_URL { "https://github.com/williamm0/BackType" }
    }
};
