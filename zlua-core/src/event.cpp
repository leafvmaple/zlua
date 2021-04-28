#include "event.h"
#include "zlua.h"
#include "bp.h"
#include "json.h"
#include "stringbuffer.h"
#include "writer.h"
#include "connect.h"

typedef int (*fn)(conn_t* conn, const rapidjson::Document& document);

fn events[(int)MESSAGE::COUNT];

std::set<lua_State*> states;

void on_hook(lua_State* L, lua_Debug* ar) {
    if (ar->event == LUA_HOOKLINE) {
        conn_t* conn = bp_find(L, ar);
        if (conn) {
            std::vector<stack_t*> stacks;
            rt2stacks(stacks, L);

            rapidjson::Document document;
            document.SetObject();
            auto& alloc = document.GetAllocator();

            document.AddMember("cmd", static_cast<int>(MESSAGE::BREAK_NOTIFY), alloc);
            stacks2json(document, stacks, alloc);

            rapidjson::StringBuffer buffer;
            rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
            document.Accept(writer);

            conn_send(conn, static_cast<int>(MESSAGE::BREAK_NOTIFY), buffer.GetString(), buffer.GetSize());
        }
    }
}

int event_init_req(conn_t* conn, const rapidjson::Document& document) {
    for (auto L : states)
        lua_sethook(L, on_hook, LUA_MASKCALL | LUA_MASKLINE | LUA_MASKRET, 0);

    return true;
}

int event_add_bp_req(conn_t* conn, const rapidjson::Document& document) {
    if (document.HasMember("clear")) {
        const auto isclear = document["clear"].GetBool();
        if (isclear) {
            bp_clear();
        }
    }

    if (document.HasMember("breakPoints")) {
        const auto json_bp = document["breakPoints"].GetArray();
        for (auto it = json_bp.begin(); it != json_bp.end(); it++) {
            breakpoint_t bp;
            bp.conn = conn;
            json2bp(bp, *it);
            bp_add(bp);
        }
    }

    return true;
}

void event_init() {
    events[(int)MESSAGE::INIT_REQUEST]            = event_init_req;
    events[(int)MESSAGE::ADD_BREAK_POINT_REQUEST] = event_add_bp_req;
}

void event_add_state(lua_State* L)
{
    states.insert(L);
}

int event_handle(conn_t* conn, const char* buf, size_t len) {

    rapidjson::Document document;
    document.Parse(buf, len);

    const int type = document["cmd"].GetInt();
    if (!events[type])
        return false;

    return events[type](conn, document);
}

int event_send(MESSAGE proto, const rapidjson::Document& document) {
    int cmd = static_cast<int>(proto);
}