#include "tasks.h"
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
    rgb_led_task();
    vTaskDelay(pdMS_TO_TICKS(10));
  }
}

void tasks_init(void)
{
  xTaskCreate(WiFi_MQTT_Task, "WiFi_MQTT", TASK_STACK_SIZE, NULL, 0, &wifi_mqtt_handle);
  xTaskCreate(Led_Color_Task, "LED_Color", 2048, NULL, 0, &led_color_handle);
  Serial.println("[Tasks] WiFi_MQTT和LED任务已启动");
}
