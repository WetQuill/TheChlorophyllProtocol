#pragma once

#include <cstdio>
#include <cstdint>

#ifdef TCP_NO_DEBUG_LOG
#define TCP_DEBUG(tag, tick, fmt, ...) ((void)0)
#else
#define TCP_DEBUG(tag, tick, fmt, ...) \
    std::fprintf(stderr, "[%s] t=%lld " fmt "  (%s:%d)\n", \
                 tag, static_cast<long long>(tick) __VA_OPT__(,) __VA_ARGS__, __FILE__, __LINE__)
#endif
