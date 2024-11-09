include wwos.mk

DEFINES += -DWWOS_KERNEL

.PHONY: all tools run clean test dev memdisk.wwfs libwwos/libwwos_kernel.a applications/init/init.app applications/shell/shell.app applications/tty/tty.app dev/qemu_tracer/qemu_tracer.so qemu.log.sym

all: wwos.img tools compile_flags.txt

run: wwos.img
	qemu-system-aarch64 -machine virt -cpu cortex-a57 -kernel $< -nographic -monitor none -d int,in_asm,guest_errors -D qemu.log
	
qemu.log.sym: dev/symbolify.py qemu.log
	python3 dev/symbolify.py $(KA_BEGIN) kernel/kernel.elf <qemu.log >qemu.log.sym

debug: wwos.img
	qemu-system-aarch64 -machine virt -cpu cortex-a57 -kernel $< -nographic -monitor none -d int,in_asm,guest_errors -D qemu.log -S -s

trace: wwos.img dev/qemu_tracer/qemu_tracer.so
	# qemu-system-aarch64 -machine virt -cpu cortex-a57 -kernel $< -nographic -monitor none -plugin ./dev/qemu_tracer/qemu_tracer.so -d int,in_asm,guest_errors,exec,plugin -D qemu.log
	qemu-system-aarch64 -machine virt -cpu cortex-a57 -kernel $< -nographic -monitor none -d int,in_asm,guest_errors,exec,cpu -D qemu.log


test:
	make -C test run

wwos.img: boot/boot.img
	cat $< > $@

compile_flags.txt: Makefile
	echo $(CCFLAGS) "-xc++" | tr ' ' '\n' > $@

test/compile_flags.txt: test/Makefile
	make -C test compile_flags.txt

dev: compile_flags.txt test/compile_flags.txt
	make -C dev/qemu_tracer all

clean:
	rm -f qemu.log qemu.log.sym
	rm -f kernel/aarch64/start.s
	find . \( -name "*.o" -o -name "*.elf" -o -name "*.img" -o -name "*.ld" -o -name "*.a" -o -name "*.app"  -o -name "*.wwfs" -o -name "*.bin" -o -name "compile_flags.txt" \) -type f -delete
	make -C libwwos clean
	make -C applications/init clean
	make -C applications/shell clean
	make -C tools clean
	make -C test clean
	make -C dev/qemu_tracer clean

boot/boot.o: boot/$(BOARD)/boot.s kernel/kernel.img memdisk.wwfs
	$(AS) $(ASFLAGS) -c $< -o $@

boot/linker.ld: boot/linker.ld.tmpl
	$(CC) -P -E -xc++ $(DEFINES) $< -o $@

boot/boot.elf: boot/linker.ld boot/boot.o boot/loader.o
	$(LD) -nostdlib -T$< $(filter-out $<,$^) -o $@

boot/boot.img: boot/boot.elf
	$(OBJCOPY) -O binary $< $@

boot/loader.o: boot/$(BOARD)/loader.cc
	$(CC) $(CCFLAGS) -c $< -o $@

libwwos/libwwos_kernel.a:
	$(MAKE) -C libwwos libwwos_kernel.a

KERNEL_OBJS = kernel/main.o kernel/logging.o kernel/global.o kernel/memory.o kernel/syscall.o kernel/process.o kernel/filesystem.o kernel/scheduler.o kernel/drivers/gic2.o kernel/drivers/pl011.o
KERNEL_AARCH64_OBJS = kernel/aarch64/memory.o kernel/aarch64/time.o kernel/aarch64/interrupt.o kernel/aarch64/exception.o kernel/aarch64/start.o

kernel/aarch64/start.s: kernel/aarch64/start.s.tmpl
	$(CC) -P -E -xc++ $(DEFINES) $< -o $@

kernel/aarch64/start.o: kernel/aarch64/start.s
	$(AS) $(ASFLAGS) -c $< -o $@

kernel/aarch64/exception.o: kernel/aarch64/exception.s
	$(AS) $(ASFLAGS) -c $< -o $@

kernel/%.o: kernel/%.cc
	$(CC) $(CCFLAGS) -c $< -o $@

kernel/linker.ld: kernel/linker.ld.tmpl
	$(CC) -P -E -xc++ $(DEFINES) $< -o $@

kernel/kernel.elf: kernel/linker.ld $(KERNEL_OBJS) $(KERNEL_AARCH64_OBJS) libwwos/libwwos_kernel.a
	$(LD) -nostdlib -T$< $(filter-out $<,$^) -o $@

kernel/kernel.img: kernel/kernel.elf
	$(OBJCOPY) -O binary $< $@


memdisk.wwfs: tools applications/init/init.app applications/shell/shell.app applications/tty/tty.app
	./tools/wwfs initialize memdisk.wwfs 1024 2048 1024
	./tools/wwfs add memdisk.wwfs /app/init  applications/init/init.app
	./tools/wwfs add memdisk.wwfs /app/shell applications/shell/shell.app
	./tools/wwfs add memdisk.wwfs /app/tty   applications/tty/tty.app

applications/init/init.app:
	$(MAKE) -C applications/init init.app

applications/shell/shell.app:
	$(MAKE) -C applications/shell shell.app

applications/tty/tty.app:
	$(MAKE) -C applications/tty tty.app

dev/qemu_tracer/qemu_tracer.so:
	$(MAKE) -C dev/qemu_tracer qemu_tracer.so

tools:
	$(MAKE) -C tools
