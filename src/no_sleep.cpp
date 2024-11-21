#include "no_sleep/no_sleep.hpp"
#include <cassert>

#if defined(_WIN32)
#include <mutex>
#include "windows.h"

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
static auto previous_execution_state() -> EXECUTION_STATE&
{
    static EXECUTION_STATE instance{};
    return instance;
};

namespace {
class ImplPlatform : public no_sleep::internal::Impl {
public:
    explicit ImplPlatform(const char* /* reason */)
    {
        std::lock_guard<std::mutex> _{ref_count_mutex()};
        if (ref_count() == 0)
            previous_execution_state() = SetThreadExecutionState(ES_CONTINUOUS | ES_SYSTEM_REQUIRED | ES_DISPLAY_REQUIRED);
        ref_count()++;
    }
    ~ImplPlatform() override
    {
        std::lock_guard<std::mutex> _{ref_count_mutex()};
        assert(ref_count() > 0);
        ref_count()--;
        if (ref_count() == 0)
            SetThreadExecutionState(previous_execution_state());
    }

    ImplPlatform(ImplPlatform const&)                = delete;
    ImplPlatform& operator=(ImplPlatform const&)     = delete;
    ImplPlatform(ImplPlatform&&) noexcept            = delete;
    ImplPlatform& operator=(ImplPlatform&&) noexcept = delete;
};
} // namespace
#endif
#if defined(__linux__)
namespace {
class ImplPlatform : public no_sleep::internal::Impl {
public:
    explicit ImplPlatform(const char* /* reason */)
    {
    }
    ~ImplPlatform() override
    {
    }

    ImplPlatform(ImplPlatform const&)                = delete;
    ImplPlatform& operator=(ImplPlatform const&)     = delete;
    ImplPlatform(ImplPlatform&&) noexcept            = delete;
    ImplPlatform& operator=(ImplPlatform&&) noexcept = delete;
};
} // namespace
#endif
#if defined(__APPLE__)
#import <IOKit/pwr_mgt/IOPMLib.h>

namespace {
class ImplPlatform : public no_sleep::internal::Impl {
public:
    explicit ImplPlatform(const char* reason)
    {
        CFStringRef reasonForActivity = CFSTR(reason);
        assertion_valid               = IOPMAssertionCreateWithName(kIOPMAssertionTypeNoDisplaySleep, kIOPMAssertionLevelOn, reasonForActivity, &_assertion_id) == kIOReturnSuccess;
    }
    ~ImplPlatform() override
    {
        if (_assertion_valid)
            IOPMAssertionRelease(_assertion_id);
    }

    ImplPlatform(ImplPlatform const&)                = delete;
    ImplPlatform& operator=(ImplPlatform const&)     = delete;
    ImplPlatform(ImplPlatform&&) noexcept            = delete;
    ImplPlatform& operator=(ImplPlatform&&) noexcept = delete;

private:
    IOPMAssertionID _assertion_id{};
    bool            _assertion_valid{false};
};
} // namespace
#endif

namespace no_sleep {

Scoped::Scoped(const char* reason)
    : _pimpl{std::make_shared<ImplPlatform>(reason)}
{
    assert(strlen(reason) <= 128 && "On MacOS this is limited to 128 characters");
}

} // namespace no_sleep
