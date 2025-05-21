cmake_minimum_required(VERSION 3.18)

add_subdirectory(tools)

set(LLVM_MOS_PLATFORM rp6502)
find_package(llvm-mos-sdk REQUIRED)

project(MY-RP6502-PROJECT C CXX ASM)

add_executable(coeus)
rp6502_asset(coeus 0x1E100 images/spaceship2.bin)
rp6502_asset(coeus 0x1E180 images/Earth.bin)
rp6502_asset(coeus 0x1E980 images/enemy1.bin)
rp6502_asset(coeus 0x1EB80 images/npbattle.bin)
rp6502_asset(coeus 0x1EC00 images/fighter.bin)
rp6502_executable(coeus
    spaceship2.bin.rp6502
    Earth.bin.rp6502
    enemy1.bin.rp6502
    npbattle.bin.rp6502
    fighter.bin.rp6502
    DATA file
    RESET file
)
target_sources(coeus PRIVATE
    src/coeus.c
    src/random.c
)
