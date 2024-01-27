"""
Convenience script for compiling and running C++ programs that use SDL2.
"""

import argparse
import os
import platform
import subprocess
import sys
import typing


def panic(message: str) -> typing.NoReturn:
    sys.stderr.write(f"{message}\n")
    sys.exit(1)


REPO_DIR = os.path.dirname(os.path.realpath(__file__))

if REPO_DIR != os.getcwd():
    panic("This make script is intended to be run from the repo directory")

aparser = argparse.ArgumentParser()
aparser.add_argument("--debug", "-g", default=False, action="store_true")
aparser.add_argument("--verbose", "-v", default=False, action="store_true")
aparser.add_argument("c_file_path")
aparser.add_argument("subproc_args", nargs="*", default=[])
args = aparser.parse_args()

C_FILE_PATH: str = args.c_file_path
DEBUG: bool = args.debug
VERBOSE: bool = args.verbose
SUBPROC_ARGS: typing.List[str] = args.subproc_args

SYSTEM_WINDOWS = "Windows"
SYSTEM_MACOS = "Darwin"
SYSTEM_LINUX = "Linux"
SYSTEM = platform.system()

EXE_NAME = "a.out"

CXX_STANDARD_FLAGS = ["-std=c++20"]
WARNING_FLAGS = [
    "-Werror",
    "-Wall",
    "-Wextra",
    "-Wpedantic",
    "-Wno-unused-const-variable",
]
MACOS_SDL2_FLAGS = [
    "-rpath",
    "@executable_path/",
    "-F.",
    "-framework",
    "SDL2",
]
MACOS_SDL2_MIXER_FLAGS = ["-framework", "SDL2_mixer"]
MACOS_SDL2_TTF_FLAGS = ["-framework", "SDL2_ttf"]
DEBUG_FLAGS = ["-g", "-fsanitize=address"]
RELEASE_FLAGS = ["-O3", "-flto"]


def missing_framework(name: str) -> typing.NoReturn:
    panic(
        "ERROR:\n"
        f"  Source includes {name}, but {name}.framework is not found in working directory\n"
        f"  You might need to download the dmg file from https://github.com/libsdl-org/{name}/releases"
    )


def get_sdl_flags(c_file: str, system: str) -> typing.List[str]:
    with open(c_file) as f:
        contents = f.read()

    sdl_mixer = "#include <SDL2_mixer/SDL_mixer.h>" in contents
    sdl_ttf = "#include <SDL2_ttf/SDL_ttf.h>" in contents
    sdl_core = sdl_mixer or sdl_ttf or ("#include <SDL2/SDL.h>" in contents)

    if system == SYSTEM_MACOS:
        if sdl_core and not os.path.exists("SDL2.framework"):
            missing_framework("SDL2")
        if sdl_mixer and not os.path.exists("SDL2_mixer.framework"):
            missing_framework("SDL2_mixer")
        if sdl_ttf and not os.path.exists("SDL2_ttf.framework"):
            missing_framework("SDL2_ttf")
        return (
            (MACOS_SDL2_FLAGS if sdl_core else [])
            + (MACOS_SDL2_MIXER_FLAGS if sdl_mixer else [])
            + (MACOS_SDL2_TTF_FLAGS if sdl_ttf else [])
        )

    panic(f"System not yet supported: {system} (get_sdl_flags)")


def compile(
    c_file: str = C_FILE_PATH,
    debug: bool = DEBUG,
    exe_name: str = EXE_NAME,
    system: str = SYSTEM,
) -> None:
    args: typing.List[str] = []
    if system == SYSTEM_MACOS:
        args.extend(
            [
                "clang++",
                *CXX_STANDARD_FLAGS,
                *WARNING_FLAGS,
                *get_sdl_flags(c_file, system),
                *(DEBUG_FLAGS if debug else RELEASE_FLAGS),
                f"-o{exe_name}",
                c_file,
            ]
        )
    else:
        panic(f"System not yet supported: {system} (compile)")

    if VERBOSE:
        print("COMPILING PROGRAM:")
        print(f"  {args[0]}\n    " + "\n    ".join(args[1:]))
        print()

    returncode = subprocess.run(args).returncode
    if returncode != 0:
        panic(f"COMPILE FAILED: args = {args}")


def run(
    exe_name: str = EXE_NAME, subproc_args: typing.List[str] = SUBPROC_ARGS
) -> None:
    args = [os.path.join(".", exe_name)] + subproc_args
    if VERBOSE:
        print("RUNNING COMPILED PROGRAM:")
        print(f"  {args[0]}\n    " + "\n    ".join(args[1:]))

    returncode = subprocess.run(args).returncode
    if returncode != 0:
        panic(f"RUN FINISHED WITH NON-ZERO EXIT CODE")


def main() -> None:
    compile()
    run()


if __name__ == "__main__":
    main()
