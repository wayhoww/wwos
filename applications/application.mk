WWOS_ROOT = ../..

include $(WWOS_ROOT)/wwos.mk

DEFINES = -DWWOS_APPLICATION

CCFLAGS = -I$(WWOS_ROOT)/include -Wall -Werror -O0 -mgeneral-regs-only -ffreestanding -nostdlib -nostdinc -std=c++20 -fno-exceptions -fno-threadsafe-statics -fno-use-cxa-atexit $(DEFINES)

ifneq ($(GNU),1)
	CCFLAGS += --target=aarch64-elf 
endif

../linker.ld: ../linker.ld.tmpl
	$(CC) -P -E -xc++ $(DEFINES) $< -o $@
