BOARD = aarch64-virt9

SIZE_PREKERNEL = 0x2000000 # 32 MB

CPU = cortex-a57

ifeq ($(BOARD),aarch64-virt9)
	QEMU_FLAGS = -machine virt -cpu $(CPU) -monitor none
	PA_ENTRY = 0x40080000
	KA_BEGIN = 0xffffff8000000000
	PA_UART_LOGGING = 0x09000000ull
	GICD_BASE = 0x08000000ull
	GICC_BASE = 0x08010000ull
	MEMORY_BEGIN = 0x40000000
	MEMORY_SIZE = 0x20000000
	BOOT_ASM = boot-virt9.s
	DEFINES += -DWWOS_BOARD_VIRT9
else ifeq ($(BOARD),aarch64-raspi4b)
	QEMU_FLAGS = -machine raspi4b -cpu $(CPU) -monitor none -serial stdio 
	PA_ENTRY = 0x80000
	KA_BEGIN = 0xffffff8000000000
	PA_UART_LOGGING = 0xFE201000ull
	GICD_BASE = 0xff841000ull
	GICC_BASE = 0xff842000ull
	MEMORY_BEGIN = 0
	MEMORY_SIZE = 0x20000000
	BOOT_ASM = boot-raspi4b.s
	DEFINES += -DWWOS_BOARD_RASPI4B
else
	$(error Unknown board)
endif

# -dtb emulation/bcm2711-rpi-4-b.dtb

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

DEFINES += -DPA_ENTRY=$(PA_ENTRY) -DKA_BEGIN=$(KA_BEGIN) -DSIZE_PREKERNEL=$(SIZE_PREKERNEL) -DPA_UART_LOGGING=$(PA_UART_LOGGING) -DWWOS_GICC_BASE=$(GICC_BASE) -DWWOS_GICD_BASE=$(GICD_BASE) -DWWOS_MEMORY_BEGIN=$(MEMORY_BEGIN) -DWWOS_MEMORY_SIZE=$(MEMORY_SIZE)


CCFLAGS += -Iinclude -Wall -Werror -O2 -mgeneral-regs-only -ffreestanding -nostdlib -nostdinc -std=c++17 -fno-exceptions -fno-threadsafe-statics -fno-use-cxa-atexit -fno-rtti -funwind-tables $(DEFINES) 
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

ifeq ($(LOG_ERET),1)
	CCFLAGS += -DWWOS_LOG_ERET
endif

ifeq ($(LOG_PAGE),1)
	CCFLAGS += -DWWOS_LOG_PAGE
endif