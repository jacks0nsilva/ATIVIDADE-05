#include "funcoes.h"
#include "libs/definicoes.h" // Definições de pinos e estruturas
#include "libs/ssd1306.h" // Biblioteca para controle do display OLED SSD1306
#include "hardware/i2c.h" // Biblioteca da Raspberry Pi Pico para comunicação I2C
#include "hardware/adc.h" // Biblioteca da Raspberry Pi Pico para manipulação do ADC
#include "hardware/pwm.h" // Biblioteca da Raspberry Pi Pico para manipulação de PWM
#include "hardware/clocks.h" // Biblioteca da Raspberry Pi Pico para manipulação de clocks


void alert_lights(bool alert)
{
    if(alert){
        gpio_put(LED_RED_PIN, true);
        beep(BUZZER_A, 500); 
        gpio_put(LED_RED_PIN, false);
    } else{
        gpio_put(LED_RED_PIN, false);
    }
}

uint16_t verify_luminosity(){
    adc_select_input(ADC_CHANNEL); // Seleciona o canal ADC
    uint16_t adc_value = adc_read();
    uint16_t luminosity = (adc_value * 100) / MAX_ADC_VALUE; // Convertendo para porcentagem
    return luminosity;
    sleep_ms(100);
}

void control_lights(uint16_t luminosity_value){
    if(luminosity_value < 10){
        gpio_put(LED_BLUE_PIN, 1);
    } else {
        gpio_put(LED_BLUE_PIN, 0);
    }
}

void init_display(ssd1306_t *ssd1306)
{
    i2c_init(I2C_PORT, 400 * 1000);
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA);
    gpio_pull_up(I2C_SCL);

    ssd1306_init(ssd1306, WIDTH, HEIGHT, false, ADRESS, I2C_PORT);
    ssd1306_config(ssd1306);
    ssd1306_fill(ssd1306, false);

    ssd1306_send_data(ssd1306);
}

// Função para inicializar o buzzer PWM
void pwm_init_buzzer(uint pin) {
    gpio_set_function(pin, GPIO_FUNC_PWM);
    uint slice_num = pwm_gpio_to_slice_num(pin);
    pwm_config config = pwm_get_default_config();
    pwm_config_set_clkdiv(&config, clock_get_hz(clk_sys) / (5000 * 4096));  // Frequência do PWM
    pwm_init(slice_num, &config, true);
    pwm_set_gpio_level(pin, 0);  // Inicializa com o PWM desligado
}

// Função para emitir o sinal PWM (beep) com duração especificada
void beep(uint pin, uint duration_ms) {
    uint slice_num = pwm_gpio_to_slice_num(pin);
    pwm_set_gpio_level(pin, 2048);  // Configura o duty cycle para 50% (ativo)
    sleep_ms(duration_ms);         // Duração do beep
    pwm_set_gpio_level(pin, 0);    // Desativa o PWM
}