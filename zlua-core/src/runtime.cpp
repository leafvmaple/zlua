#include "runtime.h"
#include <string>

#define CACHE_TABLE_NAME "_cache_table_"

int cache_gen = 0;

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

void RuntimeInit()
{
}

const char* GetFile(lua_State* L, lua_Debug& ar) {
    const char* res = ar.source;
    if (strlen(res) > 0 && res[0] == '@')
        ++res;
#ifdef _WIN32
    if (strlen(res) > 0 && (res[0] == '\\' || res[0] == '/'))
        ++res;
#endif
    return res;
}

int ParseMetable(std::string& val, lua_State* L, int idx, const char* method, int param_cnt) {
    int result = false;
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

        if (!result) {
            return false;
        }

        val = lua_tostring(L, -1);
        lua_pop(L, 1);

        return true;
    }
    return false;
}

int ParsePoint(std::string& val, lua_State* L, int index) {
    char res[ZLUA_FILE_MAX];
    sprintf(res, "%s(0x%X)", lua_typename(L, lua_type(L, index)), lua_topointer(L, index));
    val = res;
    return true;
}

int ParseTable(std::string& val, lua_State* L, int index, int size) {
    char res[ZLUA_FILE_MAX];
    sprintf(res, "table(0x%X, size = %d)", lua_topointer(L, index), size);
    val = res;
    return true;
}

