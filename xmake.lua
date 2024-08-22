add_rules(
    "mode.tsan", "mode.ubsan", "mode.asan", 
    "mode.debug", "mode.release", "mode.valgrind"
)

add_requires(
    "benchmark", "gtest"
)

set_languages("c++23", "c17")
set_policy("build.warning", true)
set_policy("build.optimization.lto", false)
set_toolset("cc", "mold", {force = true}) 

if is_mode("asan") then
    add_cxxflags("-fno-inline", {force = true})
    set_optimize("none", {force = true})
end

if is_mode("release") then
    set_optimize("fastest", {force = true})
end

target("nbtlru")
    set_kind("headeronly")
    set_warnings("all", "error")
    add_cxflags("-Wconversion", { force = true })
    add_includedirs(
        "include", 
        { public = true }
    )

target("test")
    set_kind("binary")
    add_cxflags("-Wconversion", { force = true })
    add_deps("nbtlru")
    add_files("test/*.cc")
    add_packages("gtest")
    set_warnings("all", "error")
    after_build(function (target)
        os.execv(target:targetfile(), {"--gtest_color=yes"})
        print("xmake: unittest complete.")
    end)
    on_run(function (target)
        --nothing
    end)

target("benchmark")
    set_kind("binary")
    add_packages("benchmark")
    add_cxflags("-Wconversion", { force = true })
    add_files("benchmark/*.cc")
    add_packages("spdlog")
    set_policy("build.warning", true)
    add_packages(
        "gflags", 
        "concurrentqueue"
    )
    
