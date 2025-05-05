 #include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/i2c.h"
#include "hardware/pwm.h"
#include "hardware/pio.h"
#include "hardware/clocks.h"
#include "pio_matrix.pio.h"
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

typedef struct {
  double r;
  double g;
  double b;
} Pixel;

Pixel desenho[25] = { 
    {0.0, 0.0, 0.0}, {0.01, 0.01, 0.01}, {0.01, 0.01, 0.01}, {0.01, 0.01, 0.01}, {0.0, 0.0, 0.0},
    {0.0, 0.0, 0.0}, {0.01, 0.01, 0.01}, {0.0, 0.0, 0.0},    {0.01, 0.01, 0.01}, {0.0, 0.0, 0.0},
    {0.0, 0.0, 0.0}, {0.01, 0.01, 0.01}, {0.0, 0.0, 0.0},    {0.01, 0.01, 0.01}, {0.0, 0.0, 0.0},
    {0.0, 0.0, 0.0}, {0.01, 0.01, 0.01}, {0.0, 0.0, 0.0},    {0.01, 0.01, 0.01}, {0.0, 0.0, 0.0},
    {0.0, 0.0, 0.0}, {0.01, 0.01, 0.01}, {0.01, 0.01, 0.01}, {0.01, 0.01, 0.01}, {0.0, 0.0, 0.0}
};

//rotina para definição da intensidade de cores do led
uint32_t matrix_rgb(double b, double r, double g)
{
  unsigned char R, G, B;
  R = r * 255;
  G = g * 255;
  B = b * 255;
  return (G << 24) | (R << 16) | (B << 8);
}

//rotina para acionar a matrix de leds - ws2812b
void desenho_pio(PIO pio, uint sm) {
    uint32_t valor_led;

    for (int16_t i = 0; i < 25; i++) {
        valor_led = matrix_rgb(
        desenho[24-i].b,  // azul
        desenho[24-i].r,  // vermelho
        desenho[24-i].g   // verde
        );
        pio_sm_put_blocking(pio, sm, valor_led);
    }
}

// Modifica a cor de píxels individuais na matriz desenho que representa a matriz de LEDs
void set_pixel_color(int led_index, double r, double g, double b) {
  if (led_index >= 0 && led_index < 25) {
    desenho[led_index].r = r;
    desenho[led_index].g = g;
    desenho[led_index].b = b;
  }
}

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
                vTaskDelay(pdMS_TO_TICKS(1000));

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
                vTaskDelay(pdMS_TO_TICKS(1500));

                activeLed = 1; // Change to the next LED
            }
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

void vDisplayTask() {
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
        //  Atualiza o conteúdo do display com animações
        ssd1306_fill(&ssd, !cor);                          // Limpa o display
        ssd1306_rect(&ssd, 3, 3, 122, 60, cor, !cor);      // Desenha um retângulo
        ssd1306_line(&ssd, 20, 7, 45, 7, cor);           // Desenha uma linha
        ssd1306_line(&ssd, 20, 7, 20, 57, cor);          // Desenha uma linha
        ssd1306_line(&ssd, 20, 57, 45, 57, cor);           // Desenha uma linha
        ssd1306_line(&ssd, 45, 7, 45, 57, cor);           // Desenha uma linha
        vTaskDelay(pdMS_TO_TICKS(250)); // Delay para evitar uso excessivo da CPU

        if(isNocturneModeOn) {
            ssd1306_rect(&ssd, 12, 28, 11, 10, cor, !cor);      // Desenha um retângulo
            ssd1306_rect(&ssd, 27, 28, 11, 10, cor, cor);      // Desenha um retângulo
            ssd1306_rect(&ssd, 42, 28, 11, 10, cor, !cor);  
        } else {
            ssd1306_rect(&ssd, 12, 28, 11, 10, cor, activeLed==1?cor:!cor);      // Desenha um retângulo
            ssd1306_rect(&ssd, 27, 28, 11, 10, cor, activeLed==2?cor:!cor);      // Desenha um retângulo
            ssd1306_rect(&ssd, 42, 28, 11, 10, cor, activeLed==3?cor:!cor);      // Desenha um retângulo
        }

        ssd1306_draw_string(&ssd, activeLed==1?"Verde":activeLed==2?"Amarelo":"Vermelho", 50, 29);    // Desenha uma string
        ssd1306_send_data(&ssd);    // Atualiza o display
        vTaskDelay(1);
    }
}

void vLedMatrixTask() {
    //configurações da PIO
    PIO pio = pio0;
    set_sys_clock_khz(128000, false);
    uint offset = pio_add_program(pio, &pio_matrix_program);
    uint sm = pio_claim_unused_sm(pio, true);
    pio_matrix_program_init(pio, sm, offset, 7);

    while(1) {
        set_pixel_color(7, 0, activeLed==1?0.1:0, 0);
        set_pixel_color(12, activeLed==2?0.1:0, activeLed==2?0.1:0, 0);
        set_pixel_color(17, activeLed==3?0.1:0, 0, 0);
        desenho_pio(pio, sm);
        vTaskDelay(1); // Yield para outras tarefas
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
    stdio_init_all();

    // Para ser utilizado o modo BOOTSEL com botão B
    gpio_init(botaoB);
    gpio_set_dir(botaoB, GPIO_IN);
    gpio_pull_up(botaoB);
    gpio_set_irq_enabled_with_callback(botaoB, GPIO_IRQ_EDGE_FALL, true, &gpio_irq_handler);

    // Botão A usado para alternar entre os modos noturno e diurno
    gpio_init(botaoA);
    gpio_set_dir(botaoA, GPIO_IN);
    gpio_pull_up(botaoA);
    gpio_set_irq_enabled_with_callback(botaoA, GPIO_IRQ_EDGE_FALL, true, &gpio_irq_handler);

    xTaskCreate(vBlinkLedTask, "Blink Led Task", configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY, NULL);
    xTaskCreate(vBuzzerTask, "Buzzer Task", configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY, NULL);
    xTaskCreate(vDisplayTask, "Display Task", configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY+1, NULL);
    xTaskCreate(vLedMatrixTask, "LedMatrix Task", configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY+1, NULL);
    vTaskStartScheduler();
    panic_unsupported();
}
