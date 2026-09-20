.SILENT:

CC      := clang
LD      := ld.lld
GRUB    := grub-mkrescue
QEMU    := qemu-system-x86_64

BUILD_DIR := build
ISO_DIR   := $(BUILD_DIR)/iso
BOOT_DIR  := $(ISO_DIR)/boot
GRUB_DIR  := $(BOOT_DIR)/grub

ROOTFS_SRC_DIR := $(BUILD_DIR)/rootfs
INITRAMFS_IMG  := $(BUILD_DIR)/initramfs.cpio

MUSL_DIR  := world/musl
MUSL_INC  := -I$(MUSL_DIR)/include

TARGET_FLAGS := -target i686-unknown-elf

SYS_CFLAGS  := $(TARGET_FLAGS) -Isys/kernel/arch/i686/include -Isys/kernel/include -Isys/kernel/drivers/include -Isys/kernel/fs/include -Iinclude -ffreestanding -nostdlib -fno-stack-protector -fno-pic -O0 -Wall -Wextra -MMD
USER_CFLAGS := $(TARGET_FLAGS) -Iworld $(MUSL_INC) -nostdinc -ffreestanding -mno-sse -mno-sse2 -fno-stack-protector -fno-pic -O0 -Wall -Wextra -MMD

LDFLAGS := -m elf_i386 -T linker.ld -n

SYS_SRCS   := $(shell find sys -name '*.c' 2>/dev/null)
SYS_OBJS   := $(SYS_SRCS:sys/%.c=$(BUILD_DIR)/sys/%.o)

USER_SRCS  := $(shell find world -name '*.c' ! -path "world/musl/*" 2>/dev/null)
USER_ELFS  := $(USER_SRCS:world/%.c=$(BUILD_DIR)/world/%.elf)

ALL_OBJS   := $(SYS_OBJS)
DEP_FILES  := $(ALL_OBJS:%.o=%.d) $(USER_ELFS:%.elf=%.d)

