#pragma once

#define BACKTYPE_NAME "BackType"
#define BACKTYPE_MATCH_NAME "jx.BackType"
#define BACKTYPE_CATEGORY "jx plugins"

#define BACKTYPE_VERSION_MAJOR 0
#define BACKTYPE_VERSION_MINOR 1
#define BACKTYPE_VERSION_PATCH 0
#define BACKTYPE_VERSION_BUILD 1

#define BACKTYPE_DEFAULT_TEXT "BackType"
#define BACKTYPE_DEFAULT_CURSOR "|"

/*
    Recent After Effects SDKs expose native string parameters. If your SDK uses a
    different field name for the string value, define BACKTYPE_AE_STRING_VALUE in
    the project settings instead of editing the render code.
*/
#ifndef BACKTYPE_AE_STRING_VALUE
#define BACKTYPE_AE_STRING_VALUE(PARAM_PTR) ((PARAM_PTR)->u.str_d.value)
#endif
