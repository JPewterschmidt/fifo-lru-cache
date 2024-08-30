add_rules(
    "mode.tsan", "mode.ubsan", "mode.asan", 
    "mode.debug", "mode.release", "mode.valgrind"
)

add_requires(
    "benchmark", "gtest", "csv2"
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

includes("toolpex")

target("nbtlru")
    set_kind("headeronly")
    set_warnings("all", "error")
    add_includedirs(
        "include", 
        { public = true }
    )
    add_includedirs(
        "smhasher/src",
        { public = false }
    )

target("test")
    set_kind("binary")
    add_deps("nbtlru")
    add_files(
        "test/*.cc",
        "smhasher/src/Murmur*.cpp"
    )
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
    add_files(
        "benchmark/*.cc", 
        "smhasher/src/Murmur*.cpp"
    )
    add_includedirs("dirtyzipf")
    add_packages("csv2")
    set_policy("build.warning", true)
    add_deps("toolpex")
    add_packages(
        "gflags", 
        "concurrentqueue"
    )
    
