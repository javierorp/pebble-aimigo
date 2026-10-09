#include <pebble.h>
#include "comm.h"
#include "dictation.h"
#include "audio_player.h"
#include "i18n.h"

#define STATUS_MAX 64
#define QUESTION_MAX 256
#define ANSWER_MAX 512
#define HOLD_REPLAY_MS 600
#define TEXT_BOX_MAX_H 20000
#define DESCENDER_PAD 6
#define TITLE_H 24
#define PLAY_W 10
#define PLAY_H 12

#if defined(PBL_ROUND)
#define TITLE_Y 12
#define SCROLL_Y 64
#define BOTTOM_MARGIN 32
#define TEXT_PAD_X 24
#define CONTENT_PAD_V 14
#define DIVIDER_PAD_X TEXT_PAD_X
#else
#define TITLE_Y 0
#define SCROLL_Y 50
#define BOTTOM_MARGIN 8
#define TEXT_PAD_X 8
#define CONTENT_PAD_V 0
#define DIVIDER_PAD_X 0
#endif
#define STATUS_Y (TITLE_Y + TITLE_H + 2)

#if defined(PBL_BW)
#define COLOR_SECONDARY GColorBlack
#else
#define COLOR_SECONDARY GColorDarkGray
#endif

static Window *s_window;
static TextLayer *s_title_layer;
static TextLayer *s_status_layer;
static ScrollLayer *s_scroll_layer;
static TextLayer *s_question_layer;
static Layer *s_divider_layer;
static TextLayer *s_answer_layer;
static Layer *s_play_layer;
static GPath *s_play_path;

static char s_status[STATUS_MAX];
static char s_question[QUESTION_MAX];
static char s_answer[ANSWER_MAX];
static GFont s_title_font;
static GFont s_status_font;
static GFont s_question_font;
static GFont s_answer_font;
static bool s_ui_ready;
static int16_t s_screen_w;
static int16_t s_content_w;
static int16_t s_scroll_h;

static AppTimer *s_hold_timer;
static bool s_hold_fired;
static AppTimer *s_start_timer;

static void prv_size_content(void);

static const GPathInfo s_play_path_info = {
  .num_points = 3,
  .points = (GPoint[]) {{0, 0}, {0, PLAY_H}, {PLAY_W - 1, PLAY_H / 2}},
};

static void prv_update_texts(void) {
  text_layer_set_text(s_status_layer, s_status);
  text_layer_set_text(s_question_layer, s_question);
  text_layer_set_text(s_answer_layer, s_answer);
}

static void prv_set_status(const char *status) {
  strncpy(s_status, status, sizeof(s_status) - 1);
  s_status[sizeof(s_status) - 1] = '\0';
  if (!s_ui_ready) return;
  text_layer_set_text(s_status_layer, s_status);

  bool show_play = strcmp(s_status, i18n_str(STR_DONE)) == 0;
  layer_set_hidden(s_play_layer, !show_play);
  if (show_play) {
    GSize tw = graphics_text_layout_get_content_size(s_status, s_status_font,
                                                     GRect(0, 0, s_screen_w, 40),
                                                     GTextOverflowModeWordWrap,
                                                     GTextAlignmentCenter);
    int16_t tx = (s_screen_w - tw.w) / 2 - 14;
    if (tx < 0) tx = 0;
    layer_set_frame(s_play_layer, GRect(tx, STATUS_Y + (22 - PLAY_H) / 2, PLAY_W, PLAY_H));
  }
}

static void prv_refresh(void) {
  if (!s_ui_ready) return;
  prv_update_texts();
  prv_size_content();
}

static void prv_divider_draw(Layer *layer, GContext *ctx) {
  GRect bounds = layer_get_bounds(layer);
  graphics_context_set_stroke_color(ctx, GColorBlack);
  graphics_draw_line(ctx, GPoint(0, 0), GPoint(bounds.size.w - 1, 0));
}

static void prv_play_draw(Layer *layer, GContext *ctx) {
  graphics_context_set_fill_color(ctx, GColorBlack);
  gpath_draw_filled(ctx, s_play_path);
}

