#pragma once

#include "document.h"
#include "rapidjson.h"

#include "bp.h"

#include <vector>

int var2json(rapidjson::Value& container, const variable_t* var, rapidjson::MemoryPoolAllocator<>& alloc);
int vars2json(rapidjson::Value& container, const std::vector<variable_t*>& vars, rapidjson::MemoryPoolAllocator<>& alloc);
int stacks2json(rapidjson::Document& document, std::vector<stack_t*>& stacks, rapidjson::MemoryPoolAllocator<>& alloc);

int json2bp(breakpoint_t& bp, const rapidjson::Value& value);