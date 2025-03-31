add_rules("mode.release", "mode.debug")

set_languages("c++20")

set_project("attr")
set_version("0.1.0")

target("attr")
    set_kind("headeronly")
    add_headerfiles("include/attr/*.hpp", {install = true})
    add_includedirs("include/attr")

includes("test")