#pragma once

namespace SamplerKit
{
// #define PRINT_DEBUG
#define PRINT_LOG

#ifdef PRINT_DEBUG
template<typename... Ts>
static void pr_dbg(Ts... args) {
	printf_(args...);
}
#else
[[maybe_unused]] static void pr_dbg(...) {
}
#endif

#ifdef PRINT_LOG
template<typename... Ts>
static void pr_log(Ts... args) {
	printf_(args...);
}
#else
static void pr_log(...) {
}
#endif

} // namespace SamplerKit
