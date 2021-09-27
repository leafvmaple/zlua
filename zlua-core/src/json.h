#pragma once

#include "model.h"
#include "runtime.h"
#include "rapidjson.h"
#include "stringbuffer.h"

#include <vector>

class Evaluate;

class Json
{
public:
    Json();

    int FromRuntime(lua_State* L);
    int FromEvaluate(Evaluate* evalute);

    int AddCommand(int cmd);
    int Write(rapidjson::StringBuffer& buffer);

private:
    rapidjson::Document document_;
};

int GetBreakPoint(BreakPoint& bp, const rapidjson::Value& value);
int json2eval(Evaluate& eval, const rapidjson::Value& value);