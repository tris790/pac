const std = @import("std");

pub fn build(b: *std.build.Builder) !void {
    // Standard target options allows the person running `zig build` to choose
    // what target to build for. Here we do not override the defaults, which
    // means any target is allowed, and the default is native. Other options
    // for restricting supported target set are available.
    const target = b.standardTargetOptions(.{});

    // Standard release options allow the person running `zig build` to select
    // between Debug, ReleaseSafe, ReleaseFast, and ReleaseSmall.
    const mode = b.standardReleaseOptions();

    try build_sdl2(b);

    const pac_exe = b.addExecutable("pac", null);

    pac_exe.setTarget(target);
    pac_exe.setBuildMode(mode);

    pac_exe.linkSystemLibrary("c");
    pac_exe.linkSystemLibrary("c++");

    pac_exe.addCSourceFiles(&.{
        "src/server/main.cpp",
    }, &.{
        "-std=c++17",
        "-Wall",
        "-W",
        "-Wstrict-prototypes",
        "-Wwrite-strings",
        "-Wno-missing-field-initializers",
    });

    pac_exe.addIncludeDir("include");
    pac_exe.addIncludeDir("deps/SDL/include");

    pac_exe.addLibPath("deps/SDL/build/Debug");

    pac_exe.linkSystemLibrary("msvcrt");
    pac_exe.linkSystemLibrary("Ws2_32");
    pac_exe.linkSystemLibrary("gstreamer-1.0");
    pac_exe.linkSystemLibrary("glib-2.0");
    pac_exe.linkSystemLibrary("gstapp-1.0");
    pac_exe.linkSystemLibrary("gobject-2.0");
    pac_exe.linkSystemLibrary("SDL2maind");
    pac_exe.linkSystemLibrary("SDL2d");

    pac_exe.install();

    const run_cmd = pac_exe.run();
    run_cmd.step.dependOn(b.getInstallStep());
    if (b.args) |args| {
        run_cmd.addArgs(args);
    }

    const run_step = b.step("run", "Run the app");
    run_step.dependOn(&run_cmd.step);
}

fn fetch_deps() !void {
    _ = "https://github.com/libsdl-org/SDL.git";
}

fn build_sdl2(b: *std.build.Builder) !void {
    const cmake_prebuild = b.addSystemCommand(&[_][]const u8{
        "cmake",
        "-B",
        "deps/SDL/build",
        "-S",
        "deps/SDL",
        "-Thost=x64",
        "-A x64",
        "-DCMAKE_BUILD_TYPE=Debug",
        "-DSDL_SHARED=OFF",
        "-DCMAKE_MSVC_RUNTIME_LIBRARY=MultiThreadedDebug",
    });
    try cmake_prebuild.step.make();
    const cmake_build = b.addSystemCommand(&[_][]const u8{
        "cmake",
        "--build",
        "deps/SDL/build",
        "-j",
    });
    try cmake_build.step.make();
}
