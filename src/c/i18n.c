#include "i18n.h"

#define PERSIST_LANG_KEY 1

static const char *s_strings_es[STR_COUNT] = {
    "SELECT: hablar",
    "Pulsa SELECT y habla con AImigo.",
    "Recibiendo audio…",
    "Reproduciendo…",
    "Mantén UP",
    "Audio interrumpido",
    "Sin memoria para audio",
    "No se pudo enviar al móvil",
    "Audio %d%%",
    "Transcripción cancelada",
    "Transcripción fallida",
    "Demasiados errores de transcripción",
    "No se detectó voz",
    "Sin conexión con el móvil",
    "Dictado desactivado en la app móvil",
    "Error interno del dictado",
    "No se pudo transcribir",
    "Error de dictado",
    "Mensaje perdido (conexión lenta)",
};

static const char *s_strings_en[STR_COUNT] = {
    "SELECT: talk",
    "Press SELECT and talk to AImigo.",
    "Receiving audio…",
    "Playing…",
    "Hold UP",
    "Audio interrupted",
    "No memory for audio",
    "Could not send to phone",
    "Audio %d%%",
    "Transcription cancelled",
    "Transcription failed",
    "Too many transcription errors",
    "No speech detected",
    "No phone connection",
    "Dictation disabled in the phone app",
    "Dictation internal error",
    "Could not transcribe",
    "Dictation error",
    "Message dropped (slow connection)",
};

static bool s_spanish = true;

void i18n_init(void)
{
  char code[3] = {0};
  if (persist_exists(PERSIST_LANG_KEY) && persist_read_string(PERSIST_LANG_KEY, code, sizeof(code)) > 0)
  {
    s_spanish = strncmp(code, "es", 2) == 0;
  }
}

void i18n_set_from_code(const char *code)
{
  s_spanish = (code && strncmp(code, "es", 2) == 0);
  persist_write_string(PERSIST_LANG_KEY, s_spanish ? "es" : "en");
}

const char *i18n_str(StrId id)
{
  return (s_spanish ? s_strings_es : s_strings_en)[id];
}
