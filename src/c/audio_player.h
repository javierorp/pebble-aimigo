#pragma once
#include <pebble.h>

void audio_player_init(void (*on_done)(bool completed_ok));
void audio_player_deinit(void);
bool audio_player_prepare(uint32_t total_bytes);
void audio_player_write_chunk(const uint8_t *data, uint16_t len);
bool audio_player_has_audio(void);
uint32_t audio_player_filled(void);
uint32_t audio_player_capacity(void);
bool audio_player_play(void);
void audio_player_stop(void);
