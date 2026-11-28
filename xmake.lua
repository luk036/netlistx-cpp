add_rules("mode.debug", "mode.release", "mode.coverage")
add_requires("fmt", "doctest", "spdlog", "nlohmann_json", "nanobench")
-- cppcoro unavailable via network; using hand-rolled Generator<T> instead

set_languages("c++20")
add_includedirs("include", "../py2cpp/include", "../xnetwork-cpp/include")

if is_plat("linux") then
	set_rundir("./build/linux/")
elseif is_plat("windows") then
	set_rundir("./build/windows/")
end

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
	-- Termux/Android ships its libc++ headers under $PREFIX
	local prefix = os.getenv("PREFIX")
	if prefix then
		add_cxflags("-Wno-unused-command-line-argument", {force = true})
		add_sysincludedirs(prefix .. "/include/c++/v1", prefix .. "/include", {public = true})
	end
elseif is_plat("windows") then
	-- NOTE: keep the standard level from set_languages() above; an explicit
	-- /std:c++latest here would override /std:c++20 (warning D9025)
	add_cxflags("/EHsc /utf-8 /W4 /WX /wd4702", {force = true})
end

if is_mode("coverage") then
	add_cxflags("-ftest-coverage", "-fprofile-arcs", {force = true})
end

target("NetlistX")
	set_kind("static")
	add_files("source/*.cpp")
	add_packages("fmt", "spdlog", "nlohmann_json")
	if has_cuda then
		add_files("source/*.cu")
		set_policy("build.cuda.devlink", true)
		add_defines("HAS_CUDA", {public = true})
		add_cuflags("--extended-lambda", "--gpu-architecture=compute_75", {force = true})
		add_cuflags("-Xcompiler=/utf-8", {force = true})
		add_links("cudart")
		print("[xmake] NetlistX: GPU acceleration enabled (CUDA)")
	else
		print("[xmake] NetlistX: CPU-only build")
	end

target("test_netlistx")
	add_deps("NetlistX")
	add_files("test/source/*.cpp")
	add_packages("fmt", "doctest", "spdlog", "nlohmann_json")
	add_tests("default")

target("bench_yosys")
	add_deps("NetlistX")
	add_files("bench/source/bench_yosys.cpp")
	add_packages("fmt", "spdlog", "nlohmann_json", "nanobench")

target("bench_cross")
	add_deps("NetlistX")
	add_files("bench/source/bench_cross.cpp", "../xnetwork-cpp/source/*.cpp")
	add_packages("fmt", "spdlog", "nlohmann_json", "nanobench")

-- rapidcheck is optionally provided by a CMake build (build/ or build_test/)
local rapidcheck_dir = nil
local rapidcheck_lib_dir = nil
for _, build_dir in ipairs({"build", "build_test"}) do
	local src = path.join(os.projectdir(), build_dir, "_deps", "rapidcheck-src")
	local lib_dir = path.join(os.projectdir(), build_dir, "_deps", "rapidcheck-build")
	local lib = nil
	if is_plat("windows") then
		lib_dir = path.join(lib_dir, "Release")
		lib = path.join(lib_dir, "rapidcheck.lib")
	else
		lib = path.join(lib_dir, "librapidcheck.a")
	end
	if os.isdir(src) and os.isfile(lib) then
		rapidcheck_dir = src
		rapidcheck_lib_dir = lib_dir
		break
	end
end

if rapidcheck_dir then
	add_includedirs(path.join(rapidcheck_dir, "include"))
	add_linkdirs(rapidcheck_lib_dir)
	add_links("rapidcheck")
	add_defines("RAPIDCHECK_H")
end
