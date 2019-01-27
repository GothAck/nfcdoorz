#include <map>
#include <vector>
#include <string>

#pragma once

extern std::map<std::string, int> call_count;
extern std::map<std::string, std::vector<std::vector<void *>>> calls;
