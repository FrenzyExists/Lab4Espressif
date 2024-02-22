#include <stdio.h>
#include "math.h"
#include "string.h"
#include "stdbool.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/ledc.h"
#include "driver/gpio.h"
#include <freertos/semphr.h>

#define BUZZER_PIN      GPIO_NUM_15
#define BUZZER_CHANNEL  LEDC_CHANNEL_0
#define BUZZER_RESOLUTION   LEDC_TIMER_10_BIT

#define SOFT_BUTTON_PIN GPIO_NUM_22

#define A GPIO_NUM_27
#define B GPIO_NUM_25
#define C GPIO_NUM_33
#define D GPIO_NUM_32
#define E GPIO_NUM_18
#define F GPIO_NUM_19
#define G GPIO_NUM_5

#define S7_1 GPIO_NUM_21
#define S7_2 GPIO_NUM_4

// Define the frequencies for the buzzer
int buzzer_frequencies[] = {500, 1000, 1500, 2000, 3000, 9500};
int num_frequencies = 6;
int i = 0;
volatile int myCount = 0;

const int digitalBinary[16] = {
   // abcdefg
    0b0000001,  // 0
    0b1001111,  // 1
    0b0010010,  // 2
    0b0000110,  // 3
    0b1001100,  // 4
    0b0100100,  // 5
    0b0100000,  // 6
    0b0001111,  // 7
    0b0000000,  // 8
    0b0001100,  // 9
    0b0001000,  // A
    0b1100000,  // B
    0b0110000,  // C
    0b1000010,  // D
    0b0111000   // F
};

SemaphoreHandle_t buttonSemaphore;

void setup_btn() {
    gpio_set_direction(SOFT_BUTTON_PIN, GPIO_MODE_INPUT);
    gpio_set_intr_type(SOFT_BUTTON_PIN, GPIO_INTR_NEGEDGE); // Trigger interrupt on falling edge
}

void setup_seven_segment() {
    gpio_set_direction(A,   GPIO_MODE_OUTPUT);
    gpio_set_direction(B,   GPIO_MODE_OUTPUT);
    gpio_set_direction(C,   GPIO_MODE_OUTPUT);
    gpio_set_direction(D,   GPIO_MODE_OUTPUT);
    gpio_set_direction(E,   GPIO_MODE_OUTPUT);
    gpio_set_direction(F,   GPIO_MODE_OUTPUT);
    gpio_set_direction(G,   GPIO_MODE_OUTPUT);

    gpio_set_direction(S7_1,   GPIO_MODE_OUTPUT);
    gpio_set_direction(S7_2,   GPIO_MODE_OUTPUT);
}

void setup_ledc() {
    ledc_timer_config_t ledc_timer = {
        .speed_mode = LEDC_HIGH_SPEED_MODE,
        .duty_resolution = BUZZER_RESOLUTION,
        .timer_num = LEDC_TIMER_0,
        .freq_hz = 2000  // Initial frequency
    };
    ledc_timer_config(&ledc_timer);

    ledc_channel_config_t ledc_channel = {
        .gpio_num = BUZZER_PIN,
        .speed_mode = LEDC_HIGH_SPEED_MODE,
        .channel = BUZZER_CHANNEL,
        .intr_type = LEDC_INTR_DISABLE,
        .timer_sel = LEDC_TIMER_0,
        .duty = 0
    };
    ledc_channel_config(&ledc_channel);
}

void IRAM_ATTR triggerBuzz(void *args) {
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    xSemaphoreGiveFromISR(buttonSemaphore, &xHigherPriorityTaskWoken);

    if (xHigherPriorityTaskWoken) {
        portYIELD_FROM_ISR();
    }
}

void buzzer_on(int frequency) {
    ledc_set_freq(LEDC_HIGH_SPEED_MODE, LEDC_TIMER_0, frequency);
    ledc_set_duty(LEDC_HIGH_SPEED_MODE, BUZZER_CHANNEL, 500); // 50% duty cycle
    ledc_update_duty(LEDC_HIGH_SPEED_MODE, BUZZER_CHANNEL);
}

