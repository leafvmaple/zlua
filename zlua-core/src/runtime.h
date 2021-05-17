#pragma once

#include "zlua.h"
#include <vector>

struct variable_t {
    char name[ZLUA_FILE_MAX];
    int name_type;
    char value[ZLUA_FILE_MAX];
    int value_type;
    const char* value_type_name;
    std::vector<variable_t*> childs;
    int cache_id;

    ~variable_t() {
        for (auto var : childs)
            delete var;
    }
};

struct stack_t {
    const char* file;
    const char* chunk;
    int level;
    int line;
    std::vector<variable_t*> local_vars;
    std::vector<variable_t*> upvalue_vars;

    ~stack_t() {
        for (auto var : local_vars)
            delete var;
        for (auto var : upvalue_vars)
            delete var;
    }
};

struct eval_t {
    char expr[ZLUA_FILE_MAX];
    char error[ZLUA_FILE_MAX];
    int seq;
    int level;
    int depth;
    int cache_id;
    variable_t result;
    bool success;
};

void rt_clear_cache(lua_State* L);
int rt_eval_cache(eval_t& eval, lua_State* L);

int rt_set_parser(int type, fn_parser parser);

int rt2stacks(std::vector<stack_t*>& stacks, lua_State* L);