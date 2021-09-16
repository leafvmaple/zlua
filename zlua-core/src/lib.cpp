#include <assert.h>

#include "zlua.h"
#include "event.h"
#include "connect.h"
#include "runtime.h"

#ifdef LUA_51
int lua_absindex(lua_State* L, int idx) {
    if (idx > 0) {
        return idx;
    }
    return lua_gettop(L) + idx + 1;
}
#endif

int zlua_init(lua_State* L) {
    rt_init();
    event_init(L);

    lua_newtable(L, -1);
    lua_setglobal(L, "zlua");

    return true;
}

int zlua_listen(const char* host, int port, char* err){
    conn_t* conn = conn_new();
    if (!conn)
        return 0;

    return conn_listen(conn, host, port, err);
}

int zlua_set_parser(int type, fn_parser parser) {
    return rt_set_parser(type, parser);
}