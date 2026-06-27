#include <esp_log.h>
#include <FreeRTOSConfig.h>
#include <portmacro.h>
#include <stdint.h>
#include <driver/gpio.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include "esp_err.h"
#include "esp_zigbee_core.h"
#include "nvs_flash.h"
#include "ha/esp_zigbee_ha_standard.h"
// #include "driver/ledc.h"

#define CONFIG_LED_LIGHT_ENABLED 1
#define CONFIG_LED_LIGHT_ENABLED_TIME_MS 500
#define CONFIG_LED_LIGHT_DISABLED_TIME_MS 100
#define CONFIG_LED_LIGHT_PIN 15
#define CONFIG_LED_LIGHT_BRIGHTNESS 100
#define CONFIG_RGB_LIGHT_ENABLED 1
#define CONFIG_RGB_LIGHT_ENABLED_TIME_MS 100
#define CONFIG_RGB_LIGHT_DISABLED_TIME_MS 100
#define CONFIG_RGB_LIGHT_BRIGHTNESS 100
#define CONFIG_RGB_LIGHT_PIN 8
#define CONFIG_RGB_LIGHT_USE_RTM 1

static const char *TAG = "app_main";

// typedef enum {
//     APP_LED_LIGHT_ENABLED,
//     APP_LED_LIGHT_DISABLED,
// } app_led_light_state_t;
//
// typedef struct {
//     app_led_light_state_t state;
// } app_led_light_runtime_t;
//
// typedef struct {
//     uint32_t enabled_time;
//     uint32_t disabled_time;
//     uint8_t gpio_pin;
// } app_led_light_config_t;
//
// esp_err_t app_led_light_config_init(const app_led_light_config_t *config) {
//     ESP_ERROR_CHECK(
//         gpio_reset_pin(config->gpio_pin)
//     );
//
//     ESP_ERROR_CHECK(
//         gpio_set_direction(config->gpio_pin, GPIO_MODE_OUTPUT)
//     );
//
//     return ESP_OK;
// }
//
// static esp_err_t app_led_light_enable(const gpio_num_t gpio_pin) {
//     return gpio_set_level(gpio_pin, 1);
// }
//
// static esp_err_t app_led_light_disable(const gpio_num_t gpio_pin) {
//     return gpio_set_level(gpio_pin, 0);
// }
//
// static esp_err_t app_led_light_switch_state(const app_led_light_config_t *config, app_led_light_runtime_t *runtime) {
//     esp_err_t err = ESP_OK;
//
//     switch (runtime->state) {
//         case APP_LED_LIGHT_ENABLED:
//             err = app_led_light_disable(config->gpio_pin);
//
//             runtime->state = APP_LED_LIGHT_DISABLED;
//
//             break;
//         case APP_LED_LIGHT_DISABLED:
//             err = app_led_light_enable(config->gpio_pin);
//
//             runtime->state = APP_LED_LIGHT_ENABLED;
//
//             break;
//         default:
//             return ESP_FAIL;
//     }
//
//     return err;
// }
//
// static esp_err_t app_led_light_switch_delay(const app_led_light_config_t *config, const app_led_light_runtime_t *runtime) {
//     switch (runtime->state) {
//         case APP_LED_LIGHT_ENABLED:
//             vTaskDelay(config->enabled_time / portTICK_PERIOD_MS);
//
//             break;
//         case APP_LED_LIGHT_DISABLED:
//             vTaskDelay(config->disabled_time / portTICK_PERIOD_MS);
//
//             break;
//         default:
//             return ESP_FAIL;
//     }
//
//     return ESP_OK;
// }
//
// esp_err_t app_led_light_switch(const app_led_light_config_t *config, app_led_light_runtime_t *runtime) {
//     app_led_light_switch_state(config, runtime);
//     app_led_light_switch_delay(config, runtime);
//
//     return ESP_OK;
// }
//
// void app_led_light_switch_task(void*) {
//     const app_led_light_config_t config = {
//         .enabled_time = CONFIG_LED_LIGHT_ENABLED_TIME_MS,
//         .disabled_time = CONFIG_LED_LIGHT_DISABLED_TIME_MS,
//         .gpio_pin = CONFIG_LED_LIGHT_PIN,
//     };
//
//     ESP_LOGD(TAG, "Using enabled delay %d ms", config.enabled_time);
//     ESP_LOGD(TAG, "Using disabled delay %d ms", config.disabled_time);
//     ESP_LOGD(TAG, "Using gpio pin %d", config.gpio_pin);
//
//     ESP_ERROR_CHECK(
//         app_led_light_config_init(&config)
//     );
//
//     app_led_light_runtime_t runtime = {
//         .state = APP_LED_LIGHT_DISABLED,
//     };
//
//     ESP_LOGI(TAG, "Initialized LED config");
//
//     ESP_LOGD(TAG, "Entering infinite loop");
//
//     while (1) {
//         ESP_ERROR_CHECK(
//             app_led_light_switch(&config, &runtime)
//         );
//
//         ESP_LOGD(TAG, "Switched led runtime state to %d", runtime.state);
//     }
// }

