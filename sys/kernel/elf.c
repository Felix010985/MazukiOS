/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Masix Kernel
 * Copyright (C) 2026, FelixProfi. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.
 */
#include <stdint.h>
#include <stddef.h>
#include <drivers/serial.h>
#include <vmm.h>
#include <pmm.h>

typedef struct {
    unsigned char e_ident[16];
    uint16_t      e_type;
    uint16_t      e_machine;
    uint32_t      e_version;
    uint32_t      e_entry;
    uint32_t      e_phoff;
    uint32_t      e_shoff;
    uint32_t      e_flags;
    uint16_t      e_ehsize;
    uint16_t      e_phentsize;
    uint16_t      e_phnum;
    uint16_t      e_shentsize;
    uint16_t      e_shnum;
    uint16_t      e_shstrndx;
} Elf32_Ehdr;

typedef struct {
    uint32_t p_type;
    uint32_t p_offset;
    uint32_t p_vaddr;
    uint32_t p_paddr;
    uint32_t p_filesz;
    uint32_t p_memsz;
    uint32_t p_flags;
    uint32_t p_align;
} Elf32_Phdr;

#define PT_LOAD 1

extern void* memcpy(void* dst, const void* src, unsigned int n);
extern void* memset(void* dst, int value, unsigned int n);
extern void printf(const char* fmt, ...);

void* elf_load_binary(uint32_t file_start) {
    Elf32_Ehdr* elf_header = (Elf32_Ehdr*)file_start;

    if (elf_header->e_ident[0] != 0x7F || elf_header->e_ident[1] != 'E'  ||
        elf_header->e_ident[2] != 'L'  || elf_header->e_ident[3] != 'F')
    {
        puts_com1("Masix: ELF: Invalid magic signature!\n");
        return NULL;
    }

    /* Создаем новое независимое пространство виртуальной памяти для процесса */
    if (vmm_userspace_create() != 0) {
        puts_com1("Masix: ELF: Failed to create user space page directory!\n");
        return NULL;
    }

    Elf32_Phdr* phdr = (Elf32_Phdr*)(file_start + elf_header->e_phoff);

    for (int i = 0; i < elf_header->e_phnum; i++) {
        if (phdr[i].p_type == PT_LOAD) {

            uint32_t start_vaddr = phdr[i].p_vaddr;
            uint32_t end_vaddr = start_vaddr + phdr[i].p_memsz;

            uint32_t page_start = start_vaddr & ~0xFFF;
            uint32_t page_end = (end_vaddr + 0xFFF) & ~0xFFF;

            /* Идем по страницам, выделяем память и мапим её */
            for (uint32_t vaddr = page_start; vaddr < page_end; vaddr += PAGE_SIZE) {
                uint32_t phys = pmm_alloc_zeroed();
                if (!phys) {
                    puts_com1("Masix: ELF: Out of physical memory!\n");
                    return NULL;
                }
                // Мапим виртуальный адрес на выделенную физическую страницу
                // Флаг PTE_W берем, если сегмент ELF разрешает запись.
                uint32_t flags = (phdr[i].p_flags & 0x2) ? PAGE_WRITE : 0;
                vmm_user_map(vaddr, phys, flags);
            }

            /* Копируем данные из initramfs в только что замапленную виртуальную память */
            if (phdr[i].p_filesz > 0) {
                memcpy((void*)start_vaddr, (void*)(file_start + phdr[i].p_offset), phdr[i].p_filesz);
            }

            /* Если размер в памяти больше размера в файле, обнуляем остаток */
            if (phdr[i].p_memsz > phdr[i].p_filesz) {
                uint32_t bss_start = start_vaddr + phdr[i].p_filesz;
                uint32_t bss_size = phdr[i].p_memsz - phdr[i].p_filesz;
                memset((void*)bss_start, 0, bss_size);
            }
        }
    }

    /* Загружаем свежесозданный каталог страниц в регистр CR3 процессора,
     * чтобы активировать изолированное адресное пространство этой программы.
     */
    vmm_user_switch();
    uint32_t entry = elf_header->e_entry;
    __asm__ volatile("" : : "a"(entry));

    return (void*)entry;
    //return (void*)elf_header->e_entry;
}
