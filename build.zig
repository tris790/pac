const std = @import("std");
const builtin = @import("builtin");

pub fn build(b: *std.build.Builder) !void {
    // Standard target options allows the person running `zig build` to choose
    // what target to build for. Here we do not override the defaults, which
    // means any target is allowed, and the default is native. Other options
    // for restricting supported target set are available.
    const target = b.standardTargetOptions(.{});

    // Standard release options allow the person running `zig build` to select
    // between Debug, ReleaseSafe, ReleaseFast, and ReleaseSmall.
    const mode = b.standardReleaseOptions();

    try fetch_depencencies(b);
    try build_sdl2(b, mode);

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

    if (builtin.os.tag == .windows) {
        pac_exe.linkSystemLibrary("msvcrt");
        pac_exe.linkSystemLibrary("Ws2_32");
    }
    pac_exe.linkSystemLibrary("gstreamer-1.0");
    pac_exe.linkSystemLibrary("glib-2.0");
    pac_exe.linkSystemLibrary("gstapp-1.0");
    pac_exe.linkSystemLibrary("gobject-2.0");

    if (mode == .Debug) {
        pac_exe.addLibPath("deps/SDL/build");
        pac_exe.addLibPath("deps/SDL/build/Debug");
        pac_exe.linkSystemLibrary("SDL2d");
        pac_exe.linkSystemLibrary("SDL2maind");
    } else {
        pac_exe.addLibPath("deps/SDL/build/Release");
        pac_exe.linkSystemLibrary("SDL2");
        pac_exe.linkSystemLibrary("SDL2main");
    }

    pac_exe.install();

    const run_cmd = pac_exe.run();
    run_cmd.step.dependOn(b.getInstallStep());
    if (b.args) |args| {
        run_cmd.addArgs(args);
    }

    const run_step = b.step("run", "Run the app");
    run_step.dependOn(&run_cmd.step);
}

fn fetch_deps(b: *std.build.Builder) !void {
    const git_repos = [_]u8{"https://github.com/libsdl-org/SDL.git",};
    for (git_repos) |repo| {
        const git_step = b.addSystemCommand(&[_][]const u8{
            "git",
            "clone",
            repo,
        });
        try cmake_prebuild.step.make();
    }
}

fn build_sdl2(b: *std.build.Builder, mode: std.builtin.Mode) !void {
    const cmake_prebuild = b.addSystemCommand(&[_][]const u8{
        "cmake",
        "-B",
        "deps/SDL/build",
        "-S",
        "deps/SDL",
        if (mode == .Debug) "-DCMAKE_BUILD_TYPE=Debug" else "-DCMAKE_BUILD_TYPE=Release",
        "-DSDL_SHARED=OFF",
        //"-DCMAKE_MSVC_RUNTIME_LIBRARY=MultiThreadedDebug",
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
