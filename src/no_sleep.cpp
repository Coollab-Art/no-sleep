#include "no_sleep/no_sleep.hpp"
#include <cassert>
#include <mutex>
#include "windows.h"

namespace no_sleep {

#if defined(_WIN32)
static void disable_sleep()
{
    SetThreadExecutionState(ES_CONTINUOUS | ES_SYSTEM_REQUIRED | ES_DISPLAY_REQUIRED);
}
static void enable_sleep()
{
    SetThreadExecutionState(ES_CONTINUOUS);
}
#endif
#if defined(__linux__)
static void disable_sleep()
{
}
static void enable_sleep()
{
}
#endif
#if defined(__APPLE__)
static void disable_sleep()
{
}
static void enable_sleep()
{
}
#endif

static auto ref_count() -> int&
{
    static int instance{0};
    return instance;
}
static auto ref_count_mutex() -> std::mutex&
{
    static std::mutex instance{};
    return instance;
};

static void increment_ref_count()
{
    std::lock_guard<std::mutex> _{ref_count_mutex()};
    if (ref_count() == 0)
        disable_sleep();
    ref_count()++;
}

static void decrement_ref_count()
{
    std::lock_guard<std::mutex> _{ref_count_mutex()};
    assert(ref_count() > 0);
    ref_count()--;
    if (ref_count() == 0)
        enable_sleep();
}

Scoped::Scoped()
{
    increment_ref_count();
}

Scoped::Scoped(Scoped const&)
{
    increment_ref_count();
}

Scoped::Scoped(Scoped&&) noexcept
{
    increment_ref_count();
}

Scoped::~Scoped()
{
    decrement_ref_count();
}

} // namespace no_sleep
