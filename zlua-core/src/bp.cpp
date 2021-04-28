#include "bp.h"

#include <windows.h>
#include <string>
#include <map>
#include <vector>
#include <mutex>

std::mutex mutex_bp;
std::map<DWORD, std::vector<breakpoint_t>> bp_set;

const char* rt2file(lua_State* L, lua_Debug& ar) {
    const char* res = ar.source;
    if (strlen(res) > 0 && res[0] == '@')
        ++res;
    return res;
}

int rt2meta(lua_State* L, int valueIndex, const char* method, int numResults, int& result) {
    return true;
}

int rt2var(variable_t* var, lua_State* L, int index, int depth) {
    const int t1 = lua_gettop(L);
    index = lua_absindex(L, index);

    int type = lua_type(L, index);

    var->value_type = type;
    var->value_type_name = lua_typename(L, var->value_type);

    switch (type) {
    case LUA_TNIL: {
        strcpy(var->value, "nil");
        break;
    }
    case LUA_TNUMBER: {
        strcpy(var->value, lua_tostring(L, index));
        break;
    }
    case LUA_TBOOLEAN: {
        strcpy(var->value, lua_toboolean(L, index) ? "true" : "false");
        break;
    }
    case LUA_TSTRING: {
        strcpy(var->value, lua_tostring(L, index));
        break;
    }
    case LUA_TUSERDATA: {
        auto string = lua_tostring(L, index);
        if (!string) {
            int result;
            if (rt2meta(L, t1, "__tostring", 1, result) && result == 0) {
                string = lua_tostring(L, -1);
                lua_pop(L, 1);
            }
        }
        if (string) {
            strcpy(var->value, string);
        }
        else {
            sprintf(var->value, "(0x%x)", lua_topointer(L, index));
        }
        if (depth > 1) {
            if (lua_getmetatable(L, index)) {
                rt2var(var, L, -1, depth);
                lua_pop(L, 1); //pop meta
            }
        }
        break;
    }
    case LUA_TFUNCTION:
    case LUA_TLIGHTUSERDATA:
    case LUA_TTHREAD: {
        sprintf(var->value, "(0x%x)", lua_topointer(L, index));
        break;
    }
    }

    return true;
}

int rt2stacks(std::vector<stack_t*>& stacks, lua_State* L) {
    stacks.clear();

    for (int level = 0;; level++) {
        lua_Debug ar;
        if (!lua_getstack(L, level, &ar)) {
            break;
        }
        if (!lua_getinfo(L, "nSlu", &ar)) {
            continue;
        }

        auto stack = new stack_t{rt2file(L, ar), ar.name, level, ar.currentline};
        stacks.push_back(stack);

        for (int i = 1;; i++) {
            const char* name = lua_getlocal(L, &ar, i);
            if (name == nullptr) {
                break;
            }
            if (name[0] == '(') {
                lua_pop(L, 1);
                continue;
            }

            auto var = new variable_t;
            stack->local_vars.push_back(var);

            var->name = name;
            rt2var(var, L, -1, 1);
            lua_pop(L, 1);
        }

        if (lua_getinfo(L, "f", &ar)) {
            const int fIdx = lua_gettop(L);
            for (int i = 1;; i++) {
                const char* name = lua_getupvalue(L, fIdx, i);
                if (!name) {
                    break;
                }

                // add up variable
                auto var = new variable_t;
                stack->local_vars.push_back(var);

                var->name = name;
                rt2var(var, L, -1, 1);
                lua_pop(L, 1);
            }
            // pop function
            lua_pop(L, 1);
        }

    }
    return true;
}

int str2paths(path_t& path, const char* str) {
    char t[STR_LEN];
    if (strlen(str) >= STR_LEN) {
        return false;
    }
    strcpy(t, str);
    for (int i = strlen(t); i >= 0; i--) {
        if (t[i] == '\\' || t[i] == '/') {
            path.vals.push_back(&t[i + 1]);
            t[i] = '\0';
        }
        else {
            t[i] = tolower(t[i]);
        }
    }
    return true;
}

int bp_process(lua_State* L, breakpoint_t* bp) {
    return true;
}

conn_t* bp_find(lua_State* L, lua_Debug* ar) {
    const int cl = ar->currentline;
    if (cl >= 0) {
        auto it_set = bp_set.find(cl);
        if (it_set != bp_set.end()) {
            lua_getinfo(L, "S", ar);
            for (auto bp : it_set->second) {
                path_t path;
                if (str2paths(path, ar->source) && bp.path == path && bp_process(L, &bp))
                    return bp.conn;
            }
        }
    }
    return 0;
}

int bp_clear()
{
    std::lock_guard <std::mutex> lock(mutex_bp);
    bp_set.clear();

    return true;
}

int bp_add(const breakpoint_t& bp) {
    std::lock_guard <std::mutex> lock(mutex_bp);
    bp_set[bp.line].push_back(bp);

    return true;
}