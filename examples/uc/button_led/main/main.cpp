#include "esp_log.h"
#include "sys/sys.hpp"

#include "sys/task.hpp"

#include "uc/gpio.hpp"

#define TAG "BUTTON_LED"

#define GPIO_LED    GPIO_NUM_2    // ON board led at ESP32 DevKitC
#define GPIO_BUTTON GPIO_NUM_0    // BOOT button at ESP32 DevKitC

extern "C" void app_main() {
  uc::gpio led(GPIO_LED, GPIO_MODE_OUTPUT);
  uc::gpio btn(GPIO_BUTTON, GPIO_MODE_INPUT /* , GPIO_PULLUP_ONLY */);

  auto turn_on_off = [&]{
    led.write(btn.read());
  };

  turn_on_off();

  // Must call it if want to use install
  uc::gpio::install_isr();

  sys::task_handle handler = sys::current_task_handler();
  btn.add_isr(GPIO_INTR_ANYEDGE, [](void* args){
    sys::notify_from_ISR(*static_cast<sys::task_handle*>(args));
  }, &handler);

  while (true) {
    ESP_LOGI(TAG, "Waiting signal");
    sys::notify_wait();
    turn_on_off();
  }
}