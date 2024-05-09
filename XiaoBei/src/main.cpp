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
#define CONVERSATION_TIMEOUT (180000)
#define AWAKE_PIN           34

const char* ssid = "your_network_ssid";
const char* password = "your_network_password";
const char* server_ip = "server_ip_address";
const uint16_t server_port = 12347;

uint8_t buffer[BUFFER_SIZE];
size_t sent_index = 0; // 上次发送的位置
bool is_awake = false;
bool conversation_end = true;
bool is_speaking = false;

TaskHandle_t taskHandle1 = NULL;
TaskHandle_t taskHandle2 = NULL;
TaskHandle_t taskHandle3 = NULL;

void initI2S() {
    i2s_config_t i2s_config = {
        .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX),
        .sample_rate = SAMPLE_RATE,
        .bits_per_sample = (i2s_bits_per_sample_t)SAMPLE_BITS,
        .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
        .communication_format = I2S_COMM_FORMAT_STAND_I2S,
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
    unsigned long silence_start_time = 0;
    unsigned long conversation_end_time = 0;

    while (1) {
        size_t bytes_read = 0;
        i2s_read(I2S_NUM_0, buffer, BUFFER_SIZE, &bytes_read, portMAX_DELAY);

        if (is_awake) {
            uint32_t rms = calculateRMS(buffer, bytes_read);
            Serial.print("RMS: ");
            Serial.println(rms);

            if (rms > THRESHOLD_RMS) {
                is_speaking = true;
                conversation_end = false;
                silence_start_time = 0;
                conversation_end_time = 0;
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
            // 检测是否一段对话结束
            if(!is_speaking){
                if(conversation_end == 0){
                    conversation_end_time = millis();
                }else{
                    if(millis() - conversation_end_time > CONVERSATION_TIMEOUT){
                        conversation_end = true;
                        is_awake = false;
                    }
                    else conversation_end = false;
            }
            }
        }

        delay(10); // 延时等待
    }
}

// 数据发送任务
void sendDataTask(void * parameter) {
    while (1) {
        while (is_speaking) {
            StaticJsonDocument<512> doc;
            JsonArray data_array = doc.createNestedArray("data");
//序列分段
            size_t num_slices = BUFFER_SIZE / 1024 + (BUFFER_SIZE % 1024 != 0);
            for (size_t i = 0; i < num_slices; ++i) {
                size_t start_index = i * 1024;
                size_t end_index = min((i + 1) * 1024, (size_t)BUFFER_SIZE);

                for (size_t j = start_index; j < end_index; ++j) {
                    int16_t sample = (buffer[j * 2 + 1] << 8) | buffer[j * 2];
                    data_array.add(sample);
                }
                
                doc["state"] = 0;
                doc["encoding"] = "utf-8";

                String json_data;
                serializeJson(doc, json_data);

                WiFiClient send_client;
                if (send_client.connect(server_ip, server_port)) {
                    send_client.print(json_data);
                    send_client.stop();
                } else {
                    Serial.println("Connection to server failed");
                }

                delay(10); // 延时等待
            }
        }
        // 发送结束标志
            StaticJsonDocument<128> doc;
            doc["state"] = 1; // 一句话结束
            doc["encoding"] = "utf-8";

            String json_data;
            serializeJson(doc, json_data);

            WiFiClient send_client;
            if (send_client.connect(server_ip, server_port)) {
                send_client.print(json_data);
                send_client.stop();
            } else {
                Serial.println("Connection to server failed");
            }
    }
}


// TTS 播放任务
void playAudioTask(void * parameter) {
    WiFiClient listen_client;
    while (1) {
        if (is_awake && conversation_end) {
            // 连接到服务器并监听回传的文字信息
            if (listen_client.connect(server_ip, server_port)) {
                while (listen_client.connected()) {
                    while (listen_client.available()) {
                        // 读取回传的文字信息并存储到 buffer 中
                        char c = listen_client.read();
                        // 将字符存储到 buffer 中（这里假设 buffer 为全局变量）
                        // buffer[index++] = c;
                    }
                }
                listen_client.stop();
            } else {
                Serial.println("Connection to server failed");
            }
        }

        delay(100); // 延时等待
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
    
    // 创建 TTS 播放任务
    xTaskCreatePinnedToCore(playAudioTask, "PlayAudioTask", 4096, NULL, 2, &taskHandle3, 0);
}

void loop(){
    //检测唤醒信号
    if(digitalRead(AWAKE_PIN) == HIGH){
        is_awake = true;
        Serial.println("Awake signal received");
    }
}