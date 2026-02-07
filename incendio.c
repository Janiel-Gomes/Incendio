#include "lwip/dns.h"
#include "lwip/ip_addr.h"
#include "lwip/tcp.h"
#include "pico/cyw43_arch.h"
#include "pico/stdlib.h"
#include <stdio.h>
#include <string.h>

// FreeRTOS includes
#include "FreeRTOS.h"
#include "task.h"

// FreeRTOS Hooks
void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName) {
  (void)xTask;
  (void)pcTaskName;
  printf("STACK OVERFLOW in task: %s\n", pcTaskName);
  while (1)
    ;
}

void vApplicationTickHook(void) {}

void vApplicationIdleHook(void) {}

void vApplicationGetIdleTaskMemory(StaticTask_t **ppxIdleTaskStatus,
                                   StackType_t **ppxIdleTaskStackBuffer,
                                   uint32_t *pulIdleTaskStackSize) {
  static StaticTask_t xIdleTaskAccumulator;
  static StackType_t uxIdleTaskStack[configMINIMAL_STACK_SIZE];

  *ppxIdleTaskStatus = &xIdleTaskAccumulator;
  *ppxIdleTaskStackBuffer = uxIdleTaskStack;
  *pulIdleTaskStackSize = configMINIMAL_STACK_SIZE;
}

void vApplicationGetTimerTaskMemory(StaticTask_t **ppxTimerTaskStatus,
                                    StackType_t **ppxTimerTaskStackBuffer,
                                    uint32_t *pulTimerTaskStackSize) {
  static StaticTask_t xTimerTaskAccumulator;
  static StackType_t uxTimerTaskStack[configTIMER_TASK_STACK_DEPTH];

  *ppxTimerTaskStatus = &xTimerTaskAccumulator;
  *ppxTimerTaskStackBuffer = uxTimerTaskStack;
  *pulTimerTaskStackSize = configTIMER_TASK_STACK_DEPTH;
}

// Local includes
#include "aht10.h"

// Pin definitions
#define FLAME_SENSOR_PIN 16 // Updated for BitDogLab Header
#define BUZZER_PIN 14
#define LED_PIN CYW43_WL_GPIO_LED_PIN

// I2C definitions
#define I2C_PORT i2c0
#define SDA_PIN 4
#define SCL_PIN 5

// WiFi & Cloud Config - CHANGE THESE
#define WIFI_SSID ""
#define WIFI_PASS ""
#define SERVER_IP "" // Seu IP real detectado
#define SERVER_PORT 5000

// Global state
typedef struct {
  float temperature;
  float humidity;
  int flame_detected;
  int alarm_status;
} system_state_t;

system_state_t g_state = {0};

// Task: Sensor Reading
void Task_Sensor(void *pvParameters) {
  aht10_data_t data;
  while (1) {
    if (aht10_read(I2C_PORT, &data)) {
      g_state.temperature = data.temperature;
      g_state.humidity = data.humidity;
    }

    // Active LOW flame sensor
    g_state.flame_detected = !gpio_get(FLAME_SENSOR_PIN);

    printf("[Sensor] T: %.1f, H: %.1f, F: %d\n", g_state.temperature,
           g_state.humidity, g_state.flame_detected);

    vTaskDelay(pdMS_TO_TICKS(1000));
  }
}

// Callback for TCP connection
static err_t http_sent_callback(void *arg, struct tcp_pcb *tpcb, u16_t len) {
  printf("[Network] Data sent successfully\n");
  tcp_close(tpcb);
  return ERR_OK;
}

