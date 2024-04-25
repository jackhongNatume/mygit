#include "Arduino.h"
#include "driver/i2s.h"

#define SAMPLE_RATE     (8000)          // 采样率为8kHz
#define SAMPLE_BITS     (16)            // 采样位数为16位
#define BUFFER_SIZE     (2048)          // 缓冲区大小，用于存储音频数据
#define SILENCE_TIMEOUT (5000)          // 沉默超时时间，单位毫秒

#define THRESHOLD_RMS   (500)           // RMS 阈值，用于判断是否正在说话



const char *wifiData[][2] = {
    {"111", "12345678"}, // 替换为自己常用的wifi名和密码
    {"222", "12345678"},
    // 继续添加需要的 Wi-Fi 名称和密码
};
// 缓冲区用于存储音频数据
uint8_t buffer[BUFFER_SIZE];

// 初始化 I2S 接口
void initI2S() {
    const i2s_config_t i2s_config = {
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

// 计算音频的 RMS 值
uint32_t calculateRMS(uint8_t *data, size_t len) {
    uint32_t sum = 0;
    for (size_t i = 0; i < len; i += 2) {
        int16_t sample = (data[i + 1] << 8) | data[i];
        sum += sample * sample;
    }
    uint32_t rms = sqrt(sum / (len / 2));
    return rms;
}

void setup() {
    Serial.begin(115200);
    initI2S();
}

void loop() {
    size_t bytes_read = 0;
    i2s_read(I2S_NUM_0, buffer, BUFFER_SIZE, &bytes_read, portMAX_DELAY);
    
    uint32_t rms = calculateRMS(buffer, bytes_read);
    Serial.print("RMS: ");
    Serial.println(rms);

    static unsigned long silence_start_time = 0;
    static bool is_speaking = false;

    if (rms > THRESHOLD_RMS) {
        // 如果 RMS 大于阈值，则认为正在说话
        is_speaking = true;
        silence_start_time = 0;
    } else {
        // 否则开始计时沉默时间
        if (silence_start_time == 0) {
            silence_start_time = millis();
        } else {
            // 如果沉默时间超过阈值，则认为语音结束
            if (millis() - silence_start_time > SILENCE_TIMEOUT) {
                is_speaking = false;
                silence_start_time = 0;
            }
        }
    }

    if (is_speaking) {
        // 将当前一段音频 raw 数据存入缓冲区 buffer，并发送给服务器
        // sendAudioToServer(buffer, bytes_read);
        // 这里你可以将 buffer 数据发送给服务器
    }

    delay(10); // 延时等待
}
