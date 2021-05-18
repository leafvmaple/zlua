#include "json.h"

#include <string>
#include <windows.h>

std::string GBK2UTF8(const char* strin) {
    int len = MultiByteToWideChar(CP_ACP, 0, strin, -1, NULL, 0);
    wchar_t* wstr = new wchar_t[len + 1];
    memset(wstr, 0, len + 1);
    MultiByteToWideChar(CP_ACP, 0, strin, -1, wstr, len);
    len = WideCharToMultiByte(CP_UTF8, 0, wstr, -1, NULL, 0, NULL, NULL);
    char* str = new char[len + 1];
    memset(str, 0, len + 1);
    WideCharToMultiByte(CP_UTF8, 0, wstr, -1, str, len, NULL, NULL);
    std::string res = str;

    if (wstr) delete[] wstr;
    if (str) delete[] str;

    return res;
}

int var2json(rapidjson::Value& container, const variable_t& var, rapidjson::MemoryPoolAllocator<>& alloc) {
    container.AddMember("name", std::string(var.name), alloc);
    container.AddMember("nameType", var.name_type, alloc);
    container.AddMember("value", GBK2UTF8(var.value), alloc);
    container.AddMember("valueType", var.value_type, alloc);
    container.AddMember("valueTypeName", std::string(var.value_type_name), alloc);
    container.AddMember("cacheId", var.cache_id, alloc);

    if (!var.childs.empty()) {
        rapidjson::Value json_child_vars(rapidjson::kArrayType);
        vars2json(json_child_vars, var.childs, alloc);
        container.AddMember("children", json_child_vars, alloc);
    }
    return true;
}

int vars2json(rapidjson::Value& container, const std::vector<variable_t>& vars, rapidjson::MemoryPoolAllocator<>& alloc) {
    for (auto var : vars) {
        rapidjson::Value json_var(rapidjson::kObjectType);
        var2json(json_var, var, alloc);
        container.PushBack(json_var, alloc);
    }
    return true;
}

int stacks2json(rapidjson::Document& document, std::vector<stack_t>& stacks, rapidjson::MemoryPoolAllocator<>& alloc) {
    rapidjson::Value json_stacks(rapidjson::kArrayType);
    
    for (auto stack : stacks) {
        rapidjson::Value json_stack(rapidjson::kObjectType);
        json_stack.AddMember("file", std::string(stack.file), alloc);
        json_stack.AddMember("line", stack.line, alloc);
        json_stack.AddMember("level", stack.level, alloc);
        if (stack.chunk) {
            json_stack.AddMember("functionName", std::string(stack.chunk), alloc);
        }

        rapidjson::Value json_local_vars(rapidjson::kArrayType);
        vars2json(json_local_vars, stack.local_vars, alloc);
        json_stack.AddMember("localVariables", json_local_vars, alloc);

        rapidjson::Value json_upvalue_vars(rapidjson::kArrayType);
        vars2json(json_upvalue_vars, stack.upvalue_vars, alloc);
        json_stack.AddMember("upvalueVariables", json_upvalue_vars, alloc);

        rapidjson::Value json_gloabl_vars(rapidjson::kArrayType);
        vars2json(json_gloabl_vars, stack.global_vars, alloc);
        json_stack.AddMember("globalVariables", json_gloabl_vars, alloc);

        json_stacks.PushBack(json_stack, alloc);
    }

    document.AddMember("stacks", json_stacks, alloc);
    return true;
}

int eval2json(rapidjson::Document& document, const eval_t& eval, rapidjson::MemoryPoolAllocator<>& alloc)
{
    document.AddMember("seq", eval.seq, alloc);
    document.AddMember("success", eval.success, alloc);
    if (eval.success) {
        rapidjson::Value v(rapidjson::kObjectType);
        var2json(v, eval.result, alloc);
        document.AddMember("value", v, alloc);
    }
    else {
        document.AddMember("error", eval.error, alloc);
    }
    return true;
}

int json2bp(breakpoint_t& bp, const rapidjson::Value& value) {
    if (value.HasMember("file")) {
        strcpy(bp.file, value["file"].GetString());
        str2paths(bp.path, bp.file);
    }
    if (value.HasMember("line")) {
        bp.line = value["line"].GetInt();
    }
    if (value.HasMember("condition")) {
        strcpy(bp.cond, value["condition"].GetString());
    }
    if (value.HasMember("logMessage")) {
        strcpy(bp.log, value["file"].GetString());
    }

    return true;
}

int json2eval(eval_t& eval, const rapidjson::Value& value)
{
    eval.seq = value["seq"].GetInt();
    strcpy(eval.expr, value["expr"].GetString());
    eval.level = value["stackLevel"].GetInt();
    eval.depth = value["depth"].GetInt();
    eval.cache_id = value.HasMember("cacheId") ? value["cacheId"].GetInt() : 0;
    eval.success = false;

    return true;
}
