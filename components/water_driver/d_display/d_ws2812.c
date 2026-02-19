#include "common_data.h"
#include "error_code.h"
#include "d_ws2812.h"

#include "esp_check.h"
#include "esp_log.h"
#include "driver/rmt_tx.h"
#include "driver/rmt_encoder.h"
#include "esp_private/esp_clk.h"
#include "rom/ets_sys.h"

#define WS2812_RESOLUTION_HZ  (10 * 1000 * 1000) // 10MHz -> 0.1us per tick
#define WS2812_T0H_NS         350
#define WS2812_T0L_NS         800
#define WS2812_T1H_NS         700
#define WS2812_T1L_NS         600

static const char *TAG = "DRIVER/WS2812";

static rmt_channel_handle_t s_rmt_chan = NULL;
static rmt_encoder_handle_t s_bytes_encoder = NULL;

static uint8_t s_pixel_grb[3] = {0, 0, 0}; // G, R, B

static inline uint8_t scale_u8(uint8_t v, uint8_t bright)
{
    return (uint8_t)(((uint16_t)v * (uint16_t)bright + 127) / 255);
}

uint16_t ws2812_init(void)
{
    uint16_t ret;

    if (s_rmt_chan || s_bytes_encoder) {
        ESP_LOGW(TAG, "already initialized");
        return ESP_OK;
    }

    // --- RMT TX channel ---
    rmt_tx_channel_config_t tx_chan_cfg = {
        .gpio_num = PIN_WATER_LED,
        .clk_src = RMT_CLK_SRC_DEFAULT,
        .resolution_hz = WS2812_RESOLUTION_HZ,
        .mem_block_symbols = 64,  // 충분 (LED 1개 = 24bit)
        .trans_queue_depth = 4,
        .flags = {
            .invert_out = 0,
            .with_dma = 0, // 1LED라 DMA 필요 없음
        },
    };

    ESP_GOTO_ON_ERROR(rmt_new_tx_channel(&tx_chan_cfg, &s_rmt_chan), fail, TAG, "new tx channel failed");
    ESP_GOTO_ON_ERROR(rmt_enable(s_rmt_chan), fail, TAG, "enable channel failed");

    // --- Byte encoder configured for WS2812 timings ---
    // Convert ns -> ticks
    const uint32_t tick_ns = 1000000000UL / WS2812_RESOLUTION_HZ;

    rmt_bytes_encoder_config_t bytes_encoder_cfg = {
        .bit0 = {
            .level0 = 1,
            .duration0 = (WS2812_T0H_NS + tick_ns - 1) / tick_ns,
            .level1 = 0,
            .duration1 = (WS2812_T0L_NS + tick_ns - 1) / tick_ns,
        },
        .bit1 = {
            .level0 = 1,
            .duration0 = (WS2812_T1H_NS + tick_ns - 1) / tick_ns,
            .level1 = 0,
            .duration1 = (WS2812_T1L_NS + tick_ns - 1) / tick_ns,
        },
        .flags = {
            .msb_first = 1, // WS2812: MSB first
        },
    };

    ESP_GOTO_ON_ERROR(rmt_new_bytes_encoder(&bytes_encoder_cfg, &s_bytes_encoder), fail, TAG, "new bytes encoder failed");

    // default off
    s_pixel_grb[0] = 0;
    s_pixel_grb[1] = 0;
    s_pixel_grb[2] = 0;

    ESP_LOGI(TAG, "init done. res=%dHz", WS2812_RESOLUTION_HZ);
    return ESP_OK;

fail:
    if (s_bytes_encoder) {
        rmt_del_encoder(s_bytes_encoder);
        s_bytes_encoder = NULL;
    }
    if (s_rmt_chan) {
        rmt_disable(s_rmt_chan);
        rmt_del_channel(s_rmt_chan);
        s_rmt_chan = NULL;
    }
    return ret;
}

void ws2812_set_pixel(uint8_t r, uint8_t g, uint8_t b, uint8_t bright)
{
    // apply brightness scaling
    uint8_t rr = scale_u8(r, bright);
    uint8_t gg = scale_u8(g, bright);
    uint8_t bb = scale_u8(b, bright);

    // WS2812 expects GRB order
    s_pixel_grb[0] = gg;
    s_pixel_grb[1] = rr;
    s_pixel_grb[2] = bb;
}

uint16_t ws2812_refresh(void)
{
    if (!s_rmt_chan || !s_bytes_encoder) {
        ESP_LOGE(TAG, "not initialized");
        return ESP_ERR_INVALID_STATE;
    }

    rmt_transmit_config_t tx_cfg = {
        .loop_count = 0,
        .flags = {
            .eot_level = 0, // end-of-transmission output low
        },
    };

    uint16_t ret = rmt_transmit(s_rmt_chan, s_bytes_encoder, s_pixel_grb, sizeof(s_pixel_grb), &tx_cfg);
    if (ret != ESP_OK) return ret;

    // wait until done
    ESP_RETURN_ON_ERROR(rmt_tx_wait_all_done(s_rmt_chan, -1), TAG, "wait done failed");

    // latch/reset time (>50us)
    ets_delay_us(60);

    return ESP_OK;
}
