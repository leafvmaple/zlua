#pragma once

#include "zlua.h"

#include <vector>

struct variable_t {
    char name[STR_LEN];
    int name_type;
    char value[STR_LEN];
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
    char expr[STR_LEN];
    char error[STR_LEN];
    int seq;
    int level;
    int depth;
    int cache_id;
    variable_t result;
    bool success;
};

int rt2stacks(std::vector<stack_t*>& stacks, lua_State* L);

void rt_clear_cache(lua_State* L);
int rt_eval_cache(eval_t& eval, lua_State* L);