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
 *
 *  Data: 21/05/2025
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

// Declaração das funções
void tarefa_1();
bool tarefa_2_callback(repeating_timer_t *rt);
bool tarefa_3_callback(repeating_timer_t *rt);
bool tarefa_4_callback(repeating_timer_t *rt);
bool tarefa_5_callback(repeating_timer_t *rt);

// Variáveis globais
float media;
tendencia_t t;
bool alerta_estado = false;

absolute_time_t ini_tarefa1, fim_tarefa1;
absolute_time_t ini_tarefa2, fim_tarefa2;
absolute_time_t ini_tarefa3, fim_tarefa3;
absolute_time_t ini_tarefa4, fim_tarefa4;

int main() {
    setup();  // Inicializações: ADC, DMA, interrupções, OLED, etc.

    // Ativa o watchdog com timeout de 2 segundos
    //watchdog_enable(2000, 1);

    // Inicia os timers das tarefas assíncronas
    static repeating_timer_t timer_tarefa2;
    add_repeating_timer_ms(1000, tarefa_2_callback, NULL, &timer_tarefa2);

     static repeating_timer_t timer_tarefa3;
    add_repeating_timer_ms(1000, tarefa_3_callback, NULL, &timer_tarefa3);

    static repeating_timer_t timer_tarefa4;
    add_repeating_timer_ms(1000, tarefa_4_callback, NULL, &timer_tarefa4);

    static repeating_timer_t timer_tarefa5;
    add_repeating_timer_ms(1000, tarefa_5_callback, NULL, &timer_tarefa5);

    while (true) {
        //watchdog_update();  // Alimenta o watchdog no início do ciclo

        tarefa_1();
       
        // tarefa_2, tarefa_4 e tarefa_5 são chamadas automaticamente

        // --- Cálculo dos tempos de execução ---
        int64_t tempo1_us = absolute_time_diff_us(ini_tarefa1, fim_tarefa1);
        int64_t tempo2_us = absolute_time_diff_us(ini_tarefa2, fim_tarefa2);
        int64_t tempo3_us = absolute_time_diff_us(ini_tarefa3, fim_tarefa3);
        int64_t tempo4_us = absolute_time_diff_us(ini_tarefa4, fim_tarefa4);

        // --- Exibição no terminal ---
        printf("Temperatura: %.2f °C | T1: %.3fs | T2: %.3fs | T3: %.3fs | T4: %.3fs | Tendência: %s\n",
               media,
               tempo1_us / 1e6,
               tempo2_us / 1e6,
               tempo3_us / 1e6,
               tempo4_us / 1e6,
               tendencia_para_texto(t));

        sleep_ms(1000);  // Aguarda próximo ciclo
    }

    return 0;
}

/*******************************/
void tarefa_1() {
    // --- Tarefa 1: Leitura de temperatura via DMA ---
    ini_tarefa1 = get_absolute_time();
    media = tarefa1_obter_media_temp(&cfg_temp, DMA_TEMP_CHANNEL);
    fim_tarefa1 = get_absolute_time();
}

/*******************************/
bool tarefa_2_callback(repeating_timer_t *rt) {
    // --- Tarefa 2: Análise da tendência térmica ---
    ini_tarefa2 = get_absolute_time();
    t = tarefa3_analisa_tendencia(media);
    fim_tarefa2 = get_absolute_time();
    return true;
}

/*******************************/
bool tarefa_3_callback(repeating_timer_t *rt){
    // --- Tarefa 3: Exibição no OLED ---
    ini_tarefa3 = get_absolute_time();
    tarefa2_exibir_oled(media, t);
    fim_tarefa3 = get_absolute_time();
}

/*******************************/
bool tarefa_4_callback(repeating_timer_t *rt) {
    // --- Tarefa 4: Cor da matriz NeoPixel por tendência ---
    ini_tarefa4 = get_absolute_time();
    tarefa4_matriz_cor_por_tendencia(t);
    fim_tarefa4 = get_absolute_time();
    return true;
}

/*******************************/
bool tarefa_5_callback(repeating_timer_t *rt) {
    // --- Tarefa 5: Alerta quando temperatura < 1°C ---
    if (media < 1.0f) {
        if (!alerta_estado) {
            npSetAll(COR_BRANCA);
            npWrite();
            alerta_estado = true;
        } else {
            npClear();
            npWrite();
            alerta_estado = false;
        }
    } else {
        if (alerta_estado) {
            npClear();
            npWrite();
            alerta_estado = false;
        }
    }
    return true;
}
