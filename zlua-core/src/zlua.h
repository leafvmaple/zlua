#pragma once

#include "./../external/lauxlib.h"

#ifdef ZLUA_EXPORT
#define ZLUA_API __declspec(dllexport)
#else
#define ZLUA_API __declspec(dllimport)
#endif

#define ZLUA_FILE_MAX 260
#define ZLUA_TYPE_COUNT 9

//#define EMMY

#ifdef LUA_51
extern "C" {
    int lua_absindex(lua_State* L, int idx);
}
#endif

typedef int (*parser_proto)(lua_State* L);

ZLUA_API int zlua_init(lua_State* L);
ZLUA_API int zlua_listen(const char* host, int port, char* err = nullptr);

ZLUA_API int zlua_set_parser(int type, parser_proto parser);