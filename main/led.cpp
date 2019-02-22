#include "led.h"

#ifdef __cplusplus
extern "C" {
#endif

#include "WS2812.h"

WS2812 back = WS2812(GPIO_NUM_26, 8, RMT_CHANNEL_0);
WS2812 front = WS2812(GPIO_NUM_25, 8, RMT_CHANNEL_1);

void init_led() {
  back.show();
  front.show();
}

void set_front_white() {
  for (uint16_t i = 0 ; i < 8 ; i++) {
    front.setPixel(i, 255, 255, 255);
  }
  front.show();
}

void set_front_color(uint8_t r, uint8_t g, uint8_t b) {
  for (uint16_t i = 0 ; i < 8 ; i++) {
    front.setPixel(i, r, g, b);
  }
  front.show();
}

void set_back_color(uint8_t r, uint8_t g, uint8_t b) {
  for (uint16_t i = 0 ; i < 8 ; i++) {
    back.setPixel(i, r, g, b);
  }
  back.show();
}

void set_back_first(uint8_t r, uint8_t g, uint8_t b) {
  back.setPixel(0, r, g, b);
  for (uint16_t i = 1 ; i < 8 ; i++) {
    back.setPixel(i, 0, 0, 0);
  }
  back.show();
}

void set_back_led(uint16_t led, uint8_t r, uint8_t g, uint8_t b) {
  for (uint16_t i = 0 ; i < 8 ; i++) {
    back.setPixel(i, 0, 0, 0);
  }
  back.setPixel(led, r, g, b);
  back.show();
}

#ifdef __cplusplus
}
#endif