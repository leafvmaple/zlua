#pragma once

#define STR_LEN 260
#define MIN(a, b) (((a)<(b))?(a):(b))

#include <string>
#include <vector>
#include <algorithm>
#include "zlua.h"

struct conn_t;

struct key_t {
    int line;
    std::string file;

    key_t(int l, const char* f) : line(l), file(f) {}

    bool operator < (const key_t& rh) const {
        if (line != rh.line)
            return line < rh.line;
        return file < rh.file;
    }
};

struct path_t {
    std::vector<std::string> vals;

    bool operator == (const path_t& rh) const {
        size_t mlen = MIN(vals.size(), rh.vals.size());
        for (int i = 0; i < mlen; i++)
            if (vals[i] != rh.vals[i])
                return false;
        return true;
    }
};

struct breakpoint_t {
    int line;
    char file[STR_LEN];
    char cond[STR_LEN];
    char log[STR_LEN];

    path_t path;

    conn_t* conn;

    breakpoint_t() {}

    breakpoint_t(const char* f) {
        strcpy(file, f);
    }
};

struct variable_t {
    const char* name;
    int name_type;
    char value[STR_LEN];
    int value_type;
    const char* value_type_name;
    std::vector<variable_t*> childs;
    int cacheId;

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

int str2paths(path_t& path, const char* str);
int rt2stacks(std::vector<stack_t*>& stacks, lua_State* L);

conn_t* bp_find(lua_State* L, lua_Debug* ar);
int bp_clear();
int bp_add(const breakpoint_t& bp);