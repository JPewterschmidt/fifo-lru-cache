add_rules(
    "mode.tsan", "mode.ubsan", "mode.asan", 
    "mode.debug", "mode.release", "mode.valgrind"
)

add_requires(
    "benchmark", "gtest", "csv2",
    "concurrentqueue", 
    "gperftools"
)

add_packages("csv2", "concurrentqueue")

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

add_includedirs("libcuckoo/libcuckoo")

target("nbtlru")
    set_kind("headeronly")
    add_packages("concurrentqueue");
    add_packages("atomic_queue");
    set_warnings("all", "error")
    add_includedirs(
        "include", 
        "smhasher/src",
        "rustex",
        { public = true }
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

target("benchmark-deps")
    set_kind("shared")
    add_packages("csv2")
    add_files(
        "benchmark-deps/*.cc",
        "smhasher/src/Murmur*.cpp"
    )
    add_includedirs("dirtyzipf")
    add_packages("csv2")
    set_policy("build.warning", false)
    add_deps("nbtlru")
    add_packages(
        "gflags", 
        "concurrentqueue"
    )


target("benchmark")
    set_kind("binary")
    add_deps("benchmark-deps")
    add_files(
        "benchmark/*.cc",
        "smhasher/src/Murmur*.cpp"
    )
    add_includedirs("dirtyzipf")
    add_packages("csv2")
    set_policy("build.warning", true)
    add_deps("nbtlru")
    add_packages(
        "gflags", 
        "concurrentqueue"
    )
    after_run(function (target)
        if is_mode("debug") then
            return
        end

        print("drawing pictures ... ")

        os.execv(
            "python", { 
                "scripts/different_dist.py", 
                target:targetdir() .. "/different_dist_on_naive.csv", 
                "experiment-report/pics/different_dist_on_naive.png"
            }
        )

        os.execv(
            "python", { 
                "scripts/different_dist.py", 
                target:targetdir() .. "/different_dist_on_fifo_hybrid.csv", 
                "experiment-report/pics/different_dist_on_fifo_hybrid.png"
            }
        )

        os.execv(
            "python", { 
                "scripts/different_dist.py", 
                target:targetdir() .. "/different_dist_on_sampling.csv", 
                "experiment-report/pics/different_dist_on_sampling.png"
            }
        )


        os.execv(
            "python", { 
                "scripts/multi_threads_on_both_in_total.py", 
                target:targetdir() .. "/multi_threads_on_naive_latency.csv", 
                target:targetdir() .. "/multi_threads_on_fifo_hybrid_latency.csv", 
                target:targetdir() .. "/multi_threads_on_sampling_latency.csv", 
                "experiment-report/pics/multi_threads_on_both_latency", 
                "Time Elasped (ms)", 
                "1"
            }
        )

        os.execv(
            "python", { 
                "scripts/multi_threads_on_both_in_total.py", 
                target:targetdir() .. "/multi_threads_on_naive_hitratio.csv", 
                target:targetdir() .. "/multi_threads_on_fifo_hybrid_hitratio.csv", 
                target:targetdir() .. "/multi_threads_on_sampling_hitratio.csv", 
                "experiment-report/pics/multi_threads_on_both_hitratio", 
                "Hits Ratio", 
                "0"
            }
        )
        
        print("compiling report ... ")
        old_working_dir = os.workingdir()
        os.cd('experiment-report')
        os.execv("xelatex", { "main.tex" })
        os.cd(old_working_dir)
    end)

target("gperftool-fifo-hybrid")
    set_kind("binary")
    set_symbols("debug")
    add_syslinks("tcmalloc", "profiler")
    add_files(
        "benchmark-deps/*.cc",
        "gperftool-fifo-hybrid/*.cc", 
        "smhasher/src/Murmur*.cpp"
    )
    add_includedirs("dirtyzipf")
    add_deps("nbtlru")
    on_run(function (target)
        os.setenv("HEAPPROFILE", target:name() .. ".heap")
        os.exec(target:targetfile())
    end)
    after_run(function (target)
        if (is_mode("debug")) then
            return 
        end

    end)
    
target("gperftool-naive")
    set_kind("binary")
    set_symbols("debug")
    add_syslinks("tcmalloc", "profiler")
    add_files(
        "benchmark-deps/*.cc",
        "gperftool-naive/*.cc", 
        "smhasher/src/Murmur*.cpp"
    )
    add_includedirs("dirtyzipf")
    add_deps("nbtlru")
    on_run(function (target)
        os.setenv("HEAPPROFILE", target:name() .. ".heap")
        os.exec(target:targetfile())
    end)
    after_run(function (target)
        if (is_mode("debug")) then
            return 
        end

    end)
    
target("gperftool-sampling")
    set_kind("binary")
    set_symbols("debug")
    add_syslinks("tcmalloc", "profiler")
    add_files(
        "benchmark-deps/*.cc",
        "gperftool-sampling/*.cc", 
        "smhasher/src/Murmur*.cpp"
    )
    add_includedirs("dirtyzipf")
    add_deps("nbtlru")
    on_run(function (target)
        os.setenv("HEAPPROFILE", target:name() .. ".heap")
        os.exec(target:targetfile())
    end)
    after_run(function (target)
        if (is_mode("debug")) then
            return 
        end

    end)
    
