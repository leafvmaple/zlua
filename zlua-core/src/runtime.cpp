#include "runtime.h"

#define CACHE_TABLE_NAME "_cache_table_"

int cache_gen = 0;

struct ZString {
    char s[ZLUA_FILE_MAX];
    ZString(const char* in) {
        strcpy(s, in);
    }
};

fn_parser parsers[ZLUA_TYPE_COUNT];
int filters[ZLUA_TYPE_COUNT];
std::vector<ZString> globals;


int rt2var(variable_t* var, lua_State* L, int index, int depth);

int rt_set_parser(int type, fn_parser parser) {
    parsers[type] = parser;
    return true;
}

int rt_set_filter(int type, int enable)
{
    filters[type] = enable;
    return true;
}

int rt_add_global(const char* global)
{
    globals.push_back(global);
    return true;
}


void rt_parser(variable_t* var, lua_State* L) {
    int top = lua_gettop(L);

    do 
    {
        int succes = parsers[var->value_type] && parsers[var->value_type](L);

        if (!succes) {
            lua_getglobal(L, "zlua");
            if (!lua_istable(L, -1))
                break;

            lua_getfield(L, -1, "parser");
            if (!lua_isfunction(L, -1))
                break;

            lua_pushvalue(L, top);
            int res = lua_pcall(L, 1, 1, 0);
            if (res || !lua_istable(L, -1))
                break;
        }

        if (!lua_istable(L, -1))
            break;

        lua_pushnil(L);
        while (lua_next(L, -2)) {
            variable_t v;
            v.name_type = lua_type(L, -1);

            lua_pushvalue(L, -2);
            strcpy(v.name, lua_tostring(L, -1));
            lua_pop(L, 1);

            if (rt2var(&v, L, -1, 1)) {
                var->childs.push_back(v);
            }

            lua_pop(L, 1);
        }

    } while (0);

    lua_settop(L, top);
}

/*variable_t* var_clone(variable_t* src) {
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
}*/

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

void rt_init()
{
    memset(parsers, 0, sizeof(parsers));
    memset(filters, 0, sizeof(filters));
    globals.clear();
}

void rt_clear_cache(lua_State* L) {
    lua_getfield(L, LUA_REGISTRYINDEX, CACHE_TABLE_NAME);
    if (!lua_isnil(L, -1)) {
        lua_pushnil(L);
        lua_setfield(L, LUA_REGISTRYINDEX, CACHE_TABLE_NAME);
    }
    lua_pop(L, 1);
}

const char* rt2file(lua_State* L, lua_Debug& ar) {
    const char* res = ar.source;
    if (strlen(res) > 0 && res[0] == '@')
        ++res;
#ifdef _WIN32
    if (strlen(res) > 0 && (res[0] == '\\' || res[0] == '/'))
        ++res;
#endif
    return res;
}

int rt2meta(lua_State* L, int idx, const char* method, int param_cnt, int& result) {
    if (lua_getmetatable(L, idx)) {
        const int meta_idx = lua_gettop(L);
        if (!lua_isnil(L, meta_idx)) {
            lua_pushstring(L, method);
            lua_rawget(L, meta_idx);
            if (lua_isnil(L, -1)) {
                lua_pop(L, 1);
                lua_remove(L, meta_idx);
                return false;
            }
            lua_pushvalue(L, idx);
            result = lua_pcall(L, 1, param_cnt, 0);
        }
        lua_remove(L, meta_idx);
        return true;
    }
    return false;
}

int rt2pt(char* val, lua_State* L, int index) {
    sprintf(val, "%s(0x%X)", lua_typename(L, lua_type(L, index)), lua_topointer(L, index));
    return true;
}

