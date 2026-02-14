# MazukiOS

MazukiOS это простая операционная система написанная на C


## FAQ

#### Как запустить

Любой x86 эмулятор или гипервизор, или на реальном железе через метод загрузки Legacy или подобное

#### Что такое MazukiOS и зачем она нужна

MazukiOS это простая ОС которая имеет свой API, минимальную libc и драйвера на клавиатуру/видео

#### Как ее собрать

См. ниже


## Основные функции

- libc и api для программ
- VGA видеовывод с поддержкой цветов
- Текстовая оболочка
- Файловая система (пока не готова, только ramdisk :p)


## Отзывы или идеи для следующего релиза

Если у вас есть отзыв, вопрос, идея то обратитесь ко мне в комментарии под любым длинным видео. [Канал](https://www.youtube.com/@FelixProfi0r1)

## Сборка MazukiOS
### Требования

- Linux (На Windows не будет работать)

- Компилятор x86_64-linux-gnu-gcc

- Линкер x86_64-linux-gnu-ld

- grub-mkrescue для создания ISO

- qemu-system-x86_64/любой другой эмулятор для тестирования ISO

## Инструкция по сборке

1. Создайте директорию для сборки:
```bash
mkdir -p build/iso/boot/grub
```


2. Компилируйте все исходные файлы .c из папок kernel и shell в объектные файлы:
```bash
SRC_FILES=$(find kernel shell -name '*.c')
for src in $SRC_FILES; do
    x86_64-linux-gnu-gcc -Iinclude -c "$src" -o "build/$(basename ${src%.*}).o" \
        -ffreestanding -m32 -nostdlib -fno-pic -O0
done
```


3. Линкуйте объектные файлы в ELF-бинарник:
```bash
x86_64-linux-gnu-ld -m elf_i386 -T linker.ld -o build/kernel.elf build/*.o -n --no-warn-rwx-segment
```

4. Создайте ISO с помощью GRUB:
```bash
cp build/kernel.elf build/iso/boot/
cat > build/iso/boot/grub/grub.cfg << EOF
set timeout=5
set default=0

menuentry "MazukiOS (LiveCD)" {
    multiboot2 /boot/kernel.elf
    boot
}
EOF

grub-mkrescue -o build/mazukios.iso build/iso
```


Запустите ISO в QEMU:
```bash
qemu-system-x86_64 -cdrom build/mazukios.iso -m 256M -no-reboot -no-shutdown
```
