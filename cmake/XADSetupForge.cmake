set(FORGE_INSTALL_DIR "${CMAKE_BINARY_DIR}/forge-install" CACHE PATH "Forge installation directory")

# Try to find pre-installed Forge
find_package(Forge CONFIG QUIET PATHS "${FORGE_INSTALL_DIR}" NO_DEFAULT_PATH)
if(NOT TARGET Forge::forge)
    find_package(Forge CONFIG QUIET)
endif()

if(TARGET Forge::forge)
    message(STATUS "Found installed Forge package")
else()
    message(STATUS "Forge not found - building and installing from source...")

    include(FetchContent)

    FetchContent_Declare(
        forge
        GIT_REPOSITORY https://github.com/da-roth/forge.git
        GIT_TAG        main
        GIT_SHALLOW    TRUE
    )

    FetchContent_GetProperties(forge)
    if(NOT forge_POPULATED)
        FetchContent_Populate(forge)
    endif()

    set(FORGE_BUILD_DIR "${CMAKE_BINARY_DIR}/forge-build")

    # Configure Forge using its packaging CMakeLists
    execute_process(
        COMMAND ${CMAKE_COMMAND}
            -S "${forge_SOURCE_DIR}/tools/packaging"
            -B "${FORGE_BUILD_DIR}"
            -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
            -DCMAKE_INSTALL_PREFIX=${FORGE_INSTALL_DIR}
            -G "${CMAKE_GENERATOR}"
        RESULT_VARIABLE forge_config_result
    )
    if(NOT forge_config_result EQUAL 0)
        message(FATAL_ERROR "Failed to configure Forge")
    endif()

    # Build Forge
    execute_process(
        COMMAND ${CMAKE_COMMAND} --build "${FORGE_BUILD_DIR}" --config ${CMAKE_BUILD_TYPE}
        RESULT_VARIABLE forge_build_result
    )
    if(NOT forge_build_result EQUAL 0)
        message(FATAL_ERROR "Failed to build Forge")
    endif()

    # Install Forge
    execute_process(
        COMMAND ${CMAKE_COMMAND} --install "${FORGE_BUILD_DIR}" --config ${CMAKE_BUILD_TYPE}
        RESULT_VARIABLE forge_install_result
    )
    if(NOT forge_install_result EQUAL 0)
        message(FATAL_ERROR "Failed to install Forge")
    endif()

    find_package(Forge CONFIG REQUIRED PATHS "${FORGE_INSTALL_DIR}" NO_DEFAULT_PATH)
    message(STATUS "Forge built and installed to: ${FORGE_INSTALL_DIR}")
endif()

message(STATUS "Forge JIT backend enabled")