static bool light_enabled = false;

static void bdb_start_top_level_commissioning_cb(uint8_t mode_mask)
{
    ESP_RETURN_ON_FALSE(esp_zb_bdb_start_top_level_commissioning(mode_mask) == ESP_OK, , TAG, "Failed to start Zigbee commissioning");
}

void esp_zb_app_signal_handler(const esp_zb_app_signal_t *signal_s)
{
    const uint32_t *p_sg_p       = signal_s->p_app_signal;
    const esp_err_t err_status = signal_s->esp_err_status;

    const esp_zb_app_signal_type_t sig_type = *p_sg_p;

    switch (sig_type) {
    case ESP_ZB_ZDO_SIGNAL_SKIP_STARTUP:
        ESP_LOGI(TAG, "Initialize Zigbee stack");

        esp_zb_bdb_start_top_level_commissioning(ESP_ZB_BDB_MODE_INITIALIZATION);

        break;
    case ESP_ZB_BDB_SIGNAL_DEVICE_FIRST_START:
    case ESP_ZB_BDB_SIGNAL_DEVICE_REBOOT:
        if (err_status == ESP_OK) {
            ESP_LOGI(TAG, "Configuring LED pin");

            ESP_ERROR_CHECK(
                gpio_reset_pin(15)
            );

            ESP_ERROR_CHECK(
                gpio_set_direction(15, GPIO_MODE_OUTPUT)
            );

            if (esp_zb_bdb_is_factory_new()) {
                ESP_LOGI(TAG, "Start network steering");

                esp_zb_bdb_start_top_level_commissioning(ESP_ZB_BDB_MODE_NETWORK_STEERING);
            } else {
                ESP_LOGI(TAG, "Device rebooted");
            }
        } else {
            /* commissioning failed */
            ESP_LOGW(TAG, "Failed to initialize Zigbee stack (status: %s)", esp_err_to_name(err_status));
            ESP_LOGW(TAG, "Doing factory reset...");

            esp_zb_factory_reset();
        }
        break;
    case ESP_ZB_BDB_SIGNAL_STEERING:
        if (err_status == ESP_OK) {
            esp_zb_ieee_addr_t extended_pan_id;

            esp_zb_get_extended_pan_id(extended_pan_id);

            ESP_LOGI(TAG, "Joined network successfully (Extended PAN ID: %02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x, PAN ID: 0x%04hx, Channel:%d, Short Address: 0x%04hx)",
                     extended_pan_id[7], extended_pan_id[6], extended_pan_id[5], extended_pan_id[4],
                     extended_pan_id[3], extended_pan_id[2], extended_pan_id[1], extended_pan_id[0],
                     esp_zb_get_pan_id(), esp_zb_get_current_channel(), esp_zb_get_short_address());
        } else {
            ESP_LOGI(TAG, "Network steering was not successful (status: %s)", esp_err_to_name(err_status));

            esp_zb_scheduler_alarm((esp_zb_callback_t)bdb_start_top_level_commissioning_cb, ESP_ZB_BDB_MODE_NETWORK_STEERING, 1000);
        }

        break;
    default:
        ESP_LOGI(TAG, "ZDO signal: %s (0x%x), status: %s", esp_zb_zdo_signal_to_string(sig_type), sig_type,
                 esp_err_to_name(err_status));
        break;
    }
}

