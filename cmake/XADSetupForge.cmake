include(FetchContent)

# Cache directory for dependencies (speeds up repeated builds)
set(FETCHCONTENT_BASE_DIR "${CMAKE_BINARY_DIR}/_deps" CACHE PATH "Base directory for FetchContent downloads")

# Allow using pre-installed Forge instead of fetching
set(XAD_FORGE_SOURCE "fetch" CACHE STRING "How to find Forge: 'fetch' (download from GitHub) or 'find' (use find_package)")
set_property(CACHE XAD_FORGE_SOURCE PROPERTY STRINGS fetch find)

if(XAD_FORGE_SOURCE STREQUAL "find")
    find_package(Forge REQUIRED)
    message(STATUS "Using pre-installed Forge")
else()
    message(STATUS "Fetching Forge from GitHub (https://github.com/da-roth/forge)")

    # Forge configuration options
    set(FORGE_BUILD_TESTS OFF CACHE BOOL "" FORCE)
    set(FORGE_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    set(FORGE_BUILD_BENCHMARKS OFF CACHE BOOL "" FORCE)

    FetchContent_Declare(
        forge
        GIT_REPOSITORY https://github.com/da-roth/forge.git
        GIT_TAG        main
        GIT_SHALLOW    TRUE
        GIT_PROGRESS   TRUE
    )

    FetchContent_GetProperties(forge)
    if(NOT forge_POPULATED)
        message(STATUS "Downloading Forge... (this may take a moment on first build)")
        FetchContent_Populate(forge)
        add_subdirectory(${forge_SOURCE_DIR} ${forge_BINARY_DIR} EXCLUDE_FROM_ALL)
        message(STATUS "Forge source dir: ${forge_SOURCE_DIR}")
    endif()

    if(NOT TARGET Forge::forge)
        if(TARGET forge)
            add_library(Forge::forge ALIAS forge)
        endif()
    endif()
endif()

if(NOT TARGET Forge::forge AND NOT TARGET forge)
    message(FATAL_ERROR "Forge library not found. Please check your XAD_FORGE_SOURCE setting.")
endif()

message(STATUS "Forge JIT backend enabled")
