cmake_minimum_required(VERSION 3.15.0)
project(attr VERSION 0.1.0
        DESCRIPTION "An optional-like header-only library"
        HOMEPAGE_URL https://github.com/yourusername/attr
        LANGUAGES CXX)

# 设置C++标准
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Header-only库目标
add_library(attr INTERFACE)
add_library(attr::attr ALIAS attr)
target_include_directories(attr INTERFACE
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)
target_compile_features(attr INTERFACE cxx_std_20)

# 测试配置
option(BUILD_TESTING "Build tests" ON)
if(BUILD_TESTING)
    enable_testing()
    find_package(Catch2 3 REQUIRED)
    
    add_executable(attr_test test/attr_test.cpp)
    target_link_libraries(attr_test PRIVATE
        attr
        Catch2::Catch2WithMain
    )
    target_include_directories(attr_test PRIVATE
        ${PROJECT_SOURCE_DIR}/include/attr
        ${PROJECT_SOURCE_DIR}/include
    )
endif()

# 安装配置
include(GNUInstallDirs)
install(TARGETS attr
    EXPORT attr-targets
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)
install(DIRECTORY include/attr
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    FILES_MATCHING PATTERN "*.hpp"
)
install(EXPORT attr-targets
    FILE attr-targets.cmake
    NAMESPACE attr::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/attr
)