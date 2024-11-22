# Try to locate pkg-config
find_package(PkgConfig REQUIRED)

# Use pkg-config to find DBus
pkg_check_modules(DBUS REQUIRED dbus-1)

# Set DBus include directories
set(DBUS_INCLUDE_DIRS ${DBUS_INCLUDE_DIRS})

# Set DBus library
set(DBUS_LIBRARIES ${DBUS_LIBRARIES})

# Set DBus flags
set(DBUS_CFLAGS_OTHER ${DBUS_CFLAGS_OTHER})

# Optionally, mark DBus as found
find_package_handle_standard_args(DBus REQUIRED_VARS DBUS_INCLUDE_DIRS DBUS_LIBRARIES)