int GetRuntimeStacks(std::vector<Stack>& stacks, lua_State* L) {
    stacks.clear();

    for (int level = 0;; level++) {
        lua_Debug ar;
        if (!lua_getstack(L, level, &ar)) {
            break;
        }
        if (!lua_getinfo(L, "nSlu", &ar)) {
            continue;
        }

        Stack stack{ GetFile(L, ar), ar.name, level, ar.currentline };

        for (int i = 1;; i++) {
            const char* name = lua_getlocal(L, &ar, i);
            if (name == nullptr) {
                break;
            }
            if (name[0] == '(') {
                lua_pop(L, 1);
                continue;
            }

            Variable var ;
            var.name_ = name;
            if (var.Parse(L, -1, 1)) {
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

                Variable var;
                var.name_ = name;
                if (var.Parse(L, -1, 1)) {
                    stack.upvalue_vars.push_back(var);
                }

                lua_pop(L, 1);
            }
            // pop function
            lua_pop(L, 1);
        }

        /*for (auto global : globals) {
            lua_getglobal(L, global.c_str());
            if (!lua_isnil(L, -1)) {
                Variable var;
                var.name_ = global;
                if (var.Parse(L, -1, 1)) {
                    stack.global_vars.push_back(var);
                }
            }
            lua_pop(L, 1);
        }*/
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

int CreateEnv(lua_State* L, int level) {
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

int Evaluate::FromJson(const rapidjson::Document& value)
{
    seq_ = value["seq"].GetInt();
    expr_ = value["expr"].GetString();
    level_ = value["stackLevel"].GetInt();
    depth_ = value["depth"].GetInt();
    cache_id_ = value.HasMember("cacheId") ? value["cacheId"].GetInt() : 0;
    success_ = false;

    return true;
}

int Evaluate::FromRuntime(lua_State* L)
{
    if (cache_id_ > 0) {
        lua_getfield(L, LUA_REGISTRYINDEX, CACHE_TABLE_NAME);
        if (lua_type(L, -1) == LUA_TTABLE) {
            char key[10];
            sprintf(key, "%d", cache_id_);;
            lua_getfield(L, -1, key);

            result_.name_ = expr_;
            result_.Parse(L, -1, depth_);
            lua_pop(L, 1);

            success_ = true;
        }
        lua_pop(L, 1);
    }
    else {
        do
        {
            int r = luaL_loadstring(L, ("return " + expr_).c_str());
            if (r == LUA_ERRSYNTAX) {
                error_ = "syntax err:  " + expr_;
                break;
            }

            const int idx = lua_gettop(L);

            if (!CreateEnv(L, level_))
                break;

            lua_setfenv(L, idx);

            r = lua_pcall(L, 0, 1, 0);
            if (!r) {
                result_.name_ = expr_;
                result_.Parse(L, -1, depth_);
                lua_pop(L, 1);
                success_ = true;
            }
            if (r == LUA_ERRRUN) {
                error_ = lua_tostring(L, -1);
                lua_pop(L, 1);
            }

        } while (0);

    }
    return success_;
}

Variable::Variable() : name_type_(0), value_type_(0) {}

Variable::Variable(std::string name) : Variable()
{
    name_ = name;
}


int Variable::Parse(lua_State* L, int index, int depth)
{
    const int t1 = lua_gettop(L);
    index = lua_absindex(L, index);

    int type = lua_type(L, index);

    if (filters[type]) {
        return false;
    }

    name_type_ = LUA_TSTRING;
    value_type_ = type;
    value_type_name_ = lua_typename(L, value_type_);
    cache_id_ = 0;

    if (type == LUA_TUSERDATA || type == LUA_TTABLE) {
        AddCache(L, index);
        if (depth > 1) {
            CustomParse(L);
        }
    }

    switch (type) {
    case LUA_TNIL: {
        value_ = "nil";
        break;
    }
    case LUA_TNUMBER: {
        value_ = lua_tostring(L, index);
        break;
    }
    case LUA_TBOOLEAN: {
        value_ = lua_toboolean(L, index) ? "true" : "false";
        break;
    }
    case LUA_TSTRING: {
        value_ = lua_tostring(L, index);
        break;
    }
    case LUA_TUSERDATA: {
        auto string = lua_tostring(L, index);
        if (string) {
            value_ = string;
        }
        else if (!ParseMetable(value_, L, t1, "__tostring", 1)) {
            ParsePoint(value_, L, index);
        }
        if (depth > 1) {
            if (lua_getmetatable(L, index)) {
                Parse(L, -1, depth);
                lua_pop(L, 1); // pop meta
            }
        }
        break;
    }
    case LUA_TFUNCTION:
    case LUA_TLIGHTUSERDATA:
    case LUA_TTHREAD: {
        ParsePoint(value_, L, index);
        break;
    }
    case LUA_TTABLE: {
        int size = 0;
        lua_pushnil(L);
        while (lua_next(L, index)) {
            if (depth > 1) {
                Variable v;
                const auto name_type = lua_type(L, -2);
                v.name_type_ = name_type;
                if (name_type == LUA_TSTRING || name_type == LUA_TNUMBER || name_type == LUA_TBOOLEAN) {
                    lua_pushvalue(L, -2);
                    v.name_ = lua_tostring(L, -1);
                    lua_pop(L, 1);
                }
                else {
                    ParsePoint(v.name_, L, -2);
                }

                if (v.Parse(L, -1, depth - 1)) {
                    childs_.push_back(v);
                }
            }
            lua_pop(L, 1);
            size++;
        }

        if (lua_getmetatable(L, index)) {
            Variable meta;
            meta.name_ = "metatable";
            meta.name_type_ = LUA_TSTRING;

            if (meta.Parse(L, -1, 2)) {
                childs_.push_back(meta);
            }

            {
                lua_getfield(L, -1, "__index");
                if (!lua_isnil(L, -1)) {
                    Variable v;
                    v.Parse(L, -1, 2);
                    if (depth > 1) {
                        for (auto child : v.childs_) {
                            childs_.push_back(child);
                        }
                    }
                    size += v.childs_.size();
                }
                lua_pop(L, 1);
            }

            lua_pop(L, 1);
        }

        ParseTable(value_, L, index, size);
        break;
    }
    }

    return true;
}

int Variable::CustomParse(lua_State* L)
{
    int top = lua_gettop(L);

    do
    {
        int succes = parsers[value_type_] && parsers[value_type_](L);

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
            Variable v;
            v.name_type_ = lua_type(L, -1);

            lua_pushvalue(L, -2);
            v.name_ = lua_tostring(L, -1);
            lua_pop(L, 1);

            if (v.Parse(L, -1, 1)) {
                childs_.push_back(v);
            }

            lua_pop(L, 1);
        }

    } while (0);

    lua_settop(L, top);
}


int Variable::AddCache(lua_State* L, int idx)
{
    const int type = lua_type(L, idx);
    if (type == LUA_TUSERDATA || type == LUA_TTABLE) {
        cache_id_ = ++cache_gen;
        const int top = lua_gettop(L);
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
        lua_settop(L, top);
    }
    else {
        cache_id_ = 0;
    }
    return true;
}


void Variable::ClearCache(lua_State* L)
{
    lua_getfield(L, LUA_REGISTRYINDEX, CACHE_TABLE_NAME);
    if (!lua_isnil(L, -1)) {
        lua_pushnil(L);
        lua_setfield(L, LUA_REGISTRYINDEX, CACHE_TABLE_NAME);
    }
    lua_pop(L, 1);
}

void Variable::SetCustomParser(int type, parser_proto parser)
{
    parsers[type] = parser;
}

void Variable::SetFilter(int type, int enable)
{
    filters[type] = enable;
}
