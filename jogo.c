#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>

#define PORTA "/dev/ttyUSB0"
#define BAUD B9600
#define LINHAS 8
#define COLUNAS 20

// Mapa do jogo: # = parede, espaço = livre
char mapa[LINHAS][COLUNAS+1] = {
    "####################",
    "#                  #",
    "#                  #",
    "#                  #",
    "#                  #",
    "#                  #",
    "#                  #",
    "####################"
};

// Posição do jogador
int px = 2, py = 2;

// Limpa a tela
void limpar() {
    write(1, "\033[2J\033[H", 7);
}

// Desenha o mapa com o jogador
void desenhar() {
    limpar();
    
    // Copia o mapa e coloca o jogador
    char tela[LINHAS][COLUNAS+2];
    for (int i = 0; i < LINHAS; i++) {
        strcpy(tela[i], mapa[i]);
        tela[i][COLUNAS] = '\n';
        tela[i][COLUNAS+1] = '\0';
    }
    tela[py][px] = '@';
    
    // Desenha linha por linha
    for (int i = 0; i < LINHAS; i++) {
        write(1, tela[i], strlen(tela[i]));
    }
    
    write(1, "\nUse o joystick para mover!\n", 27);
}

// Configura a porta serial (igual ao código anterior)
int configurar_serial(const char *porta) {
    int fd = open(porta, O_RDWR | O_NOCTTY | O_NDELAY);
    if (fd < 0) {
        perror("Erro ao abrir porta serial");
        return -1;
    }

    struct termios tty;
    tcgetattr(fd, &tty);
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
    tcsetattr(fd, TCSANOW, &tty);

    return fd;
}

int main() {
    int fd = configurar_serial(PORTA);
    if (fd < 0) return 1;

    desenhar();

    char buf[256];
    while (1) {
        memset(buf, 0, sizeof(buf));
        int n = read(fd, buf, sizeof(buf) - 1);
        
        if (n > 0) {
            // Extrai a direção do formato "X:512,Y:498,D:CIMA"
            char *d = strstr(buf, "D:");
            if (d == NULL) continue;
            d += 2; // pula o "D:"

            int nx = px, ny = py;

            if (strncmp(d, "CIMA",    4) == 0) ny--;
            else if (strncmp(d, "BAIXO",   5) == 0) ny++;
            else if (strncmp(d, "ESQUERDA",8) == 0) nx--;
            else if (strncmp(d, "DIREITA", 7) == 0) nx++;

            // Só move se não for parede
            if (mapa[ny][nx] != '#') {
                px = nx;
                py = ny;
            }

            desenhar();
        }
    }

    close(fd);
    return 0;
}
