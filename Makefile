BOARD 	= qemu_aarch64_virt9
ACCEL   = ON
LOGGING = OFF

ifeq ($(BOARD),raspi4b)
    QEMU_FLAGS = -machine raspi4b -serial stdio -dtb emulation/bcm2711-rpi-4-b.dtb -nographic -monitor none
	BOOT_OFFSET = 0x80000
	IMAGE_TYPE = BIN
	BINARY = kernel8.img
else ifeq ($(BOARD),qemu_aarch64_virt9)
    QEMU_FLAGS = -machine virt -monitor none -serial stdio -nographic
	ifeq ($(ACCEL),ON)
		QEMU_FLAGS += -accel hvf -cpu host
	else
		QEMU_FLAGS += -cpu cortex-a72 
	endif
	BOOT_OFFSET = 0x40100000
	IMAGE_TYPE = ELF
	BINARY = wwos
else
    $(error "Unknown BOARD value")
endif

ifeq ($(LOGGING),ON)
	QEMU_FLAGS += -d int,in_asm,guest_errors -D qemu.log
else
	QEMU_FLAGS += -d guest_errors
endif

.PHONY: build
build: export RUSTFLAGS = --cfg WWOS_BOARD="$(BOARD)"
build: intermediate/linker.ld
	cargo +nightly build --package wwshell -Z build-std=core,alloc --target buildscripts/aarch64-unknown-wwos.json
	llvm-objcopy -O binary target/aarch64-unknown-wwos/debug/wwshell intermediate/wwos.blob

	cargo clean --package wwos_blob --target buildscripts/aarch64-unknown-none.json

	cargo +nightly build --package wwos -Z build-std=core,alloc --target buildscripts/aarch64-unknown-none.json

ifeq ($(IMAGE_TYPE),BIN)
	llvm-objcopy -O binary target/aarch64-unknown-none/debug/wwos target/aarch64-unknown-none/debug/$(BINARY);
endif

intermediate/linker.ld: buildscripts/aarch64.ld
	mkdir -p intermediate
	gcc -E -P -x c buildscripts/aarch64.ld -o intermediate/linker.ld -DBOOT_OFFSET=$(BOOT_OFFSET)

.PHONY: run
run: build
	qemu-system-aarch64 $(QEMU_FLAGS) -kernel target/aarch64-unknown-none/debug/$(BINARY)

.PHONY: debug
debug: build
	qemu-system-aarch64 $(QEMU_FLAGS) -kernel target/aarch64-unknown-none/debug/$(BINARY) -S -s

.PHONY: clean
clean:
	rm -rf target
	rm -rf intermediate
	rm -f qemu.log