static void prv_size_content(void) {
  bool has_question = s_question[0] != '\0';
  layer_set_hidden(text_layer_get_layer(s_question_layer), !has_question);
  layer_set_hidden(s_divider_layer, !has_question);

  int16_t text_w = s_content_w - 2 * TEXT_PAD_X;
  int16_t y = CONTENT_PAD_V;
  if (has_question) {
    GSize q = graphics_text_layout_get_content_size(s_question, s_question_font,
                                                    GRect(0, 0, text_w, TEXT_BOX_MAX_H),
                                                    GTextOverflowModeWordWrap,
                                                    GTextAlignmentLeft);
    layer_set_frame(text_layer_get_layer(s_question_layer), GRect(TEXT_PAD_X, y, text_w, q.h + DESCENDER_PAD));
    y = q.h + DESCENDER_PAD + 5 + CONTENT_PAD_V;
    layer_set_frame(s_divider_layer, GRect(DIVIDER_PAD_X, y, s_content_w - 2 * DIVIDER_PAD_X, 2));
    y += 9 + CONTENT_PAD_V;
  }

  GSize a = graphics_text_layout_get_content_size(s_answer, s_answer_font,
                                                  GRect(0, 0, text_w, TEXT_BOX_MAX_H),
                                                  GTextOverflowModeWordWrap,
                                                  GTextAlignmentLeft);
  layer_set_frame(text_layer_get_layer(s_answer_layer), GRect(TEXT_PAD_X, y, text_w, a.h + DESCENDER_PAD));

  int16_t content_h = y + a.h + DESCENDER_PAD + 6 + CONTENT_PAD_V;
  if (content_h < s_scroll_h) content_h = s_scroll_h;
  scroll_layer_set_content_size(s_scroll_layer, GSize(s_content_w, content_h));
  scroll_layer_set_content_offset(s_scroll_layer, GPoint(0, 0), false);
}

static void prv_reset_texts(void) {
  strncpy(s_status, i18n_str(STR_HINT), sizeof(s_status) - 1);
  s_status[sizeof(s_status) - 1] = '\0';
  strncpy(s_answer, i18n_str(STR_WELCOME), sizeof(s_answer) - 1);
  s_answer[sizeof(s_answer) - 1] = '\0';
  prv_refresh();
}

static void prv_feedback_start(void) {
#if defined(PBL_SPEAKER)
  speaker_play_tone(880, 80, 60, SpeakerWaveformSine);
#else
  vibes_short_pulse();
#endif
}

static void prv_feedback_done(void) {
#if defined(PBL_SPEAKER)
  speaker_play_tone(1320, 120, 60, SpeakerWaveformSine);
#else
  vibes_double_pulse();
#endif
}

static void prv_audio_done(bool completed_ok) {
  if (completed_ok) {
    prv_set_status(i18n_str(STR_DONE));
    prv_feedback_done();
  } else {
    prv_set_status(i18n_str(STR_INTERRUPTED));
  }
}

static void prv_handle_status(const char *status) {
  prv_set_status(status);
}

static void prv_handle_language(const char *language) {
  i18n_set_from_code(language);
  prv_reset_texts();
}

static void prv_handle_answer(const char *answer) {
  strncpy(s_answer, answer, sizeof(s_answer) - 1);
  s_answer[sizeof(s_answer) - 1] = '\0';
  prv_feedback_start();
  prv_refresh();
}

static void prv_handle_audio_meta(uint32_t total_bytes) {
  if (!audio_player_prepare(total_bytes)) {
    prv_set_status(i18n_str(STR_NO_MEMORY));
    return;
  }
  prv_set_status(i18n_str(STR_RECEIVING));
}

static void prv_handle_chunk(uint32_t index, const uint8_t *data, uint16_t len) {
  audio_player_write_chunk(data, len);
  uint32_t cap = audio_player_capacity();
  if (cap > 0) {
    static char buf[16];
    uint32_t pct = (audio_player_filled() * 100) / cap;
    snprintf(buf, sizeof(buf), i18n_str(STR_AUDIO_PCT), (int)pct);
    prv_set_status(buf);
  }
}

