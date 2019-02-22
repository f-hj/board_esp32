#ifndef BUZZER_H
#define BUZZER_H

#ifdef __cplusplus
extern "C" {
#endif


void init_buzz();

void buzz_short(void *pvParameters);
void buzz_double_short(void *pvParameters);
void buzz_triple_short(void *pvParameters);

void buzz_triple_long(void *pvParameters);


#ifdef __cplusplus
};
#endif

#endif