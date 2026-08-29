# ==============================================================================
#                      Прiвет я Фелiкс а это мой Makefile!
# ==============================================================================

CC      := x86_64-linux-gnu-gcc
LD      := x86_64-linux-gnu-ld
GRUB    := grub-mkrescue
QEMU    := qemu-system-x86_64

CFLAGS  := -Iinclude -ffreestanding -m32 -nostdlib -fno-stack-protector -fno-pic -O0 -Wall -Wextra -MMD
LDFLAGS := -m elf_i386 -T linker.ld -n --no-warn-rwx-segment

BUILD_DIR := build
ISO_DIR   := $(BUILD_DIR)/iso
BOOT_DIR  := $(ISO_DIR)/boot
GRUB_DIR  := $(BOOT_DIR)/grub

SRC_FILES := $(shell find sys world -name '*.c' 2>/dev/null)
OBJ_FILES := $(SRC_FILES:%.c=$(BUILD_DIR)/%.o)
DEP_FILES := $(OBJ_FILES:%.o=%.d)

.PHONY: all
all: $(BUILD_DIR)/kernel.elf
	@echo "==== Компиляция и линковка ядра Masix успешно завершена! ===="

.PHONY: iso
iso: $(BUILD_DIR)/mazukios.iso
	@echo "==== ISO-образ MazukiOS полностью готов к тестированию! ===="

.PHONY: run
run: iso
	@echo "==== Запуск MazukiOS в QEMU ===="
	$(QEMU) -cdrom $(BUILD_DIR)/mazukios.iso -m 256M -serial stdio -no-reboot -no-shutdown

$(BUILD_DIR)/mazukios.iso: $(BUILD_DIR)/kernel.elf $(GRUB_DIR)/grub.cfg
	@echo "==== Создание ISO-образа через grub-mkrescue ===="
	@mkdir -p $(BOOT_DIR)
	cp $(BUILD_DIR)/kernel.elf $(BOOT_DIR)/
	$(GRUB) -o $@ $(ISO_DIR) 2>/dev/null
	@echo "ISO успешно собран: $@"

$(BUILD_DIR)/kernel.elf: $(OBJ_FILES) linker.ld
	@echo "==== Линковка ядра Masix ===="
	@mkdir -p $(dir $@)
	$(LD) $(LDFLAGS) -o $@ $(OBJ_FILES)
	@echo "=== Проверка структуры бинарника ==="
	@echo "Размер файла: $$(wc -c < $@) байт"
	@echo "Секции ядра:"
	@x86_64-linux-gnu-readelf -S $@ | head -20

$(BUILD_DIR)/%.o: %.c
	@mkdir -p $(dir $@)
	@echo "CC  $< -> $@"
	$(CC) $(CFLAGS) -c $< -o $@

$(GRUB_DIR)/grub.cfg:
	@mkdir -p $(GRUB_DIR)
	@echo "==== Генерация конфигурации GRUB ===="
	#@echo 'set timeout=0' > $@
	@echo 'set default=0' >> $@
	@echo '' >> $@
	@echo 'menuentry "MazukiOS (LiveCD)" {' >> $@
	@echo '    multiboot2 /boot/kernel.elf' >> $@
	@echo '    boot' >> $@
	@echo '}' >> $@

.PHONY: clean
clean:
	@echo "==== Очистка рабочей директории build/ ===="
	rm -rf $(BUILD_DIR)

-include $(DEP_FILES)