int rt2var(variable_t* var, lua_State* L, int index, int depth) {
    const int t1 = lua_gettop(L);
    index = lua_absindex(L, index);
    
    int type = lua_type(L, index);

    if (filters[type]) {
        return false;
    }

    var->name_type = LUA_TSTRING;
    var->value_type = type;
    var->value_type_name = lua_typename(L, var->value_type);
    var->cache_id = 0;

    if (type == LUA_TUSERDATA || type == LUA_TTABLE) {
        rt_add_cache(L, index, var);
        if (depth > 1) {
            rt_parser(var, L);
        }
    }

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
        strncpy(var->value, lua_tostring(L, index), ZLUA_FILE_MAX);
        var->value[ZLUA_FILE_MAX - 1] = '/0';
        break;
    }
    case LUA_TUSERDATA: {
        auto string = lua_tostring(L, index);
        if (!string) {
            int result;
            if (rt2meta(L, t1, "__tostring", 1, result) && !result) {
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
                variable_t v;
                const auto name_type = lua_type(L, -2);
                v.name_type = name_type;
                if (name_type == LUA_TSTRING || name_type == LUA_TNUMBER || name_type == LUA_TBOOLEAN) {
                    lua_pushvalue(L, -2);
                    strcpy(v.name, lua_tostring(L, -1));
                    lua_pop(L, 1);
                }
                else {
                    rt2pt(v.name, L, -2);
                }

                if (rt2var(&v, L, -1, depth - 1)) {
                    var->childs.push_back(v);
                }
            }
            lua_pop(L, 1);
            size++;
        }

        if (lua_getmetatable(L, index)) {
            variable_t meta;
            strcpy(meta.name, "metatable");
            meta.name_type = LUA_TSTRING;

            if (rt2var(&meta, L, -1, 2)) {
                var->childs.push_back(meta);
            }

            {
                lua_getfield(L, -1, "__index");
                if (!lua_isnil(L, -1)) {
                    variable_t v;
                    rt2var(&v, L, -1, 2);
                    if (depth > 1) {
                        for (auto child : v.childs) {
                            var->childs.push_back(child);
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

int rt2stacks(std::vector<stack_t>& stacks, lua_State* L) {
    stacks.clear();

    for (int level = 0;; level++) {
        lua_Debug ar;
        if (!lua_getstack(L, level, &ar)) {
            break;
        }
        if (!lua_getinfo(L, "nSlu", &ar)) {
            continue;
        }

        stack_t stack{ rt2file(L, ar), ar.name, level, ar.currentline };

        for (int i = 1;; i++) {
            const char* name = lua_getlocal(L, &ar, i);
            if (name == nullptr) {
                break;
            }
            if (name[0] == '(') {
                lua_pop(L, 1);
                continue;
            }

            variable_t var ;
            strcpy(var.name, name);
            if (rt2var(&var, L, -1, 1)) {
                stack.local_vars.push_back(var);
            }
            lua_pop(L, 1);
        }

        if (lua_getinfo(L, "f", &ar)) {
            const int fIdx = lua_gettop(L);
            for (int i = 1;; i++) {
                const char* name = lua_getupvalue(L, fIdx, i);
                if (!name) {
                    break;
                }

                variable_t var;
                strcpy(var.name, name);
                if (rt2var(&var, L, -1, 1)) {
                    stack.upvalue_vars.push_back(var);
                }

                lua_pop(L, 1);
            }
            // pop function
            lua_pop(L, 1);
        }

        for (auto global : globals) {
            lua_getglobal(L, global.s);
            if (!lua_isnil(L, -1)) {
                variable_t var;
                strcpy(var.name, global.s);
                if (rt2var(&var, L, -1, 1)) {
                    stack.global_vars.push_back(var);
                }
            }
            lua_pop(L, 1);
        }
        stacks.push_back(stack);
    }
    return true;
}

int cclosure(lua_State* L) {
    const int locals = lua_upvalueindex(1);
    const int upvalues = lua_upvalueindex(2);
    const char* name = lua_tostring(L, 2);

    lua_getfield(L, upvalues, name);
    if (!lua_isnil(L, -1)) {
        return 1;
    }
    lua_pop(L, 1);

    lua_getfield(L, locals, name);
    if (!lua_isnil(L, -1)) {
        return 1;
    }
    lua_pop(L, 1);

    lua_getglobal(L, name);
    if (!lua_isnil(L, -1)) {
        return 1;
    }
    lua_pop(L, 1);

    return 0;
}

int create_env(lua_State* L, int level) {
    lua_Debug ar {};
    if (!lua_getstack(L, level, &ar)) {
        return false;
    }
    if (!lua_getinfo(L, "nSlu", &ar)) {
        return false;
    }

    lua_newtable(L);
    const int env = lua_gettop(L);
    lua_newtable(L);
    const int meta = lua_gettop(L);
    lua_newtable(L);
    const int locals = lua_gettop(L);
    lua_newtable(L);
    const int upvalues = lua_gettop(L);

    for (int i = 1; ; i++) {
        const char* name = lua_getlocal(L, &ar, i);
        if (name == nullptr)
            break;
        if (name[0] == '(') {
            lua_pop(L, 1);
            continue;
        }
        lua_setfield(L, locals, name);
    }
 
    if (lua_getinfo(L, "f", &ar)) {
        const int fIdx = lua_gettop(L);
        for (int i = 1; ; i++) {
            const char* name = lua_getupvalue(L, fIdx, i);
            if (name == nullptr)
                break;
            lua_setfield(L, upvalues, name);
        }
        lua_pop(L, 1);
    }
    int top = lua_gettop(L);

    lua_pushcclosure(L, cclosure, 2);

    lua_setfield(L, meta, "__index");
    lua_setmetatable(L, env);

    top = lua_gettop(L);
    return true;
}

int rt_eval(eval_t& eval, lua_State* L)
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
    else {
        do 
        {
            char expr[ZLUA_FILE_MAX];
            sprintf(expr, "return %s", eval.expr);

            int r = luaL_loadstring(L, expr);
            if (r == LUA_ERRSYNTAX) {
                sprintf(eval.error, "syntax err:  %s", eval.expr);
                break;
            }

            const int idx = lua_gettop(L);

            if (!create_env(L, eval.level))
                break;

            lua_setfenv(L, idx);

            r = lua_pcall(L, 0, 1, 0);
            if (!r) {
                strcpy(eval.result.name, eval.expr);
                rt2var(&eval.result, L, -1, eval.depth);
                lua_pop(L, 1);
                eval.success = true;
            }
            if (r == LUA_ERRRUN) {
                strcpy(eval.error, lua_tostring(L, -1));
                lua_pop(L, 1);
            }

        } while (0);
        
    }
    return eval.success;
}