#pragma once
#include <pebble.h>

typedef enum {
  STR_HINT,
  STR_WELCOME,
  STR_RECEIVING,
  STR_PLAYING,
  STR_DONE,
  STR_INTERRUPTED,
  STR_NO_MEMORY,
  STR_SEND_FAILED,
  STR_AUDIO_PCT,
  STR_DICT_REJECTED,
  STR_DICT_FAILED,
  STR_DICT_TOO_MANY,
  STR_DICT_NO_SPEECH,
  STR_DICT_NO_CONN,
  STR_DICT_DISABLED,
  STR_DICT_INTERNAL,
  STR_DICT_RECOGNIZER,
  STR_DICT_GENERIC,
  STR_MSG_DROPPED,
  STR_COUNT
} StrId;

void i18n_init(void);
void i18n_set_from_code(const char *code);
const char *i18n_str(StrId id);
