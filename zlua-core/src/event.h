#pragma once

#include "zlua.h"
#include <set>

struct conn_t;

enum class MESSAGE {
    UNKNOWN,

    INIT_REQUEST,
    INTI_RESPOND,

    READY_REQUEST,
    READY_RESPOND,

    ADD_BREAK_POINT_REQUEST,
    ADD_BREAK_POINT_RESPOND,

    REMOVE_BREAK_POINT_REQUEST,
    REMOVE_BREAK_POINT_RESPOND,

    ActionReq,
    ActionRsp,

    EvalReq,
    EvalRsp,

    // debugger -> ide
    BREAK_NOTIFY,
    AttachedNotify,

    StartHookReq,
    StartHookRsp,

    // debugger -> ide
    LogNotify,

    COUNT,
};

void event_init();
void event_add_state(lua_State* L);

int event_handle(conn_t* conn, const char* buf, size_t len);