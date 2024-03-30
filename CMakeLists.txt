cmake_minimum_required(VERSION 3.18)
add_subdirectory(tools)
set(LLVM_MOS_PLATFORM rp6502)
find_package(llvm-mos-sdk REQUIRED)
project(MY-RP6502-PROJECT)
add_executable(hello)
rp6502_executable(hello)
target_sources(hello PRIVATE
    src/main.c
)

add_executable(graphictest)
rp6502_asset(graphictest 0x1E100 images/spaceship.bin)
rp6502_executable(graphictest
    spaceship.bin.rp6502
)

target_sources(graphictest PRIVATE
    src/graphictest.c
)