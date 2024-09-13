# #!/bin/bash

# cargo +nightly build -Z build-std=core,alloc --target buildscripts/aarch64-unknown-none.json || exit 1
# qemu-system-aarch64 -machine virt -cpu cortex-a57 -nographic -monitor none -kernel target/aarch64-unknown-none/debug/wwos
# # qemu-system-aarch64 -machine virt -cpu host -accel hvf -nographic -monitor none -kernel target/aarch64-unknown-none/debug/wwos 


# # qemu-system-aarch64 -machine virt -cpu cortex-a57 -nographic -monitor none -kernel target/aarch64-unknown-none/debug/wwos -S -s

# qemu-system-aarch64 -machine virt -cpu cortex-a57 -nographic -monitor none -kernel target/aarch64-unknown-none/debug/wwos

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
# IMAGE_TYPE = BIN
# BINARY = kernel8.img
else
    $(error "Unknown BOARD value")
endif

target/linker.ld: buildscripts/aarch64.ld
	rm -rf target
	mkdir -p target
	gcc -E -P -x c buildscripts/aarch64.ld -o target/linker.ld -DBOOT_OFFSET=$(BOOT_OFFSET)

.PHONY: build
build: target/linker.ld
	RUSTFLAGS='--cfg WWOS_BOARD="$(BOARD)"' cargo +nightly build -Z build-std=core,alloc --target buildscripts/aarch64-unknown-none.json
    ifeq ($(IMAGE_TYPE),BIN)
		llvm-objcopy -O binary target/aarch64-unknown-none/debug/wwos target/aarch64-unknown-none/debug/$(BINARY)
    endif

.PHONY: clippy
clippy:
	RUSTFLAGS='--cfg WWOS_BOARD="$(BOARD)"' cargo +nightly clippy --fix -Z build-std=core,alloc --target buildscripts/aarch64-unknown-none.json

.PHONY: run
run: build
	qemu-system-aarch64 $(QEMU_FLAGS) -kernel target/aarch64-unknown-none/debug/$(BINARY) -d guest_errors

.PHONY: debug
debug: build
	qemu-system-aarch64 $(QEMU_FLAGS) -kernel target/aarch64-unknown-none/debug/$(BINARY) -S -s

.PHONY: clean
clean:
	cargo clean
	rm -rf target
	rm -f qemu.log

