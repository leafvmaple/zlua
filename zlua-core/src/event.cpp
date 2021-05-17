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

typedef int (*fn)(conn_t* conn, const rapidjson::Document& document);
fn events[(int)PROTO::count];

int event_send(conn_t* conn, PROTO proto, rapidjson::Document& document, rapidjson::MemoryPoolAllocator<>& alloc);

std::set<lua_State*> states;

void send_data(conn_t* conn, lua_State* L) {
    std::vector<stack_t*> stacks;
    rt2stacks(stacks, L);

    rapidjson::Document document;
    document.SetObject();
    rapidjson::MemoryPoolAllocator<>& alloc = document.GetAllocator();

    stacks2json(document, stacks, alloc);
    event_send(conn, PROTO::s2c_break_point_msg, document, alloc);

    action_break(L);
}

void on_hook(lua_State* L, lua_Debug* ar) {
    if (ar->event == LUA_HOOKLINE) {
        conn_t* conn = bp_find(L, ar);
        if (conn) {
            send_data(conn, L);
        }
        conn = action_find(L, ar);
        if (conn) {
            send_data(conn, L);
        }
    }
}

int event_init(conn_t* conn, const rapidjson::Document& document) {
    for (auto L : states)
        lua_sethook(L, on_hook, LUA_MASKCALL | LUA_MASKLINE | LUA_MASKRET, 0);

    return true;
}

int event_add_bp(conn_t* conn, const rapidjson::Document& document) {
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

int event_eval(conn_t* conn, const rapidjson::Document& document) {
    eval_t eval;
    json2eval(eval, document);
    for (auto L : states)
        if (rt_eval_cache(eval, L)) {
            rapidjson::Document document;
            document.SetObject();
            auto& alloc = document.GetAllocator();

            eval2json(document, eval, alloc);
            event_send(conn, PROTO::s2c_eval, document, alloc);
        }

    return true;
}

int event_action(conn_t* conn, const rapidjson::Document& document) {
    const auto action = static_cast<Action>(document["action"].GetInt());
    for (auto L : states)
        action_excute(action, conn, L);

    return true;
}

void event_init() {
    register_event(c2s_init,            event_init);
    register_event(c2s_add_break_point, event_add_bp);
    register_event(c2s_action,          event_action);
    register_event(c2s_eval,            event_eval);
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

int event_send(conn_t* conn, PROTO proto, rapidjson::Document& document, rapidjson::MemoryPoolAllocator<>& alloc) {
    document.AddMember("cmd", static_cast<int>(proto), alloc);

    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
    document.Accept(writer);

    conn_send(conn, static_cast<int>(proto), buffer.GetString(), buffer.GetSize());

    return true;
}