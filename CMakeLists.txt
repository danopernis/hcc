cmake_minimum_required (VERSION 3.1.0)

project (hcc
    VERSION 0.0.1
    LANGUAGES C CXX
    )

find_package (Threads)
find_package (PkgConfig)
pkg_check_modules (GTKMM gtkmm-3.0)

add_compile_options (-Wall -Wextra -pedantic -std=c++14)

enable_testing ()
add_subdirectory (src)
add_subdirectory (tests)
