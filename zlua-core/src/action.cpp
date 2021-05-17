#include "action.h"
#include "model.h"
#include "runtime.h"

#include <mutex>
#include <map>

std::mutex mutex_run;
std::condition_variable cv;

struct action_t;
typedef int (*fn)(action_t* action, lua_State* L, lua_Debug* ar);

struct action_t {
    char file[ZLUA_FILE_MAX];
    int line = 0;
    int stack_level;

    conn_t* conn;
    fn cur_action;
};

std::map<conn_t*, action_t> actions;

void action_break(lua_State* L) {
    std::unique_lock<std::mutex> lock(mutex_run);
    cv.wait(lock);

    rt_clear_cache(L);
}

int get_stack_level(lua_State* L) {
    int level = 0;
    lua_Debug ar{};
    for (int i = 0; lua_getstack(L, i, &ar); i++) {
        lua_getinfo(L, "l", &ar);
        if (ar.currentline >= 0)
            level++;
    }
    return level;
}

int update_step_over(action_t* action, lua_State* L, lua_Debug* ar) {
    int stack_level = get_stack_level(L);

    if (stack_level < action->stack_level) {
        return true;
    }
    if (stack_level == action->stack_level && ar->currentline != action->line) {
        lua_getinfo(L, "Sl", ar);
        if (!strcmp(action->file, ar->source) || action->line == -1) {
            return true;
        }
    }

    return false;
}

int update_step_in(action_t* action, lua_State* L, lua_Debug* ar) {
    if (ar->currentline != action->line) {
        return true;
    }

    return false;
}

int update_step_out(action_t* action, lua_State* L, lua_Debug* ar) {
    int stack_level = get_stack_level(L);

    if (stack_level < action->stack_level) {
        return true;
    }

    return false;
}

conn_t* action_find(lua_State* L, lua_Debug* ar) {
    for (auto& it : actions) {
        action_t* action = &it.second;
        if (action->cur_action) {
            if (action->cur_action(action, L, ar)) {
                memset(&it.second, 0, sizeof(it.second));
                return it.first;
            }
        }
    }

    return 0;
}

void action_continue(conn_t* conn, lua_State* L) {
    cv.notify_all();
}

void action_step_over(conn_t* conn, lua_State* L) {
    lua_Debug ar{};
    action_t* action = &actions[conn];

    action->stack_level = get_stack_level(L);

    lua_getstack(L, 0, &ar);
    lua_getinfo(L, "nSl", &ar);
    strcpy(action->file, ar.source);
    action->line = ar.currentline;

    cv.notify_all();
    action->cur_action = update_step_over;
}

void action_step_in(conn_t* conn, lua_State* L) {
    action_t* action = &actions[conn];

    action->stack_level = get_stack_level(L);

    cv.notify_all();
    action->cur_action = update_step_in;
}

void action_step_out(conn_t* conn, lua_State* L) {
    lua_Debug ar{};
    action_t* action = &actions[conn];

    action->stack_level = get_stack_level(L);

    lua_getstack(L, 0, &ar);
    lua_getinfo(L, "nSl", &ar);
    strcpy(action->file, ar.source);
    action->line = ar.currentline;

    cv.notify_all();
    action->cur_action = update_step_out;
}

void action_stop(conn_t* conn, lua_State* L) {
    bp_clear();

    memset(&actions[conn], 0, sizeof(actions[conn]));

    cv.notify_all();
}

void action_excute(Action action, conn_t* conn, lua_State* L) {
    switch (action)
    {
    case Action::ide_break:
        action_break(L);
        break;
    case Action::ide_continue:
        action_continue(conn, L);
        break;
    case Action::ide_step_oever:
        action_step_over(conn, L);
        break;
    case Action::ide_step_in:
        action_step_in(conn, L);
        break;
    case Action::ide_step_out:
        action_step_out(conn, L);
        break;
    case Action::ide_stop:
        action_stop(conn, L);
        break;
    default:
        break;
    }
}