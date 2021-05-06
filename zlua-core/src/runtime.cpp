#include "runtime.h"

#define CACHE_TABLE_NAME "_cache_table_"

int cache_gen = 0;

variable_t* var_clone(variable_t* src) {
    variable_t* res = new variable_t;

    strcpy(res->name, src->name);
    res->name_type = src->name_type;
    strcpy(res->value, src->value);
    res->value_type = src->value_type;
    res->value_type_name = src->value_type_name;

    for (auto child : src->childs) {
        res->childs.push_back(var_clone(child));
    }
    res->cache_id = src->cache_id;

    return res;
}

const char* rt2file(lua_State* L, lua_Debug& ar) {
    const char* res = ar.source;
    if (strlen(res) > 0 && res[0] == '@')
        ++res;
    return res;
}

int rt2meta(lua_State* L, int valueIndex, const char* method, int numResults, int& result) {
    return true;
}

int rt2pt(char* val, lua_State* L, int index) {
    sprintf(val, "(0x%X)", lua_topointer(L, index));
    return true;
}

int rt_add_cache(lua_State* L, int idx, variable_t* var) {
    const int type = lua_type(L, idx);
    if (type == LUA_TUSERDATA || type == LUA_TTABLE) {
        var->cache_id = ++cache_gen;
        const int t1 = lua_gettop(L);
        lua_getfield(L, LUA_REGISTRYINDEX, CACHE_TABLE_NAME);
        if (lua_isnil(L, -1)) {
            lua_pop(L, 1);
            lua_newtable(L);
            lua_setfield(L, LUA_REGISTRYINDEX, CACHE_TABLE_NAME);
            lua_getfield(L, LUA_REGISTRYINDEX, CACHE_TABLE_NAME);
        }
        lua_pushvalue(L, idx);
        char key[10];
        sprintf(key, "%d", cache_gen);
        lua_setfield(L, -2, key);
        lua_settop(L, t1);
    }
    else {
        var->cache_id = 0;
    }
    return true;
}

void rt_clear_cache(lua_State* L) {
    lua_getfield(L, LUA_REGISTRYINDEX, CACHE_TABLE_NAME);
    if (!lua_isnil(L, -1)) {
        lua_pushnil(L);
        lua_setfield(L, LUA_REGISTRYINDEX, CACHE_TABLE_NAME);
    }
    lua_pop(L, 1);
}

int rt2var(variable_t* var, lua_State* L, int index, int depth) {
    const int t1 = lua_gettop(L);
    index = lua_absindex(L, index);
    rt_add_cache(L, index, var);
    int type = lua_type(L, index);

    var->name_type = LUA_TSTRING;
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
            rt2pt(var->value, L, index);
        }
        if (depth > 1) {
            if (lua_getmetatable(L, index)) {
                rt2var(var, L, -1, depth);
                lua_pop(L, 1); // pop meta
            }
        }
        break;
    }
    case LUA_TFUNCTION:
    case LUA_TLIGHTUSERDATA:
    case LUA_TTHREAD: {
        rt2pt(var->value, L, index);
        break;
    }
    case LUA_TTABLE: {
        int size = 0;
        lua_pushnil(L);
        while (lua_next(L, index)) {
            if (depth > 1) {
                variable_t* v = new variable_t;
                const auto name_type = lua_type(L, -2);
                v->name_type = name_type;
                if (name_type == LUA_TSTRING || name_type == LUA_TNUMBER || name_type == LUA_TBOOLEAN) {
                    lua_pushvalue(L, -2);
                    strcpy(v->name, lua_tostring(L, -1));
                    lua_pop(L, 1);
                }
                else {
                    rt2pt(v->name, L, -2);
                }
                rt2var(v, L, -1, depth - 1);
                var->childs.push_back(v);
            }
            lua_pop(L, 1); // pop value
            size++;
        }

        if (lua_getmetatable(L, index)) {
            variable_t* meta = new variable_t;
            strcpy(meta->name, "metatable");
            meta->name_type = LUA_TSTRING;

            rt2var(meta, L, -1, 2);
            var->childs.push_back(meta);

            {
                lua_getfield(L, -1, "__index");
                if (!lua_isnil(L, -1)) {
                    variable_t v;
                    rt2var(&v, L, -1, 2);
                    if (depth > 1) {
                        for (auto* child : v.childs) {
                            var->childs.push_back(var_clone(child));
                        }
                    }
                    size += v.childs.size();
                }
                lua_pop(L, 1);
            }

            lua_pop(L, 1);
        }

        sprintf(var->value, "table(0x%X, size = %d)", lua_topointer(L, index), size);
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

        auto stack = new stack_t{ rt2file(L, ar), ar.name, level, ar.currentline };
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

            strcpy(var->name, name);
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
                stack->upvalue_vars.push_back(var);

                strcpy(var->name, name);
                rt2var(var, L, -1, 1);
                lua_pop(L, 1);
            }
            // pop function
            lua_pop(L, 1);
        }

    }
    return true;
}

int rt_eval_cache(eval_t& eval, lua_State* L)
{
    if (eval.cache_id > 0) {
        lua_getfield(L, LUA_REGISTRYINDEX, CACHE_TABLE_NAME);
        if (lua_type(L, -1) == LUA_TTABLE) {
            char key[10];
            sprintf(key, "%d", eval.cache_id);;
            lua_getfield(L, -1, key);

            strcpy(eval.result.name, eval.expr);
            rt2var(&eval.result, L, -1, eval.depth);
            lua_pop(L, 1);

            eval.success = true;
        }
        lua_pop(L, 1);
    }
    return eval.success;
}