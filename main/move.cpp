#ifdef __cplusplus
extern "C" {
#endif


#include "WS2812.h"
#include "move.h"

float current_duty_cycle = 40;

bool degraded = false;

bool lights = true;
bool bt_anim_on = false;
bool back_anim_on = false;
bool back_timeout_on = false;

void back_anim(void *pvParameters) {
  while (back_anim_on) {
    set_back_color(255, 0, 0);
    vTaskDelay(140 / portTICK_PERIOD_MS);
    if (!back_anim_on) break;
    set_back_color(10, 0, 0);
    vTaskDelay(140 / portTICK_PERIOD_MS);
  }
  vTaskDelete(NULL);
}

void back_timeout(void *pvParameters) {
  set_back_color(255, 0, 0);
  vTaskDelay(4000 / portTICK_PERIOD_MS);
  if (back_timeout_on) {
    set_back_color(60, 0, 0);
  }
  back_timeout_on = false;
  vTaskDelete(NULL);
}

int16_t led = 0;
bool bt_anim_clockwise = true;

void bt_anim(void *pvParameters) {
  while (bt_anim_on) {
    if (!bt_anim_on) break;

    if (bt_anim_clockwise) led++; else led--;
    if (led == 8) {
      bt_anim_clockwise = false;
      led = 7;
    } else if (led == -1) {
      bt_anim_clockwise = true;
      led = 0;
    }

    set_back_led(led, 0, 0, 10);
    vTaskDelay(400 / portTICK_PERIOD_MS);
  }
  led = 0;
  vTaskDelete(NULL);
}

void lights_check(float duty_cycle) {
  if (!lights) return;
  bt_anim_on = false;

  // front full white
  set_front_color(255, 255, 255);

  // slowing down
  if (duty_cycle == 40) {
    // back full red
    back_anim_on = false;
    back_timeout_on = true;
    xTaskCreate(back_timeout, "back_timeout", configMINIMAL_STACK_SIZE, NULL, 5, NULL);
  }

  // accelerate
  else if (duty_cycle > 40) {
    // back mid red
    back_anim_on = false;
    back_timeout_on = false;
    set_back_color(60, 0, 0);
  }

  // breaking
  else if (duty_cycle < 40) {
    if (back_anim_on) return;

    back_timeout_on = false;
    back_anim_on = true;
    xTaskCreate(back_anim, "back_anim", configMINIMAL_STACK_SIZE, NULL, 5, NULL);
    // start task
  }
}

void switch_lights(bool l) {
  lights = l;
  lights_check(current_duty_cycle);
}

void real_move_forward(float duty_cycle) {
  if (!degraded) {
    brushed_motor_forward(MCPWM_UNIT_0, MCPWM_TIMER_0, duty_cycle);
    return;
  }

  // TODO: uart command
  // (duty_cycle - 40) / 40
}

void real_move_stop() {
  if (!degraded) {
    brushed_motor_stop(MCPWM_UNIT_0, MCPWM_TIMER_0);
    return;
  }

  // TODO: uart stop
}

// moving or breaking
void move_forward(float duty_cycle) {
  current_duty_cycle = duty_cycle;

  lights_check(duty_cycle);
  real_move_forward(duty_cycle);
}

// stoping everything (free wheel)
void move_stop() {
  real_move_stop();
  set_front_color(0, 0, 0);

  bt_anim_on = true;
  xTaskCreate(bt_anim, "bt_anim", configMINIMAL_STACK_SIZE, NULL, 5, NULL);
}

int lights_state() {
  return lights ? 1 : 0;
}

void switch_mode(bool is_degraded) {
  degraded = is_degraded;

  brushed_motor_stop(MCPWM_UNIT_0, MCPWM_TIMER_0);
  // TODO: uart stop
}


#ifdef __cplusplus
}
#endif