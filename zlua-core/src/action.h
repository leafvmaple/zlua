#pragma once

#include "event.h"

struct lua_State;

Connection* action_find(lua_State* L, lua_Debug* ar);
void action_break(lua_State* L);
void action_excute(Action action, Connection* conn, lua_State* L);