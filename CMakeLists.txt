cmake_minimum_required(VERSION 3.13)
project("vde switch"
    DESCRIPTION "An improved version of the old vde switch"
    VERSION 0.0.1
    LANGUAGES C
)

set(CMAKE_C_STANDARD 99)

# include(GNUInstallDirs)
# include(CheckIncludeFile)
# include(CheckSymbolExists)

add_executable(vde_switch vde_switch.c)

add_subdirectory(vsconf)

target_link_libraries(vde_switch vsconf)

target_include_directories(vde_switch PUBLIC
                           "${PROJECT_BINARY_DIR}"
                           "${PROJECT_SOURCE_DIR}/vsconf")