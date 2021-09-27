#pragma once

#include "zlua.h"
#include "rapidjson.h"
#include "document.h"

#include <vector>
#include <string>

class Variable {
public:
    Variable();
    Variable(std::string name);

    int Parse(lua_State* L, int index, int depth);
    int CustomParse(lua_State* L);

    int AddCache(lua_State* L, int idx);

    static void ClearCache(lua_State* L);

    static void SetCustomParser(int type, parser_proto parser);
    static void SetFilter(int type, int enable);

    std::string name_;
    int name_type_;
    std::string value_;
    int value_type_;
    std::string value_type_name_;
    std::vector<Variable> childs_;
    int cache_id_;

    static parser_proto parsers[ZLUA_TYPE_COUNT];
    static int filters[ZLUA_TYPE_COUNT];
};

struct Stack {
    const char* file;
    const char* chunk;
    int level;
    int line;
    std::vector<Variable> local_vars;
    std::vector<Variable> upvalue_vars;
    std::vector<Variable> global_vars;
};

class Evaluate {
public:
    int FromJson(const rapidjson::Document& value);
    int FromRuntime(lua_State* L);

    std::string expr_;
    std::string error_;
    int seq_;
    int level_;
    int depth_;
    int cache_id_;
    Variable result_;
    bool success_;
};

void RuntimeInit();

int GetRuntimeStacks(std::vector<Stack>& stacks, lua_State* L);