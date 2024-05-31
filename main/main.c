#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "driver/adc.h"
#include "esp_log.h"
#include "esp_http_client.h"

#define NUM_ELECTRODES 16


// 16個の電極
const gpio_num_t electrodePins[NUM_ELECTRODES] = {
    GPIO_NUM_2, GPIO_NUM_4, GPIO_NUM_5, GPIO_NUM_12,
    GPIO_NUM_13, GPIO_NUM_14, GPIO_NUM_15, GPIO_NUM_16,
    GPIO_NUM_17, GPIO_NUM_18, GPIO_NUM_19, GPIO_NUM_21,
    GPIO_NUM_22, GPIO_NUM_23, GPIO_NUM_25, GPIO_NUM_26
};

#define ADC_CHANNEL ADC1_CHANNEL_0

void error_handler(esp_err_t err) {
    if (err == ESP_OK) {
        ESP_LOGI("Data sent successfully");
    } else {
        ESP_LOGE("Failed to send data: %s", esp_err_to_name(err));
    }
}

void send_data(const char* data) {
    esp_http_client_config_t config = {
    .url = "",
    };  // set server url
    esp_http_client_handle_t client = esp_http_client_init(&config); // initialize http client

    esp_http_client_set_method(client, HTTP_METHOD_POST); // set method to POST
    esp_http_client_set_post_field(client, data, strlen(data)); // set body
    esp_http_client_set_header(client, "Context-Type", "text/csv"); // set header

    esp_err_t err = esp_http_client_perform(client); // send data

    error_handler(err);    // error handling

    esp_http_client_cleanup(client); // clean up after sending data
}

void app_main() {
    for(int i = 0; i < NUM_ELECTRODES; i++) {
        gpio_config(electrodePins[i]); // set gpio config
        esp_err_t err = gpio_set_direction(electrodePins[i], GPIO_MODE_OUTPUT); // set gpio direction
        error_handler(err); // error handling
        gpio_set_level(electrodePins[i], 0); // don't flow at first
    }

    adc1_config_width(ADC_WIDTH_BIT_12); // set adc width, this is for the resolution of the adc
    // set adc attenuation, attenuation means the ratio of the input voltage to the output voltage
    adc1_config_channel_atten(ADC_CHANNEL, ADC_ATTEN_DB_0);

    char data[256]; // data buffer

    while(1) {
        // measure for each electrode pair
        for(int i = 0; i < NUM_ELECTRODES; i++) {
            for(int j = i + i; j < NUM_ELECTRODES; j++) {
                gpio_set_level(electrodePins[i], 1); // set electrode i to 1
                gpio_set_level(electrodePins[j], 0); // set electrode j to 0

                vTaskDelay(100 / portTICK_PERIOD_MS); // delay for 100ms

                // read data from other electrodes
                for(int k = 0; k < NUM_ELECTRODES; k++) {
                    if(k != i && k != j) {
                        // read electrode k
                        int val = adc1_get_raw(ADC_CHANNEL); // get raw data from adc
                        sprintf(data, "%d,%d,%d,%d\n", i, j, k, val); // format data
                        send_data(data); // send data to server
                    }
                }

                gpio_set_level(electrodePins[i], 0); // set electrode i to 0
                gpio_set_level(electrodePins[j], 0); // set electrode j to 0

                vTaskDelay(500 / portTICK_PERIOD_MS); // delay for 500ms
            }
        }
    }
}