void buzzer_off() {
    ledc_set_duty(LEDC_HIGH_SPEED_MODE, BUZZER_CHANNEL, 0);
    ledc_update_duty(LEDC_HIGH_SPEED_MODE, BUZZER_CHANNEL);
}

void buzzerTask(void *pvParams) {
    while (1) {
        if (xSemaphoreTake(buttonSemaphore, portMAX_DELAY) == pdTRUE) {
            i = (i + 1) % num_frequencies;  // Move to the next frequency
        }
        buzzer_on(buzzer_frequencies[i]);
        vTaskDelay(15.51 / portTICK_PERIOD_MS);
    }
}

void countTask(void *pvParameters) {
    int pinNumber;
    for(;;) {
        myCount = (myCount + 1) % 100;
        printf("My Count Count: %d\n", myCount);
        vTaskDelay(1000 / portTICK_PERIOD_MS); // Wait 1 second
    }
}

void sevenSegmentTask(void *pvParameters) {

    for (;;) {
        gpio_set_level(A, 1);
        gpio_set_level(B, 1);
        gpio_set_level(C, 1);
        gpio_set_level(D, 1);
        gpio_set_level(E, 1);
        gpio_set_level(F, 1);
        gpio_set_level(G, 1);

        gpio_set_level(S7_1, 0);
        gpio_set_level(S7_2, 1);

        gpio_set_level(A, (digitalBinary[(int)floor(myCount / 16)] >> 6) & 1);
        gpio_set_level(B, (digitalBinary[(int)floor(myCount / 16)] >> 5) & 1);
        gpio_set_level(C, (digitalBinary[(int)floor(myCount / 16)] >> 4) & 1);
        gpio_set_level(D, (digitalBinary[(int)floor(myCount / 16)] >> 3) & 1);
        gpio_set_level(E, (digitalBinary[(int)floor(myCount / 16)] >> 2) & 1);
        gpio_set_level(F, (digitalBinary[(int)floor(myCount / 16)] >> 1) & 1);
        gpio_set_level(G, (digitalBinary[(int)floor(myCount / 16)] >> 0) & 1);

        vTaskDelay(1 / portTICK_PERIOD_MS); // Delay for 1 second

        gpio_set_level(A, 1);
        gpio_set_level(B, 1);
        gpio_set_level(C, 1);
        gpio_set_level(D, 1);
        gpio_set_level(E, 1);
        gpio_set_level(F, 1);
        gpio_set_level(G, 1);

        gpio_set_level(S7_1, 1);
        gpio_set_level(S7_2, 0);

        gpio_set_level(A, (digitalBinary[myCount % 16] >> 6) & 1);
        gpio_set_level(B, (digitalBinary[myCount % 16] >> 5) & 1);
        gpio_set_level(C, (digitalBinary[myCount % 16] >> 4) & 1);
        gpio_set_level(D, (digitalBinary[myCount % 16] >> 3) & 1);
        gpio_set_level(E, (digitalBinary[myCount % 16] >> 2) & 1);
        gpio_set_level(F, (digitalBinary[myCount % 16] >> 1) & 1);
        gpio_set_level(G, (digitalBinary[myCount % 16] >> 0) & 1);

        vTaskDelay(1/portTICK_PERIOD_MS); // Delay for 1 second
    }
}

void app_main() {
    setup_btn();
    setup_ledc();
    setup_seven_segment();

    buttonSemaphore = xSemaphoreCreateBinary();

    gpio_install_isr_service(0);
    gpio_isr_handler_add(SOFT_BUTTON_PIN, triggerBuzz, (void *)1);

    xTaskCreatePinnedToCore(&buzzerTask,        "BUZZER",         0x8FFUL, NULL, 0, NULL, tskNO_AFFINITY); 
    xTaskCreatePinnedToCore(&sevenSegmentTask, "SEVEN_SEGMENT", 0xFFFFUL, NULL, 0, NULL, tskNO_AFFINITY); 
    xTaskCreatePinnedToCore(&countTask,        "COUNT",         0x8FFUL, NULL, 0, NULL, tskNO_AFFINITY); 
}
