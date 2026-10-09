#pragma once
#include <pebble.h>

void dictation_init(void (*on_result)(const char *text, const char *error));
void dictation_start(void);
void dictation_deinit(void);
