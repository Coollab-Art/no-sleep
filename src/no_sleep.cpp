#include "no_sleep/no_sleep.hpp"
#include <cassert>
#include <cstring>

#if defined(_WIN32)
#include <mutex>
#include "windows.h"

static auto display_required_count() -> int&
{
    static int instance{0};
    return instance;
}
static auto system_required_count() -> int&
{
    static int instance{0};
    return instance;
}
static auto mutex() -> std::mutex&
{
    static std::mutex instance{};
    return instance;
};

namespace {
class ImplPlatform : public no_sleep::internal::Impl {
public:
    explicit ImplPlatform(const char* /* app_name */, const char* /* reason */, no_sleep::Mode mode)
        : _mode{mode}
    {
        std::lock_guard<std::mutex> lock{mutex()};

        count_for_current_mode()++;
        if (count_for_current_mode() == 1)
            set_thread_execution_state();
    }

    ~ImplPlatform() override
    {
        std::lock_guard<std::mutex> lock{mutex()};

        assert(count_for_current_mode() > 0);
        count_for_current_mode()--;
        if (count_for_current_mode() == 0)
            set_thread_execution_state();
    }

    ImplPlatform(ImplPlatform const&)                = delete;
    ImplPlatform& operator=(ImplPlatform const&)     = delete;
    ImplPlatform(ImplPlatform&&) noexcept            = delete;
    ImplPlatform& operator=(ImplPlatform&&) noexcept = delete;

    void set_thread_execution_state()
    {
        SetThreadExecutionState(
            ES_CONTINUOUS
            | (display_required_count() > 0 ? ES_DISPLAY_REQUIRED : 0)
            | (system_required_count() > 0 ? ES_SYSTEM_REQUIRED : 0)
        );
    }

    auto count_for_current_mode() -> int&
    {
        return _mode == no_sleep::Mode::KeepScreenOnAndKeepComputing
                   ? display_required_count()
                   : system_required_count();
    }

private:
    no_sleep::Mode _mode;
};
} // namespace
#endif
#if defined(__linux__)
#include <dbus/dbus.h>

namespace {
class ImplPlatform : public no_sleep::internal::Impl {
public:
    explicit ImplPlatform(const char* app_name, const char* reason, no_sleep::Mode mode)
    {
        DBusConnection* connection = dbus_bus_get(DBUS_BUS_SYSTEM, nullptr);
        if (!connection)
            return;

        DBusMessage* message = dbus_message_new_method_call("org.freedesktop.login1", "/org/freedesktop/login1", "org.freedesktop.login1.Manager", "Inhibit");
        if (!message)
            return;

        const char* arg1 = mode == no_sleep::Mode::KeepScreenOnAndKeepComputing ? "idle" : "sleep";
        const char* arg2 = app_name;
        const char* arg3 = reason;
        const char* arg4 = "block";

        DBusMessageIter args;
        dbus_message_iter_init_append(message, &args);
        if (!dbus_message_iter_append_basic(&args, DBUS_TYPE_STRING, &arg1) || !dbus_message_iter_append_basic(&args, DBUS_TYPE_STRING, &arg2) || !dbus_message_iter_append_basic(&args, DBUS_TYPE_STRING, &arg3) || !dbus_message_iter_append_basic(&args, DBUS_TYPE_STRING, &arg4))
            return;

        DBusPendingCall* pending;
        if (!dbus_connection_send_with_reply(connection, message, &pending, -1))
            return;

        dbus_connection_flush(connection);
        dbus_message_unref(message);
        dbus_pending_call_block(pending);

        DBusMessage* reply = dbus_pending_call_steal_reply(pending);
        if (reply)
        {
            DBusMessageIter reply_iter;
            dbus_message_iter_init(reply, &reply_iter);

            if (dbus_message_iter_get_arg_type(&reply_iter) == DBUS_TYPE_UNIX_FD)
                dbus_message_iter_get_basic(&reply_iter, &inhibit_fd);

            dbus_message_unref(reply);
        }

        dbus_pending_call_unref(pending);
        dbus_connection_unref(connection);
    }

    ~ImplPlatform() override
    {
        if (inhibit_fd >= 0)
            close(inhibit_fd);
    }

    ImplPlatform(ImplPlatform const&)                = delete;
    ImplPlatform& operator=(ImplPlatform const&)     = delete;
    ImplPlatform(ImplPlatform&&) noexcept            = delete;
    ImplPlatform& operator=(ImplPlatform&&) noexcept = delete;

private:
    int inhibit_fd = -1;
};
} // namespace
#endif
#if defined(__APPLE__)
#include <IOKit/pwr_mgt/IOPMLib.h>

namespace {
class ImplPlatform : public no_sleep::internal::Impl {
public:
    explicit ImplPlatform(const char* /* app_name */, const char* reason, no_sleep::Mode mode)
    {
        _reason = CFStringCreateWithCString(
            kCFAllocatorDefault,  // Default allocator
            reason,               // The C string to convert
            kCFStringEncodingUTF8 // The encoding (UTF-8 in this case)
        );
        if (_reason == nullptr)
        {
            assert(false);
            return;
        }
        _assertion_valid = IOPMAssertionCreateWithName(mode == no_sleep::Mode::KeepScreenOnAndKeepComputing ? kIOPMAssertionTypeNoDisplaySleep : kIOPMAssertionTypeNoIdleSleep, kIOPMAssertionLevelOn, _reason, &_assertion_id) == kIOReturnSuccess;
    }

    ~ImplPlatform() override
    {
        if (_assertion_valid)
            IOPMAssertionRelease(_assertion_id);
        if (_reason != nullptr)
            CFRelease(_reason);
    }

    ImplPlatform(ImplPlatform const&)                = delete;
    ImplPlatform& operator=(ImplPlatform const&)     = delete;
    ImplPlatform(ImplPlatform&&) noexcept            = delete;
    ImplPlatform& operator=(ImplPlatform&&) noexcept = delete;

private:
    IOPMAssertionID _assertion_id{};
    bool            _assertion_valid{false};
    CFStringRef     _reason{nullptr};
};
} // namespace
#endif

namespace no_sleep {

Scoped::Scoped(const char* app_name, const char* reason, Mode mode)
    : _pimpl{std::make_shared<ImplPlatform>(app_name, reason, mode)}
{
    assert(strlen(reason) <= 128 && "On MacOS this is limited to 128 characters");
}

} // namespace no_sleep
