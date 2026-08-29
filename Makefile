# Прiвет я фелiкс
CC      := x86_64-linux-gnu-gcc
LD      := x86_64-linux-gnu-ld
GRUB    := grub-mkrescue
QEMU    := qemu-system-x86_64

CFLAGS  := -Iinclude -c -ffreestanding -m32 -nostdlib -fno-stack-protector -fno-pic -O0
LDFLAGS := -m elf_i386 -T linker.ld -n --no-warn-rwx-segment

BUILD_DIR := build
ISO_DIR   := $(BUILD_DIR)/iso
BOOT_DIR  := $(ISO_DIR)/boot
GRUB_DIR  := $(BOOT_DIR)/grub

SRC_FILES := $(shell find sys world -name '*.c' 2>/dev/null)
OBJ_FILES := $(SRC_FILES:%.c=$(BUILD_DIR)/%.o)

.PHONY: all
all: $(BUILD_DIR)/mazukios.iso #git_commit

.PHONY: run
run: all
	@echo "Запуск QEMU..."
	$(QEMU) -cdrom $(BUILD_DIR)/mazukios.iso -m 256M -serial stdio -no-reboot -no-shutdown

$(BUILD_DIR)/mazukios.iso: $(BUILD_DIR)/kernel.elf $(GRUB_DIR)/grub.cfg
	@echo "Создание ISO..."
	cp $(BUILD_DIR)/kernel.elf $(BOOT_DIR)/
	$(GRUB) -o $@ $(ISO_DIR) 2>/dev/null

$(BUILD_DIR)/kernel.elf: $(OBJ_FILES)
	@echo "Линковка..."
	$(LD) $(LDFLAGS) -o $@ $(OBJ_FILES)
	@echo "=== Проверка бинарника ==="
	@echo "Размер файла: $$(wc -c < $@) байт"
	@echo "Секции:"
	x86_64-linux-gnu-readelf -S $@ | head -20

$(BUILD_DIR)/%.o: %.c
	@mkdir -p $(dir $@)
	@echo "Компиляция $< -> $@"
	$(CC) $(CFLAGS) $< -o $@

$(GRUB_DIR)/grub.cfg:
	@mkdir -p $(GRUB_DIR)
	@echo "Генерация grub.cfg..."
	@echo 'set timeout=5' > $@
	@echo 'set default=0' >> $@
	@echo '' >> $@
	@echo 'menuentry "MazukiOS (LiveCD)" {' >> $@
	@echo '    multiboot2 /boot/kernel.elf' >> $@
	@echo '    boot' >> $@
	@echo '}' >> $@

# .PHONY: git_commit
# git_commit:
# 	@if [ -d ".git" ]; then \
# 		git add .; \
# 		git commit -m "Auto-commit $$(date '+%Y-%m-%d %H:%M:%S')"; \
# 		echo "Автокоммит создан."; \
# 	else \
# 		echo "Git репозиторий не найден, автокоммит пропущен."; \
# 	fi

.PHONY: clean
clean:
	@echo "Очистка..."
	rm -rf $(BUILD_DIR)
