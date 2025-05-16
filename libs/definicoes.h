#ifndef DEFINICOES_H
#define DEFINICOES_H

#define LED_BLUE_PIN 12                 // GPIO12 - LED azul
#define LED_RED_PIN 13                  // GPIO13 - LED vermelho
#define I2C_PORT i2c1           
#define I2C_SDA 14                      
#define I2C_SCL 15                      
#define ADRESS 0x3c
#define BUTTON_A 5
#define BUTTON_B 6
#define ADC_GPIO 27 // 27 para joystick e 28 para LDR
#define ADC_CHANNEL 1 // Canal ADC (1 para eixo X, 2 para GPIO28 LDR)
#define MAX_ADC_VALUE 4095 // Valor máximo do ADC  | 4095 para o joystick e 3700 para o LDR (O valor medido utilizando LDR pode sofrer variações dependendo da resistência utilizada)

typedef struct
{
    uint pin_gpio; // Pino GPIO do LED/botão
    uint pin_dir;  // Direção do pino (0 = entrada, 1 = saída)
} PIN_GPIO;

#endif