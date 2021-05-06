#include "model.h"

#include <windows.h>
#include <string>
#include <map>
#include <vector>
#include <mutex>

std::mutex mutex_bp;
std::map<DWORD, std::vector<breakpoint_t>> bp_set;

int str2paths(path_t& path, const char* str) {
    char t[STR_LEN];
    if (strlen(str) >= STR_LEN) {
        return false;
    }
    strcpy(t, str);
    for (int i = strlen(t); i >= 0; i--) {
        if (t[i] == '\\' || t[i] == '/') {
            path.vals.push_back(&t[i + 1]);
            t[i] = '\0';
        }
        else {
            t[i] = tolower(t[i]);
        }
    }
    return true;
}

int bp_process(lua_State* L, breakpoint_t* bp) {
    return true;
}

conn_t* bp_find(lua_State* L, lua_Debug* ar) {
    const int cl = ar->currentline;
    if (cl >= 0) {
        auto it_set = bp_set.find(cl);
        if (it_set != bp_set.end()) {
            lua_getinfo(L, "S", ar);
            for (auto bp : it_set->second) {
                path_t path;
                if (str2paths(path, ar->source) && bp.path == path && bp_process(L, &bp))
                    return bp.conn;
            }
        }
    }
    return 0;
}

int bp_clear()
{
    std::lock_guard <std::mutex> lock(mutex_bp);
    bp_set.clear();

    return true;
}

int bp_add(const breakpoint_t& bp) {
    std::lock_guard <std::mutex> lock(mutex_bp);
    bp_set[bp.line].push_back(bp);

    return true;
}