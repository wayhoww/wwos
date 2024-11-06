CC = $(LLVM)/bin/clang++
AS = $(LLVM)/bin/clang
LD = $(LLVM)/bin/ld.lld
AR = $(LLVM)/bin/llvm-ar

DEFINES = -DWWOS_APPLICATION

WWOS_ROOT = ../..

CCFLAGS = --target=aarch64-elf -I$(WWOS_ROOT)/include -Wall -Werror -O0 -mgeneral-regs-only -ffreestanding -nostdlib -nostdinc -nostdinc++ -std=c++20 -fno-exceptions -fno-threadsafe-statics -fno-use-cxa-atexit $(DEFINES)

../linker.ld: ../linker.ld.tmpl
	$(CC) -P -E -xc++ $(DEFINES) $< -o $@