static esp_err_t app_zb_attribute_handler(const esp_zb_zcl_set_attr_value_message_t *message) {
    bool light_state = 0;

    ESP_RETURN_ON_FALSE(message, ESP_FAIL, TAG, "Empty message");
    ESP_RETURN_ON_FALSE(message->info.status == ESP_ZB_ZCL_STATUS_SUCCESS, ESP_ERR_INVALID_ARG, TAG, "Received message: error status(%d)",
                        message->info.status);
    ESP_LOGI(TAG, "Received message: endpoint(%d), cluster(0x%x), attribute(0x%x), data size(%d)", message->info.dst_endpoint, message->info.cluster,
             message->attribute.id, message->attribute.data.size);

    if (message->info.dst_endpoint != 10) {
        ESP_LOGW(TAG, "Received message from an unknown endpoint(%d)", message->info.dst_endpoint);

        return ESP_OK;
    }

    if (message->info.cluster != ESP_ZB_ZCL_CLUSTER_ID_ON_OFF) {
        ESP_LOGW(TAG, "Received message from an endpoint(%d) with unknown cluster(%d)", message->info.dst_endpoint, message->info.cluster);

        return ESP_OK;
    }

    if (message->attribute.id != ESP_ZB_ZCL_ATTR_ON_OFF_ON_OFF_ID || message->attribute.data.type != ESP_ZB_ZCL_ATTR_TYPE_BOOL) {
        ESP_LOGW(TAG, "Received message from an endpoint(%d) with unsupported attribute(%d) or invalid type(%d)", message->info.dst_endpoint, message->attribute.id, message->attribute.data.type);

        return ESP_OK;
    }

    light_state = message->attribute.data.value ? *(bool *)message->attribute.data.value : light_state;

    ESP_LOGI(TAG, "Light sets to %s", light_state ? "On" : "Off");

    if (light_enabled) {
        light_enabled = false;

        return gpio_set_level(15, 0);
    }

    light_enabled = true;

    return gpio_set_level(15, 1);
}

static esp_err_t app_zb_action_handler(esp_zb_core_action_callback_id_t callback_id, const void *message) {
    switch (callback_id) {
    case ESP_ZB_CORE_SET_ATTR_VALUE_CB_ID:
        return app_zb_attribute_handler((esp_zb_zcl_set_attr_value_message_t *)message);
    default:
        ESP_LOGW(TAG, "Receive Zigbee action(0x%x) callback", callback_id);
        return ESP_FAIL;
    }
}

void app_zigbee_task(void*) {
    esp_zb_platform_config_t zb_platform_config = {
        .radio_config = {
            .radio_mode = CONFIG_ZB_RADIO_NATIVE
        },
        .host_config = {
            .host_connection_mode = ZB_HOST_CONNECTION_MODE_NONE
        },
    };

    ESP_ERROR_CHECK(
        nvs_flash_init()
    );

    ESP_ERROR_CHECK(
        esp_zb_platform_config(&zb_platform_config)
    );

    esp_zb_cfg_t zed_config = {
        .esp_zb_role = ESP_ZB_DEVICE_TYPE_ED,
        .install_code_policy = false,
        .nwk_cfg.zed_cfg = {
            .ed_timeout = ESP_ZB_ED_AGING_TIMEOUT_2MIN,
            .keep_alive = 3000
        },
    };

    esp_zb_init(&zed_config);

    esp_zb_on_off_light_cfg_t light_config = {
        .basic_cfg = {
            .zcl_version = ESP_ZB_ZCL_BASIC_ZCL_VERSION_DEFAULT_VALUE,
            .power_source = ESP_ZB_ZCL_BASIC_POWER_SOURCE_BATTERY
        },
        .identify_cfg = {
            .identify_time = 5
        },
        .groups_cfg = {
            .groups_name_support_id = ESP_ZB_ZCL_GROUPS_NAME_SUPPORT_DEFAULT_VALUE
        },
        .scenes_cfg = {
            .scenes_count = ESP_ZB_ZCL_SCENES_SCENE_COUNT_DEFAULT_VALUE,
            .current_scene = ESP_ZB_ZCL_SCENES_CURRENT_SCENE_DEFAULT_VALUE,
            .current_group = ESP_ZB_ZCL_SCENES_CURRENT_GROUP_DEFAULT_VALUE,
            .scene_valid = ESP_ZB_ZCL_SCENES_SCENE_VALID_DEFAULT_VALUE,
            .name_support = ESP_ZB_ZCL_SCENES_NAME_SUPPORT_DEFAULT_VALUE,
        },
        .on_off_cfg = {
            .on_off = ESP_ZB_ZCL_ON_OFF_ON_OFF_DEFAULT_VALUE
        }
    };

    esp_zb_ep_list_t *esp_zb_on_off_light_ep = esp_zb_on_off_light_ep_create(10, &light_config);

    esp_zb_device_register(esp_zb_on_off_light_ep);

    esp_zb_core_action_handler_register(app_zb_action_handler);
    esp_zb_set_primary_network_channel_set(ESP_ZB_TRANSCEIVER_ALL_CHANNELS_MASK);

    ESP_ERROR_CHECK(
        esp_zb_start(false)
    );

    ESP_LOGI(TAG, "Entering zigbee stack`s main loop");

    esp_zb_stack_main_loop();
}

