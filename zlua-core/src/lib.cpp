#include <assert.h>

#include "zlua.h"

#include "runtime.h"
#include "event.h"
#include "connect.h"

Connection* conn = nullptr;

#ifdef LUA_51
int lua_absindex(lua_State* L, int idx) {
    if (idx > 0) {
        return idx;
    }
    return lua_gettop(L) + idx + 1;
}
#endif

int zlua_init(lua_State* L) {
    RuntimeInit();
    EventInit(L);

    lua_newtable(L);
    lua_setglobal(L, "zlua");

    return true;
}

int zlua_listen(const char* host, int port, char* err) {
    conn = new Connection;
    if (!conn)
        return 0;

    return conn->Listen(host, port, err);
}

int zlua_set_parser(int type, parser_proto parser) {
    Variable::SetCustomParser(type, parser);
    return true;
}