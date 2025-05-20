#include <stdio.h>  // Biblioteca padrão para entrada e saída
#include <string.h> // Biblioteca manipular strings
#include <stdlib.h> // funções para realizar várias operações, incluindo alocação de memória dinâmica 
#include "pico/stdlib.h"    // Biblioteca da Raspberry Pi Pico para funções padrão 
#include "hardware/adc.h"    // Biblioteca da Raspberry Pi Pico para manipulação do ADC
#include "hardware/timer.h" // Biblioteca da Raspberry Pi Pico para manipulação de temporizadores
#include "pico/cyw43_arch.h"    // Biblioteca para arquitetura Wi-Fi da Pico com CYW43  
#include "lwip/pbuf.h"  // Lightweight IP stack - manipulação de buffers de pacotes de rede
#include "lwip/tcp.h"   // Lightweight IP stack - fornece funções e estruturas para trabalhar com o protocolo TCP
#include "lwip/netif.h" // Lightweight IP stack - fornece funções e estruturas para trabalhar com interfaces de rede (netif)

/* BIBLIOTECAS LOCALS */
#include "libs/ssd1306.h" // Biblioteca para controle do display OLED SSD1306
#include "libs/definicoes.h" // Definições de pinos e estruturas
#include "libs/funcoes.h" // Funções auxiliares para controle de LEDs e leitura de ADC

// Credenciais WIFI - Altere para sua rede
#define WIFI_SSID "SUA REDE WIFI"
#define WIFI_PASSWORD "SUA SENHA"

// Definição das variáveis
static volatile uint32_t last_time = 0; // Variável para armazenar o último tempo em microssegundos
static volatile bool alert_general = false; // Alerta geral
static volatile bool alert_1, alert_2 = false; // Alerta 1 e Alerta 2
bool controle_automatico = true; // Controle automático (fotorresistor) das luzes
bool luz_manual = false; // Acionamento manual das luzes
uint16_t luminosity_value = 0; // Variável para armazenar o valor de luminosidade
ssd1306_t ssd; // Struct para o display OLED


// Definição dos pinos GPIO para acionamento dos LEDs e botões da BitDogLab
PIN_GPIO gpio_bitdog[4] = {
    {LED_BLUE_PIN, GPIO_OUT},
    {LED_RED_PIN, GPIO_OUT},
    {BUTTON_A, GPIO_IN},
    {BUTTON_B, GPIO_IN}
};

// DECLRAÇÃO DAS FUNÇÕES 

// Inicializa os Pinos GPIO para acionamento dos LEDs e botões da BitDogLab
void init_gpio_bitdog(); 

// Função de callback ao aceitar conexões TCP
static err_t tcp_server_accept(void *arg, struct tcp_pcb *newpcb, err_t err); 

// Função de callback para processar requisições HTTP
static err_t tcp_server_recv(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err); 

// Tratamento do request do usuário
void user_request(char **request); 


// Função de interrupção para os botões
void gpio_irq_handler(uint gpio, uint32_t events); 

