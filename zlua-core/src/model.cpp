#include "model.h"

int BreakPoint::Add(const BreakPoint& bp)
{
    std::lock_guard <std::mutex> lock(mutex_);
    map_[bp.line_].push_back(bp);

    return true;
}

void BreakPoint::Clear()
{
    std::lock_guard <std::mutex> lock(mutex_);
    map_.clear();
}

int BreakPoint::LoadFromJson(const rapidjson::Value& value)
{
    if (value.HasMember("file")) {
        path_ = value["file"].GetString();
    }
    if (value.HasMember("line")) {
        line_ = value["line"].GetInt();
    }
    if (value.HasMember("condition")) {
        cond_ = value["condition"].GetString();
    }
    if (value.HasMember("logMessage")) {
        log_ = value["file"].GetString();
    }

    return true;
}

Connection* BreakPoint::Find(lua_State* L, lua_Debug* ar)
{
    const int cl = ar->currentline;
    if (cl >= 0) {
        auto it = BreakPoint::map_.find(cl);
        if (it != map_.end()) {
            lua_getinfo(L, "S", ar);
            for (auto breakpoint : it->second) {
                if (breakpoint.path_ == ar->source && breakpoint.Process(L))
                    return breakpoint.conn_;
            }
        }
    }
    return 0;
}

int BreakPoint::Process(lua_State* L)
{
    return true;
}