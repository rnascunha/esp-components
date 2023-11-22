#include <chrono>

#include "esp_log.h"

#include "sys/error.hpp"
#include "sys/sys.hpp"
#include "sys/time.h"

#include "uc/gpio.hpp"

#include "facility/debounce.hpp"

#define TAG "DEBOUNCE"

#define GPIO_LED    GPIO_NUM_2    // ON board led at ESP32 DevKitC
#define GPIO_BUTTON GPIO_NUM_0    // BOOT button at ESP32 DevKitC

extern "C" void app_main() {
  uc::gpio led(GPIO_LED, GPIO_MODE_INPUT_OUTPUT);
  led.write(1);

  // Must call it if want to use install
  uc::gpio::install_isr();

  using namespace std::chrono_literals;
  facility::debounce_timer btn(uc::gpio(GPIO_BUTTON, GPIO_MODE_INPUT), 2s, [&led](void*) {
    led.write(!led.read());
  });
  btn.start(500ms);

  while (true)
    sys::delay(sys::time::max);
}
