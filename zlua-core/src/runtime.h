#pragma once

#include "zlua.h"
#include <vector>
#include <string>

struct variable_t {
    std::string name;
    int name_type;
    std::string value;
    int value_type;
    const char* value_type_name;
    std::vector<variable_t> childs;
    int cache_id;
};

struct stack_t {
    const char* file;
    const char* chunk;
    int level;
    int line;
    std::vector<variable_t> local_vars;
    std::vector<variable_t> upvalue_vars;
    std::vector<variable_t> global_vars;
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

void rt_init();

void rt_clear_cache(lua_State* L);
int rt_eval(eval_t& eval, lua_State* L);

int rt_set_parser(int type, fn_parser parser);
int rt_set_filter(int type, int enable);

int rt_add_global(const char* global);

int rt2stacks(std::vector<stack_t>& stacks, lua_State* L);