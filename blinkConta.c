 #include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/i2c.h"
#include "hardware/pwm.h"
#include "hardware/clocks.h"
#include "lib/ssd1306.h"
#include "lib/font.h"
#include "FreeRTOS.h"
#include "FreeRTOSConfig.h"
#include "task.h"
#include <stdio.h>

#define I2C_PORT i2c1
#define I2C_SDA 14
#define I2C_SCL 15
#define endereco 0x3C

uint8_t activeLed = 1;
bool buzzerActive = true; // Variável para controlar o estado do buzzer
bool isNocturneModeOn = false; // Variável para controlar a alternância entre os modos 
int buzzerFrequency = 2000; // Frequência do buzzer em Hz

void vBlinkLedTask() {
    gpio_init(11);
    gpio_set_dir(11, GPIO_OUT);
    gpio_init(12);
    gpio_set_dir(12, GPIO_OUT);
    gpio_init(13);
    gpio_set_dir(13, GPIO_OUT);

    while(true) {
        if(!isNocturneModeOn) {
            if(activeLed == 1) {
                gpio_put(11, true);
                gpio_put(12, false);
                gpio_put(13, false);

                buzzerActive = true;
                vTaskDelay(pdMS_TO_TICKS(1000));
                buzzerActive = false;

                activeLed = 2; // Change to the next LED
            } else if(activeLed == 2) {
                gpio_put(11, true);
                gpio_put(12, false);
                gpio_put(13, true);

                buzzerActive = true;
                vTaskDelay(pdMS_TO_TICKS(250));
                buzzerActive = false;
                vTaskDelay(pdMS_TO_TICKS(250));
                buzzerActive = true;
                vTaskDelay(pdMS_TO_TICKS(250));
                buzzerActive = false;
                vTaskDelay(pdMS_TO_TICKS(250));
                buzzerActive = true;
                vTaskDelay(pdMS_TO_TICKS(250));
                buzzerActive = false;
                vTaskDelay(pdMS_TO_TICKS(250));

                activeLed = 3; // Change to the next LED
            } else if(activeLed == 3) {
                gpio_put(11, false);
                gpio_put(12, false);
                gpio_put(13, true);

                buzzerActive = true;
                vTaskDelay(pdMS_TO_TICKS(500));
                buzzerActive = false;
                vTaskDelay(pdMS_TO_TICKS(500));

                activeLed = 1; // Change to the next LED
            }

            vTaskDelay(pdMS_TO_TICKS(1000)); // Delay para evitar uso excessivo da CPU
        } else {
            gpio_put(11, true);
            gpio_put(12, false);
            gpio_put(13, true);

            buzzerActive = true;
            vTaskDelay(pdMS_TO_TICKS(250));
            buzzerActive = false;
            vTaskDelay(pdMS_TO_TICKS(2000));
        }
    }
}

void vBuzzerTask() {
    // Configuração ÚNICA (fora do loop)
    gpio_set_function(21, GPIO_FUNC_PWM);
    uint slice_num = pwm_gpio_to_slice_num(21);
    pwm_config config = pwm_get_default_config();
    pwm_config_set_clkdiv(&config, 4.0f);
    pwm_init(slice_num, &config, false); // Inicia desligado
    
    // Pré-calcula wrap (constante para frequência fixa)
    uint32_t wrap = (clock_get_hz(clk_sys) / 4 / buzzerFrequency) - 1;
    pwm_set_wrap(slice_num, wrap);
    pwm_set_chan_level(slice_num, pwm_gpio_to_channel(21), wrap / 2);

    while(true) {
        if(buzzerActive) {
            pwm_set_enabled(slice_num, true); // Apenas liga/desliga
        } else {
            pwm_set_enabled(slice_num, false);
        }
        vTaskDelay(1); // Yield para outras tarefas
    }
}

void vDisplay3Task() {
    // I2C Initialisation. Using it at 400Khz.
    i2c_init(I2C_PORT, 400 * 1000);

    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);                    // Set the GPIO pin function to I2C
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);                    // Set the GPIO pin function to I2C
    gpio_pull_up(I2C_SDA);                                        // Pull up the data line
    gpio_pull_up(I2C_SCL);                                        // Pull up the clock line
    ssd1306_t ssd;                                                // Inicializa a estrutura do display
    ssd1306_init(&ssd, WIDTH, HEIGHT, false, endereco, I2C_PORT); // Inicializa o display
    ssd1306_config(&ssd);                                         // Configura o display
    ssd1306_send_data(&ssd);                                      // Envia os dados para o display
    // Limpa o display. O display inicia com todos os pixels apagados.
    ssd1306_fill(&ssd, false);
    ssd1306_send_data(&ssd);

    char str_y[5]; // Buffer para armazenar a string
    int contador = 0;
    bool cor = true;

    while (true) {
        sprintf(str_y, "%d", contador); // Converte em string
        contador++;                     // Incrementa o contador
        ssd1306_fill(&ssd, !cor);                          // Limpa o display
        ssd1306_rect(&ssd, 3, 3, 122, 60, cor, !cor);      // Desenha um retângulo
        ssd1306_line(&ssd, 3, 25, 123, 25, cor);           // Desenha uma linha
        ssd1306_line(&ssd, 3, 37, 123, 37, cor);           // Desenha uma linha
        ssd1306_draw_string(&ssd, "CEPEDI   TIC37", 8, 6); // Desenha uma string
        ssd1306_draw_string(&ssd, "EMBARCATECH", 20, 16);  // Desenha uma string
        ssd1306_draw_string(&ssd, "  FreeRTOS", 10, 28); // Desenha uma string
        ssd1306_draw_string(&ssd, "Contador  LEDs", 10, 41);    // Desenha uma string
        ssd1306_draw_string(&ssd, str_y, 40, 52);          // Desenha uma string
        ssd1306_send_data(&ssd);                           // Atualiza o display
        sleep_ms(735);
    }
}

// Trecho para modo BOOTSEL com botão B
#include "pico/bootrom.h"
#define botaoA 5
#define botaoB 6
void gpio_irq_handler(uint gpio, uint32_t events) {
    uint32_t currentTime = to_us_since_boot(get_absolute_time());
    static uint32_t lastTime = 0; // Variável estática para armazenar o último tempo de interrupção

    if(currentTime - lastTime > 300000) { // Debounce
        if(gpio == botaoA) {
            isNocturneModeOn = !isNocturneModeOn; // Alterna entre os modos
        } else if(gpio == botaoB) {
            reset_usb_boot(0, 0);
        }
        
        lastTime = currentTime;
    }
}

int main() {
    // Para ser utilizado o modo BOOTSEL com botão B
    gpio_init(botaoB);
    gpio_set_dir(botaoB, GPIO_IN);
    gpio_pull_up(botaoB);
    gpio_set_irq_enabled_with_callback(botaoB, GPIO_IRQ_EDGE_FALL, true, &gpio_irq_handler);

    // Botão A(5) usado para alternar entre os modos noturno e diurno
    gpio_init(botaoA);
    gpio_set_dir(botaoA, GPIO_IN);
    gpio_pull_up(botaoA);
    gpio_set_irq_enabled_with_callback(botaoA, GPIO_IRQ_EDGE_FALL, true, &gpio_irq_handler);

    stdio_init_all();

    xTaskCreate(vBlinkLedTask, "Blink Led Task", configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY, NULL);
    xTaskCreate(vBuzzerTask, "Buzzer Task", configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY, NULL);
    xTaskCreate(vDisplay3Task, "Cont Task Disp3", configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY, NULL);
    vTaskStartScheduler();
    panic_unsupported();
}
