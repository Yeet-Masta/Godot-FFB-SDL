#!/usr/bin/env python
import os
import sys

env = SConscript("godot-cpp/SConstruct")

# For reference:
# - CCFLAGS are compilation flags shared between C and C++
# - CFLAGS are for C-specific compilation flags
# - CXXFLAGS are for C++-specific compilation flags
# - CPPFLAGS are for pre-processor flags
# - CPPDEFINES are for pre-processor defines
# - LINKFLAGS are for linking flags

if env["platform"] == "windows":
    env.Append(CPPPATH=['include/'])
    env.Append(LIBPATH=['lib/SDL3/'])
    print("CXX is: " + env["CXX"])
    if env["CXX"] == "$CC":
        # Different .lib-file for MS Visual C.
        # You can download these with the dlls from SDL's releases: https://github.com/libsdl-org/SDL/releases
        env.Append(LIBS=['SDL3'])
    else:
        env.Append(LIBS=['SDL3.dll.dll'])

elif env["platform"] == "linux":
    env.Append(CPPPATH=['include/'])
    env.Append(LINKFLAGS='-lSDL3')

env.Append(CPPPATH=["src/"])
sources = Glob("src/*.cpp")

library = env.SharedLibrary(
    "bin/libffbplugin{}{}".format(env["suffix"], env["SHLIBSUFFIX"]),
    source=sources,
)

Default(library)