// Função principal
int main()
{
    //Inicializa todos os tipos de bibliotecas stdio padrão presentes que estão ligados ao binário.
    stdio_init_all();

    // Inicializa os Pinos GPIO para acionamento dos LEDs da BitDogLab
    init_gpio_bitdog(); 

    // Inicializa o display OLED
    init_display(&ssd); 

    pwm_init_buzzer(BUZZER_A); // Inicializa o buzzer

    //Inicializa a arquitetura do cyw43
    while (cyw43_arch_init())
    {
        printf("Falha ao inicializar Wi-Fi\n");
        sleep_ms(100);
        return -1;
    }

    // Ativa o Wi-Fi no modo Station, de modo a que possam ser feitas ligações a outros pontos de acesso Wi-Fi.
    cyw43_arch_enable_sta_mode();

    // Conectar à rede WiFI - fazer um loop até que esteja conectado
    printf("Conectando ao Wi-Fi...\n");
    ssd1306_fill(&ssd, false);
    ssd1306_draw_string(&ssd, "Conectando ao  Wi-Fi...", 0, 1);
    ssd1306_send_data(&ssd);

    while (cyw43_arch_wifi_connect_timeout_ms(WIFI_SSID, WIFI_PASSWORD, CYW43_AUTH_WPA2_AES_PSK, 20000))
    {
        printf("Falha ao conectar ao Wi-Fi\n");
        sleep_ms(100);
        return -1;
    }
    printf("Conectado ao Wi-Fi\n");
  

    // Caso seja a interface de rede padrão - imprimir o IP do dispositivo.
    if (netif_default)
    {
        printf("IP do dispositivo: %s\n", ipaddr_ntoa(&netif_default->ip_addr));
    }

    // Configura o servidor TCP - cria novos PCBs TCP. É o primeiro passo para estabelecer uma conexão TCP.
    struct tcp_pcb *server = tcp_new();
    if (!server)
    {
        printf("Falha ao criar servidor TCP\n");
        return -1;
    }

    //vincula um PCB (Protocol Control Block) TCP a um endereço IP e porta específicos.
    if (tcp_bind(server, IP_ADDR_ANY, 80) != ERR_OK)
    {
        printf("Falha ao associar servidor TCP à porta 80\n");
        return -1;
    }

    // Coloca um PCB (Protocol Control Block) TCP em modo de escuta, permitindo que ele aceite conexões de entrada.
    server = tcp_listen(server);

    // Define uma função de callback para aceitar conexões TCP de entrada. É um passo importante na configuração de servidores TCP.
    tcp_accept(server, tcp_server_accept);
    printf("Servidor ouvindo na porta 80\n");

    adc_init(); // Inicializa o conversor ADC
    adc_gpio_init(ADC_GPIO); // Eixo X do Joystick

    // Habilita interrupções para os botões A e B
    gpio_set_irq_enabled_with_callback(BUTTON_A, GPIO_IRQ_EDGE_FALL, true, &gpio_irq_handler);
    gpio_set_irq_enabled_with_callback(BUTTON_B, GPIO_IRQ_EDGE_FALL, true, &gpio_irq_handler);

    while (true)
    {
        luminosity_value = verify_luminosity();
        // Se o controle automático estiver ativado, controla as luzes com base na luminosidade
        // Caso contrário, as luzes são controladas manualmente
        if (controle_automatico) {
            control_lights(luminosity_value);
        } else {
            gpio_put(LED_BLUE_PIN, luz_manual); 
        }

        char luminosity_convert[6];
        sprintf(luminosity_convert, "%d %s", luminosity_value, "%");


        ssd1306_fill(&ssd, false);
        ssd1306_rect(&ssd,0,0, WIDTH, HEIGHT, true, false); // Desenha o contorno do display
        ssd1306_draw_string(&ssd, "Luz: ", 4, 3);
        ssd1306_draw_string(&ssd, luminosity_convert, 44, 3);

        ssd1306_draw_string(&ssd, "Alerta 1:", 4, 14);
        ssd1306_draw_string(&ssd, alert_1 ? "ON" : "OFF", 84, 17);

        ssd1306_draw_string(&ssd, "Alerta 2:", 4, 27);
        ssd1306_draw_string(&ssd, alert_2 ? "ON" : "OFF", 84, 29);

        ssd1306_send_data(&ssd);

        alert_lights(alert_general);
        cyw43_arch_poll(); // Necessário para manter o Wi-Fi ativo
        sleep_ms(1000);      // Reduz o uso da CPU
    }

    //Desligar a arquitetura CYW43.
    cyw43_arch_deinit();
    return 0;
}

// -------------------------------------- Funções ---------------------------------


// Função de callback ao aceitar conexões TCP
static err_t tcp_server_accept(void *arg, struct tcp_pcb *newpcb, err_t err)
{
    tcp_recv(newpcb, tcp_server_recv);
    return ERR_OK;
}

// Tratamento do request do usuário - digite aqui
void user_request(char **request){
 if (strstr(*request, "GET /toggle_auto") != NULL) {
        controle_automatico = !controle_automatico;
    } else if (strstr(*request, "GET /toggle_light") != NULL) {
        luz_manual = !luz_manual;
    }
    else if (strstr(*request, "GET /toggle_alarm") != NULL) {
        alert_1 = false;
        alert_2 = false;
        alert_general = false;
    }
}


