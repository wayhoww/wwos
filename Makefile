BOARD = raspi4b

ifeq ($(BOARD),raspi4b)
    QEMU_FLAGS = -machine raspi4b -serial stdio -dtb emulation/bcm2711-rpi-4-b.dtb
	BOOT_OFFSET = 0x80000
	IMAGE_TYPE = BIN
	BINARY = kernel8.img
else ifeq ($(BOARD),qemu_aarch64_virt9)
    QEMU_FLAGS = -machine virt -cpu cortex-a72 -nographic -monitor none
	BOOT_OFFSET = 0x40100000
	IMAGE_TYPE = ELF
	BINARY = wwos
else
    $(error "Unknown BOARD value")
endif

.PHONY: build
build:
	mkdir -p intermediate
	gcc -E -P -x c buildscripts/aarch64.ld -o intermediate/linker.ld -DBOOT_OFFSET=$(BOOT_OFFSET)
	RUSTFLAGS='--cfg WWOS_BOARD="$(BOARD)"' cargo +nightly build -Z build-std=core,alloc --target buildscripts/aarch64-unknown-none.json
ifeq ($(IMAGE_TYPE),BIN)
	llvm-objcopy -O binary target/aarch64-unknown-none/debug/wwos target/aarch64-unknown-none/debug/$(BINARY);
endif

.PHONY: run
run: build
	qemu-system-aarch64 $(QEMU_FLAGS) -kernel target/aarch64-unknown-none/debug/$(BINARY) -d guest_errors

.PHONY: debug
debug: build
	qemu-system-aarch64 $(QEMU_FLAGS) -kernel target/aarch64-unknown-none/debug/$(BINARY) -S -s

.PHONY: clean
clean:
	rm -rf target
	rm -rf intermediate
	rm -f qemu.log
