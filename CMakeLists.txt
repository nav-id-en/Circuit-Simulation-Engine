cmake_minimum_required(VERSION 3.21)

project(ProteusLabSDL VERSION 2.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")

option(PROTEUS_BUILD_GUI "Build the SDL2 desktop application" ON)
option(PROTEUS_BUILD_TESTS "Build automated tests" ON)
option(PROTEUS_ENABLE_SANITIZERS "Enable ASan and UBSan" OFF)

set(PROTEUS_CORE_SOURCES
    src/core/Component.cpp
    src/core/Circuit.cpp
    src/components/Components.cpp
    src/simulation/SimulationEngine.cpp
    src/simulation/FirmwareLoader.cpp
    src/persistence/SimpleJson.cpp
    src/persistence/CircuitSerializer.cpp
)

add_library(proteus_core STATIC ${PROTEUS_CORE_SOURCES})
target_include_directories(proteus_core PUBLIC include)
target_compile_features(proteus_core PUBLIC cxx_std_20)

function(proteus_warnings target)
    if(MSVC)
        target_compile_options(${target} PRIVATE /W4 /permissive-)
    else()
        target_compile_options(${target} PRIVATE
            -Wall -Wextra -Wpedantic
        )
    endif()
endfunction()

proteus_warnings(proteus_core)

if(PROTEUS_ENABLE_SANITIZERS AND NOT MSVC)
    target_compile_options(proteus_core PUBLIC
        -fsanitize=address,undefined -fno-omit-frame-pointer
    )
    target_link_options(proteus_core PUBLIC
        -fsanitize=address,undefined
    )
endif()

if(PROTEUS_BUILD_GUI)
    find_package(SDL2 CONFIG REQUIRED)

    add_executable(ProteusLabSDL
        src/main.cpp
        src/ui/Draw.cpp
        src/ui/StudioApp.cpp
        src/ui/Render.cpp
        src/ui/Geometry.cpp
        src/ui/Model.cpp
        src/ui/Input.cpp
    )
    target_include_directories(ProteusLabSDL PRIVATE include)
    target_compile_definitions(ProteusLabSDL PRIVATE SDL_MAIN_HANDLED)
    target_link_libraries(ProteusLabSDL PRIVATE proteus_core SDL2::SDL2)
    proteus_warnings(ProteusLabSDL)

    add_custom_command(TARGET ProteusLabSDL POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_SOURCE_DIR}/examples"
            "$<TARGET_FILE_DIR:ProteusLabSDL>/examples"
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_SOURCE_DIR}/firmware"
            "$<TARGET_FILE_DIR:ProteusLabSDL>/firmware"
        VERBATIM
    )

    install(TARGETS ProteusLabSDL RUNTIME DESTINATION .)
    install(DIRECTORY examples firmware docs DESTINATION .)
    install(FILES README.md README_FA.md DESTINATION .)
endif()

if(PROTEUS_BUILD_TESTS)
    enable_testing()
    add_executable(proteus_core_tests tests/core_tests.cpp)
    target_link_libraries(proteus_core_tests PRIVATE proteus_core)
    proteus_warnings(proteus_core_tests)
    add_test(NAME proteus_core_tests COMMAND proteus_core_tests)
endif()
