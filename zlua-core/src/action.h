#pragma once

#include "event.h"

struct lua_State;

void do_action(Action action, lua_State* L);