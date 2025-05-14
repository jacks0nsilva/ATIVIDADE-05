#ifndef DEFINICOES_H
#define DEFINICOES_H

#define LED_BLUE_PIN 12                 // GPIO12 - LED azul
#define LED_GREEN_PIN 11                // GPIO11 - LED verde
#define LED_RED_PIN 13                  // GPIO13 - LED vermelho
#define I2C_PORT i2c1
#define I2C_SDA 14
#define I2C_SCL 15
#define ADRESS 0x3c
#define BUTTON_A 5
#define BUTTON_B 6

typedef struct
{
    uint pin_gpio; // Pino GPIO do LED/botão
    uint pin_dir;  // Direção do pino (0 = entrada, 1 = saída)
} PIN_GPIO;

#endif