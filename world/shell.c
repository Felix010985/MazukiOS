// shell для проверки взаимодействия с ядром - теперь полностью рабочий!
// #include "shell.h"
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>

#define MAX_INPUT 128

#define ANSI_GREEN   "\033[1;32m"
#define ANSI_RED     "\033[1;31m"
#define ANSI_MAGENTA "\033[1;35m"
#define ANSI_RESET   "\033[0m"

bool is_blank(const char* str) {
    int i = 0;
    while (str[i]) {
        if (str[i] != ' ' && str[i] != '\t' && str[i] != '\n' && str[i] != '\r')
            return false;
        i++;
    }
    return true;
}

void trim_newline(char* str) {
    int len = strlen(str);
    if (len > 0 && str[len - 1] == '\n') {
        str[len - 1] = '\0';
    }
}

void main(void) {
    setvbuf(stdout, NULL, _IONBF, 0);
    setvbuf(stderr, NULL, _IONBF, 0);
    char input[MAX_INPUT];
    char user[32];

    while (1) {
        printf("Enter your username for the session: ");


        if (fgets(user, sizeof(user), stdin) == NULL) {
            continue;
        }

        trim_newline(user);

        if (is_blank(user)) {
            printf("username is empty!\n");
            continue;
        } else {
            break;
        }
    }


    printf("\033[2J\033[H");

    const char art[] =
    ",--.   ,--.                       ,--.    ,--. ,-----.  ,---.\n"
    "|   `.'   | ,--,--.,-----.,--.,--.|  |,-. `--''  .-.  ''   .-'\n"
    "|  |'.'|  |' ,-.  |`-.  / |  ||  ||     / ,--.|  | |  |`.  `-.\n"
    "|  |   |  |\\ '-'  | /  `-.'  ''  '|  \\  \\ |  |'  '-'  '.-'    |\n"
    "`--'   `--' `--`--'`-----' `----' `--'`--'`--' `-----' `-----'\n\n";

    printf("%s%s%s", ANSI_GREEN, art, ANSI_RESET);

    while (1) {
        printf("%s%s$%s", ANSI_GREEN, user, ANSI_RESET);
        printf("%sroot@%s", ANSI_RED, ANSI_RESET);
        printf("%sLiveCD: %s", ANSI_MAGENTA, ANSI_RESET);

        if (fgets(input, sizeof(input), stdin) == NULL) {
            continue;
        }

        trim_newline(input);

        if (is_blank(input)) {
            continue;
        }

        if (strcmp(input, "help") == 0) {
            printf(
                "Available commands:\n"
                "help       - show help\n"
                "c          - clear the screen\n"
                "shutdown   - shutdown the system (stub)\n"
                "ver        - show OS version\n"
                "time       - show current system time\n"
                "pushup     - do some exercise\n"
                "panic      - trigger kernel halt\n"
            );
        }
        else if (strcmp(input, "c") == 0) {
            printf("\033[2J\033[H");
        }
        else if (strcmp(input, "shutdown") == 0) {
            printf("Shutdown call sent to kernel...\n");
        }
        else if (strcmp(input, "ver") == 0) {
            printf("MazukiOS 0.2.5\n");
        }
        else if (strcmp(input, "panic") == 0) {
            __asm__ __volatile__("cli");
        }
        else if (strcmp(input, "time") == 0) {
            printf("System time feature is not implemented yet!\n");
        }
        else if (strcmp(input, "div") == 0) {
            __asm__ __volatile__(
                "movl $10, %%eax \n\t"
                "movl $0, %%edx \n\t"
                "movl $0, %%ecx \n\t"
                "div %%ecx"
                :
                :
                : "eax", "edx", "ecx"
            );
        }
        else if (strcmp(input, "mazenvfetch") == 0) {
            printf("     _______    user@host\n");
            printf("  _ \______ \   os:       MazukiOS\n");
            printf(" | \  ___  \ |  pkg:      none [0]\n");
            printf(" | | /   \ | |  ram:      NaN / NaN MiB\n");
            printf(" | | \___/ | |  cpu:      Sugomachip\n");
            printf(" | \______ \_|  init:     sbsh\n");
            printf("  \_______\     shell:    sbsh\n");
            printf("                de/wm:    tty\n");
            printf("                envfetch: 3.4.7-mazukios-test\n"); // для скриншотиков пока нормального envfetch'а нету
        }
        else if (strcmp(input, "fork") == 0) {
            printf("Masix Userland: Executing fork() system call...\n");

            int pid = fork();

            if (pid < 0) {
                printf("Error: fork() failed!\n");
            }
            else if (pid == 0) {
                printf("\n[CHILD] Hello from the cloned process! PID: %d\n", getpid());
                printf("[CHILD] I'm running on a completely separate stack.\n");
                printf("[CHILD] Exiting now...\n");


                exit(0);
            }
            else {
                printf("[PARENT] Successfully spawned child task!\n");
                printf("[PARENT] Child PID assigned by Masix kernel: %d\n", pid);
                printf("[PARENT] Main shell continues working smoothly...\n");
            }
        }
        else {
            printf("Unrecognized command: %s\n", input);
        }
    }
}
