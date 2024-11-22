cmake_minimum_required(VERSION 3.8)

set(WARNINGS_AS_ERRORS_FOR_NO_SLEEP OFF CACHE BOOL "ON iff you want to treat warnings as errors")

add_library(no_sleep)
add_library(no_sleep::no_sleep ALIAS no_sleep)
target_compile_features(no_sleep PUBLIC cxx_std_11)

# ---Add source files---
if(WARNINGS_AS_ERRORS_FOR_NO_SLEEP)
    target_include_directories(no_sleep PUBLIC include)
else()
    target_include_directories(no_sleep SYSTEM PUBLIC include)
endif()

file(GLOB_RECURSE SRC_FILES CONFIGURE_DEPENDS src/*.cpp)
target_sources(no_sleep PRIVATE ${SRC_FILES})

# ---Set warning level---
if(MSVC)
    target_compile_options(no_sleep PRIVATE /W4)
else()
    target_compile_options(no_sleep PRIVATE -Wall -Wextra -Wpedantic -pedantic-errors -Wconversion -Wsign-conversion -Wimplicit-fallthrough)
endif()

# ---Maybe enable warnings as errors---
if(WARNINGS_AS_ERRORS_FOR_NO_SLEEP)
    if(MSVC)
        target_compile_options(no_sleep PRIVATE /WX)
    else()
        target_compile_options(no_sleep PRIVATE -Werror)
    endif()
endif()

# ---Add DBus lib---
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}")
    find_package(Dbus1 REQUIRED)
    target_include_directories(no_sleep PRIVATE ${DBUS_INCLUDE_DIRS})
    target_link_libraries(no_sleep PRIVATE ${DBUS_LIBRARIES})
endif()