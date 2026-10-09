#include "audio_player.h"

#if defined(PBL_SPEAKER)

#define AUDIO_MARGIN_BYTES (4 * 1024)
#define PUMP_WRITE_MAX 1024
#define PUMP_WRITE_MIN 128
#define PUMP_STALL_MAX 60

static uint8_t *s_buffer;
static uint32_t s_capacity;
static uint32_t s_filled;
static uint32_t s_position;
static bool s_stream_open;
static bool s_playback_active;
static AppTimer *s_pump_timer;
static uint32_t s_write_size;
static uint32_t s_stall_count;
static void (*s_on_done)(bool completed_ok);

static void prv_report_done(bool completed_ok) {
  if (!s_playback_active) return;
  s_playback_active = false;
  if (s_on_done) s_on_done(completed_ok);
}

static void prv_pump(void *context);

static void prv_schedule_pump(uint32_t delay_ms) {
  if (s_pump_timer) app_timer_cancel(s_pump_timer);
  s_pump_timer = app_timer_register(delay_ms, prv_pump, NULL);
}

static void prv_cancel_pump(void) {
  if (s_pump_timer) {
    app_timer_cancel(s_pump_timer);
    s_pump_timer = NULL;
  }
}

static void prv_speaker_finished(SpeakerFinishReason reason, void *context) {
  if (reason == SpeakerFinishReasonStopped) return;
  s_stream_open = false;
  prv_report_done(reason == SpeakerFinishReasonDone);
}

static void prv_pump(void *context) {
  s_pump_timer = NULL;
  if (!s_stream_open || !s_buffer) return;

  uint32_t remaining = s_filled - s_position;
  if (remaining == 0) {
    s_stream_open = false;
    speaker_stream_close();
    prv_report_done(true);
    return;
  }

  uint32_t to_write = (remaining < s_write_size) ? remaining : s_write_size;
  uint32_t written = speaker_stream_write(s_buffer + s_position, to_write);
  if (written > to_write) written = to_write;
  s_position += written;

  if (written > 0) {
    s_stall_count = 0;
    if (written == to_write && s_write_size < PUMP_WRITE_MAX) s_write_size *= 2;
    prv_schedule_pump(20);
    return;
  }

  if (++s_stall_count >= PUMP_STALL_MAX) {
    APP_LOG(APP_LOG_LEVEL_WARNING, "audio stream stalled, aborting");
    s_stream_open = false;
    speaker_stop();
    prv_report_done(false);
    return;
  }
  if (s_write_size > PUMP_WRITE_MIN) s_write_size /= 2;
  prv_schedule_pump(40);
}

void audio_player_init(void (*on_done)(bool completed_ok)) {
  s_on_done = on_done;
  speaker_set_finish_callback(prv_speaker_finished, NULL);
}

void audio_player_deinit(void) {
  audio_player_stop();
  if (s_buffer) {
    free(s_buffer);
    s_buffer = NULL;
  }
}

bool audio_player_prepare(uint32_t total_bytes) {
  audio_player_stop();
  if (s_buffer) {
    free(s_buffer);
    s_buffer = NULL;
  }
  s_capacity = 0;
  s_filled = 0;
  s_position = 0;
  s_playback_active = false;

  uint32_t available = (uint32_t)heap_bytes_free();
  uint32_t capacity = total_bytes;
  if (capacity > available) {
    if (available <= AUDIO_MARGIN_BYTES) return false;
    capacity = available - AUDIO_MARGIN_BYTES;
  }
  if (capacity == 0) return false;

  s_buffer = malloc(capacity);
  if (!s_buffer) return false;
  s_capacity = capacity;
  return true;
}

void audio_player_write_chunk(const uint8_t *data, uint16_t len) {
  if (!s_buffer) return;
  uint32_t room = s_capacity - s_filled;
  if (len > room) len = (uint16_t)room;
  if (len == 0) return;
  memcpy(s_buffer + s_filled, data, len);
  s_filled += len;
}

bool audio_player_has_audio(void) {
  return s_buffer && s_filled > 0;
}

uint32_t audio_player_filled(void) {
  return s_filled;
}

uint32_t audio_player_capacity(void) {
  return s_capacity;
}

bool audio_player_play(void) {
  if (!s_buffer || s_filled == 0) return false;
  if (s_stream_open) return true;
  speaker_stop();
  if (!speaker_stream_open(SpeakerPcmFormat_8kHz_8bit, 100)) return false;
  s_stream_open = true;
  s_playback_active = true;
  s_position = 0;
  s_write_size = PUMP_WRITE_MAX;
  s_stall_count = 0;
  prv_schedule_pump(10);
  return true;
}

void audio_player_stop(void) {
  prv_cancel_pump();
  s_playback_active = false;
  if (s_stream_open) {
    s_stream_open = false;
    speaker_stop();
  }
}

#else

void audio_player_init(void (*on_done)(bool completed_ok)) {
  (void)on_done;
}

void audio_player_deinit(void) {
}

bool audio_player_prepare(uint32_t total_bytes) {
  (void)total_bytes;
  return true;
}

void audio_player_write_chunk(const uint8_t *data, uint16_t len) {
  (void)data;
  (void)len;
}

bool audio_player_has_audio(void) {
  return false;
}

uint32_t audio_player_filled(void) {
  return 0;
}

uint32_t audio_player_capacity(void) {
  return 0;
}

bool audio_player_play(void) {
  return false;
}

void audio_player_stop(void) {
}

#endif
