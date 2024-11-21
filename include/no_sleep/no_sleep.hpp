#pragma once

namespace no_sleep {

/// Prevents the computer from going to sleep as long as at least one of these objects is alive
class Scoped {
public:
    Scoped();
    ~Scoped();

    Scoped(Scoped const&);
    Scoped& operator=(Scoped const&) = default;
    Scoped(Scoped&&) noexcept;
    Scoped& operator=(Scoped&&) noexcept = default;
};

} // namespace no_sleep
