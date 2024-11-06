BOARD = aarch64-virt9

CC = $(LLVM)/bin/clang++
AS = $(LLVM)/bin/clang
LD = $(LLVM)/bin/ld.lld
AR = $(LLVM)/bin/llvm-ar


# PA_ENTRY = 0x40000000
# 0x40000000 is the beginning of main memory
# however, setting PA_ENTRY to 0x40000000 causes an issue:
# one of objcopy OR qemu automatically relocates all other sections except .text.wwos.boot to 0x40080000
# there will be some issue caused by this relocation..

PA_ENTRY = 0x40080000
KA_BEGIN = 0xffffff8000000000
PA_UART_LOGGING = 0x09000000
SIZE_PREKERNEL = 0x2000000 # 32 MB

DEFINES = -DPA_ENTRY=$(PA_ENTRY) -DKA_BEGIN=$(KA_BEGIN) -DSIZE_PREKERNEL=$(SIZE_PREKERNEL) -DPA_UART_LOGGING=$(PA_UART_LOGGING)


CCFLAGS = --target=aarch64-elf -Iinclude -Wall -Werror -O0 -mgeneral-regs-only -ffreestanding -nostdlib -nostdinc -nostdinc++ -std=c++20 -fno-exceptions -fno-threadsafe-statics -fno-use-cxa-atexit -fno-rtti $(DEFINES) 
ASFLAGS = --target=aarch64-elf