void app_main(void) {
    ESP_LOGI(TAG, "Started executing app_main");

    // xTaskCreatePinnedToCore(app_led_light_switch_task, "app_led_light_switch_loop", 1024, NULL, 5, nullptr, 0);
    //
    // ESP_LOGI(TAG, "Spawned light blinker task");

    xTaskCreatePinnedToCore(app_zigbee_task, "app_zigbee_loop", 4096, NULL, 5, nullptr, 0);

    ESP_LOGI(TAG, "Spawned zigbee task");

    // const ledc_timer_config_t timer_config = {
    //     .duty_resolution = LEDC_TIMER_13_BIT,
    //     .speed_mode = LEDC_LOW_SPEED_MODE,
    //     .timer_num = LEDC_TIMER_1,
    //     .freq_hz = 4000,
    //     .clk_cfg = LEDC_AUTO_CLK,
    //     .deconfigure = false,
    // };
    //
    // ESP_ERROR_CHECK(
    //     ledc_timer_config(&timer_config)
    // );
    //
    // ESP_LOGI(TAG, "Initialized ledc_timer_config");
    //
    // const ledc_channel_config_t channel_config = {
    //     .gpio_num = CONFIG_LED_LIGHT_PIN,
    //     .channel = LEDC_CHANNEL_0,
    //     .timer_sel = LEDC_TIMER_1,
    //     .duty = 0,
    //     .hpoint = 0,
    //     .sleep_mode = LEDC_SLEEP_MODE_NO_ALIVE_NO_PD,
    //     .speed_mode = LEDC_LOW_SPEED_MODE,
    //     .flags.output_invert = 0,
    //     .deconfigure = false,
    // };
    //
    // ESP_ERROR_CHECK(
    //     ledc_channel_config(&channel_config)
    // );
    //
    // ESP_LOGI(TAG, "Initialized ledc_channel_config");
    //
    // ESP_ERROR_CHECK(
    //     ledc_fade_func_install(0)
    // );
    //
    // ESP_LOGI(TAG, "Installed fade function");
    //
    // while (1) {
    //     ESP_LOGI(TAG, "Starting fading up");
    //
    //     ESP_ERROR_CHECK(
    //         ledc_set_fade_with_time(channel_config.speed_mode, channel_config.channel, 4000, 3000)
    //     );
    //
    //     ESP_ERROR_CHECK(
    //         ledc_fade_start(channel_config.speed_mode, channel_config.channel, LEDC_FADE_WAIT_DONE)
    //     );
    //
    //     ESP_LOGI(TAG, "Starting fading down");
    //
    //     ESP_ERROR_CHECK(
    //         ledc_set_fade_with_time(channel_config.speed_mode, channel_config.channel, 0, 3000)
    //     );
    //
    //     ESP_ERROR_CHECK(
    //         ledc_fade_start(channel_config.speed_mode, channel_config.channel, LEDC_FADE_WAIT_DONE)
    //     );
    // }
}
