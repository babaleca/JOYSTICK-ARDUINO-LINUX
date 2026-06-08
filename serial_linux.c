#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <errno.h>

#define PORTA "/dev/ttyUSB0"
#define BAUD B9600

int main() {
    // Abre a porta serial como arquivo
    int fd = open(PORTA, O_RDWR | O_NOCTTY | O_NDELAY);
    if (fd < 0) {
        perror("Erro ao abrir porta serial");
        return 1;
    }
    printf("Porta %s aberta! Descritor: %d\n", PORTA, fd);

    // Configura a serial via POSIX
    struct termios tty;
    if (tcgetattr(fd, &tty) != 0) {
        perror("Erro tcgetattr");
        return 1;
    }

    cfsetispeed(&tty, BAUD);
    cfsetospeed(&tty, BAUD);
    tty.c_cflag |= (CLOCAL | CREAD);
    tty.c_cflag &= ~PARENB;
    tty.c_cflag &= ~CSTOPB;
    tty.c_cflag &= ~CSIZE;
    tty.c_cflag |= CS8;
    tty.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
    tty.c_iflag &= ~(IXON | IXOFF | IXANY);
    tty.c_oflag &= ~OPOST;
    tty.c_cc[VMIN]  = 0;
    tty.c_cc[VTIME] = 10;

    if (tcsetattr(fd, TCSANOW, &tty) != 0) {
        perror("Erro tcsetattr");
        return 1;
    }

    printf("Serial configurada! Lendo dados...\n");
    printf("Pressione Ctrl+C para sair\n\n");

    char buf[256];
    while (1) {
        memset(buf, 0, sizeof(buf));
        int n = read(fd, buf, sizeof(buf) - 1);
        if (n > 0) {
            buf[n] = '\0';
            printf("%s", buf);
            fflush(stdout);
        }
    }

    close(fd);
    return 0;
}
