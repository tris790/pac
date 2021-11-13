const std = @import("std");
const builtin = @import("builtin");

const Dependency = struct { name: []const u8, url: []const u8, add_dependency_fn: anytype };

pub fn build(b: *std.build.Builder) !void {
    // Standard target options allows the person running `zig build` to choose
    // what target to build for. Here we do not override the defaults, which
    // means any target is allowed, and the default is native. Other options
    // for restricting supported target set are available.
    const target = b.standardTargetOptions(.{});
    // Standard release options allow the person running `zig build` to select
    // between Debug, ReleaseSafe, ReleaseFast, and ReleaseSmall.
    const mode = b.standardReleaseOptions();

    const git_repos = [_]Dependency{
        Dependency{ .name = "SDL", .url = "https://github.com/libsdl-org/SDL.git", .add_dependency_fn = build_sdl2 },
        Dependency{ .name = "SDL_mixer", .url = "https://github.com/tris790/SDL_mixer", .add_dependency_fn = build_sdl2_mixer },
    };

    // Clone all dependencies if they are not present
    inline for (git_repos) |repo| {
        const repo_path = "deps/" ++ repo.name;
        if (!dependency_exists(repo_path)) {
            try fetch_depencency(b, repo.url, repo_path);
        }
        try repo.add_dependency_fn(b, mode);
    }

    // Build pac executables
    build_pac_server_executable(b, target, mode);
    build_pac_client_executable(b, target, mode);

    // Copy the config file from the source to the executable directory
    try copy_configuration_files(b);
}

fn copy_configuration_files(b: *std.build.Builder) !void {
    b.installFile("src/client/client.conf", "bin/client.conf");
    b.installFile("src/server/server.conf", "bin/server.conf");
}

fn dependency_exists(name: []const u8) bool {
    return file_exists: {
        _ = std.fs.cwd().openDir(name, .{}) catch break :file_exists false;
        break :file_exists true;
    };
}

fn fetch_depencency(b: *std.build.Builder, repository_url: []const u8, ouput_path: []const u8) !void {
    const git_step = b.addSystemCommand(&[_][]const u8{
        "git",
        "clone",
        repository_url,
        ouput_path,
    });
    try git_step.step.make();
}

fn build_sdl2(b: *std.build.Builder, mode: std.builtin.Mode) !void {
    const cmake_prebuild_cmd = &[_][]const u8{
        "cmake",
        "-B",
        "deps/SDL/build",
        "-S",
        "deps/SDL",
        if (mode == .Debug) "-DCMAKE_BUILD_TYPE=Debug" else "-DCMAKE_BUILD_TYPE=Release",
        "-DCMAKE_MSVC_RUNTIME_LIBRARY=MultiThreaded",
        "-DSDL_SHARED=OFF",
    };

    const cmake_build_cmd = &[_][]const u8{
        "cmake",
        "--build",
        "deps/SDL/build",
        "-j",
        if (mode != .Debug and builtin.os.tag == .windows) "--config Release" else "",
    };

    const cmake_prebuild = b.addSystemCommand(cmake_prebuild_cmd);
    try cmake_prebuild.step.make();

    const cmake_build = b.addSystemCommand(cmake_build_cmd);
    try cmake_build.step.make();
}

fn nop(_: *std.build.Builder, _: std.builtin.Mode) !void {}
fn build_sdl2_mixer(b: *std.build.Builder, mode: std.builtin.Mode) !void {
    const sdl_path = try (try std.fs.cwd().openDir("deps/SDL", .{})).realpathAlloc(std.testing.allocator, "");
    const cmake_sdl_path = try std.mem.concat(std.testing.allocator, u8, &[_][]const u8{ "-DSDL_PATH=", sdl_path });

    const cmake_prebuild = b.addSystemCommand(&[_][]const u8{
        "cmake",
        "-B",
        "deps/SDL_mixer/build",
        "-S",
        "deps/SDL_mixer",
        cmake_sdl_path,
        if (mode == .Debug) "-DCMAKE_BUILD_TYPE=Debug" else "-DCMAKE_BUILD_TYPE=Release",
        "-DCMAKE_MSVC_RUNTIME_LIBRARY=MultiThreaded",
    });

    try cmake_prebuild.step.make();
    const cmake_build = b.addSystemCommand(&[_][]const u8{
        "cmake",
        "--build",
        "deps/SDL_mixer/build",
        "-j",
        if (mode != .Debug and builtin.os.tag == .windows) "--config Release" else "",
    });
    try cmake_build.step.make();
    std.debug.print("Built sdl2 mixer\n", .{});
}

