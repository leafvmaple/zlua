#pragma once


#ifndef lua_h

struct lua_State;

#define LUA_IDSIZE	60

struct lua_Debug {
    int event;
    const char* name;	/* (n) */
    const char* namewhat;	/* (n) `global', `local', `field', `method' */
    const char* what;	/* (S) `Lua', `C', `main', `tail' */
    const char* source;	/* (S) */
    int currentline;	/* (l) */
    int nups;		/* (u) number of upvalues */
    int linedefined;	/* (S) */
    int lastlinedefined;	/* (S) */
    char short_src[LUA_IDSIZE]; /* (S) */
    /* private part */
    int i_ci;  /* active function */
};

#define LUA_HOOKCALL	0
#define LUA_HOOKRET	1
#define LUA_HOOKLINE	2
#define LUA_HOOKCOUNT	3
// LUA 5.1
#define LUA_HOOKTAILRET 4
// LUA 5.2+
#define LUA_HOOKTAILCALL 4

#define LUA_MASKCALL	(1 << LUA_HOOKCALL)
#define LUA_MASKRET		(1 << LUA_HOOKRET)
#define LUA_MASKLINE	(1 << LUA_HOOKLINE)
#define LUA_MASKCOUNT	(1 << LUA_HOOKCOUNT)

#define LUA_TNONE		(-1)
#define LUA_TNIL		0
#define LUA_TBOOLEAN		1
#define LUA_TLIGHTUSERDATA	2
#define LUA_TNUMBER		3
#define LUA_TSTRING		4
#define LUA_TTABLE		5
#define LUA_TFUNCTION		6
#define LUA_TUSERDATA		7
#define LUA_TTHREAD		8

#define ZLUA_API extern

extern "C"
{
    typedef void (*lua_Hook) (lua_State* L, lua_Debug* ar);
    typedef int (*lua_CFunction) (lua_State* L);

    ZLUA_API int lua_getstack(lua_State* L, int level, lua_Debug* ar);
    ZLUA_API int lua_getinfo(lua_State* L, const char* what, lua_Debug* ar);
    ZLUA_API const char* lua_getlocal(lua_State* L, const lua_Debug* ar, int n);
    ZLUA_API const char* lua_getupvalue(lua_State* L, int funcindex, int n);
    ZLUA_API int lua_getmetatable(lua_State* L, int objindex);

    ZLUA_API void lua_getfield(lua_State* L, int idx, const char* k);
    ZLUA_API void lua_setfield(lua_State* L, int idx, const char* k);
    ZLUA_API void lua_rawset(lua_State* L, int idx);

    ZLUA_API int lua_sethook(lua_State* L, lua_Hook func, int mask, int count);

    ZLUA_API void lua_settop(lua_State* L, int idx);
    ZLUA_API int lua_gettop(lua_State* L);
    ZLUA_API void lua_remove(lua_State* L, int idx);

    ZLUA_API void lua_rawget(lua_State* L, int idx);

    ZLUA_API int lua_absindex(lua_State* L, int idx);
    ZLUA_API int lua_type(lua_State* L, int idx);
    ZLUA_API int lua_toboolean(lua_State* L, int idx);

    ZLUA_API const char* lua_typename(lua_State* L, int t);
    ZLUA_API const char* lua_tolstring(lua_State* L, int idx, size_t* len);
    ZLUA_API size_t lua_objlen (lua_State* L, int idx);
    ZLUA_API const void* lua_topointer(lua_State* L, int index);
    
    ZLUA_API int lua_next(lua_State* L, int idx);

    ZLUA_API void lua_pushnil(lua_State* L);
    ZLUA_API void lua_pushstring(lua_State* L, const char* s);
    ZLUA_API void lua_pushcclosure(lua_State* L, lua_CFunction fn, int n);
    ZLUA_API void lua_pushvalue(lua_State* L, int idx);

    ZLUA_API void lua_createtable(lua_State* L, int narr, int nrec);

    ZLUA_API int lua_pcall(lua_State* L, int nargs, int nresults, int errfunc);
}

#endif

#ifndef LUA_GLOBALSINDEX
#define LUA_GLOBALSINDEX (-10002)
#endif

#ifndef LUA_REGISTRYINDEX
#define LUA_REGISTRYINDEX (-10000)
#endif

#ifndef LUA_OK
#define LUA_OK 0
#endif

#ifndef lua_pop
#define lua_pop(L,n) lua_settop(L, -(n)-1)
#endif

#ifndef lua_tostring
#define lua_tostring(L,i) lua_tolstring(L, (i), NULL)
#endif

#ifndef lua_isnil
#define lua_isnil(L,n) (lua_type(L, (n)) == LUA_TNIL)
#endif


#ifndef lua_newtable
#define lua_newtable(L) lua_createtable(L, 0, 0)
#endif

#ifndef lua_pushcfunction
#define lua_pushcfunction(L,f) lua_pushcclosure(L, (f), 0)
#endif

#ifndef lua_getglobal
#define lua_getglobal(L,s) lua_getfield(L, LUA_GLOBALSINDEX, (s))
#endif

#ifndef lua_setglobal
#define lua_setglobal(L,s) lua_setfield(L, LUA_GLOBALSINDEX, (s))
#endif

#ifndef lua_isfunction
#define lua_isfunction(L,n) (lua_type(L, (n)) == LUA_TFUNCTION)
#endif

#ifndef lua_istable
#define lua_istable(L,n) (lua_type(L, (n)) == LUA_TTABLE)
#endif

#ifndef lua_strlen
#define lua_strlen(L,i)		lua_objlen(L, (i))
#endif

#define ZLUA_FILE_MAX 260
#define ZLUA_TYPE_COUNT 9


//#define EMMY

#ifdef LUA_51
extern "C" {
    int lua_absindex(lua_State* L, int idx);
}
#endif

typedef int (*fn_parser)(lua_State* L);

int zlua_init();
int zlua_listen(const char* host, int port, char* err = nullptr);
int zlua_add_state(lua_State* L);

extern int zlua_set_parser(int type, fn_parser parser);