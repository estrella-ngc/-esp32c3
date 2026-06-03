#include "tasks.h"
#include "exit.h"
#include "rgb_led.h"
#include "wifi_manager.h"
#include "mqtt.h"

portMUX_TYPE shared_mux = portMUX_INITIALIZER_UNLOCKED;

static TaskHandle_t wifi_mqtt_handle = NULL;
static TaskHandle_t led_color_handle = NULL;

static void WiFi_MQTT_Task(void *pvParameters)
{
  (void)pvParameters;
  while (1) {
    wifi_update();
    mqtt_update();
    vTaskDelay(pdMS_TO_TICKS(5));
  }
}

static void Led_Color_Task(void *pvParameters)
{
  (void)pvParameters;
  while (1) {
    uint8_t enabled, connected;

    portENTER_CRITICAL(&shared_mux);
    enabled = system_enabled;
    connected = wifi_is_connected();
    portEXIT_CRITICAL(&shared_mux);

    if (enabled) {
      if (connected) {
        rgb_led_set(0, 80, 0);
      } else {
        rgb_led_set(80, 0, 0);
      }
    } else {
      rgb_led_off();
    }

    vTaskDelay(10 / portTICK_PERIOD_MS);
  }
}

void tasks_init(void)
{
  xTaskCreate(WiFi_MQTT_Task, "WiFi_MQTT", TASK_STACK_SIZE, NULL, 0, &wifi_mqtt_handle);
  xTaskCreate(Led_Color_Task, "LED_Color", 2048, NULL, 0, &led_color_handle);
  Serial.println("[Tasks] WiFi_MQTT和LED任务已启动");
}
