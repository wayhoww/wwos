BOARD = aarch64-virt9

ifeq ($(GNU), 1)
	CC = aarch64-elf-g++
	AS = aarch64-elf-gcc
	LD = aarch64-elf-ld
	AR = aarch64-elf-ar
	OBJCOPY = aarch64-elf-objcopy
else
	ifeq ($(LLVM),)
		CC = clang++
		AS = clang
		LD = ld.lld
		AR = llvm-ar
		OBJCOPY = llvm-objcopy
	else
		CC = $(LLVM)/bin/clang++
		AS = $(LLVM)/bin/clang
		LD = $(LLVM)/bin/ld.lld
		AR = $(LLVM)/bin/llvm-ar
		OBJCOPY = $(LLVM)/bin/llvm-objcopy
	endif
endif

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


CCFLAGS = -Iinclude -Wall -Werror -O0 -g -mgeneral-regs-only -ffreestanding -nostdlib -nostdinc -nostdinc++ -std=c++17 -fno-exceptions -fno-threadsafe-statics -fno-use-cxa-atexit -fno-rtti -fPIC $(DEFINES) 
ASFLAGS = 

ifneq ($(GNU),1)
    CCFLAGS += --target=aarch64-elf
    ASFLAGS += --target=aarch64-elf
else
    $(error GNU compiler support is deprecated)
    CCFLAGS += 
endif

ifeq ($(LOG),1)
    CCFLAGS += -DWWOS_LOG
endif

ifeq ($(DIRECT_LOGGING),1)
	CCFLAGS += -DWWOS_DIRECT_LOGGING
endif