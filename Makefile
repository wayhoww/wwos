include wwos.mk

DEFINES += -DWWOS_KERNEL

.PHONY: all tools run log trace clean test dev memdisk.wwfs libwwos/libwwos_kernel.a \
		applications/init/init.app \
		applications/shell/shell.app \
		applications/tty/tty.app \
		applications/demo1/demo1.app \
		applications/demo2/demo2.app \
		applications/hello/hello.app \
		qemu.log.sym

all: wwos.img tools compile_flags.txt

run: wwos.img
	qemu-system-aarch64 $(QEMU_FLAGS) -nographic  -kernel $< 

demo: wwos.img
	qemu-system-aarch64 $(QEMU_FLAGS) -serial vc:400x300  -kernel $< 

log: wwos.img
	qemu-system-aarch64 $(QEMU_FLAGS) -nographic -kernel $< -d int,in_asm,guest_errors -D qemu.log

trace: wwos.img
	qemu-system-aarch64 $(QEMU_FLAGS) -nographic -kernel $< -d int,in_asm,guest_errors,exec -D qemu.log

debug: wwos.img
	qemu-system-aarch64 $(QEMU_FLAGS) -nographic -kernel $< -d int,in_asm,guest_errors -D qemu.log -S -s

qemu.log.sym: dev/symbolify.py qemu.log
	python3 dev/symbolify.py $(KA_BEGIN) kernel/kernel.elf <qemu.log >qemu.log.sym

test:
	make -C test run

wwos.img: boot/boot.img
	cat $< > $@

compile_flags.txt: Makefile
	echo $(CCFLAGS) "-xc++" | tr ' ' '\n' > $@

test/compile_flags.txt: test/Makefile
	make -C test compile_flags.txt

dev: compile_flags.txt test/compile_flags.txt

clean:
	rm -f qemu.log qemu.log.sym
	rm -f kernel/aarch64/start.s
	find . \( -name "*.o" -o -name "*.elf" -o -name "*.img" -o -name "*.ld" -o -name "*.a" -o -name "*.app"  -o -name "*.wwfs" -o -name "*.bin" -o -name "compile_flags.txt" \) -type f -delete
	make -C libwwos clean
	make -C applications/init clean
	make -C applications/shell clean
	make -C applications/tty clean
	make -C applications/demo1 clean
	make -C applications/demo2 clean
	make -C applications/hello clean
	make -C tools clean
	make -C test clean

boot/boot.o: boot/aarch64/$(BOOT_ASM) kernel/kernel.img memdisk.wwfs
	$(AS) $(ASFLAGS) -c $< -o $@

boot/linker.ld: boot/linker.ld.tmpl
	$(CC) -P -E -xc++ $(DEFINES) $< -o $@

boot/boot.elf: boot/linker.ld boot/boot.o boot/loader.o
	$(LD) -nostdlib -T$< $(filter-out $<,$^) -o $@

boot/boot.img: boot/boot.elf
	$(OBJCOPY) -O binary $< $@

boot/loader.o: boot/aarch64/loader.cc
	$(CC) $(CCFLAGS) -c $< -o $@

libwwos/libwwos_kernel.a:
	$(MAKE) -C libwwos libwwos_kernel.a

KERNEL_OBJS  = kernel/main.o 
KERNEL_OBJS += kernel/logging.o
KERNEL_OBJS += kernel/global.o
KERNEL_OBJS += kernel/memory.o
KERNEL_OBJS += kernel/syscall.o
KERNEL_OBJS += kernel/process.o
KERNEL_OBJS += kernel/filesystem.o
KERNEL_OBJS += kernel/scheduler.o
KERNEL_OBJS += kernel/drivers/gic2.o
KERNEL_OBJS += kernel/drivers/pl011.o

KERNEL_AARCH64_OBJS  = kernel/aarch64/memory.o 
KERNEL_AARCH64_OBJS += kernel/aarch64/time.o 
KERNEL_AARCH64_OBJS += kernel/aarch64/interrupt.o 
KERNEL_AARCH64_OBJS += kernel/aarch64/exception.o 
KERNEL_AARCH64_OBJS += kernel/aarch64/start.o

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


memdisk.wwfs: tools applications/init/init.app applications/shell/shell.app applications/tty/tty.app applications/demo1/demo1.app applications/demo2/demo2.app applications/hello/hello.app
	./tools/wwfs initialize memdisk.wwfs 1024 2048 1024
	./tools/wwfs add memdisk.wwfs /app/init  		  applications/init/init.app
	./tools/wwfs add memdisk.wwfs /app/shell 		  applications/shell/shell.app
	./tools/wwfs add memdisk.wwfs /app/tty   		  applications/tty/tty.app
	./tools/wwfs add memdisk.wwfs /app/demo1   		  applications/demo1/demo1.app
	./tools/wwfs add memdisk.wwfs /app/demo2   		  applications/demo2/demo2.app
	./tools/wwfs add memdisk.wwfs /app/hello   		  applications/hello/hello.app
	./tools/wwfs add memdisk.wwfs /data/hello.txt     data/hello.txt
	python3 ./dev/add_sources.py ./tools/wwfs memdisk.wwfs


applications/init/init.app:
	$(MAKE) -C applications/init init.app

applications/shell/shell.app:
	$(MAKE) -C applications/shell shell.app

applications/tty/tty.app:
	$(MAKE) -C applications/tty tty.app

applications/demo1/demo1.app:
	$(MAKE) -C applications/demo1 demo1.app

applications/demo2/demo2.app:
	$(MAKE) -C applications/demo2 demo2.app

applications/hello/hello.app:
	$(MAKE) -C applications/hello hello.app

tools:
	$(MAKE) -C tools
