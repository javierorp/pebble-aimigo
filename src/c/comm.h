#pragma once
#include <pebble.h>

typedef struct CommHandlers {
  void (*on_status)(const char *status);
  void (*on_answer)(const char *answer);
  void (*on_language)(const char *language);
  void (*on_audio_meta)(uint32_t total_bytes);
  void (*on_chunk)(uint32_t index, const uint8_t *data, uint16_t len);
  void (*on_audio_end)(uint32_t total_chunks);
} CommHandlers;

void comm_init(const CommHandlers *handlers);
bool comm_send_question(const char *text);
