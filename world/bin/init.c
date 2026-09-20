#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/mount.h>
#include <fcntl.h>

void mount_essential_fs() {
    mount("none", "/proc", "proc", 0, NULL);
    mount("none", "/sys", "sysfs", 0, NULL);
    mount("none", "/dev", "devtmpfs", 0, NULL);
}

int main() {
    mount_essential_fs();

    int fd = open("/dev/tty0", O_RDWR);
    if (fd >= 0) {
        dup2(fd, 0); // stdin
        dup2(fd, 1); // stdout
        dup2(fd, 2); // stderr
        if (fd > 2) close(fd);
    }

    printf("\n\033[1;36m[ INFO ] PID 1 (init) успешно запущен!\033[0m\n");
    int enter;


    while (1) {
        pid_t pid = fork();

        if (pid == 0) {
            printf("\n\033[1;36m[ INFO ] pid==0\033[0m\n");
            while (1) {
                scanf("%d", &enter);
            }
            char* shell_args[] = {"/bin/shell", NULL};
            char* envp[] = {NULL};
            execve(shell_args[0], shell_args, envp);
            perror("ошибка запуска /bin/shell");
            while (1) {
                __asm__ volatile("hlt");
            }
            exit(1);
        } else if (pid > 0) {
            printf("\n\033[1;36m[ INFO ] pid > 0\033[0m\n");
            // while (1) {
            //     scanf("%d", &enter);
            // }
            int status;
            pid_t exited_pid;

            while ((exited_pid = wait(&status)) > 0) {
                if (exited_pid == pid) {
                    printf("\n\033[1;33m[ WARN ] Сессия шелла завершена. Перезапуск...\033[0m\n");
                    break;
                }
            }
        } else {
            printf("\n\033[1;36m[ INFO ] else...\033[0m\n");
            while (1) {
                scanf("%d", &enter);
            }
            perror("fork упал в init");
            sleep(2);
        }
    }

    return 0;
}