fn build_pac_client_executable(b: *std.build.Builder, target: std.zig.CrossTarget, mode: std.builtin.Mode) void {
    var pac_client_exe = b.addExecutable("pac-client", null);

    pac_client_exe.setTarget(target);
    pac_client_exe.setBuildMode(mode);

    pac_client_exe.linkSystemLibrary("c");
    pac_client_exe.linkSystemLibrary("c++");

    pac_client_exe.addCSourceFiles(&.{
        "src/client/main.cpp",
    }, &.{
        "-std=c++17",
        "-Wall",
        "-W",
        "-Wstrict-prototypes",
        "-Wwrite-strings",
        "-Wno-missing-field-initializers",
    });

    pac_client_exe.addIncludeDir("include");
    pac_client_exe.addIncludeDir("deps/SDL/include");
    pac_client_exe.addIncludeDir("deps/SDL_mixer/include");

    if (builtin.os.tag == .windows) {
        pac_client_exe.linkSystemLibrary("msvcrt");
    }
    pac_client_exe.linkSystemLibrary("gstreamer-1.0");
    pac_client_exe.linkSystemLibrary("glib-2.0");
    pac_client_exe.linkSystemLibrary("gstapp-1.0");
    pac_client_exe.linkSystemLibrary("gobject-2.0");

    pac_client_exe.addLibPath("deps/SDL/build");
    if (mode == .Debug) {
        pac_client_exe.addLibPath("deps/SDL/build/Debug");
        pac_client_exe.linkSystemLibrary("SDL2d");
        pac_client_exe.linkSystemLibrary("SDL2maind");
        pac_client_exe.addLibPath("deps/SDL_mixer/build/Debug");
    } else {
        pac_client_exe.addLibPath("deps/SDL/build/Release");
        pac_client_exe.linkSystemLibrary("SDL2");
        pac_client_exe.linkSystemLibrary("SDL2main");
        pac_client_exe.addLibPath("deps/SDL_mixer/build/Release");
    }
    pac_client_exe.linkSystemLibrary("SDL2_MIXER");

    pac_client_exe.install();
    std.debug.print("Built pac-client\n", .{});

    const client_run_cmd = pac_client_exe.run();
    client_run_cmd.step.dependOn(b.getInstallStep());
    if (b.args) |args| {
        client_run_cmd.addArgs(args);
    }
    const client_run_step = b.step("run", "Run the client");
    client_run_step.dependOn(&client_run_cmd.step);
}

fn build_pac_server_executable(b: *std.build.Builder, target: std.zig.CrossTarget, mode: std.builtin.Mode) void {
    const pac_server_exe = b.addExecutable("pac-server", null);

    pac_server_exe.setTarget(target);
    pac_server_exe.setBuildMode(mode);

    pac_server_exe.linkSystemLibrary("c");
    pac_server_exe.linkSystemLibrary("c++");

    pac_server_exe.addCSourceFiles(&.{
        "src/server/main.cpp",
    }, &.{
        "-std=c++17",
        "-Wall",
        "-W",
        "-Wstrict-prototypes",
        "-Wwrite-strings",
        "-Wno-missing-field-initializers",
    });

    pac_server_exe.addIncludeDir("include");
    pac_server_exe.addIncludeDir("deps/SDL/include");

    if (builtin.os.tag == .windows) {
        pac_server_exe.linkSystemLibrary("msvcrt");
    }
    pac_server_exe.linkSystemLibrary("gstreamer-1.0");
    pac_server_exe.linkSystemLibrary("glib-2.0");
    pac_server_exe.linkSystemLibrary("gstapp-1.0");
    pac_server_exe.linkSystemLibrary("gobject-2.0");

    if (mode == .Debug) {
        pac_server_exe.addLibPath("deps/SDL/build");
        pac_server_exe.addLibPath("deps/SDL/build/Debug");
        pac_server_exe.linkSystemLibrary("SDL2d");
        pac_server_exe.linkSystemLibrary("SDL2maind");
    } else {
        pac_server_exe.addLibPath("deps/SDL/build/Release");
        pac_server_exe.linkSystemLibrary("SDL2");
        pac_server_exe.linkSystemLibrary("SDL2main");
    }

    pac_server_exe.install();
    std.debug.print("Built pac-server\n", .{});

    const server_run_cmd = pac_server_exe.run();
    server_run_cmd.step.dependOn(b.getInstallStep());
    if (b.args) |args| {
        server_run_cmd.addArgs(args);
    }
    const server_run_step = b.step("run", "Run the server");
    server_run_step.dependOn(&server_run_cmd.step);
}
