#pragma once

#include "zlua.h"

class Connection;

enum class PROTO {
    unknown,

    c2s_init,
    s2c_init,

    c2s_ready,
    s2c_ready,

    c2s_add_break_point,
    s2c_add_break_point,

    c2s_remove_break_point,
    s2c_remove_break_point,

    c2s_action,
    s2c_action,

    c2s_eval,
    s2c_eval,

    s2c_break_point_msg,
    s2c_attach_msg,

    c2s_start_hook,
    s2c_start_hook,

    s2c_log_msg,

    count,
};

enum class Action {
    ide_break,
    ide_continue,
    ide_step_oever,
    ide_step_in,
    ide_step_out,
    ide_stop,
    count,
};


void EventInit(lua_State* L);

int EventHandle(Connection* conn, const char* buf, size_t len);