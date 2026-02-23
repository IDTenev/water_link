#include "common_data.h"
#include "p_can.h"

#include "esp_log.h"
#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define TAG "PORT/CAN"

// -------------------------
// Internal state
// -------------------------
static bool s_can_inited = false;

// 간단하게 esp_err_t를 uint16_t로 축약
static inline uint16_t err_u16(esp_err_t err)
{
    if (err == ESP_OK) return 0;
    // ESP_ERR_xxx 값이 음수/큰 값일 수 있어서 안전하게 매핑
    return (uint16_t)(0x8000u | ((uint32_t)err & 0x7FFFu));
}

static twai_timing_config_t timing_from_kbps(int kbps)
{
    // ESP32 TWAI는 APB 80MHz 기준으로 많이 씀.
    // 아래 값들은 IDF 예제/일반적으로 널리 쓰이는 세팅(샘플 포인트 약 80% 근처)
    // (brp, tseg1, tseg2, sjw) 조합.

    // NOTE: 값은 twai_timing_config_t 구조체 필드명에 맞춰야 함.
    // IDF 5.5.2 기준으로 brp, tseg_1, tseg_2, sjw, triple_sampling 필드가 존재.

    twai_timing_config_t t = {0};
    t.triple_sampling = false;

    switch (kbps) {
        case 25:
            t.brp = 160; t.tseg_1 = 15; t.tseg_2 = 4; t.sjw = 3;
            break;
        case 50:
            t.brp = 80;  t.tseg_1 = 15; t.tseg_2 = 4; t.sjw = 3;
            break;
        case 100:
            t.brp = 40;  t.tseg_1 = 15; t.tseg_2 = 4; t.sjw = 3;
            break;
        case 125:
            t.brp = 32;  t.tseg_1 = 15; t.tseg_2 = 4; t.sjw = 3;
            break;
        case 250:
            t.brp = 16;  t.tseg_1 = 15; t.tseg_2 = 4; t.sjw = 3;
            break;
        case 500:
            t.brp = 8;   t.tseg_1 = 15; t.tseg_2 = 4; t.sjw = 3;
            break;
        case 800:
            t.brp = 5;   t.tseg_1 = 15; t.tseg_2 = 4; t.sjw = 3;
            break;
        case 1000:
            t.brp = 4;   t.tseg_1 = 15; t.tseg_2 = 4; t.sjw = 3;
            break;
        default:
            ESP_LOGW(TAG, "Unknown bitrate %d kbps, fallback 500 kbps", kbps);
            t.brp = 8;   t.tseg_1 = 15; t.tseg_2 = 4; t.sjw = 3;
            break;
    }
    return t;
}

static twai_filter_config_t build_filter_config(void)
{
#if CAN_USE_RX_FILTER

    twai_filter_config_t f_config = {0};

    uint32_t id   = CAN_FILTER_ID;
    uint32_t mask = CAN_FILTER_MASK;

    if (CAN_FILTER_EXTENDED) {
        f_config.acceptance_code = (id << 3);
        f_config.acceptance_mask = ~(mask << 3);
    } else {
        f_config.acceptance_code = (id << 21);
        f_config.acceptance_mask = ~(mask << 21);
    }

    f_config.single_filter = true;

    ESP_LOGI(TAG, "RX Filter enable: ID=0x%X MASK=0x%X EXT=%d",
             (unsigned)id, (unsigned)mask, (int)CAN_FILTER_EXTENDED);

    return f_config;

#else
    ESP_LOGI(TAG, "RX Filter disabled (ACCEPT ALL)");

    // 매크로를 변수로 받았다가 return
    twai_filter_config_t f = TWAI_FILTER_CONFIG_ACCEPT_ALL();
    return f;
#endif
}


