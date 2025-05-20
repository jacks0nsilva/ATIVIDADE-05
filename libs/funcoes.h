#ifndef FUNCOES_H
#define FUNCOES_H

#include "ssd1306.h" 

void alert_lights(bool alert); // Função para acender o LED vermelho quando o alarme estiver ativo
uint16_t verify_luminosity(); // Função para verificar a luminosidade
void control_lights(uint16_t luminosity_value); // Função para controlar as luzes do jardim com base na luminosidade
void init_display(ssd1306_t *ssd1306); // Inicializa o display OLED

void pwm_init_buzzer(uint pin); // Função para inicializar o buzzer PWM
void beep(uint pin, uint duration); // Função para emitir o sinal PWM (beep) com duração especificada

#endif 