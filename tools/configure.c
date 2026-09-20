#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>

const char* APPS[] = {"clang", "ld.lld", "grub-mkrescue", "qemu-system-x86_64", "cpio", "git"};
#define COUNT 6

const char* MANAGERS[] = {
    "/usr/bin/xbps-install",
    "/sbin/apk",
    "/usr/sbin/slackpkg",
    "/usr/sbin/pkg",
    "/usr/bin/pacman",
    "/usr/bin/apt-get"
};

char* VOID_ARGS[]    = {"/usr/bin/sudo", "xbps-install", "-Sy", "clang", "lld", "cpio", "grub", "qemu", "xorriso", "git", NULL};
char* ALPINE_ARGS[]  = {"/usr/bin/sudo", "apk", "add", "clang", "lld", "cpio", "grub", "xorriso", "qemu-system-x86_64", "git", NULL};
char* SLACK_ARGS[]   = {"/usr/bin/sudo", "slackpkg", "install", "clang", "lld", "cpio", "grub", "qemu", "git", NULL};
char* FREEBSD_ARGS[] = {"/usr/bin/sudo", "pkg", "install", "-y", "llvm", "lld", "cpio", "grub2-mkrescue", "qemu-devel", "git", NULL};
char* ARCH_ARGS[]    = {"/usr/bin/sudo", "pacman", "-S", "--needed", "--noconfirm", "clang", "lld", "cpio", "grub", "xorriso", "qemu-desktop", "git", NULL};
char* DEBIAN_ARGS[]  = {"/usr/bin/sudo", "apt-get", "install", "-y", "clang", "lld", "cpio", "grub-pc-bin", "xorriso", "qemu-system-x86", "git", NULL};

char** ARGS_MAP[] = {VOID_ARGS, ALPINE_ARGS, SLACK_ARGS, FREEBSD_ARGS, ARCH_ARGS, DEBIAN_ARGS};

int check_bin_in_path(const char* app) {
    char* path_env = getenv("PATH");
    if (!path_env) return 0;

    char* path_copy = malloc(strlen(path_env) + 1);
    if (!path_copy) return 0;
    strcpy(path_copy, path_env);

    char full_path[1024];
    int found = 0;

    char* token = strtok(path_copy, ":");
    while (token != NULL) {
        snprintf(full_path, sizeof(full_path), "%s/%s", token, app);
        if (access(full_path, X_OK) == 0) {
            found = 1;
            break;
        }
        token = strtok(NULL, ":");
    }

    free(path_copy);
    return found;
}

int run_command(char** args) {
    pid_t pid = fork();
    if (pid == 0) {
        execv(args[0], args);
        exit(1);
    } else if (pid > 0) {
        int status;
        waitpid(pid, &status, 0);
        if (WIFEXITED(status) && WEXITSTATUS(status) == 0) {
            return 0;
        }
        return 1;
    }
    return 1;
}

int setup_submodules() {
    if (access("world/musl/configure", F_OK) == 0) {
        return 0;
    }
    char* git_init[] = {
        "/usr/bin/git",
        "submodule",
        "update",
        "--init",
        "--recursive",
        "--depth=1",
        NULL
    };

    return run_command(git_init);
}

int main() {
    const char* CLR_RESET   = "\033[0m";
    const char* CLR_GREEN   = "\033[1;32m";
    const char* CLR_CYAN    = "\033[1;36m";
    const char* CLR_RED     = "\033[1;31m";

    printf("[%s INFO %s] Проверка env для MazukiOS\n", CLR_CYAN, CLR_RESET);

    int missing = 0;
    for (size_t i = 0; i < COUNT; i++) {
        if (check_bin_in_path(APPS[i])) {
            printf("[%s  OK  %s] Проверка утилиты: %s\n", CLR_GREEN, CLR_RESET, APPS[i]);
        } else {
            printf("[%s FAIL %s] Проверка утилиты: %s не найдена\n", CLR_RED, CLR_RESET, APPS[i]);
            missing++;
        }
    }

    if (missing > 0) {
        printf("\n[%s INFO %s] Обнаружен неполный тулчейн, определение пакетного менеджера...\n", CLR_CYAN, CLR_RESET);

        char** selected_args = NULL;
        int is_debian = 0;

        for (size_t i = 0; i < 6; i++) {
            if (access(MANAGERS[i], X_OK) == 0) {
                selected_args = ARGS_MAP[i];
                if (strcmp(MANAGERS[i], "/usr/bin/apt-get") == 0) {
                    is_debian = 1;
                }
                break;
            }
        }

        if (!selected_args) {
            printf("[%s FAIL %s] Ошибка env: пакетный менеджер хоста не определен\n", CLR_RED, CLR_RESET);
            return 1;
        }

        if (is_debian) {
            char* update_args[] = {"/usr/bin/sudo", "apt-get", "update", NULL};
            run_command(update_args);
        }

        printf("[%s INFO %s] Запуск автоматической установки недостающих пакетов...\n", CLR_CYAN, CLR_RESET);
        if (!run_command(selected_args)) {
            printf("[%s FAIL %s] Ошибка env: сбой при установке зависимостей\n", CLR_RED, CLR_RESET);
            return 1;
        }
        printf("[%s  OK  %s] Все системные пакеты успешно установлены\n", CLR_GREEN, CLR_RESET);
    }

    printf("[%s INFO %s] Проверка и инициализация Git-субмодулей...\n", CLR_CYAN, CLR_RESET);
    if (setup_submodules()) {
        printf("[%s FAIL %s] Ошибка при инициализации субмодуля musl\n", CLR_RED, CLR_RESET);
        return 1;
    }
    printf("[%s  OK  %s] Субмодули проверены и готовы\n", CLR_GREEN, CLR_RESET);

    printf("[%s  OK  %s] Конфигурация успешно завершена! Env готов к работе\n", CLR_GREEN, CLR_RESET);
    return 0;
}
