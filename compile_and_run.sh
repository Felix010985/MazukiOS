#!/bin/bash
set -e

BUILD=build
ISO=$BUILD/iso
LOG=$BUILD/build.log

echo "Очистка..."
rm -rf $BUILD
mkdir -p $BUILD
mkdir -p $ISO/boot/grub

exec > >(tee -a "$LOG") 2>&1

echo "Компиляция ядра и оболочки..."
SRC_FILES=($(find kernel shell -name '*.c'))

OBJ_FILES=()
for src in "${SRC_FILES[@]}"; do
    obj="$BUILD/$(basename ${src%.*}).o"
    x86_64-linux-gnu-gcc -Iinclude -c "$src" -o "$obj" \
        -ffreestanding -m32 -nostdlib -fno-pic -O0
    OBJ_FILES+=("$obj")
done

echo "Линковка..."
x86_64-linux-gnu-ld -m elf_i386 \
    -T linker.ld \
    -o $BUILD/kernel.elf \
    "${OBJ_FILES[@]}" \
    -n \
    --no-warn-rwx-segment

echo "=== Проверка бинарника ==="
echo "Размер файла:" $(wc -c < $BUILD/kernel.elf) "байт"
echo "Секции:"
x86_64-linux-gnu-readelf -S $BUILD/kernel.elf | head -20

echo "Создание ISO..."
cp $BUILD/kernel.elf $ISO/boot/

cat > $ISO/boot/grub/grub.cfg << EOF
set timeout=5
set default=0

menuentry "MazukiOS (LiveCD)" {
    multiboot2 /boot/kernel.elf
    boot
}
EOF

grub-mkrescue -o $BUILD/mazukios.iso $ISO 2>/dev/null
echo "Запуск QEMU..."
sleep 0.5
qemu-system-x86_64 \
    -cdrom $BUILD/mazukios.iso \
    -m 256M \
    -serial stdio \
    -no-reboot \
    -no-shutdown \
