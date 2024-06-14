#include <stdio.h>

#include "esp_log_level.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "driver/adc.h"
#include "esp_log.h"
#include "esp_http_client.h"
#include "nvs_flash.h"
#include "esp_wifi.h"

#define NUM_ELECTRODES 16

#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT1

static EventGroupHandle_t s_wifi_event_group;
static int s_retry_num = 0;

const char* WIFI_SSID = "";
const char* WIFI_PASS = "";

// 16個の電極
const gpio_num_t electrodePins[NUM_ELECTRODES] = {
    GPIO_NUM_2, GPIO_NUM_4, GPIO_NUM_5, GPIO_NUM_12,
    GPIO_NUM_13, GPIO_NUM_14, GPIO_NUM_15, GPIO_NUM_16,
    GPIO_NUM_17, GPIO_NUM_18, GPIO_NUM_19, GPIO_NUM_21,
    GPIO_NUM_22, GPIO_NUM_23, GPIO_NUM_25, GPIO_NUM_26
};

#define ADC_CHANNEL 4

void error_handler(esp_err_t err) {
    if (err == ESP_OK) {
        ESP_LOGI("ERT", "Data sent successfully");
    } else {
        ESP_LOGE("ERT", "Error: %s", esp_err_to_name(err));
    }
}

void send_data(const char* data) {
    esp_http_client_config_t config = {
        .url = "http://your_server_url.com", // サーバーURLを設定
    };
    esp_http_client_handle_t client = esp_http_client_init(&config); // HTTPクライアントの初期化

    esp_http_client_set_method(client, HTTP_METHOD_POST); // POSTメソッドを設定
    esp_http_client_set_post_field(client, data, strlen(data)); // ボディを設定
    esp_http_client_set_header(client, "Content-Type", "text/csv"); // ヘッダーを設定

    esp_err_t err = esp_http_client_perform(client); // データ送信

    error_handler(err); // エラーハンドリング

    esp_http_client_cleanup(client); // クライアントのクリーンアップ
}

static void event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data) {
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        if (s_retry_num < 5) {
            esp_wifi_connect();
            s_retry_num++;
            ESP_LOGI("ERT", "retry to connect to the AP");
        } else {
            xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
        }
        ESP_LOGI("ERT","connect to the AP fail");
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI("ERT", "got ip:" IPSTR, IP2STR(&event->ip_info.ip));
        s_retry_num = 0;
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
    }
}

void wifi_init(void) {
    // null check
    if (WIFI_SSID == NULL || WIFI_PASS == NULL) {
        ESP_LOGE("ERT", "WiFi SSID or Password not set in environment variables.");
        return;
    }

    s_wifi_event_group = xEventGroupCreate();

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &event_handler,
                                                        NULL,
                                                        &instance_any_id));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                                                        IP_EVENT_STA_GOT_IP,
                                                        &event_handler,
                                                        NULL,
                                                        &instance_got_ip));

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = {0}, // Initialize with zeroes
            .password = {0}, // Initialize with zeroes
            .threshold.authmode = WIFI_AUTH_WPA2_PSK,
            .pmf_cfg = {
                .capable = true,
                .required = false
            },
        },
    };

    // copy ssid and password into the structure
    strncpy((char*)wifi_config.sta.ssid, WIFI_SSID, sizeof(wifi_config.sta.ssid) - 1);
    strncpy((char*)wifi_config.sta.password, WIFI_PASS, sizeof(wifi_config.sta.password) - 1);

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA) );
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config) );
    ESP_ERROR_CHECK(esp_wifi_start() );

    EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group,
            WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
            pdFALSE,
            pdFALSE,
            portMAX_DELAY);

    if (bits & WIFI_CONNECTED_BIT) {
        ESP_LOGI("ERT", "connected to ap SSID:%s password:%s",
                 WIFI_SSID, WIFI_PASS);
    } else if (bits & WIFI_FAIL_BIT) {
        ESP_LOGI("ERT", "Failed to connect to SSID:%s, password:%s",
                 WIFI_SSID, WIFI_PASS);
    } else {
        ESP_LOGE("ERT", "UNEXPECTED EVENT");
    }
}

void app_main() {
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }

    ESP_ERROR_CHECK(ret);

    wifi_init(); // WiFiの初期化

    for(int i = 0; i < NUM_ELECTRODES; i++) {
        gpio_reset_pin(electrodePins[i]); // GPIO設定
        esp_err_t err = gpio_set_direction(electrodePins[i], GPIO_MODE_OUTPUT); // GPIO方向設定
        error_handler(err); // エラーハンドリング
        gpio_set_level(electrodePins[i], 0); // 最初は電流を流さない
    }

    adc1_config_width(ADC_WIDTH_BIT_12); // ADC幅設定
    adc1_config_channel_atten(ADC_CHANNEL, ADC_ATTEN_DB_0); // ADC減衰設定

    char data[256]; // データバッファ

    while(1) {
        // 各電極ペアの測定
        for(int i = 0; i < NUM_ELECTRODES; i++) {
            for(int j = i + 1; j < NUM_ELECTRODES; j++) {
                gpio_set_level(electrodePins[i], 1); // 電極iを1に設定
                gpio_set_level(electrodePins[j], 0); // 電極jを0に設定

                vTaskDelay(100 / portTICK_PERIOD_MS); // 100msの遅延

                // 他の電極からデータを読み取る
                for(int k = 0; k < NUM_ELECTRODES; k++) {
                    if(k != i && k != j) {
                        // 電極kのデータを読み取る
                        int val = adc1_get_raw(ADC_CHANNEL); // ADCから生データを取得
                        sprintf(data, "%d,%d,%d,%d\n", i, j, k, val); // データをフォーマット
                        printf("%s", data); // データを出力
                        //send_data(data); // データをサーバーに送信
                    }
                }

                gpio_set_level(electrodePins[i], 0); // 電極iを0に設定
                gpio_set_level(electrodePins[j], 0); // 電極jを0に設定

                vTaskDelay(500 / portTICK_PERIOD_MS); // 500msの遅延
            }
        }
    }
}
