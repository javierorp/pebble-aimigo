#include "comm.h"
#include "i18n.h"

static CommHandlers s_handlers;

static void prv_inbox_dropped(AppMessageResult reason, void *context) {
  if (s_handlers.on_status) s_handlers.on_status(i18n_str(STR_MSG_DROPPED));
}

static void prv_inbox_received(DictionaryIterator *iterator, void *context) {
  Tuple *t;
  uint32_t index = 0;

  t = dict_find(iterator, MESSAGE_KEY_AudioIndex);
  if (t) index = t->value->uint32;

  t = dict_find(iterator, MESSAGE_KEY_ResStatus);
  if (t && s_handlers.on_status) {
    s_handlers.on_status(t->value->cstring);
  }

  t = dict_find(iterator, MESSAGE_KEY_Language);
  if (t && s_handlers.on_language) {
    s_handlers.on_language(t->value->cstring);
  }

  t = dict_find(iterator, MESSAGE_KEY_ResAnswer);
  if (t && s_handlers.on_answer) {
    s_handlers.on_answer(t->value->cstring);
  }

  t = dict_find(iterator, MESSAGE_KEY_AudioMeta);
  if (t && s_handlers.on_audio_meta) {
    s_handlers.on_audio_meta(t->value->uint32);
  }

  t = dict_find(iterator, MESSAGE_KEY_AudioChunk);
  if (t && t->length > 0 && s_handlers.on_chunk) {
    s_handlers.on_chunk(index, (const uint8_t *)t->value->data, (uint16_t)t->length);
  }

  t = dict_find(iterator, MESSAGE_KEY_AudioEnd);
  if (t && s_handlers.on_audio_end) {
    s_handlers.on_audio_end(t->value->uint32);
  }
}

void comm_init(const CommHandlers *handlers) {
  s_handlers = *handlers;
  app_message_register_inbox_received(prv_inbox_received);
  app_message_register_inbox_dropped(prv_inbox_dropped);
  app_message_open(app_message_inbox_size_maximum(), app_message_outbox_size_maximum());
}

bool comm_send_question(const char *text) {
  DictionaryIterator *iterator;
  AppMessageResult result = app_message_outbox_begin(&iterator);
  if (result != APP_MSG_OK) return false;
  dict_write_cstring(iterator, MESSAGE_KEY_ReqQuestion, text);
  result = app_message_outbox_send();
  return result == APP_MSG_OK;
}
