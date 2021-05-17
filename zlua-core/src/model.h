#pragma once

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
    char file[ZLUA_FILE_MAX];
    char cond[ZLUA_FILE_MAX];
    char log[ZLUA_FILE_MAX];

    path_t path;

    conn_t* conn;

    breakpoint_t() {}

    breakpoint_t(const char* f) {
        strcpy(file, f);
    }
};

int str2paths(path_t& path, const char* str);

conn_t* bp_find(lua_State* L, lua_Debug* ar);
int bp_clear();
int bp_add(const breakpoint_t& bp);