#include "action.h"
#include "model.h"
#include "runtime.h"

#include <mutex>

std::mutex mutex_run;
std::condition_variable cv;

void action_break(lua_State* L) {
    std::unique_lock<std::mutex> lock(mutex_run);
    cv.wait(lock);

    rt_clear_cache(L);
}

void action_continue(lua_State* L) {
    cv.notify_all();
}

void action_stop(lua_State* L) {
    bp_clear();
    cv.notify_all();
}

void do_action(Action action, lua_State* L) {
    switch (action)
    {
    case Action::ide_break:
        action_break(L);
        break;
    case Action::ide_continue:
        action_continue(L);
        break;
    case Action::ide_step_oever:
        break;
    case Action::ide_step_in:
        break;
    case Action::ide_step_out:
        break;
    case Action::ide_stop:
        action_stop(L);
        break;
    default:
        break;
    }
}