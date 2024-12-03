add_rules(
    "mode.tsan", "mode.ubsan", "mode.asan", 
    "mode.debug", "mode.release", "mode.valgrind"
)

add_requires(
    "gtest",
    "concurrentqueue"
)

add_packages("concurrentqueue")

set_languages("c++20", "c17")
set_policy("build.warning", true)
set_policy("build.optimization.lto", false)
set_toolset("cc", "mold", {force = true}) 

if is_mode("asan") then
    add_cxxflags("-fno-inline", {force = true})
    set_optimize("none", {force = true})
end


target("fifo-lru-cache")
    set_kind("headeronly")
    add_packages("concurrentqueue");
    set_warnings("all", "error")
    add_includedirs(
        "include", 
        "smhasher/src",
        "libcuckoo/libcuckoo", 
        { public = true }
    )

target("fifo-lru-cache-test")
    set_kind("binary")
    add_deps("fifo-lru-cache")
    add_files(
        "test/*.cc",
        "smhasher/src/Murmur*.cpp", 
        "libcuckoo/libcuckoo"
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