// Função de callback para processar requisições HTTP
static err_t tcp_server_recv(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err)
{
    if (!p)
    {
        tcp_close(tpcb);
        tcp_recv(tpcb, NULL);
        return ERR_OK;
    }

    // Alocação do request na memória dinámica
    char *request = (char *)malloc(p->len + 1);
    memcpy(request, p->payload, p->len);
    request[p->len] = '\0';

    printf("Request: %s\n", request);

    // Tratamento de request - Controle dos LEDs
    user_request(&request);
    


// Cria a resposta HTML
char html[4000];
const char *bg_color = (alert_general ? "#ed1c05" : "#f0f8ff"); // vermelho claro se alarme ativo

snprintf(html, sizeof(html),
"HTTP/1.1 200 OK\r\n"
"Content-Type: text/html\r\n"
"\r\n"
"<!DOCTYPE html>\n"
"<html>\n"
"<head>\n"
"<meta charset=\"UTF-8\">\n"
"<title>Residência Inteligente</title>\n"
"<script defer>\n"
"setInterval(function(){location.href='/';},2000);\n"
"</script>\n"
"<style>\n"
"body {\n"
"    font-family: Arial, sans-serif;\n"
"    text-align: center;\n"
"    background-color: %s;\n"
"    margin: 0;\n"
"    padding: 0;\n"
"}\n"
"h1 {\n"
"    color: #006400;\n"
"    margin-top: 20px;\n"
"}\n"
".container {\n"
"    max-width: 600px;\n"
"    margin: auto;\n"
"    background: #ffffffcc;\n"
"    padding: 20px;\n"
"    border-radius: 15px;\n"
"    box-shadow: 0 0 15px rgba(0,0,0,0.2);\n"
"}\n"
".status {\n"
"    font-size: 20px;\n"
"    margin: 15px 0;\n"
"}\n"
".luminosity {\n"
"    font-size: 40px;\n"
"    color: %s;\n"
"}\n"
".button-group {\n"
"    margin-top: 20px;\n"
"}\n"
"form {\n"
"    display: inline-block;\n"
"    margin: 10px;\n"
"}\n"
"button {\n"
"    padding: 12px 24px;\n"
"    font-size: 16px;\n"
"    border: none;\n"
"    border-radius: 8px;\n"
"    cursor: pointer;\n"
"    box-shadow: 2px 2px 6px rgba(0,0,0,0.2);\n"
"}\n"
"button:hover {\n"
"    opacity: 0.9;\n"
"}\n"
".update-btn { background-color: #4682B4; color: white; }\n"
".auto-btn { background-color: #20B2AA; color: white; }\n"
".light-btn { background-color: #FFD700; color: black; }\n"
"</style>\n"
"</head>\n"
"<body>\n"
"<div class=\"container\">\n"
"<h1>Residência Inteligente</h1>\n"
"<div class=\"luminosity\">Luminosidade: %d%%</div>\n"
"<div class=\"status\">Modo: %s</div>\n"
"<div class=\"status\">Luzes do jardim: %s</div>\n"
"<div class=\"status\">Alarme 1: <span style=\"color:%s; font-weight:bold;\">%s</span></div>\n"
"<div class=\"status\">Alarme 2: <span style=\"color:%s; font-weight:bold;\">%s</span></div>\n"
"<div class=\"button-group\">\n"
"<form action=\"/toggle_auto\"><button class=\"auto-btn\">%s modo automático</button></form>\n"
"<form action=\"/toggle_alarm\"><button class=\"auto-btn\">Desativar alarmes</button></form>\n"
"<form action=\"/toggle_light\"><button class=\"light-btn\">%s luz manual</button></form>\n"
"</div>\n"
"</div>\n"
"</body>\n"
"</html>",
bg_color,
(luminosity_value < 10 ? "red" : "green"),
luminosity_value,
(controle_automatico ? "Automático" : "Manual"),
(controle_automatico ? (luminosity_value < 10 ? "LIGADAS" : "DESLIGADAS") : (luz_manual ? "LIGADAS" : "DESLIGADAS")),
(alert_1 ? "red" : "green"), (alert_1 ? "ATIVADO" : "DESATIVADO"),
(alert_2 ? "red" : "green"), (alert_2 ? "ATIVADO" : "DESATIVADO"),
(controle_automatico ? "Desativar" : "Ativar"),
(luz_manual ? "Desligar" : "Ligar")
);

    
    // Escreve dados para envio (mas não os envia imediatamente).
    tcp_write(tpcb, html, strlen(html), TCP_WRITE_FLAG_COPY);

    // Envia a mensagem
    tcp_output(tpcb);

    //libera memória alocada dinamicamente
    free(request);
    
    //libera um buffer de pacote (pbuf) que foi alocado anteriormente
    pbuf_free(p);

    return ERR_OK;
}

void gpio_irq_handler(uint gpio, uint32_t events)
{
    uint32_t current_time = to_us_since_boot(get_absolute_time());
    
    if (current_time - last_time > 200000) {
        last_time = current_time;
        
        if (gpio == BUTTON_A) {
            alert_1 = !alert_1;
        } else if (gpio == BUTTON_B) {
            alert_2 = !alert_2;
        }

        // Alerta geral é ativado se qualquer um estiver ativo
        alert_general = alert_1 || alert_2;
    }
}

void init_gpio_bitdog(){
    for(int i = 0; i< 4; i++){
        gpio_init(gpio_bitdog[i].pin_gpio);
        gpio_set_dir(gpio_bitdog[i].pin_gpio, gpio_bitdog[i].pin_dir);

        if(gpio_bitdog[i].pin_dir == GPIO_IN){
            gpio_pull_up(gpio_bitdog[i].pin_gpio);
        }
        else{
            gpio_put(gpio_bitdog[i].pin_gpio, false);
        }
    }
}