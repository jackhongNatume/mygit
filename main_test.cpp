#include <Arduino.h>
#include <WiFi.h>
#include <driver/i2s.h>
#include <ArduinoJson.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_wifi.h>

#define SAMPLE_RATE         (8000)
#define SAMPLE_BITS         (16)
#define BUFFER_SIZE         (2048)
#define SILENCE_TIMEOUT     (5000)
#define THRESHOLD_RMS       (500)
#define AWAKE_PIN           34

const char* ssid = "your_network_ssid";
const char* password = "your_network_password";
const char* server_ip = "server_ip_address";
const uint16_t server_port = 12347;

uint8_t buffer[BUFFER_SIZE];
size_t sent_index = 0; // 上次发送的位置
bool is_awake = false;
bool conversation_end = false;

TaskHandle_t taskHandle1 = NULL;
TaskHandle_t taskHandle2 = NULL;

void initI2S() {
    i2s_config_t i2s_config = {
        .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX),
        .sample_rate = SAMPLE_RATE,
        .bits_per_sample = (i2s_bits_per_sample_t)SAMPLE_BITS,
        .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
        .communication_format = I2S_COMM_FORMAT_I2S,
        .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
        .dma_buf_count = 8,
        .dma_buf_len = 64,
        .use_apll = false
    };
    i2s_pin_config_t pin_config = {
        .bck_io_num = 26,
        .ws_io_num = 25,
        .data_out_num = I2S_PIN_NO_CHANGE,
        .data_in_num = 22
    };
    i2s_driver_install(I2S_NUM_0, &i2s_config, 0, NULL);
    i2s_set_pin(I2S_NUM_0, &pin_config);
}

uint32_t calculateRMS(uint8_t *data, size_t len) {
    uint32_t sum = 0;
    for (size_t i = 0; i < len; i += 2) {
        int16_t sample = (data[i + 1] << 8) | data[i];
        sum += sample * sample;
    }
    uint32_t rms = sqrt(sum / (len / 2));
    return rms;
}

void connectWiFi() {
    WiFi.begin(ssid, password);
    Serial.print("Connecting to WiFi");
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("Connected to WiFi");
}

// 声音检测任务
void audioTask(void * parameter) {
    while (1) {
        size_t bytes_read = 0;
        i2s_read(I2S_NUM_0, buffer, BUFFER_SIZE, &bytes_read, portMAX_DELAY);

        if (is_awake) {
            uint32_t rms = calculateRMS(buffer, bytes_read);
            Serial.print("RMS: ");
            Serial.println(rms);

            static unsigned long silence_start_time = 0;
            static bool is_speaking = false;

            if (rms > THRESHOLD_RMS) {
                is_speaking = true;
                silence_start_time = 0;
            } else {
                if (silence_start_time == 0) {
                    silence_start_time = millis();
                } else {
                    if (millis() - silence_start_time > SILENCE_TIMEOUT) {
                        is_speaking = false;
                        silence_start_time = 0;
                    }
                }
            }

            if (!is_speaking && conversation_end) {
                // 进入待机状态
                is_awake = false;
                Serial.println("Entering sleep mode");
                esp_wifi_stop();
                delay(100); // 等待 WiFi 停止
                esp_deep_sleep_start();
            }
        }

        delay(10); // 延时等待
    }
}

// 数据发送任务
void sendDataTask(void * parameter) {
    while (1) {
        if (is_awake) {
            StaticJsonDocument<512> doc;
            JsonArray data_array = doc.createNestedArray("data");
//序列分段
            size_t num_slices = BUFFER_SIZE / 1024 + (BUFFER_SIZE % 1024 != 0);
            for (size_t i = 0; i < num_slices; ++i) {
                size_t start_index = i * 1024;
                size_t end_index = min((i + 1) * 1024, BUFFER_SIZE);

                for (size_t j = start_index; j < end_index; ++j) {
                    int16_t sample = (buffer[j * 2 + 1] << 8) | buffer[j * 2];
                    data_array.add(sample);
                }

                doc["state"] = (i == num_slices - 1) ? 0 : 10;
                doc["encoding"] = "utf-8";

                String json_data;
                serializeJson(doc, json_data);

                WiFiClient client;
                if (client.connect(server_ip, server_port)) {
                    client.print(json_data);
                    client.stop();
                } else {
                    Serial.println("Connection to server failed");
                }

                delay(10); // 延时等待
            }
        }

        delay(1000); // 每秒发送一次数据（？）
    }
}

void setup() {
    Serial.begin(115200);
    pinMode(AWAKE_PIN, INPUT_PULLUP);
    connectWiFi();
    initI2S();

    // 创建声音检测任务
    xTaskCreatePinnedToCore(audioTask, "AudioTask", 4096, NULL, 1, &taskHandle1, 0);

    // 创建数据发送任务
    xTaskCreatePinnedToCore(sendDataTask, "SendDataTask", 4096, NULL, 1, &taskHandle2, 0);
}

void loop(){
    //检测唤醒信号
    if(digitalRead(AWAKE_PIN) == HIGH){
        is_awake = true;
        conversation_end = false;
        Serial.println("Awake signal received");
    }
}