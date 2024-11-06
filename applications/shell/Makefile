include ../application.mk

.PHONY: all clean $(WWOS_ROOT)/libwwos/libwwos.a

all: main.app

main.o: main.cc
	$(CC) $(CCFLAGS) -c $< -o $@

$(WWOS_ROOT)/libwwos/libwwos.a:
	$(MAKE) -C $(WWOS_ROOT)/libwwos libwwos.a

main.elf: ../linker.ld main.o $(WWOS_ROOT)/libwwos/libwwos.a
	$(LD) -nostdlib -T$< $(filter-out $<,$^) -o $@

shell.app: main.elf
	$(LLVM)/bin/llvm-objcopy -O binary $< $@

clean:
	rm -f main.o main.elf shell.app
