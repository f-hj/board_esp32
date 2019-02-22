#ifndef LED_C_CONNECTOR_H 
#define LED_C_CONNECTOR_H 

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdlib.h>

void init_led();
void set_front_white();
void set_front_color(uint8_t r, uint8_t g, uint8_t b);

void set_back_color(uint8_t r, uint8_t g, uint8_t b);
void set_back_first(uint8_t r, uint8_t g, uint8_t b);
void set_back_led(uint16_t led, uint8_t r, uint8_t g, uint8_t b);


#ifdef __cplusplus
}
#endif

#endif