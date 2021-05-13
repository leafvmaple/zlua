#include <assert.h>

#include "zlua.h"
#include "event.h"
#include "connect.h"

#ifdef LUA_51
int lua_absindex(lua_State* L, int idx) {
    if (idx > 0) {
        return idx;
    }
    return lua_gettop(L) + idx + 1;
}
#endif

int zlua_listen(const char* host, int port, char* err){
    conn_t* conn = conn_new();
    if (!conn)
        return 0;

    event_init();
    return conn_listen(conn, host, port, err);
}

int zlua_add(lua_State* L) {
    event_add_state(L);
    return true;
}