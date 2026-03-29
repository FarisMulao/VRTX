cmake_minimum_required(VERSION 3.20)
project(VRTX VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Avoid treating standard functions as insecure via MSVC
add_compile_definitions(_CRT_SECURE_NO_WARNINGS NOMINMAX WIN32_LEAN_AND_MEAN)

include(FetchContent)

FetchContent_Declare(
    imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG        v1.90.4
)
FetchContent_MakeAvailable(imgui)

set(SOURCES
    src/main.cpp
    src/app/Application.cpp
    src/app/Window.cpp
    src/graphics/D3D12Context.cpp
    src/graphics/Renderer.cpp
    src/graphics/Camera.cpp
    src/graphics/Mesh.cpp
    src/physics/PhysicsWorld.cpp
    src/input/Input.cpp
    ${imgui_SOURCE_DIR}/imgui.cpp
    ${imgui_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_SOURCE_DIR}/imgui_tables.cpp
    ${imgui_SOURCE_DIR}/imgui_widgets.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_win32.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_dx12.cpp
)

set(HEADERS
    src/app/Application.h
    src/app/Window.h
    src/graphics/D3D12Context.h
    src/graphics/Renderer.h
    src/graphics/Camera.h
    src/graphics/Mesh.h
    src/physics/PhysicsWorld.h
    src/physics/VerletBody.h
    src/physics/Constraint.h
    src/input/Input.h
    src/math/Vec3.h
    src/math/Mat4.h
)

# HLSL Shaders: copy to build dir on every build so edits are always picked up
set(SHADER_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/src/shaders/PhysicsCompute.hlsl
    ${CMAKE_CURRENT_SOURCE_DIR}/src/shaders/InstancedRenderer.hlsl
    ${CMAKE_CURRENT_SOURCE_DIR}/src/shaders/ContainerShader.hlsl
)
foreach(SHADER ${SHADER_FILES})
    get_filename_component(SHADER_NAME ${SHADER} NAME)
    add_custom_command(
        OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${SHADER_NAME}
        COMMAND ${CMAKE_COMMAND} -E copy_if_different ${SHADER} ${CMAKE_CURRENT_BINARY_DIR}/${SHADER_NAME}
        DEPENDS ${SHADER}
        COMMENT "Copying ${SHADER_NAME} to build directory"
    )
    list(APPEND SHADER_OUTPUTS ${CMAKE_CURRENT_BINARY_DIR}/${SHADER_NAME})
endforeach()
add_custom_target(CopyShaders ALL DEPENDS ${SHADER_OUTPUTS})

add_executable(${PROJECT_NAME} WIN32 ${SOURCES} ${HEADERS})
add_dependencies(${PROJECT_NAME} CopyShaders)

target_include_directories(${PROJECT_NAME} PRIVATE src ${imgui_SOURCE_DIR} ${imgui_SOURCE_DIR}/backends)

# Pass source dir so Renderer.cpp can find shaders by absolute path as fallback
target_compile_definitions(${PROJECT_NAME} PRIVATE
    VRTX_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}"
)

# Link D3D12 dependencies
target_link_libraries(${PROJECT_NAME} PRIVATE
    d3d12
    dxgi
    d3dcompiler
)

if (MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE /W3 /Zc:preprocessor)
endif()
