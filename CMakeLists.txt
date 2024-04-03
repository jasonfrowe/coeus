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
rp6502_asset(graphictest 0x1E100 images/spaceship2.bin)
rp6502_asset(graphictest 0x1E180 images/Earth.bin)
rp6502_asset(graphictest 0x1E980 images/enemy1.bin)
rp6502_asset(graphictest 0x1EB80 images/npbattle.bin)
rp6502_asset(graphictest 0x1EC00 images/fighter.bin)
rp6502_executable(graphictest
    spaceship2.bin.rp6502
    Earth.bin.rp6502
    enemy1.bin.rp6502
    npbattle.bin.rp6502
    fighter.bin.rp6502
)

target_sources(graphictest PRIVATE
    src/graphictest.c
)