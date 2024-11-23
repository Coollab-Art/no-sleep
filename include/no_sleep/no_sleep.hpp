#pragma once
#include <memory>

namespace no_sleep {

namespace internal {
class Impl {
public:
    Impl()                           = default;
    virtual ~Impl()                  = default;
    Impl(Impl const&)                = delete;
    Impl& operator=(Impl const&)     = delete;
    Impl(Impl&&) noexcept            = delete;
    Impl& operator=(Impl&&) noexcept = delete;
};
} // namespace internal

enum class Mode {
    KeepScreenOnAndKeepComputing,
    ScreenCanTurnOffButKeepComputing,
};

/// Prevents the computer from going to sleep as long as at least one of these objects is alive
class Scoped {
public:
    Scoped(const char* app_name, const char* reason, Mode mode);

private:
    std::shared_ptr<internal::Impl> _pimpl;
};

} // namespace no_sleep
