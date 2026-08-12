add_rules("mode.debug", "mode.release", "mode.coverage")
add_requires("fmt", { alias = "fmt" })
add_requires("doctest", { alias = "doctest" })
add_requires("spdlog", { alias = "spdlog" })
add_requires("nlohmann_json", { alias = "nlohmann_json" })
add_requires("nanobench", { alias = "nanobench" })
-- cppcoro unavailable via network; using hand-rolled Generator<T> instead

set_languages("c++20")

-- CUDA detection (optional)
local has_cuda = false
local cuda_path = os.getenv("CUDA_PATH") or os.getenv("CUDA_ROOT")
if cuda_path then
    local nvcc = path.join(cuda_path, "bin", "nvcc.exe")
    if os.isfile(nvcc) then
        has_cuda = true
        print("[xmake] CUDA detected -- enabling GPU acceleration")
    end
else
    print("[xmake] CUDA not found -- CPU-only mode")
end

if is_plat("linux") then
	set_warnings("all", "error")
	-- add_cxflags("-Wconversion", {force = true})
	-- Check if we're on Termux/Android
	local termux_prefix = os.getenv("PREFIX")
	if termux_prefix then
        add_cxflags("-Wno-unused-command-line-argument", {force = true})
		add_sysincludedirs(termux_prefix .. "/include/c++/v1", { public = true })
		add_sysincludedirs(termux_prefix .. "/include", { public = true })
	end
	-- add_cxflags("-nostdinc++", {force = true})
	-- add_sysincludedirs(os.getenv("PREFIX") .. "/include/c++/v1", {public = true})
	-- add_sysincludedirs(os.getenv("PREFIX") .. "/include", {public = true})
elseif is_plat("windows") then
    -- NOTE: keep standard level in sync with set_languages() above;
    -- an explicit /std:c++latest here would override /std:c++20 (warning D9025)
    add_cxflags("/EHsc /utf-8 /W4 /WX /wd4702", { force = true })
end

if is_mode("coverage") then
	add_cxflags("-ftest-coverage", "-fprofile-arcs", { force = true })
end

target("NetlistX")
    set_kind("static")
    add_includedirs("include", { public = true })
    add_includedirs("../py2cpp/include", { public = true })
    add_includedirs("../xnetwork-cpp/include", { public = true })
    add_files("source/*.cpp")
    if has_cuda then
        add_files("source/*.cu")
        set_languages("c++20")
        set_policy("build.cuda.devlink", true)
        add_defines("HAS_CUDA", { public = true })
        add_cuflags("--extended-lambda", "--gpu-architecture=compute_75", { force = true })
        add_cuflags("-Xcompiler=/utf-8", { force = true })
        add_links("cudart")
        print("[xmake] NetlistX: GPU acceleration enabled (CUDA)")
    else
        print("[xmake] NetlistX: CPU-only build")
    end
    add_packages("fmt", "spdlog", "nlohmann_json")

target("test_netlistx")
    set_kind("binary")
    add_deps("NetlistX")
    add_includedirs("include", { public = true })
    add_includedirs("../py2cpp/include", { public = true })
    add_includedirs("../xnetwork-cpp/include", { public = true })
    add_files("test/source/*.cpp")
    add_packages("fmt", "doctest", "spdlog", "nlohmann_json")
	if is_plat("linux") then
		set_rundir("./build/linux/")
	elseif is_plat("windows") then
		set_rundir("./build/windows/")
	end
	add_tests("default")

target("bench_yosys")
    set_kind("binary")
    add_deps("NetlistX")
    add_includedirs("include", { public = true })
    add_includedirs("../py2cpp/include", { public = true })
    add_includedirs("../xnetwork-cpp/include", { public = true })
    add_files("bench/source/bench_yosys.cpp")
    add_packages("fmt", "spdlog", "nlohmann_json", "nanobench")
	if is_plat("linux") then
		set_rundir("./build/linux/")
	elseif is_plat("windows") then
		set_rundir("./build/windows/")
	end

