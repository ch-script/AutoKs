#include <stdio.h>
//#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/time.h>
#include <linux/uinput.h>
#include <termios.h>
#include <getopt.h>
#include <ctype.h>

// T-T
unsigned short get_keycode(char c) {
    c = (char)tolower(c);
    switch (c) {
        case 'a': return KEY_A;
        case 'b': return KEY_B;
        case 'c': return KEY_C;
        case 'd': return KEY_D;
        case 'e': return KEY_E;
        case 'f': return KEY_F;
        case 'g': return KEY_G;
        case 'h': return KEY_H;
        case 'i': return KEY_I;
        case 'j': return KEY_J;
        case 'k': return KEY_K;
        case 'l': return KEY_L;
        case 'm': return KEY_M;
        case 'n': return KEY_N;
        case 'o': return KEY_O;
        case 'p': return KEY_P;
        case 'q': return KEY_Q;
        case 'r': return KEY_R;
        case 's': return KEY_S;
        case 't': return KEY_T;
        case 'u': return KEY_U;
        case 'v': return KEY_V;
        case 'w': return KEY_W;
        case 'x': return KEY_X;
        case 'y': return KEY_Y;
        case 'z': return KEY_Z;
        case '1': return KEY_1;
        case '2': return KEY_2;
        case '3': return KEY_3;
        case '4': return KEY_4;
        case '5': return KEY_5;
        case '6': return KEY_6;
        case '7': return KEY_7;
        case '8': return KEY_8;
        case '9': return KEY_9;
        case '0': return KEY_0;
        case ' ': return KEY_SPACE;
        case '\n': return KEY_ENTER;
        default:  return 0;
    }
}

void terminal_conf(struct termios *oldt) {
    struct termios newt;
    tcgetattr(STDIN_FILENO, oldt);
    newt = *oldt;

    newt.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);

    int flags = fcntl(STDIN_FILENO, F_GETFL, 0);
    fcntl(STDIN_FILENO, F_SETFL, flags | O_NONBLOCK);
}

void reset_terminal(struct termios *oldt) {
    tcsetattr(STDIN_FILENO, TCSANOW, oldt);
}

void send_event(int fd, unsigned short type, unsigned short code, int val) {
    struct input_event input;
    memset(&input, 0, sizeof(input));
    gettimeofday(&input.time, NULL);
    input.type = type;
    input.code = code;
    input.value = val;
    write(fd, &input, sizeof(input));

    memset(&input, 0, sizeof(input));
    gettimeofday(&input.time, NULL);
    input.type = EV_SYN;
    input.code = SYN_REPORT;
    input.value = 0;
    write(fd, &input, sizeof(input));
}

void print_usage(const char *prog_name) {
    printf("Usage: %s [-k <key>] [--key <key>]\n", prog_name);
    printf("Options:\n");
    printf("  -k, --key <char>    Key to spam (default: 'f')\n");
    printf("  -h, --help          Show this help message\n");
}

int main(int argc, char *argv[]) {
    char target_char = 'f';

    // Parse command line arguments
    static struct option long_options[] = {
        {"key",  required_argument, 0, 'k'},
        {"help", no_argument,       0, 'h'},
        {0, 0, 0, 0}
    };

    int opt;
    while ((opt = getopt_long(argc, argv, "k:h", long_options, NULL)) != -1) {
        switch (opt) {
            case 'k':
                if (optarg && strlen(optarg) > 0) {
                    target_char = optarg[0];
                }
                break;
            case 'h':
                print_usage(argv[0]);
                return 0;
            default:
                print_usage(argv[0]);
                return 1;
        }
    }

    unsigned short target_keycode = get_keycode(target_char);
    if (target_keycode == 0) {
        fprintf(stderr, "Error: Key '%c' is unsupported.\n", target_char);
        return 1;
    }

    struct termios original;

    int fd = open("/dev/uinput", O_WRONLY | O_NONBLOCK);
    if (fd < 0) {
        perror("Error opening /dev/uinput");
        return 1;
    }

    // assign
    ioctl(fd, UI_SET_EVBIT, EV_KEY);
    ioctl(fd, UI_SET_KEYBIT, target_keycode);

    struct uinput_setup usetup;
    memset(&usetup, 0, sizeof(usetup));
    strcpy(usetup.name, "Autoclicker");
    ioctl(fd, UI_DEV_SETUP, &usetup);
    ioctl(fd, UI_DEV_CREATE);

    sleep(1); // register device

    // set terminal to non-canonical mode and flush initial buffer
    terminal_conf(&original);
    char dump;
    while (read(STDIN_FILENO, &dump, 1) > 0);

    printf("Clicking %c, Press only 'c' or 'C' to quit.\n", target_char);

    int exec = 1;
    while (exec) { // read pressed key
        char ch;
        if (read(STDIN_FILENO, &ch, 1) > 0) {
            if (ch == 'c' || ch == 'C') {
                printf("bye.\n");
                exec = 0;
            }
        }

        if (exec) {
            // press key
            send_event(fd, EV_KEY, target_keycode, 1);
            usleep(50000);

            // release key
            send_event(fd, EV_KEY, target_keycode, 0);

            // delay
            usleep(3000000);
        }
    }

    // Cleanup on exit
    ioctl(fd, UI_DEV_DESTROY);
    close(fd);
    reset_terminal(&original);

    return 0;
}
