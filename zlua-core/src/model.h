#pragma once

#define MIN(a, b) (((a)<(b))?(a):(b))

#include "zlua.h"
#include "document.h"

#include <string>
#include <vector>
#include <algorithm>
#include <string>
#include <map>
#include <mutex>

class Connection;

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

class RelativePath {
    std::vector<std::string> values;

public:
    RelativePath() {};
    RelativePath(const char* raw_path) {
        // Unsafe
        int len = strlen(raw_path);
        char* path = new char[len];
        memcpy(path, raw_path, len);
        for (int i = strlen(path); i >= 0; i--) {
            if (path[i] == '\\' || path[i] == '/') {
                values.push_back(&path[i + 1]);
                path[i] = '\0';
            }
            else {
                path[i] = tolower(path[i]);
            }
        }
        delete[] path;
    }

    bool operator == (const RelativePath& rh) const {
        size_t mlen = MIN(values.size(), rh.values.size());
        for (int i = 0; i < mlen; i++)
            if (values[i] != rh.values[i])
                return false;
        return true;
    }
};

class BreakPoint {
public:
    static std::mutex mutex_;
    static std::map<unsigned int, std::vector<BreakPoint>> map_;

    int line_;
    Connection* conn_;

    std::string cond_;
    std::string log_;

    RelativePath path_;

    BreakPoint() : line_(0), conn_(nullptr) {}
    BreakPoint(const char* raw_path) : BreakPoint() {
        path_ = raw_path;
    }

    int LoadFromJson(const rapidjson::Value& value);

    static Connection* Find(lua_State* L, lua_Debug* ar);
    static int Add(const BreakPoint& bp);
    static void Clear();

    int Process(lua_State* L);
};
