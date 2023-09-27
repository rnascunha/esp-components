/**
 * @file websocket.cpp
 * @author Rafael Cunha (rnascunha@gmail.com)
 * @brief 
 * @version 0.1
 * @date 2023-09-25
 * 
 * @copyright Copyright (c) 2023
 * 
 */
#include "inttypes.h"
#include <cassert>
#include <chrono>

#include "sys/task.hpp"
#include "sys/sys.hpp"

#include "esp_log.h"

#include "esp_ota_ops.h"
// #include "esp_app_format.h"
// #include "esp_flash_partitions.h"
// #include "esp_partition.h"

#include "ota/packet.hpp"
#include "ota/websocket.hpp"

#define OTA_TASK_NAME         "ota_task"
#define OTA_TASK_STACK_SIZE   8192
#define OTA_TASK_PRIORITY     5

static constexpr const char*
TAG = "OTA WS";

namespace ota {

static void ota_task(void* arg) {
  ESP_LOGI(TAG, "OTA task initated");

  info& inf = *((info*)arg);

  /* update handle : set by esp_ota_begin(), must be freed via esp_ota_end() */
  // esp_ota_handle_t update_handle = 0 ;

  ESP_LOGI(TAG, "Starting OTA example task");

  const esp_partition_t *configured = esp_ota_get_boot_partition();
  const esp_partition_t *running = esp_ota_get_running_partition();

  if (configured != running) {
      ESP_LOGW(TAG, "Configured OTA boot partition at offset 0x%08" PRIx32 ", but running from offset 0x%08" PRIx32,
                configured->address, running->address);
      ESP_LOGW(TAG, "(This can happen if either the OTA boot data or preferred boot image become corrupted somehow.)");
  }
  ESP_LOGI(TAG, "Running partition type %d subtype %d (offset 0x%08" PRIx32 ")",
            running->type, running->subtype, running->address);

  const esp_partition_t *update_partition = esp_ota_get_next_update_partition(nullptr);
  assert(update_partition != nullptr);
  ESP_LOGI(TAG, "Writing to partition subtype %d at offset 0x%" PRIx32,
            update_partition->subtype, update_partition->address);

  ESP_LOGI(TAG, "OTA task finished");
  send_state(true, 0, *inf.server);

  using namespace std::chrono_literals;
  sys::delay(1s);

  inf.task = nullptr;
  inf.client.clear();
  vTaskDelete(nullptr);
}

sys::task_handle
wesocket_task(info& info) noexcept {
  sys::task_handle handler;
  xTaskCreate(&ota_task, OTA_TASK_NAME, OTA_TASK_STACK_SIZE, &info, OTA_TASK_PRIORITY, &handler);
  return handler;
}

}  // namespace ota
