#include "shell.h"
#include "kernel/api.h"
#include "kernel/power.h"
#include "kernel/vga.h"
#include "kernel/serial.h"
#include <stdbool.h>
#include "kernel/filesystem.h"
#include <libc.h>
// #include <string.h>

bool is_blank(const char* str) {
    int i = 0;
    while (str[i]) {
        if (str[i] != ' ' && str[i] != '\t')
            return false;
        i++;
    }
    return true;
}

void shell_main(void) {
    char input[128];
    const char art[] =
    ",--.   ,--.                       ,--.    ,--. ,-----.  ,---.\n"
    "|   `.'   | ,--,--.,-----.,--.,--.|  |,-. `--''  .-.  ''   .-'\n"
    "|  |'.'|  |' ,-.  |`-.  / |  ||  ||     / ,--.|  | |  |`.  `-.\n"
    "|  |   |  |\\ '-'  | /  `-.'  ''  '|  \\  \\ |  |'  '-'  '.-'    |\n"
    "`--'   `--' `--`--'`-----' `----' `--'`--'`--' `-----' `-----'\n\n";
    print(art, VGA_LIGHT_GREEN);
    while (1) {
        print("user$", VGA_LIGHT_GREEN);
		print("root@", VGA_RED);
		print("LiveCD: ", VGA_MAGENTA);
        read_line(input, VGA_LIGHT_GRAY);
		if (is_blank(input)) {
        continue;
    	}
        if (strcmp(input, "help") == 0) {
            puts_com1("cmd: help!\n");
            set_color(VGA_LIGHT_GREEN);
            printf(
                "Available commands:\n"
                "help       - show help\n"
                "c          - clear the screen\n"
                "shutdown   - shutdown the system (poorly implemented)\n"
                "ver        - show OS version\n"
                "time       - show current system time\n"
                "Await russian translation soon\n"
            );
            reset_color();
        }
 //       else if (strcmp(input, "clear") == 0) {
 //           print("Not implemented yet!\n", VGA_RED);
 //       }
        else if (strcmp(input, "shutdown") == 0) {
            puts_com1("cmd: ");
            puts_com1(input);
            puts_com1("!\n");
            system_shutdown();
        }
        else if (strcmp(input, "ver") == 0) {
            puts_com1("cmd: ");
            puts_com1(input);
            puts_com1("!\n");
            print("MazukiOS 0.1.0\n", VGA_RED);
        }
        else if (strcmp(input, "c") == 0) {
            puts_com1("cmd: ");
            puts_com1(input);
            puts_com1("!\n");
            cls();
            // cursor_x = 0;
            // cursor_y = 0;
            // vga_update_cursor();
        }
        else if (strcmp(input, "time") == 0) {
            puts_com1("cmd: ");
            puts_com1(input);
            puts_com1("!\n");
            print("Not implemented yet!\n", VGA_RED);
        }
        else if (strcmp(input, "reboot") == 0) {
            puts_com1("cmd: ");
            puts_com1(input);
            puts_com1("!\n");
            // второй shutdown и заглушка для reboot
            shutdown_by_panic();
        }
        else if (strcmp(input, "write") == 0) {
            puts_com1("cmd: ");
            puts_com1(input);
            puts_com1("!\n");
            int f = fs_create("test.txt");

            unsigned char data[] = "Hello, MazukiOS!";
            unsigned int data_size = sizeof(data);

            fs_write(f, data, data_size);

            unsigned char buffer[32];
            fs_read(f, buffer, data_size);

            print(file_table[f].name, VGA_LIGHT_GRAY);
            print("\n", VGA_LIGHT_GRAY);
        }
		else {
            puts_com1("cmd: ");
            puts_com1(input);
            puts_com1("\n");
			print("Unrecognized command: ", VGA_LIGHT_RED);
            print(input, VGA_LIGHT_RED);
            print("\n", VGA_LIGHT_GRAY);
		}
    }
}
