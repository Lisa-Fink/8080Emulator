cmake_minimum_required(VERSION 3.6)
project(8080Emulator C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -lmingw32")
set(CMAKE_EXE_LINKER_FLAGS "-static-libgcc -static-libstdc++")

# Include SDL2 headers and link directories
include_directories(${CMAKE_SOURCE_DIR}/SDL2/include)
link_directories(${CMAKE_SOURCE_DIR}/SDL2/lib)

# Add the executable
add_executable(8080Emulator
        8080emulator.c
        Disassembler/disassembler.c
        EmulateSpaceInvaders.c
        sound.c graphics.c input.c)

# Link against SDL2main and SDL2 (order matters)
target_link_libraries(8080Emulator mingw32 SDL2main SDL2)

# Set subsystem to console (for main entry point)
set_target_properties(8080Emulator PROPERTIES
        LINK_FLAGS "-Wl,-subsystem,console"
        )

# Copy SDL2.dll to the output directory after build
add_custom_command(TARGET 8080Emulator POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_SOURCE_DIR}/SDL2/bin/SDL2.dll
        $<TARGET_FILE_DIR:8080Emulator>
        )