include(GNUInstallDirs)

configure_file(version.hpp.in ${ATTR_GENERATED_INCLUDEDIR}/attr/version.hpp @ONLY)

add_library(attr INTERFACE)
add_library(attr::attr ALIAS attr)
target_compile_features(attr INTERFACE cxx_std_${CMAKE_CXX_STANDARD})
target_include_directories(attr INTERFACE
        $<BUILD_INTERFACE:${ATTR_GENERATED_INCLUDEDIR}>
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)