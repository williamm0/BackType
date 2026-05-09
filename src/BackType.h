#pragma once

#define BACKTYPE_NAME "BackType"
#define BACKTYPE_MATCH_NAME "jx.BackType"
#define BACKTYPE_CATEGORY "jx plugins"

#define BACKTYPE_VERSION_MAJOR 0
#define BACKTYPE_VERSION_MINOR 1
#define BACKTYPE_VERSION_PATCH 3
#define BACKTYPE_VERSION_BUILD 2

// Keep this in sync with PF_VERSION(0, 1, 3, PF_Stage_DEVELOP, build).
// The PiPL resource cannot safely include the full AE_Effect.h macro set, so
// both code and resource share this encoded value instead.
#define BACKTYPE_AE_EFFECT_VERSION 0x00009802
