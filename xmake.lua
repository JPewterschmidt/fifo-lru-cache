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
        "libcuckoo/libcuckoo", 
        { public = true }
    )
