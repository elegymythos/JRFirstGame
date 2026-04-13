cmake_minimum_required(VERSION 3.15)
project(JRFirstGame VERSION 1.0.0 LANGUAGES CXX C)

# 构建选项
option(BUILD_STATIC "静态链接生成独立可执行文件" ON)
option(EMBED_RESOURCES "将字体嵌入到可执行文件中" ON)   # 你已经有了 embedded_font.hpp，设为 ON

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# 静态链接配置
if(BUILD_STATIC)
    message(STATUS "静态链接模式已启用")
    if(MSVC)
        set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
    elseif(MINGW)
        set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static-libgcc -static-libstdc++")
    endif()
endif()

# 查找 SFML
set(SFML_STATIC_LIBRARIES ${BUILD_STATIC})
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
    # 手动查找
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

# SQLite3 源文件
set(SQLITE_SOURCES external/sqlite3.c)
set(SQLITE_HEADERS external/sqlite3.h)

# 资源嵌入（使用你已经生成的 embedded_font.hpp）
if(EMBED_RESOURCES)
    # 如果还没有 embedded_font.hpp，则调用脚本生成
    if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/src/embedded_font.hpp")
        message(STATUS "生成字体嵌入头文件...")
        execute_process(
            COMMAND python3 ${CMAKE_CURRENT_SOURCE_DIR}/embed_font.py
                ${CMAKE_CURRENT_SOURCE_DIR}/assets/SmileySans-Oblique-2.ttf
                ${CMAKE_CURRENT_SOURCE_DIR}/src/embedded_font.hpp
                embedded_font_data
                embedded_font_size
        )
    else()
        message(STATUS "字体嵌入头文件已存在: src/embedded_font.hpp")
    endif()
    set(EMBED_HEADER ${CMAKE_CURRENT_SOURCE_DIR}/src/embedded_font.hpp)
endif()

# 源文件列表
set(SOURCES
    src/Utils.cpp
    src/Database.cpp
    src/Network.cpp
    src/GameLogic.cpp
    src/UI.cpp
    src/main.cpp
    ${SQLITE_SOURCES}
)

set(HEADERS
    src/Utils.hpp
    src/Database.hpp
    src/Network.hpp
    src/GameLogic.hpp
    src/UI.hpp
    ${SQLITE_HEADERS}
    ${EMBED_HEADER}
)

# 可执行文件
add_executable(JRFirstGame ${SOURCES} ${HEADERS})

target_include_directories(JRFirstGame PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/external
    ${SFML_INCLUDE_DIRS}
)

# 链接 SFML
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

# Linux 静态链接额外系统库
if(UNIX AND NOT APPLE AND BUILD_STATIC)
    find_package(OpenGL REQUIRED)
    target_link_libraries(JRFirstGame PRIVATE
        ${OPENGL_LIBRARIES}
        X11 Xrandr Xcursor Xi pthread dl
    )
endif()

# SQLite 编译选项
set_source_files_properties(${SQLITE_SOURCES} PROPERTIES COMPILE_DEFINITIONS 
    "SQLITE_THREADSAFE=1;SQLITE_ENABLE_FTS5;SQLITE_DEFAULT_MEMSTATUS=0")

# 启用嵌入字体宏
if(EMBED_RESOURCES)
    target_compile_definitions(JRFirstGame PRIVATE EMBEDDED_FONT)
endif()

# 安装
install(TARGETS JRFirstGame RUNTIME DESTINATION bin)
if(NOT EMBED_RESOURCES)
    install(DIRECTORY assets/ DESTINATION bin/assets)
endif()

message(STATUS "构建配置完成。")