uint16_t can_init(void)
{
    if (s_can_inited) {
        ESP_LOGW(TAG, "can already inited");
        return 0;
    }

    ESP_LOGI(TAG, "can init (TX=%d RX=%d bitrate=%dkbps)", PIN_CAN_TX, PIN_CAN_RX, CAN_BITRATE_KBPS);

    // General config: Normal mode, TX/RX pin, queue sizes
    twai_general_config_t g_config = TWAI_GENERAL_CONFIG_DEFAULT(
        (gpio_num_t)PIN_CAN_TX,
        (gpio_num_t)PIN_CAN_RX,
        TWAI_MODE_NORMAL
    );

    // 큐는 프로젝트 상황에 맞게 조절
    g_config.tx_queue_len = 16;
    g_config.rx_queue_len = 32;

    // Timing config
    twai_timing_config_t t_config = timing_from_kbps(CAN_BITRATE_KBPS);

    // Filter config: accept all
    twai_filter_config_t f_config = build_filter_config();

    esp_err_t err = twai_driver_install(&g_config, &t_config, &f_config);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "twai_driver_install failed: %s", esp_err_to_name(err));
        return err_u16(err);
    }

    err = twai_start();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "twai_start failed: %s", esp_err_to_name(err));
        (void)twai_driver_uninstall();
        return err_u16(err);
    }

    s_can_inited = true;
    ESP_LOGI(TAG, "can start OK");
    return 0;
}

uint16_t can_send(uint32_t id,
                  const uint8_t *data,
                  uint8_t dlc,
                  bool extended,
                  bool rtr,
                  uint32_t timeout_ms)
{
    if (!s_can_inited) {
        ESP_LOGE(TAG, "can not inited");
        return 0x0001;
    }

    if (dlc > 8) {
        ESP_LOGE(TAG, "dlc overflow: %u", dlc);
        return 0x0002;
    }

    twai_message_t msg = {0};
    msg.identifier = id;
    msg.data_length_code = dlc;

    if (extended) msg.extd = 1;
    if (rtr)      msg.rtr  = 1;

    if (!rtr && dlc > 0) {
        if (data == NULL) {
            ESP_LOGE(TAG, "data is NULL but dlc=%u", dlc);
            return 0x0003;
        }
        for (int i = 0; i < dlc; i++) msg.data[i] = data[i];
    }

    TickType_t to = pdMS_TO_TICKS(timeout_ms);
    esp_err_t err = twai_transmit(&msg, to);
    if (err != ESP_OK) {
        // timeout도 여기로 옴 (ESP_ERR_TIMEOUT)
        ESP_LOGW(TAG, "tx failed: %s (id=0x%X ext=%d rtr=%d dlc=%d)",
                 esp_err_to_name(err), (unsigned)id, (int)extended, (int)rtr, (int)dlc);
        return err_u16(err);
    }

    return 0;
}

uint16_t can_read(twai_message_t *out_msg, uint32_t timeout_ms)
{
    if (!s_can_inited) {
        ESP_LOGE(TAG, "can not inited");
        return 0x0001;
    }
    if (out_msg == NULL) {
        return 0x0004;
    }

    TickType_t to = pdMS_TO_TICKS(timeout_ms);
    esp_err_t err = twai_receive(out_msg, to);
    if (err != ESP_OK) {
        // non-blocking이면 timeout이 흔함
        return err_u16(err);
    }
    return 0;
}

uint16_t can_get_status(twai_status_info_t *out_status)
{
    if (!s_can_inited) return 0x0001;
    if (out_status == NULL) return 0x0004;

    esp_err_t err = twai_get_status_info(out_status);
    if (err != ESP_OK) return err_u16(err);
    return 0;
}

uint16_t can_recover(void)
{
    if (!s_can_inited) {
        // init 전이면 그냥 init
        return can_init();
    }

    // BUS-OFF 등에서 stop/start가 필요할 수 있음
    esp_err_t err = twai_stop();
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "twai_stop: %s", esp_err_to_name(err));
    }

    err = twai_start();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "twai_start(recover) failed: %s", esp_err_to_name(err));
        return err_u16(err);
    }

    ESP_LOGI(TAG, "can recovered");
    return 0;
}
