add_rules("mode.release", "mode.debug")

set_languages("c++20")

set_project("attr")
set_version("0.1.0")

if is_mode("asan") or is_mode("tsan") or is_mode("msan") or is_mode("ubsan") then
    add_rules("sanitizer.address", "sanitizer.thread", "sanitizer.memory", "sanitizer.undefined")
end

add_cxflags("-save-temps")

if is_mode("coverage") then
    if is_plat("macosx") then
        add_cxflags("-fprofile-instr-generate", "-fcoverage-mapping")
        add_ldflags("-fprofile-instr-generate", "-fcoverage-mapping")
    elseif is_plat("linux") then
        add_cxflags("--coverage")
        add_ldflags("--coverage")
    end
end

-- 启用 Unity Build
rule("unity_build")
    on_build_files(function (target, batchjobs, sourcebatch)
        import("core.project.depend")
        local outputdir = path.join(target:autogendir(), "rules", "unity_build")
        os.mkdir(outputdir)

        local unity_source = path.join(outputdir, target:name() .. "_unity_build.cpp")
        local unity_file = io.open(unity_source, "w")

        for _, sourcefile in ipairs(sourcebatch.sourcefiles) do
            unity_file:print('#include "%s"', path.relative(sourcefile, outputdir))
        end

        unity_file:close()

        depend.on_changed(function ()
            os.cp(unity_source, sourcebatch.sourcefiles[1])
        end, {files = sourcebatch.sourcefiles, target = target})

        batchjobs:build_file(target, unity_source, target:objectfile(unity_source))
    end)
rule_end()

add_rules("unity_build", {batchsize = 4})

option("use_modules")
    set_default(false)
    set_showmenu(true)
    set_description("Use C++ modules (cppm) if enabled, otherwise use hpp")
option_end()

target("attr")
    set_kind("headeronly")

    if has_config("use_modules") then
        add_files("include/attr/*.cppm", {install = true})
    else
        add_headerfiles("include/attr/*.hpp", {install = true})
    end
    add_includedirs("include/attr")

option("test_on")
    set_default(true)
    set_showmenu(true)
    set_description("Enable test build")
option_end()

if has_config("test_on") then
    includes("test")
end