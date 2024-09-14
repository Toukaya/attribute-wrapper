add_requires("catch2 3.x", { alias = "catch2" })

target("test")
    set_kind("binary")  -- 定义为可执行文件
    add_files("attr_test.cpp")
    add_packages("catch2")
    add_deps("attr")
    add_includedirs("../include/attr")