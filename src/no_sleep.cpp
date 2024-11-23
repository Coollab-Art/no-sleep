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
    ImplPlatform(const char* /* app_name */, const char* /* reason */, no_sleep::Mode mode)
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
        auto const res = SetThreadExecutionState(
            ES_CONTINUOUS
            | (display_required_count() > 0 ? ES_DISPLAY_REQUIRED : 0)
            | (system_required_count() > 0 ? ES_SYSTEM_REQUIRED : 0)
        );
        assert(res != 0);
        std::ignore = res;
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
#include <iostream>

#define ERR_PREFIX "[no_sleep] Failed to prevent screen from sleeping: " // NOLINT(*macro-usage)

namespace {
class ImplPlatform : public no_sleep::internal::Impl {
public:
    ImplPlatform(const char* app_name, const char* reason, no_sleep::Mode /*mode*/) // NOLINT(*member-init)
    {
        // TODO: we don't handle `mode` because I couldn't figure out how to implement ScreenCanTurnOffButKeepComputing
        // So we always do KeepScreenOnAndKeepComputing

        DBusError error;
        dbus_error_init(&error);

        _connection = dbus_bus_get(DBUS_BUS_SESSION, &error); // NOLINT(*prefer-member-initializer)
        if (dbus_error_is_set(&error))
        {
            assert(_connection == nullptr);
            std::cerr << ERR_PREFIX "Error connecting to the D-Bus session bus: " << error.message << '\n';
            dbus_error_free(&error);
            return;
        }

        DBusMessage* const msg = dbus_message_new_method_call(
            "org.freedesktop.ScreenSaver",
            "/org/freedesktop/ScreenSaver",
            "org.freedesktop.ScreenSaver",
            "Inhibit"
        );
        if (!msg)
        {
            std::cerr << ERR_PREFIX "Failed to create DBus message\n";
            free_connection();
            return;
        }

        dbus_message_append_args(msg, DBUS_TYPE_STRING, &app_name, DBUS_TYPE_STRING, &reason, DBUS_TYPE_INVALID);

        DBusMessage* const reply = dbus_connection_send_with_reply_and_block(_connection, msg, -1, &error);
        dbus_message_unref(msg);

        if (dbus_error_is_set(&error))
        {
            std::cerr << ERR_PREFIX "Error sending DBus message: " << error.message << '\n';
            dbus_error_free(&error);
            free_connection();
            return;
        }

        if (!dbus_message_get_args(reply, &error, DBUS_TYPE_UINT32, &_cookie, DBUS_TYPE_INVALID))
        {
            std::cerr << ERR_PREFIX "Error reading reply: " << error.message << '\n';
            dbus_error_free(&error);
            dbus_message_unref(reply);
            free_connection();
            return;
        }

        dbus_message_unref(reply);
        _has_been_init = true;
    }

    ~ImplPlatform() override
    {
        if (!_has_been_init)
            return;

        DBusMessage* const msg = dbus_message_new_method_call(
            "org.freedesktop.ScreenSaver",
            "/org/freedesktop/ScreenSaver",
            "org.freedesktop.ScreenSaver",
            "UnInhibit"
        );
        if (!msg)
        {
            std::cerr << ERR_PREFIX "Failed to create DBus message for UnInhibit\n";
            free_connection();
            return;
        }

        dbus_message_append_args(msg, DBUS_TYPE_UINT32, &_cookie, DBUS_TYPE_INVALID);

        if (!dbus_connection_send(_connection, msg, nullptr))
            std::cerr << ERR_PREFIX "Failed to send UnInhibit message\n";

        dbus_message_unref(msg);
        free_connection();
    }

    ImplPlatform(ImplPlatform const&)                = delete;
    ImplPlatform& operator=(ImplPlatform const&)     = delete;
    ImplPlatform(ImplPlatform&&) noexcept            = delete;
    ImplPlatform& operator=(ImplPlatform&&) noexcept = delete;

private:
    void free_connection()
    {
        dbus_connection_unref(_connection);
    }

private:
    dbus_uint32_t   _cookie;
    DBusConnection* _connection;
    bool            _has_been_init{false};
};
} // namespace
#undef ERR_PREFIX
#endif
#if defined(__APPLE__)
#include <IOKit/pwr_mgt/IOPMLib.h>

namespace {
class ImplPlatform : public no_sleep::internal::Impl {
public:
    ImplPlatform(const char* /* app_name */, const char* reason, no_sleep::Mode mode)
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
        assert(_assertion_valid);
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