static void prv_handle_audio_end(uint32_t total_chunks) {
  if (audio_player_play()) {
    prv_set_status(i18n_str(STR_PLAYING));
  } else {
    prv_set_status(i18n_str(STR_DONE));
  }
}

static void prv_dictation_result(const char *text, const char *error) {
  if (error) {
    prv_set_status(error);
    return;
  }
  strncpy(s_question, text, sizeof(s_question) - 1);
  s_question[sizeof(s_question) - 1] = '\0';
  s_answer[0] = '\0';
  prv_refresh();
  audio_player_stop();
  if (!comm_send_question(s_question)) {
    prv_set_status(i18n_str(STR_SEND_FAILED));
  }
}

static void prv_select_click(ClickRecognizerRef recognizer, void *context) {
  if (!connection_service_peek_pebble_app_connection()) {
    prv_set_status(i18n_str(STR_DICT_NO_CONN));
    prv_feedback_start();
    return;
  }
  dictation_start();
}

static void prv_auto_start(void *context) {
  s_start_timer = NULL;
  if (connection_service_peek_pebble_app_connection()) {
    dictation_start();
  }
}

static void prv_scroll_down(ClickRecognizerRef recognizer, void *context) {
  scroll_layer_scroll_down_click_handler(recognizer, s_scroll_layer);
}

static void prv_up_click(ClickRecognizerRef recognizer, void *context) {
  if (s_hold_fired) return;
  scroll_layer_scroll_up_click_handler(recognizer, s_scroll_layer);
}

static void prv_hold_fire(void *context) {
  s_hold_timer = NULL;
  s_hold_fired = true;
  if (audio_player_play()) {
    prv_set_status(i18n_str(STR_PLAYING));
  }
}

static void prv_up_down(ClickRecognizerRef recognizer, void *context) {
  s_hold_fired = false;
  if (s_hold_timer) app_timer_cancel(s_hold_timer);
  s_hold_timer = app_timer_register(HOLD_REPLAY_MS, prv_hold_fire, NULL);
}

static void prv_up_up(ClickRecognizerRef recognizer, void *context) {
  if (s_hold_timer) {
    app_timer_cancel(s_hold_timer);
    s_hold_timer = NULL;
  }
}

static void prv_click_config_provider(void *context) {
  window_single_click_subscribe(BUTTON_ID_SELECT, prv_select_click);
  window_single_click_subscribe(BUTTON_ID_UP, prv_up_click);
  window_single_click_subscribe(BUTTON_ID_DOWN, prv_scroll_down);
  window_raw_click_subscribe(BUTTON_ID_UP, prv_up_down, prv_up_up, NULL);
}

static void prv_window_load(Window *window) {
  Layer *root = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(root);
  int16_t w = bounds.size.w;
  int16_t h = bounds.size.h;
  s_content_w = w;
  s_screen_w = w;
  s_scroll_h = h - SCROLL_Y - BOTTOM_MARGIN;

  window_set_background_color(window, GColorWhite);

  s_title_layer = text_layer_create(GRect(0, TITLE_Y, w, TITLE_H));
  text_layer_set_text(s_title_layer, "AImigo");
  s_title_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_AIMIGO_TITLE_18));
  text_layer_set_font(s_title_layer, s_title_font);
  text_layer_set_text_alignment(s_title_layer, GTextAlignmentCenter);
#if defined(PBL_ROUND)
  text_layer_set_text_color(s_title_layer, GColorBlack);
  text_layer_set_background_color(s_title_layer, GColorClear);
#else
  text_layer_set_text_color(s_title_layer, GColorWhite);
  text_layer_set_background_color(s_title_layer, GColorBlack);
