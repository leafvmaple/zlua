#include "json.h"

#include "stringbuffer.h"
#include "writer.h"
#include "document.h"
#include "rapidjson.h"

#include <string>
#include <windows.h>

int GetVariables(rapidjson::Value& container, const std::vector<Variable>& vars, rapidjson::MemoryPoolAllocator<>& alloc);

std::string GBK2UTF8(std::string const strin) {
    int len = MultiByteToWideChar(CP_ACP, 0, strin.c_str(), -1, NULL, 0);
    wchar_t* wstr = new wchar_t[len + 1];
    memset(wstr, 0, len + 1);
    MultiByteToWideChar(CP_ACP, 0, strin.c_str(), -1, wstr, len);
    len = WideCharToMultiByte(CP_UTF8, 0, wstr, -1, NULL, 0, NULL, NULL);
    char* str = new char[len + 1];
    memset(str, 0, len + 1);
    WideCharToMultiByte(CP_UTF8, 0, wstr, -1, str, len, NULL, NULL);
    std::string res = str;

    if (wstr) delete[] wstr;
    if (str) delete[] str;

    return res;
}

int GetVariable(rapidjson::Value& container, const Variable& var, rapidjson::MemoryPoolAllocator<>& alloc) {
    container.AddMember("name", var.name_, alloc);
    container.AddMember("nameType", var.name_type_, alloc);
    container.AddMember("value", GBK2UTF8(var.value_), alloc);
    container.AddMember("valueType", var.value_type_, alloc);
    container.AddMember("valueTypeName", var.value_type_name_, alloc);
    container.AddMember("cacheId", var.cache_id_, alloc);

    if (!var.childs_.empty()) {
        rapidjson::Value json_child_vars(rapidjson::kArrayType);
        GetVariables(json_child_vars, var.childs_, alloc);
        container.AddMember("children", json_child_vars, alloc);
    }
    return true;
}

int GetVariables(rapidjson::Value& container, const std::vector<Variable>& vars, rapidjson::MemoryPoolAllocator<>& alloc) {
    for (auto var : vars) {
        rapidjson::Value json_var(rapidjson::kObjectType);
        GetVariable(json_var, var, alloc);
        container.PushBack(json_var, alloc);
    }
    return true;
}

int GetEvaluate(rapidjson::Document& document, const Evaluate& eval, rapidjson::MemoryPoolAllocator<>& alloc)
{
    document.AddMember("seq", eval.seq_, alloc);
    document.AddMember("success", eval.success_, alloc);
    if (eval.success_) {
        rapidjson::Value v(rapidjson::kObjectType);
        GetVariable(v, eval.result_, alloc);
        document.AddMember("value", v, alloc);
    }
    else {
        document.AddMember("error", eval.error_, alloc);
    }
    return true;
}

int GetBreakPoint(BreakPoint& bp, const rapidjson::Value& value) {
    if (value.HasMember("file")) {
        bp.path_ = value["file"].GetString();
    }
    if (value.HasMember("line")) {
        bp.line_ = value["line"].GetInt();
    }
    if (value.HasMember("condition")) {
        bp.cond_ = value["condition"].GetString();
    }
    if (value.HasMember("logMessage")) {
        bp.log_ = value["file"].GetString();
    }

    return true;
}

Json::Json()
{
     document_.SetObject();
}

int Json::FromRuntime(lua_State* L)
{
    rapidjson::MemoryPoolAllocator<>& alloc = document_.GetAllocator();
    rapidjson::Value json_stacks(rapidjson::kArrayType);
    std::vector<Stack> stacks;

    GetRuntimeStacks(stacks, L);

    for (auto stack : stacks) {
        rapidjson::Value json_stack(rapidjson::kObjectType);
        json_stack.AddMember("file", std::string(stack.file), alloc);
        json_stack.AddMember("line", stack.line, alloc);
        json_stack.AddMember("level", stack.level, alloc);
        if (stack.chunk) {
            json_stack.AddMember("functionName", std::string(stack.chunk), alloc);
        }

        rapidjson::Value json_local_vars(rapidjson::kArrayType);
        GetVariables(json_local_vars, stack.local_vars, alloc);
        json_stack.AddMember("localVariables", json_local_vars, alloc);

        rapidjson::Value json_upvalue_vars(rapidjson::kArrayType);
        GetVariables(json_upvalue_vars, stack.upvalue_vars, alloc);
        json_stack.AddMember("upvalueVariables", json_upvalue_vars, alloc);

        rapidjson::Value json_gloabl_vars(rapidjson::kArrayType);
        GetVariables(json_gloabl_vars, stack.global_vars, alloc);
        json_stack.AddMember("globalVariables", json_gloabl_vars, alloc);

        json_stacks.PushBack(json_stack, alloc);
    }

    document_.AddMember("stacks", json_stacks, alloc);
    return true;
}


int Json::FromEvaluate(Evaluate* evalute)
{
    rapidjson::MemoryPoolAllocator<>& alloc = document_.GetAllocator();

    document_.AddMember("seq", evalute->seq_, alloc);
    document_.AddMember("success", evalute->success_, alloc);
    if (evalute->success_) {
        rapidjson::Value v(rapidjson::kObjectType);
        GetVariable(v, evalute->result_, alloc);
        document_.AddMember("value", v, alloc);
    }
    else {
        document_.AddMember("error", evalute->error_, alloc);
    }
    return true;
}

int Json::AddCommand(int cmd)
{
    rapidjson::MemoryPoolAllocator<>& alloc = document_.GetAllocator();
    document_.AddMember("cmd", cmd, alloc);
}

int Json::Write(rapidjson::StringBuffer& buffer)
{
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
    document_.Accept(writer);
}
