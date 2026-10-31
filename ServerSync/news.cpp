#include "third_party/json.hpp"
#include <cstring>
#include <string>
using nlohmann::json;
#ifdef _WIN32
#define EXPORT __declspec(dllexport)
#else
#define EXPORT __attribute__((visibility("default")))
#endif
#include "news.hpp"
