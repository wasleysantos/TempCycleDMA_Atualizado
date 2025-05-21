#include <stdio.h>
#include <string.h>
#include "ssd1306.h"
#include "display_utils.h"
#include "tarefa2_display.h"
#include "tarefa3_tendencia.h"

extern uint8_t ssd[];
extern struct render_area area;

void tarefa2_exibir_oled(float temperatura, tendencia_t tendencia) {
    ssd1306_clear_display(ssd);

    char* linha1 = "Temperatura";
    char* linha2 = "Media";
    char linha3[30];

    snprintf(linha3, sizeof(linha3), "TEMP: %s", tendencia_para_texto(tendencia));

    int x1 = (128 - strlen(linha1) * 6) / 2;
    int x2 = (128 - strlen(linha2) * 6) / 2;
    int x3 = (128 - strlen(linha3) * 6) / 2;

    ssd1306_draw_string(ssd, x1, 0, linha1);
    ssd1306_draw_string(ssd, x2, 16, linha2);
    mostrar_valor_grande(ssd, temperatura, 32);
    ssd1306_draw_string(ssd, 0, 56, linha3);

    render_on_display(ssd, &area);  // Chamado apenas uma vez no fim
}