#endif
  layer_add_child(root, text_layer_get_layer(s_title_layer));

  s_status_font = fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD);
  s_status_layer = text_layer_create(GRect(0, STATUS_Y, w, 22));
  text_layer_set_font(s_status_layer, s_status_font);
  text_layer_set_text_alignment(s_status_layer, GTextAlignmentCenter);
  text_layer_set_text_color(s_status_layer, COLOR_SECONDARY);
  text_layer_set_background_color(s_status_layer, GColorClear);
  layer_add_child(root, text_layer_get_layer(s_status_layer));

  s_play_path = gpath_create(&s_play_path_info);
  s_play_layer = layer_create(GRect(0, STATUS_Y, PLAY_W, PLAY_H));
  layer_set_update_proc(s_play_layer, prv_play_draw);
  layer_set_hidden(s_play_layer, true);
  layer_add_child(root, s_play_layer);

  s_scroll_layer = scroll_layer_create(GRect(0, SCROLL_Y, s_content_w, s_scroll_h));

  s_question_font = fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD);
  s_question_layer = text_layer_create(GRect(0, 0, s_content_w, 20));
  text_layer_set_font(s_question_layer, s_question_font);
  text_layer_set_text_color(s_question_layer, COLOR_SECONDARY);
  text_layer_set_background_color(s_question_layer, GColorClear);
  scroll_layer_add_child(s_scroll_layer, text_layer_get_layer(s_question_layer));

  s_divider_layer = layer_create(GRect(0, 0, s_content_w, 2));
  layer_set_update_proc(s_divider_layer, prv_divider_draw);
  scroll_layer_add_child(s_scroll_layer, s_divider_layer);

  s_answer_layer = text_layer_create(GRect(0, 0, s_content_w, 20));
#if defined(PBL_DISPLAY_HEIGHT) && PBL_DISPLAY_HEIGHT > 180
  s_answer_font = fonts_get_system_font(FONT_KEY_GOTHIC_28_BOLD);
#else
  s_answer_font = fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD);
#endif
  text_layer_set_font(s_answer_layer, s_answer_font);
  text_layer_set_text_color(s_answer_layer, GColorBlack);
  text_layer_set_background_color(s_answer_layer, GColorClear);
  scroll_layer_add_child(s_scroll_layer, text_layer_get_layer(s_answer_layer));

  layer_add_child(root, scroll_layer_get_layer(s_scroll_layer));

  s_ui_ready = true;
  prv_refresh();

#if defined(PBL_MICROPHONE)
  s_start_timer = app_timer_register(250, prv_auto_start, NULL);
#endif
}

static void prv_window_unload(Window *window) {
  if (s_start_timer) {
    app_timer_cancel(s_start_timer);
    s_start_timer = NULL;
  }
  text_layer_destroy(s_answer_layer);
  layer_destroy(s_divider_layer);
  text_layer_destroy(s_question_layer);
  scroll_layer_destroy(s_scroll_layer);
  layer_destroy(s_play_layer);
  gpath_destroy(s_play_path);
  text_layer_destroy(s_title_layer);
  fonts_unload_custom_font(s_title_font);
  text_layer_destroy(s_status_layer);
  s_ui_ready = false;
}

static void prv_handle_init(void) {
  static const CommHandlers handlers = {
    .on_status = prv_handle_status,
    .on_answer = prv_handle_answer,
    .on_language = prv_handle_language,
    .on_audio_meta = prv_handle_audio_meta,
    .on_chunk = prv_handle_chunk,
    .on_audio_end = prv_handle_audio_end,
  };

  i18n_init();
  comm_init(&handlers);
  audio_player_init(prv_audio_done);
  dictation_init(prv_dictation_result);

  s_question[0] = '\0';
  strncpy(s_status, i18n_str(STR_HINT), sizeof(s_status) - 1);
  s_status[sizeof(s_status) - 1] = '\0';
  strncpy(s_answer, i18n_str(STR_WELCOME), sizeof(s_answer) - 1);
  s_answer[sizeof(s_answer) - 1] = '\0';

  s_window = window_create();
  window_set_click_config_provider(s_window, prv_click_config_provider);
  window_set_window_handlers(s_window, (WindowHandlers) {
    .load = prv_window_load,
    .unload = prv_window_unload,
  });
  window_stack_push(s_window, true);
}

static void prv_handle_deinit(void) {
  if (s_hold_timer) {
    app_timer_cancel(s_hold_timer);
    s_hold_timer = NULL;
  }
  dictation_deinit();
  audio_player_deinit();
  window_destroy(s_window);
}

int main(void) {
  prv_handle_init();
  app_event_loop();
  prv_handle_deinit();
}
