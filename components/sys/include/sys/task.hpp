/**
 * @file task.hpp
 * @author Rafael Cunha (rnascunha@gmail.com)
 * @brief 
 * @version 0.1
 * @date 2023-09-07
 * 
 * @copyright Copyright (c) 2023
 * 
 */
#ifndef COMPONENTS_SYS_TASK_HPP__
#define COMPONENTS_SYS_TASK_HPP__

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"

#include <cstdint>
#include <cassert>

#include "sys/time.hpp"

namespace sys {

using task_handle = TaskHandle_t;

inline void
task_delete(task_handle handler = nullptr) noexcept {
  vTaskDelete(handler);
}

inline task_handle
task_create(TaskFunction_t func,
            std::uint32_t stack_size,
            UBaseType_t priority,
            void* parameter = nullptr,
            const char* name = "") noexcept {
  task_handle handler;
  xTaskCreate(func, name, stack_size, parameter, priority, &handler);
  return handler;
}

/**
 * @see https://www.freertos.org/ulTaskNotifyTake.html
 */
std::uint32_t
notify_wait(time::tick_time auto duration, BaseType_t clear_on_exit = pdTRUE) noexcept {
  return ulTaskNotifyTake(clear_on_exit, time::to_ticks(duration));
}

inline std::uint32_t
notify_wait(BaseType_t clear_on_exit = pdTRUE) noexcept {
  return ulTaskNotifyTake(clear_on_exit, time::max);
}


/**
 * @see https://www.freertos.org/xTaskNotifyGive.html
 */
BaseType_t
notify(task_handle handle) noexcept;
BaseType_t
notify(task_handle handle, UBaseType_t index) noexcept;

/**
 * @see https://www.freertos.org/vTaskNotifyGiveFromISR.html 
*/
void
notify_from_ISR(task_handle handle,
                BaseType_t *pxHigherPriorityTaskWoken = nullptr) noexcept;
void
notify_from_ISR(task_handle handle,
                UBaseType_t index,
                BaseType_t *pxHigherPriorityTaskWoken /* = nullptr */) noexcept;

/**
 * Events
 */
class event_group {
 public:
  using handle = EventGroupHandle_t;
  using bits = EventBits_t;

  event_group() noexcept;
  event_group(StaticEventGroup_t*) noexcept;

  ~event_group() noexcept;

  void erase() noexcept;

  bits wait(const bits bits_to_wait,
            time::tick_time auto duration,
            const BaseType_t clear_on_exit = pdTRUE,
            const BaseType_t wait_all_bits = pdFALSE) noexcept {
  assert(handler_ != nullptr && "EventGroup handler cant be NULL");
  return xEventGroupWaitBits(handler_, bits_to_wait, clear_on_exit, wait_all_bits, time::to_ticks(duration));
}

  bits set(const bits) noexcept;
  BaseType_t set_from_ISR(const bits,
                    BaseType_t *pxHigherPriorityTaskWoken = nullptr) noexcept;

  [[nodiscard]] bits
  get() const noexcept;
  [[nodiscard]] bits
  get_from_ISR() const noexcept;

  bits clear(const bits) noexcept;
  BaseType_t clear_from_ISR(const bits) noexcept;

  [[nodiscard]] bool
  is_set(const bits) const noexcept;

  [[nodiscard]] static bool
  is_set(const bits, const bits) noexcept;

 private:
  handle handler_;
};

}  // namespace sys

#endif  // COMPONENTS_SYS_TASK_HPP__
