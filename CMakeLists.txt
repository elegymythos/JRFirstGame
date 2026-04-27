cmake_minimum_required(VERSION 3.15)
project(JRFirstGame VERSION 1.0.0 LANGUAGES CXX C)

# =============================================================================
# MSVC 静态链接运行时库（避免依赖 vcruntime140.dll）
# =============================================================================
if(MSVC)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

# 构建选项
option(EMBED_RESOURCES "将字体嵌入到可执行文件中" ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# UTF-8 support for string literals
if(MSVC)
    add_compile_options(/utf-8)
else()
    add_compile_options(-finput-charset=UTF-8 -fexec-charset=UTF-8)
endif()

# =============================================================================
# 平台相关的静态链接配置
# =============================================================================
if(CMAKE_SYSTEM_NAME MATCHES "Windows")
    if(MINGW)
        set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static-libgcc -static-libstdc++")
        message(STATUS "MinGW: 使用静态 libgcc 和 libstdc++")
    endif()
endif()
if(APPLE)
    # 支持 Apple Silicon (arm64) 和 Intel (x86_64)
    if(CMAKE_HOST_SYSTEM_PROCESSOR MATCHES "arm64")
        list(APPEND CMAKE_PREFIX_PATH "/opt/homebrew")
    else()
        list(APPEND CMAKE_PREFIX_PATH "/usr/local")
    endif()
endif()
# =============================================================================
# 查找 SFML（默认优先动态库，通过 SFML_STATIC_LIBRARIES 可控制）
# =============================================================================
find_package(SFML 3 QUIET COMPONENTS Graphics Window System Network)
if(NOT SFML_FOUND)
    find_package(SFML 2 QUIET COMPONENTS Graphics Window System Network)
endif()

if(NOT SFML_FOUND AND UNIX AND NOT APPLE)
    find_package(PkgConfig QUIET)
    if(PkgConfig_FOUND)
        pkg_check_modules(SFML_PKG sfml-all)
        if(SFML_PKG_FOUND)
            set(SFML_FOUND TRUE)
            set(SFML_LIBRARIES ${SFML_PKG_LIBRARIES})
            set(SFML_INCLUDE_DIRS ${SFML_PKG_INCLUDE_DIRS})
            link_directories(${SFML_PKG_LIBRARY_DIRS})
        endif()
    endif()
endif()

if(NOT SFML_FOUND)
    foreach(comp graphics window system network)
        find_library(SFML_${comp}_LIB NAMES sfml-${comp} sfml-${comp}-s)
        if(NOT SFML_${comp}_LIB)
            message(FATAL_ERROR "找不到 SFML ${comp} 库")
        endif()
        list(APPEND SFML_LIBRARIES ${SFML_${comp}_LIB})
    endforeach()
    find_path(SFML_INCLUDE_DIR NAMES SFML/Graphics.hpp)
    set(SFML_INCLUDE_DIRS ${SFML_INCLUDE_DIR})
    set(SFML_FOUND TRUE)
endif()

message(STATUS "SFML 库: ${SFML_LIBRARIES}")

# =============================================================================
# SQLite3 源文件（编译为静态库）
# =============================================================================
set(SQLITE_SOURCES external/sqlite3.c)
set(SQLITE_HEADERS external/sqlite3.h)

add_library(sqlite3 STATIC ${SQLITE_SOURCES})
target_include_directories(sqlite3 PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/external)
target_compile_definitions(sqlite3 PRIVATE
    SQLITE_THREADSAFE=1
    SQLITE_ENABLE_FTS5
    SQLITE_DEFAULT_MEMSTATUS=0
)

# =============================================================================
# 资源嵌入（可选）
# =============================================================================
if(EMBED_RESOURCES)
    set(FONT_INPUT "${CMAKE_CURRENT_SOURCE_DIR}/assets/SmileySans-Oblique-2.ttf")
    set(FONT_OUTPUT "${CMAKE_CURRENT_SOURCE_DIR}/src/embedded_font.hpp")

    if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/tools/embed_resource.py")
        file(MAKE_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/tools")
        file(WRITE "${CMAKE_CURRENT_SOURCE_DIR}/tools/embed_resource.py"
"#!/usr/bin/env python3
import sys
def embed(infile, outfile, array_name, size_name):
    with open(infile, 'rb') as f:
        data = f.read()
    with open(outfile, 'w') as f:
        f.write('#pragma once\\n#include <cstddef>\\n\\n')
        f.write(f'const unsigned char {array_name}[] = {{')
        for i, b in enumerate(data):
            if i % 16 == 0: f.write('\\n    ')
            f.write(f'0x{b:02x}, ')
        f.write(f'\\n}};\\n')
        f.write(f'const size_t {size_name} = {len(data)};\\n')
if __name__ == '__main__':
    if len(sys.argv) != 5:
        print('Usage: embed.py <infile> <outfile> <array_name> <size_name>')
        sys.exit(1)
    embed(sys.argv[1], sys.argv[2], sys.argv[3], sys.argv[4])
")
    endif()

    if(EXISTS "${FONT_INPUT}")
        add_custom_command(
            OUTPUT ${FONT_OUTPUT}
            COMMAND python3 "${CMAKE_CURRENT_SOURCE_DIR}/tools/embed_resource.py"
                "${FONT_INPUT}"
                "${FONT_OUTPUT}"
                "embedded_font_data"
                "embedded_font_size"
            DEPENDS ${FONT_INPUT}
            COMMENT "将字体嵌入到头文件 ${FONT_OUTPUT}"
        )
        add_custom_target(embedded_resources DEPENDS ${FONT_OUTPUT})
        message(STATUS "资源嵌入已启用，字体将内嵌到可执行文件中")
        set(EMBED_HEADER ${FONT_OUTPUT})
    else()
        message(WARNING "字体文件不存在: ${FONT_INPUT}")
        set(EMBED_RESOURCES OFF)
    endif()
endif()

# =============================================================================
# 源文件列表
# =============================================================================
set(SOURCES
    src/Utils.cpp
    src/Database.cpp
    src/Network.cpp
    src/GameLogic.cpp
    src/WeaponList.cpp
    src/MapSystem.cpp
    src/I18n.cpp
    src/UI.cpp
    src/main.cpp
)

set(HEADERS
    src/Utils.hpp
    src/Database.hpp
    src/Network.hpp
    src/GameLogic.hpp
    src/WeaponList.hpp
    src/MapSystem.hpp
    src/I18n.hpp
    src/UI.hpp
    ${EMBED_HEADER}
)

# =============================================================================
# 创建可执行文件
# =============================================================================
add_executable(JRFirstGame ${SOURCES} ${HEADERS})

target_include_directories(JRFirstGame PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/external
    ${SFML_INCLUDE_DIRS}
)

# 链接库
target_link_libraries(JRFirstGame PRIVATE sqlite3)

if(TARGET SFML::Graphics)
    target_link_libraries(JRFirstGame PRIVATE
        SFML::Graphics
        SFML::Window
        SFML::System
        SFML::Network
    )
else()
    target_link_libraries(JRFirstGame PRIVATE ${SFML_LIBRARIES})
endif()

# ---------- 平台特定的系统库 ----------
if(UNIX AND NOT APPLE)
    find_package(OpenGL REQUIRED)
    target_link_libraries(JRFirstGame PRIVATE
        ${OPENGL_LIBRARIES}
        X11 Xrandr Xcursor Xi pthread dl
    )
endif()

if(WIN32)
    target_link_libraries(JRFirstGame PRIVATE
        winmm ws2_32 opengl32
    )
endif()

if(APPLE)
    target_link_libraries(JRFirstGame PRIVATE
        "-framework CoreFoundation"
        "-framework OpenGL"
    )
endif()

# 启用嵌入字体宏
if(EMBED_RESOURCES)
    target_compile_definitions(JRFirstGame PRIVATE EMBEDDED_FONT)
endif()

# 嵌入资源依赖
if(EMBED_RESOURCES AND TARGET embedded_resources)
    add_dependencies(JRFirstGame embedded_resources)
endif()

# =============================================================================
# 复制资源文件到输出目录
# =============================================================================
if(NOT EMBED_RESOURCES AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/assets/SmileySans-Oblique-2.ttf")
    add_custom_command(TARGET JRFirstGame POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:JRFirstGame>/assets"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${CMAKE_CURRENT_SOURCE_DIR}/assets/SmileySans-Oblique-2.ttf"
            "$<TARGET_FILE_DIR:JRFirstGame>/assets/"
        COMMENT "复制字体文件到输出目录"
    )
endif()

# 数据库文件不需要复制，程序会自动创建

# =============================================================================
# macOS 特定配置（创建 .app 包）
# =============================================================================
if(APPLE)
    set_target_properties(JRFirstGame PROPERTIES
        MACOSX_BUNDLE TRUE
        MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/Info.plist
    )
    target_link_libraries(JRFirstGame PRIVATE "-framework CoreFoundation")
endif()
# =============================================================================
# 配置摘要
# =============================================================================
message(STATUS "")
message(STATUS "======================================")
message(STATUS " JRFirstGame 构建配置")
message(STATUS "======================================")
message(STATUS "构建类型: ${CMAKE_BUILD_TYPE}")
message(STATUS "目标系统: ${CMAKE_SYSTEM_NAME}")
message(STATUS "嵌入资源: ${EMBED_RESOURCES}")
message(STATUS "SFML 库: ${SFML_LIBRARIES}")
message(STATUS "编译器: ${CMAKE_CXX_COMPILER_ID}")
message(STATUS "======================================")
message(STATUS "")

# =============================================================================
# 复制 SFML 动态库到输出目录（实现独立运行）
# =============================================================================

# Windows：复制 Release 版 SFML DLL
if(WIN32 AND DEFINED SFML_BIN_DIR)
    # 仅复制不带 -d 的 DLL（Release 版本）
    file(GLOB SFML_DLLS "${SFML_BIN_DIR}/sfml-*-3.dll")
    add_custom_command(TARGET JRFirstGame POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${SFML_DLLS}
            "$<TARGET_FILE_DIR:JRFirstGame>"
        COMMENT "复制 SFML Release DLL 文件到输出目录"
    )
    # 检查 openal32.dll 是否存在，存在则复制
    if(EXISTS "${SFML_BIN_DIR}/openal32.dll")
        add_custom_command(TARGET JRFirstGame POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${SFML_BIN_DIR}/openal32.dll"
                "$<TARGET_FILE_DIR:JRFirstGame>"
            COMMENT "复制 OpenAL32 DLL"
        )
    endif()
endif()
# Linux：复制 SO 文件并设置 RPATH
if(UNIX AND NOT APPLE AND DEFINED SFML_LIB_DIR)
    file(GLOB SFML_SOS "${SFML_LIB_DIR}/libsfml-*.so*")
    add_custom_command(TARGET JRFirstGame POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${SFML_SOS}
            "$<TARGET_FILE_DIR:JRFirstGame>"
        COMMENT "复制 SFML SO 文件到输出目录"
    )
    set_target_properties(JRFirstGame PROPERTIES
        INSTALL_RPATH "$ORIGIN"
        BUILD_WITH_INSTALL_RPATH TRUE
    )
endif()

# macOS：复制 dylib 到 .app 包内
if(APPLE AND DEFINED SFML_LIB_DIR)
    set(APP_FRAMEWORKS_DIR "$<TARGET_BUNDLE_CONTENT_DIR:JRFirstGame>/Frameworks")
    file(GLOB SFML_DYLIBS "${SFML_LIB_DIR}/libsfml-*.dylib")
    add_custom_command(TARGET JRFirstGame POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory "${APP_FRAMEWORKS_DIR}"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${SFML_DYLIBS}
            "${APP_FRAMEWORKS_DIR}"
        COMMENT "复制 SFML dylib 文件到 .app 包内"
    )
    set_target_properties(JRFirstGame PROPERTIES
        INSTALL_RPATH "@executable_path/../Frameworks"
        BUILD_WITH_INSTALL_RPATH TRUE
    )
endif()
# 复制字体许可证文件到输出目录
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/OFL.txt")
    add_custom_command(TARGET JRFirstGame POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${CMAKE_CURRENT_SOURCE_DIR}/OFL.txt"
            "$<TARGET_FILE_DIR:JRFirstGame>"
        COMMENT "复制 OFL.txt 许可证文件"
    )
endif()