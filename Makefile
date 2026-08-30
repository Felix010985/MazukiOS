# ==============================================================================
#                      Прiвет я Фелiкс а это мой Makefile!
# ==============================================================================

CC      := x86_64-linux-gnu-gcc
LD      := x86_64-linux-gnu-ld
GRUB    := grub-mkrescue
QEMU    := qemu-system-x86_64

BUILD_DIR := build
ISO_DIR   := $(BUILD_DIR)/iso
BOOT_DIR  := $(ISO_DIR)/boot
GRUB_DIR  := $(BOOT_DIR)/grub

MUSL_DIR  := world/musl
MUSL_INC  := -I$(MUSL_DIR)/include

SYS_CFLAGS  := -Iinclude -ffreestanding -m32 -nostdlib -fno-stack-protector -fno-pic -O0 -Wall -Wextra -MMD
USER_CFLAGS := -Iworld $(MUSL_INC) -nostdinc -ffreestanding -m32 -mno-sse -mno-sse2 -fno-stack-protector -fno-pic -O0 -Wall -Wextra -MMD

LDFLAGS := -m elf_i386 -T linker.ld -n --no-warn-rwx-segment

SYS_SRCS   := $(shell find sys -name '*.c' 2>/dev/null)
SYS_OBJS   := $(SYS_SRCS:sys/%.c=$(BUILD_DIR)/sys/%.o)

USER_SRCS  := $(shell find world -name '*.c' ! -path "world/musl/*" 2>/dev/null)
USER_OBJS  := $(USER_SRCS:world/%.c=$(BUILD_DIR)/world/%.o)

ALL_OBJS   := $(SYS_OBJS) $(USER_OBJS)
DEP_FILES  := $(ALL_OBJS:%.o=%.d)

.PHONY: all
all: $(BUILD_DIR)/kernel.elf $(BUILD_DIR)/world/shell.elf
	@echo "==== Компиляция и линковка ядра и юзерленда успешно завершена! ===="

.PHONY: iso
iso: $(BUILD_DIR)/mazukios.iso
	@echo "==== ISO-образ MazukiOS полностью готов к тестированию! ===="

.PHONY: run
run: iso
	@echo "==== Запуск MazukiOS в QEMU ===="
	$(QEMU) -cdrom $(BUILD_DIR)/mazukios.iso -m 256M -serial stdio -no-reboot -no-shutdown

$(BUILD_DIR)/mazukios.iso: $(BUILD_DIR)/kernel.elf $(BUILD_DIR)/world/shell.elf $(GRUB_DIR)/grub.cfg
	@echo "==== Создание ISO-образа через grub-mkrescue ===="
	@mkdir -p $(BOOT_DIR)
	cp $(BUILD_DIR)/kernel.elf $(BOOT_DIR)/
	cp $(BUILD_DIR)/world/shell.elf $(BOOT_DIR)/
	$(GRUB) -o $@ $(ISO_DIR) 2>/dev/null
	@echo "ISO успешно собран: $@"

$(BUILD_DIR)/kernel.elf: $(SYS_OBJS) linker.ld
	@echo "==== Линковка ядра Masix ===="
	@mkdir -p $(dir $@)
	$(LD) $(LDFLAGS) -o $@ $(SYS_OBJS)
	@echo "=== Проверка структуры бинарника ядра ==="
	@echo "Размер файла: $$(wc -c < $@) байт"
	@x86_64-linux-gnu-readelf -S $@ | head -15

$(BUILD_DIR)/world/shell.elf: $(USER_OBJS)
	@echo "==== Линковка юзерленда с musl libc через GCC ===="
	@mkdir -p $(dir $@)
	$(CC) -m32 -static -nostdlib -Wl,-Ttext=0x01000000 -o $@ \
		$(MUSL_DIR)/lib/crt1.o \
		$(MUSL_DIR)/lib/crti.o \
		$(USER_OBJS) \
		-L$(MUSL_DIR)/lib -lc \
		$(MUSL_DIR)/lib/crtn.o \
		-lgcc


	@echo "=== Проверка структуры юзерленда ==="
	@echo "Размер бинарника shell.elf: $$(wc -c < $@) байт"

$(BUILD_DIR)/sys/%.o: sys/%.c
	@mkdir -p $(dir $@)
	@echo "CC (Kernel)   $< -> $@"
	$(CC) $(SYS_CFLAGS) -c $< -o $@

$(BUILD_DIR)/world/%.o: world/%.c
	@mkdir -p $(dir $@)
	@echo "CC (Userland) $< -> $@"
	$(CC) $(USER_CFLAGS) -c $< -o $@

$(GRUB_DIR)/grub.cfg:
	@mkdir -p $(GRUB_DIR)
	@echo "==== Генерация конфигурации GRUB ===="
	@echo 'set timeout=0' > $@
	@echo 'set default=0' >> $@
	@echo '' >> $@
	@echo 'menuentry "MazukiOS (LiveCD)" {' >> $@
	@echo '    multiboot2 /boot/kernel.elf' >> $@
	@echo '    module2 /boot/shell.elf rootfs' >> $@
	@echo '    boot' >> $@
	@echo '}' >> $@

.PHONY: clean
clean:
	@echo "==== Очистка рабочей директории build/ ===="
	rm -rf $(BUILD_DIR)

-include $(DEP_FILES)