CLR_RESET   := \033[0m
CLR_GREEN   := \033[1;32m
CLR_YELLOW  := \033[1;33m
CLR_RED     := \033[1;31m
CLR_CYAN    := \033[1;36m
CLR_GRAY    := \033[0;90m
CLR_MAGENTA := \033[1;35m

STATUS_OK   := [$(CLR_GREEN)  OK  $(CLR_RESET)]
STATUS_WARN := [$(CLR_YELLOW) WARN $(CLR_RESET)]
STATUS_INFO := [$(CLR_CYAN) INFO $(CLR_RESET)]

.PHONY: all
all: $(BUILD_DIR)/kernel.elf $(USER_ELFS) $(INITRAMFS_IMG)
	printf "$(CLR_GREEN)==== Компиляция и линковка ядра и юзерленда успешно завершена! ====$(CLR_RESET)\n"

.PHONY: kernel
kernel: $(BUILD_DIR)/kernel.elf
	printf "$(CLR_GREEN)==== Сборка ядра Masix завершена ====$(CLR_RESET)\n"

.PHONY: initramfs
initramfs: $(INITRAMFS_IMG)
	printf "$(CLR_GREEN)==== Сборка initramfs завершена ====$(CLR_RESET)\n"

.PHONY: iso
iso: $(BUILD_DIR)/mazukios.iso
	printf "$(CLR_GREEN)==== ISO-образ MazukiOS полностью готов к тестированию! ====$(CLR_RESET)\n"

.PHONY: run
run: iso
	printf "$(CLR_MAGENTA)==== Запуск MazukiOS в QEMU ====$(CLR_RESET)\n"
	$(QEMU) -cdrom $(BUILD_DIR)/mazukios.iso -m 256M -serial stdio -nic user,model=rtl8139 -no-reboot -no-shutdown

$(BUILD_DIR)/mazukios.iso: $(BUILD_DIR)/kernel.elf $(INITRAMFS_IMG) $(GRUB_DIR)/grub.cfg
	printf "$(STATUS_INFO) Создание ISO-образа с initramfs\n"
	mkdir -p $(BOOT_DIR)
	cp $(BUILD_DIR)/kernel.elf $(BOOT_DIR)/
	cp $(INITRAMFS_IMG) $(BOOT_DIR)/
	$(GRUB) -o $@ $(ISO_DIR) 2>/dev/null
	printf "$(STATUS_OK) ISO успешно собран: $(CLR_GREEN)$@$(CLR_RESET)\n"

$(BUILD_DIR)/kernel.elf: $(SYS_OBJS) linker.ld
	printf "$(STATUS_INFO) Линковка ядра Masix...\n"
	mkdir -p $(dir $@)
	$(LD) $(LDFLAGS) -o $@ $(SYS_OBJS)
	printf "      $(CLR_GRAY)| Размер файла: $$(wc -c < $@) байт$(CLR_RESET)\n"

$(BUILD_DIR)/world/%.elf: world/%.c
	printf "$(STATUS_INFO) Линковка юзерленда с musl libc: $(CLR_CYAN)$<$(CLR_RESET) -> $@\n"
	mkdir -p $(dir $@)
	$(CC) $(USER_CFLAGS) -static -nostdlib -Wl,-Ttext=0x01000000 -o $@ \
		$(MUSL_DIR)/lib/crt1.o \
		$(MUSL_DIR)/lib/crti.o \
		$< \
		-L$(MUSL_DIR)/lib -lc \
		$(MUSL_DIR)/lib/crtn.o \
		-lgcc 2> $(BUILD_DIR)/tmp_user_err.log || ( \
			printf "[$(CLR_RED)FAIL$(CLR_RESET)] CC (Userland) $<\n"; \
			cat $(BUILD_DIR)/tmp_user_err.log; \
			rm -f $(BUILD_DIR)/tmp_user_err.log; \
			exit 1 \
		)
	WARN_COUNT=$$(grep -c "warning:" $(BUILD_DIR)/tmp_user_err.log || true); \
	if [ $$WARN_COUNT -gt 0 ]; then \
		printf "$(STATUS_WARN) CC (Userland) $(CLR_CYAN)$<$(CLR_RESET) -> $@ $(CLR_GRAY)(x$$WARN_COUNT warnings)$(CLR_RESET)\n"; \
		grep "warning:" $(BUILD_DIR)/tmp_user_err.log | sed 's/^/      | /' | head -n 2; \
		if [ $$WARN_COUNT -gt 2 ]; then \
			printf "      $(CLR_GRAY)... и еще $$(($$WARN_COUNT - 2)) свернуто$(CLR_RESET)\n"; \
		fi; \
	fi; \
	rm -f $(BUILD_DIR)/tmp_user_err.log
	#mkdir -p $(ROOTFS_SRC_DIR)/bin
	#cp $@ $(ROOTFS_SRC_DIR)/bin/$(notdir $(basename $<))

$(INITRAMFS_IMG): $(USER_ELFS)
	printf "$(STATUS_INFO) Упаковка initramfs в формат CPIO...\n"
	mkdir -p $(ROOTFS_SRC_DIR)/bin
	mkdir -p $(ROOTFS_SRC_DIR)/etc
	mkdir -p $(ROOTFS_SRC_DIR)/dev
	mkdir -p $(ROOTFS_SRC_DIR)/proc
	mkdir -p $(ROOTFS_SRC_DIR)/sys

	for elf in $(USER_ELFS); do \
		cp $$elf $(ROOTFS_SRC_DIR)/bin/$$(basename $$elf .elf); \
	done

	if [ ! -f $(ROOTFS_SRC_DIR)/bin/init ] && [ -f $(ROOTFS_SRC_DIR)/bin/shell ]; then \
		cp $(ROOTFS_SRC_DIR)/bin/shell $(ROOTFS_SRC_DIR)/bin/init; \
	fi

	if [ -f world/etc/motd ]; then \
		cp world/etc/motd $(ROOTFS_SRC_DIR)/etc/motd; \
	else \
		echo "Welcome to MazukiOS!" > $(ROOTFS_SRC_DIR)/etc/motd; \
	fi

	cd $(ROOTFS_SRC_DIR) && find . | cpio -o -H newc > ../../$(INITRAMFS_IMG) 2>/dev/null
	printf "      $(CLR_GRAY)| Размер initramfs: $$(wc -c < $(INITRAMFS_IMG)) байт$(CLR_RESET)\n" \

	rm -rf $(ROOTFS_SRC_DIR)

$(BUILD_DIR)/sys/%.o: sys/%.c
	mkdir -p $(dir $@)
	$(CC) $(SYS_CFLAGS) -c $< -o $@ 2> $(BUILD_DIR)/tmp_sys_err.log || ( \
		printf "[$(CLR_RED)FAIL$(CLR_RESET)] CC (Kernel) $<\n"; \
		cat $(BUILD_DIR)/tmp_sys_err.log; \
		rm -f $(BUILD_DIR)/tmp_sys_err.log; \
		exit 1 \
	)
	WARN_COUNT=$$(grep -c "warning:" $(BUILD_DIR)/tmp_sys_err.log || true); \
	if [ $$WARN_COUNT -gt 0 ]; then \
		printf "$(STATUS_WARN) CC (Kernel)   $(CLR_CYAN)$<$(CLR_RESET) -> $@ $(CLR_GRAY)(x$$WARN_COUNT warnings)$(CLR_RESET)\n"; \
		grep "warning:" $(BUILD_DIR)/tmp_sys_err.log | sed 's/^/      | /' | head -n 2; \
		if [ $$WARN_COUNT -gt 2 ]; then \
			printf "      $(CLR_GRAY)... и еще $$(($$WARN_COUNT - 2)) свернуто$(CLR_RESET)\n"; \
		fi; \
	else \
		printf "$(STATUS_OK) CC (Kernel)   $(CLR_CYAN)$<$(CLR_RESET) -> $@\n"; \
	fi
	rm -f $(BUILD_DIR)/tmp_sys_err.log

$(GRUB_DIR)/grub.cfg:
	mkdir -p $(GRUB_DIR)
	printf "$(STATUS_INFO) Генерация конфигурации GRUB...\n"
	echo 'set timeout=0' > $@
	@echo 'set default=0' >> $@
	@echo 'set gfxmode=1024x768x32' >> $@
	@echo 'set gfxpayload=keep' >> $@
	@echo '' >> $@
	@echo 'menuentry "MazukiOS (LiveCD)" {' >> $@
	@echo '    multiboot2 /boot/kernel.elf' >> $@
	@echo '    module2 /boot/initramfs.cpio initramfs' >> $@
	@echo '    boot' >> $@
	@echo '}' >> $@

.PHONY: clean
clean:
	printf "$(CLR_CYAN)==== Очистка рабочей директории build/ ====$(CLR_RESET)\n"
	rm -rf $(BUILD_DIR)

-include $(DEP_FILES)
