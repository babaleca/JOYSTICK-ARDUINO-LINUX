# 🕹️ linux-serial-arduino-controller

Controle e monitoramento de um dispositivo externo (Arduino Uno) via porta serial USB, em linguagem C e ambiente GNU/Linux, usando chamadas de sistema POSIX.

Projeto desenvolvido para a disciplina de **Sistemas Operacionais — AED2** na PUC Goiás.

---

## Sobre o projeto

O programa abre a porta serial `/dev/ttyUSB0` como um arquivo comum, lê os dados enviados pelo Arduino (posição do joystick) e os exibe no terminal. Todo o acesso ao dispositivo é feito diretamente via syscalls POSIX — `open`, `read`, `write`, `ioctl` e `close` — sem bibliotecas de alto nível.

O Arduino lê um joystick analógico KY-023, calcula a direção e exibe no display LCD 16x2 (I2C), enquanto envia os dados pela serial no formato:

```
X:512,Y:498,D:CENTRO
```

---

## Hardware utilizado

| Componente | Descrição |
|-----------|-----------|
| Arduino Uno R3 (CH340) | Microcontrolador principal |
| Joystick analógico KY-023 | Entrada de controle (2 eixos + botão) |
| Display LCD 16x2 com I2C | Exibe a direção em tempo real |
| Cabo USB-A para USB-B | Comunicação serial com o PC |
| Jumpers macho-fêmea | Conexões entre os componentes |

### Diagrama de ligação

**Joystick KY-023 → Arduino**
| Pino Joystick | Pino Arduino |
|--------------|-------------|
| GND | GND |
| +5V | 5V |
| VRx | A0 |
| VRy | A1 |
| SW | D2 |

**LCD 16x2 I2C → Arduino**
| Pino LCD | Pino Arduino |
|---------|-------------|
| GND | GND |
| VCC | 5V |
| SDA | A4 |
| SCL | A5 |

---

## Estrutura do repositório

```
├── serial_linux.c    # Programa principal em C — lê a serial via syscalls POSIX
├── jogo.c            # Mini jogo de terminal controlado pelo joystick
├── evidencia.log     # Log gerado pelo strace — prova das interações com o kernel
└── README.md
```

O firmware do Arduino (`.ino`) foi desenvolvido na Arduino IDE e está disponível abaixo na seção de configuração.

---

## Ambiente

- **SO:** Ubuntu 22.04 via WSL2 (Windows Subsystem for Linux 2)
- **Compilador:** GCC 15.2.0
- **Conexão USB:** usbipd-win para bridge USB → WSL2
- **Ferramenta de evidência:** strace

> O WSL2 não é simulador — executa um kernel Linux real com chamadas de sistema verdadeiras, o que permite o uso de `strace` e acesso a `/dev/ttyUSB0`.

---

## Como executar

### Pré-requisitos

- WSL2 com Ubuntu instalado
- usbipd-win instalado no Windows
- GCC instalado no WSL (`sudo apt install build-essential`)
- strace instalado no WSL (`sudo apt install strace`)

### 1. Conectar o Arduino ao WSL

No **CMD do Windows como administrador:**
```cmd
usbipd attach --wsl --busid <busid>
```

Para descobrir o busid do Arduino:
```cmd
usbipd list
```
Procure por `USB-SERIAL CH340`.

### 2. Dar permissão na porta serial

No **terminal WSL:**
```bash
sudo chmod 666 /dev/ttyUSB0
```

### 3. Compilar

```bash
gcc serial_linux.c -o serial_linux
gcc jogo.c -o jogo
```

### 4. Executar

Leitura dos dados do joystick:
```bash
./serial_linux
```

Mini jogo de terminal controlado pelo joystick:
```bash
./jogo
```

### 5. Gerar evidência com strace

```bash
strace -o evidencia.log ./serial_linux
```

---

## Chamadas de sistema utilizadas

| Syscall | Onde | O que faz |
|---------|------|-----------|
| `open()` | `serial_linux.c` | Abre `/dev/ttyUSB0` como arquivo, retorna o descritor 3 |
| `ioctl()` (via `tcgetattr`) | `serial_linux.c` | Lê a configuração atual da porta serial |
| `ioctl()` (via `tcsetattr`) | `serial_linux.c` | Aplica configuração: 9600 baud, 8 bits, sem paridade (8N1) |
| `read()` | `serial_linux.c` | Lê os bytes enviados pelo Arduino |
| `write()` | `serial_linux.c` | Escreve a saída na tela (via stdout, descritor 1) |
| `close()` | `serial_linux.c` | Libera o descritor de arquivo ao encerrar |

Todas essas chamadas aparecem no `evidencia.log` gerado pelo `strace`, comprovando que o programa interage diretamente com o kernel do Linux.

---

## Firmware do Arduino

```cpp
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

LiquidCrystal_I2C lcd(0x27, 16, 2);

#define JOY_X  A0
#define JOY_Y  A1
#define JOY_SW 2

void setup() {
  Serial.begin(9600);
  pinMode(JOY_SW, INPUT_PULLUP);
  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("Iniciando...");
  delay(1000);
  lcd.clear();
}

void loop() {
  int x = analogRead(JOY_X);
  int y = analogRead(JOY_Y);
  int btn = digitalRead(JOY_SW);

  String direcao = "CENTRO";
  if (x < 300)      direcao = "ESQUERDA";
  else if (x > 700) direcao = "DIREITA";
  else if (y < 300) direcao = "CIMA";
  else if (y > 700) direcao = "BAIXO";
  if (btn == LOW)   direcao = "BOTAO";

  lcd.setCursor(0, 0);
  lcd.print("Direcao:        ");
  lcd.setCursor(0, 1);
  lcd.print(direcao + "          ");

  Serial.print("X:");
  Serial.print(x);
  Serial.print(",Y:");
  Serial.print(y);
  Serial.print(",D:");
  Serial.println(direcao);

  delay(200);
}
```

Biblioteca necessária: **LiquidCrystal I2C** (instalar pelo Library Manager da Arduino IDE).

---

## Conceitos de SO aplicados

- **Tudo é arquivo:** a porta serial é tratada como `/dev/ttyUSB0`, um character device em `/dev`
- **Descritores de arquivo:** `open()` retorna o descritor 3 (0, 1, 2 já são stdin/stdout/stderr)
- **Chamadas de sistema:** fronteira entre modo usuário e modo kernel
- **Gerência de dispositivos de E/S:** configuração via `ioctl`, modo não-bloqueante com `EAGAIN`
- **Modo raw:** configuração `8N1` via termios para comunicação serial direta

---

## Dificuldades e decisões

O plano original era instalar Ubuntu em dual boot, mas o disco estava criptografado com BitLocker e o Windows não liberou espaço suficiente para particionamento (~6 GB de 40 GB necessários). A solução foi usar o **WSL2**, que executa um kernel Linux real via virtualização leve — as syscalls capturadas pelo strace confirmam que a execução é genuína, não simulada.

---

## Autor

**Bandarra** — [@babaleca](https://github.com/babaleca)  
PUC Goiás — Sistemas Operacionais — 2026