static err_t http_connected_callback(void *arg, struct tcp_pcb *tpcb,
                                     err_t err) {
  if (err != ERR_OK) {
    printf("[Network] Connection error: %d\n", err);
    return err;
  }

  char body[128];
  snprintf(body, sizeof(body),
           "{\"temperature\": %.2f, \"humidity\": %.2f, \"flame\": %d, "
           "\"device_id\": \"pico_01\"}",
           g_state.temperature, g_state.humidity, g_state.flame_detected);

  char request[256];
  snprintf(request, sizeof(request),
           "POST /predict HTTP/1.1\r\n"
           "Host: %s\r\n"
           "Content-Type: application/json\r\n"
           "Content-Length: %d\r\n"
           "Connection: close\r\n\r\n%s",
           SERVER_IP, (int)strlen(body), body);

  tcp_sent(tpcb, http_sent_callback);
  err_t write_err =
      tcp_write(tpcb, request, strlen(request), TCP_WRITE_FLAG_COPY);
  if (write_err != ERR_OK) {
    printf("[Network] Write error: %d\n", write_err);
  }
  tcp_output(tpcb);
  return ERR_OK;
}

// Task: Cloud Communication
void Task_Network(void *pvParameters) {
  ip_addr_t server_addr;
  ipaddr_aton(SERVER_IP, &server_addr);

  while (1) {
    if (cyw43_tcpip_link_status(&cyw43_state, CYW43_ITF_STA) == CYW43_LINK_UP) {
      struct tcp_pcb *pcb = tcp_new_ip_type(IPADDR_TYPE_V4);
      if (pcb) {
        printf("[Network] Connecting to %s:%d...\n", SERVER_IP, SERVER_PORT);
        err_t err = tcp_connect(pcb, &server_addr, SERVER_PORT,
                                http_connected_callback);
        if (err != ERR_OK) {
          printf("[Network] Failed to initiate connection: %d\n", err);
          tcp_abort(pcb);
        }
      }
    } else {
      printf("[Network] WiFi not connected. Waiting...\n");
    }

    vTaskDelay(pdMS_TO_TICKS(5000));
  }
}

// Task: Local Status (LED/Buzzer)
void Task_Status(void *pvParameters) {
  while (1) {
    if (g_state.alarm_status) {
      cyw43_arch_gpio_put(LED_PIN, 1);
      gpio_put(BUZZER_PIN, 1);
      vTaskDelay(pdMS_TO_TICKS(100));
      cyw43_arch_gpio_put(LED_PIN, 0);
      gpio_put(BUZZER_PIN, 0);
      vTaskDelay(pdMS_TO_TICKS(100));
    } else {
      cyw43_arch_gpio_put(LED_PIN, 0);
      gpio_put(BUZZER_PIN, 0);
      vTaskDelay(pdMS_TO_TICKS(1000));
    }
  }
}

int main() {
  stdio_init_all();

  // WiFi init
  if (cyw43_arch_init()) {
    printf("WiFi init failed\n");
    return -1;
  }
  cyw43_arch_enable_sta_mode();

  printf("Connecting to WiFi: %s...\n", WIFI_SSID);
  if (cyw43_arch_wifi_connect_timeout_ms(WIFI_SSID, WIFI_PASS,
                                         CYW43_AUTH_WPA2_AES_PSK, 30000)) {
    printf("Failed to connect.\n");
  } else {
    printf("Connected.\n");
  }

  // I2C init
  i2c_init(I2C_PORT, 100 * 1000);
  gpio_set_function(SDA_PIN, GPIO_FUNC_I2C);
  gpio_set_function(SCL_PIN, GPIO_FUNC_I2C);
  gpio_pull_up(SDA_PIN);
  gpio_pull_up(SCL_PIN);

  // GPIO init
  gpio_init(FLAME_SENSOR_PIN);
  gpio_set_dir(FLAME_SENSOR_PIN, GPIO_IN);
  gpio_pull_up(FLAME_SENSOR_PIN);

  gpio_init(BUZZER_PIN);
  gpio_set_dir(BUZZER_PIN, GPIO_OUT);

  // Sensor init
  if (!aht10_init(I2C_PORT)) {
    printf("AHT10 init failed\n");
  }

  printf("Starting FreeRTOS Scheduler...\n");

  // Create Tasks
  xTaskCreate(Task_Sensor, "Sensor", 256, NULL, 1, NULL);
  xTaskCreate(Task_Network, "Network", 512, NULL, 1, NULL);
  xTaskCreate(Task_Status, "Status", 128, NULL, 1, NULL);

  // Start Scheduler
  vTaskStartScheduler();

  while (1) {
    // Should never reach here
  }
}
