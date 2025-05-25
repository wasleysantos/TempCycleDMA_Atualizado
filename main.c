/**
 * ------------------------------------------------------------
 *  Arquivo: main.c
 *  Projeto: TempCycleDMA
 * ------------------------------------------------------------
 *  Descrição:
 *      Ciclo principal do sistema embarcado, baseado em um
 *      executor cíclico com tarefas com timer:
 *
 *      Tarefa 1 - Leitura da temperatura via DMA (manual)
 *      Tarefa 2 - Análise da tendência da temperatura (callback)
 *      Tarefa 3 - Exibição da temperatura e tendência no OLED
 *      Tarefa 4 - Cor da matriz NeoPixel por tendência
 *      Tarefa 5 - Alerta intermitente caso temperatura < 1°C
 *      Tarefa 6 - Impressão de status no terminal USB
 *
 *  Atualização:
 *      Tarefas sincronizadas com base na Tarefa 1,
 *      utilizando add_repeating_timer_ms e repeating_timer_callback.
 *
 *  Data: 25/05/2025
 * ------------------------------------------------------------
 */

#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/watchdog.h"
#include "hardware/timer.h"

#include "setup.h"
#include "tarefa1_temp.h"
#include "tarefa2_display.h"
#include "tarefa3_tendencia.h"
#include "tarefa4_controla_neopixel.h"
#include "neopixel_driver.h"
#include "testes_cores.h"
#include "pico/stdio_usb.h"

// Declaração das funções de tarefa
void tarefa_1();                                     // Tarefa 1: leitura de temperatura via DMA
bool tarefa_2_callback(repeating_timer_t *rt);       // Tarefa 2: tendência
bool tarefa_3_callback(repeating_timer_t *rt);       // Tarefa 3: exibição OLED
bool tarefa_4_callback(repeating_timer_t *rt);       // Tarefa 4: cor NeoPixel
bool tarefa_5_callback(repeating_timer_t *rt);       // Tarefa 5: alerta intermitente
bool tarefa_6_serial_monitor(repeating_timer_t *rt); // Tarefa 6: impressão no terminal USB
void agendar_tarefas_dependentes(void);              // Agenda tarefas 2-6 após T1

// Variáveis globais compartilhadas
float media;               // Média da temperatura lida
tendencia_t t;             // Tendência da temperatura (subida, queda ou estável)
bool alerta_estado = false; // Estado atual do alerta no NeoPixel (ligado/desligado)

// Marcação de tempo de execução de cada tarefa (para análise de desempenho)
absolute_time_t ini_tarefa1, fim_tarefa1;
absolute_time_t ini_tarefa2, fim_tarefa2;
absolute_time_t ini_tarefa3, fim_tarefa3;
absolute_time_t ini_tarefa4, fim_tarefa4;

// Timers das tarefas (usados com os callbacks)
static repeating_timer_t timer_tarefa2;
static repeating_timer_t timer_tarefa3;
static repeating_timer_t timer_tarefa4;
static repeating_timer_t timer_tarefa5;
static repeating_timer_t timer_tarefa6;

/*******************************/
int main() {
    setup();  // Inicializações de periféricos, sensores, interfaces

    while (true) {
        //watchdog_update();  // Alimenta o watchdog (se ativado)

        tarefa_1();                    // Tarefa 1: lê a temperatura via DMA e calcula média
        agendar_tarefas_dependentes(); // Reagenda as tarefas 2-6 após leitura de temperatura
    }

    return 0;
}

/*******************************/
// Executa a leitura da temperatura usando DMA e salva a média
void tarefa_1() {
    ini_tarefa1 = get_absolute_time(); // Marca o tempo de início
    media = tarefa1_obter_media_temp(&cfg_temp, DMA_TEMP_CHANNEL); // Lê e calcula a média
    fim_tarefa1 = get_absolute_time(); // Marca o tempo de fim
}

/*******************************/
// Cancela timers antigos e agenda novas execuções das tarefas dependentes
void agendar_tarefas_dependentes() {
    // Cancela os timers antigos (para evitar múltiplas execuções)
    cancel_repeating_timer(&timer_tarefa2);
    cancel_repeating_timer(&timer_tarefa3);
    cancel_repeating_timer(&timer_tarefa4);
    cancel_repeating_timer(&timer_tarefa5);
    cancel_repeating_timer(&timer_tarefa6);  

    // Agenda os callbacks para executarem 500ms após a tarefa 1
    add_repeating_timer_ms(500, tarefa_2_callback, NULL, &timer_tarefa2);
    add_repeating_timer_ms(500, tarefa_3_callback, NULL, &timer_tarefa3);
    add_repeating_timer_ms(500, tarefa_4_callback, NULL, &timer_tarefa4);
    add_repeating_timer_ms(500, tarefa_5_callback, NULL, &timer_tarefa5);
    add_repeating_timer_ms(500, tarefa_6_serial_monitor, NULL, &timer_tarefa6); // Monitor serial
}

/*******************************/
// Analisa a tendência de variação da temperatura
bool tarefa_2_callback(repeating_timer_t *rt) {
    ini_tarefa2 = get_absolute_time();              // Marca início da tarefa
    t = tarefa3_analisa_tendencia(media);           // Calcula tendência (subindo, caindo etc.)
    fim_tarefa2 = get_absolute_time();              // Marca fim da tarefa
    return false; // Timer não deve repetir (executa uma vez por ciclo)
}

/*******************************/
// Exibe a temperatura e a tendência no display OLED
bool tarefa_3_callback(repeating_timer_t *rt) {
    ini_tarefa3 = get_absolute_time();              // Início
    tarefa2_exibir_oled(media, t);                  // Mostra valores e tendência no OLED
    fim_tarefa3 = get_absolute_time();              // Fim
    return false;
}

/*******************************/
// Atualiza a cor da matriz NeoPixel de acordo com a tendência
bool tarefa_4_callback(repeating_timer_t *rt) {
    ini_tarefa4 = get_absolute_time();              // Início
    tarefa4_matriz_cor_por_tendencia(t);            // Atualiza cor conforme tendência
    fim_tarefa4 = get_absolute_time();              // Fim
    return false;
}

/*******************************/
// Pisca os LEDs em branco caso a temperatura seja inferior a 1°C
bool tarefa_5_callback(repeating_timer_t *rt) {
    if (media < 1.0f) { // Temperatura crítica
        if (!alerta_estado) {
            npSetAll(COR_BRANCA); // Acende todos os LEDs
            npWrite();
            alerta_estado = true;
        } else {
            npClear();            // Desliga todos os LEDs
            npWrite();
            alerta_estado = false;
        }
    } else {
        if (alerta_estado) {
            npClear();            // Garante que os LEDs sejam desligados se já estavam piscando
            npWrite();
            alerta_estado = false;
        }
    }
    return false;
}

/*******************************/
// Imprime as leituras e tempos de execução no terminal serial USB
bool tarefa_6_serial_monitor(repeating_timer_t *rt) {
    printf("Temperatura: %.2f °C | T1: %.3fs | T2: %.3fs | T3: %.3fs | T4: %.3fs | Tendência: %s\n",
           media,
           absolute_time_diff_us(ini_tarefa1, fim_tarefa1) / 1e6, // Tempo em segundos
           absolute_time_diff_us(ini_tarefa2, fim_tarefa2) / 1e6,
           absolute_time_diff_us(ini_tarefa3, fim_tarefa3) / 1e6,
           absolute_time_diff_us(ini_tarefa4, fim_tarefa4) / 1e6,
           tendencia_para_texto(t)); // Mostra a tendência em texto
    return false;
}