target("bench_cross")
    set_kind("binary")
    add_deps("NetlistX")
    add_includedirs("include", { public = true })
    add_includedirs("../py2cpp/include", { public = true })
    add_includedirs("../xnetwork-cpp/include", { public = true })
    add_files("bench/source/bench_cross.cpp")
	add_files("../xnetwork-cpp/source/*.cpp")
    add_packages("fmt", "spdlog", "nlohmann_json", "nanobench")
	if is_plat("linux") then
		set_rundir("./build/linux/")
	elseif is_plat("windows") then
		set_rundir("./build/windows/")
	end

-- Check if rapidcheck was built by CMake (check both build and build_test directories)
local build_dirs = { "build", "build_test" }
local rapidcheck_dir = nil
local rapidcheck_lib_dir = nil
local rapidcheck_lib = nil

for _, build_dir in ipairs(build_dirs) do
	local candidate_src = path.join(os.projectdir(), build_dir, "_deps", "rapidcheck-src")
	local candidate_lib_dir = path.join(os.projectdir(), build_dir, "_deps", "rapidcheck-build")
	local candidate_lib = nil
	if is_plat("windows") then
		candidate_lib_dir = path.join(candidate_lib_dir, "Release")
		candidate_lib = path.join(candidate_lib_dir, "rapidcheck.lib")
	else
		candidate_lib = path.join(candidate_lib_dir, "librapidcheck.a")
	end

	if os.isdir(candidate_src) and os.isfile(candidate_lib) then
		rapidcheck_dir = candidate_src
		rapidcheck_lib_dir = candidate_lib_dir
		rapidcheck_lib = candidate_lib
		break
	end
end

if rapidcheck_dir and rapidcheck_lib then
	add_includedirs(path.join(rapidcheck_dir, "include"))
	add_linkdirs(rapidcheck_lib_dir)
	add_links("rapidcheck")
	add_defines("RAPIDCHECK_H")
end

--
-- If you want to known more usage about xmake, please see https://xmake.io
--
-- ## FAQ
--
-- You can enter the project directory firstly before building project.
--
--   $ cd projectdir
--
-- 1. How to build project?
--
--   $ xmake
--
-- 2. How to configure project?
--
--   $ xmake f -p [macosx|linux|iphoneos ..] -a [x86_64|i386|arm64 ..] -m [debug|release]
--
-- 3. Where is the build output directory?
--
--   The default output directory is `./build` and you can configure the output directory.
--
--   $ xmake f -o outputdir
--   $ xmake
--
-- 4. How to run and debug target after building project?
--
--   $ xmake run [targetname]
--   $ xmake run -d [targetname]
--
-- 5. How to install target to the system directory or other output directory?
--
--   $ xmake install
--   $ xmake install -o installdir
--
-- 6. Add some frequently-used compilation flags in xmake.lua
--
-- @code
--    -- add debug and release modes
--    add_rules("mode.debug", "mode.release")
--
--    -- add macro defination
--    add_defines("NDEBUG", "_GNU_SOURCE=1")
--
--    -- set warning all as error
--    set_warnings("all", "error")
--
--    -- set language: c99, c++11
--    set_languages("c99", "c++11")
--
--    -- set optimization: none, faster, fastest, smallest
--    set_optimize("fastest")
--
--    -- add include search directories
--    add_includedirs("/usr/include", "/usr/local/include")
--
--    -- add link libraries and search directories
--    add_links("tbox")
--    add_linkdirs("/usr/local/lib", "/usr/lib")
--
--    -- add system link libraries
--    add_syslinks("z", "pthread")
--
--    -- add compilation and link flags
--    add_cxflags("-stdnolib", "-fno-strict-aliasing")
--    add_ldflags("-L/usr/local/lib", "-lpthread", {force = true})
--
-- @endcode
--
