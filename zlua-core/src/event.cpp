#include "event.h"
#include "zlua.h"
#include "model.h"
#include "json.h"
#include "stringbuffer.h"
#include "writer.h"
#include "connect.h"
#include "action.h"
#include "runtime.h"

#define register_event(event, handle) events[(int)PROTO::event]=handle;

typedef int (*fn)(Connection* conn, const rapidjson::Document& document);
fn events[(int)PROTO::count];

int EventSend(Connection* conn, PROTO proto, Json& json);

lua_State* gL = nullptr;

void SendData(Connection* conn, lua_State* L) {
    std::vector<Stack> stacks;
    Json json;

    json.FromRuntime(L);
    EventSend(conn, PROTO::s2c_break_point_msg, json);

    action_break(L);
}

void OnHook(lua_State* L, lua_Debug* ar) {
    if (ar->event == LUA_HOOKLINE) {
        Connection* conn = BreakPoint::Find(L, ar);
        if (conn) {
            SendData(conn, L);
        }
        conn = action_find(L, ar);
        if (conn) {
            SendData(conn, L);
        }
    }
}

int EventInit(Connection* conn, const rapidjson::Document& document) {
    lua_sethook(gL, OnHook, LUA_MASKCALL | LUA_MASKLINE | LUA_MASKRET, 0);

    /*if (document.HasMember("globals")) {
        const auto globals = document["globals"].GetArray();
        for (auto it = globals.begin(); it != globals.end(); it++) {
            rt_add_global(it->GetString());
        }
    }*/

    if (document.HasMember("filters")) {
        const auto filters = document["filters"].GetArray();
        for (auto it = filters.begin(); it != filters.end(); it++) {
            for (int i = 0; i < ZLUA_TYPE_COUNT; i++) {
                if (!strcmp(lua_typename(nullptr, i), it->GetString())) {
                    Variable::SetFilter(i, true);
                }
            }
        }
    }

    return true;
}

int EventAddBreakPoint(Connection* conn, const rapidjson::Document& document) {
    if (document.HasMember("clear")) {
        const auto isclear = document["clear"].GetBool();
        if (isclear) {
            BreakPoint::Clear();
        }
    }

    if (document.HasMember("breakPoints")) {
        const auto json_bp = document["breakPoints"].GetArray();
        for (auto it = json_bp.begin(); it != json_bp.end(); it++) {
            BreakPoint bp;
            bp.conn_ = conn;
            bp.LoadFromJson(*it);
            BreakPoint::Add(bp);
        }
    }

    return true;
}

int EventEval(Connection* conn, const rapidjson::Document& document) {
    Evaluate evaluate;
    evaluate.FromJson(document);
    if (evaluate.FromRuntime(gL)) {
        Json json;
        json.FromEvaluate(&evaluate);
        EventSend(conn, PROTO::s2c_eval, json);
    }

    return true;
}

int EventAction(Connection* conn, const rapidjson::Document& document) {
    const auto action = static_cast<Action>(document["action"].GetInt());
    action_excute(action, conn, gL);

    return true;
}

void EventInit(lua_State* L) {
    gL = L;

    register_event(c2s_init,            EventInit);
    register_event(c2s_add_break_point, EventAddBreakPoint);
    register_event(c2s_action,          EventAction);
    register_event(c2s_eval,            EventEval);
}

int EventHandle(Connection* conn, const char* buf, size_t len) {

    rapidjson::Document document;
    document.Parse(buf, len);

    const int type = document["cmd"].GetInt();
    if (!events[type])
        return false;

    return events[type](conn, document);
}

int EventSend(Connection* conn, PROTO proto, Json& json) {
    int cmd = static_cast<int>(proto);
    rapidjson::StringBuffer buffer;

    json.AddCommand(cmd);
    json.Write(buffer);
    conn->Send(cmd, buffer.GetString(), buffer.GetSize());

    return true;
}