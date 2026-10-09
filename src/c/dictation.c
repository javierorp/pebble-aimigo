#include "dictation.h"
#include "i18n.h"

#define DICTATION_BUFFER_LEN 256

static DictationSession *s_session;
static char s_buffer[DICTATION_BUFFER_LEN];
static void (*s_on_result)(const char *text, const char *error);

#if defined(PBL_MICROPHONE)

static const char *prv_status_message(DictationSessionStatus status) {
  switch (status) {
    case DictationSessionStatusFailureTranscriptionRejected:
      return i18n_str(STR_DICT_REJECTED);
    case DictationSessionStatusFailureTranscriptionRejectedWithError:
      return i18n_str(STR_DICT_FAILED);
    case DictationSessionStatusFailureSystemAborted:
      return i18n_str(STR_DICT_TOO_MANY);
    case DictationSessionStatusFailureNoSpeechDetected:
      return i18n_str(STR_DICT_NO_SPEECH);
    case DictationSessionStatusFailureConnectivityError:
      return i18n_str(STR_DICT_NO_CONN);
    case DictationSessionStatusFailureDisabled:
      return i18n_str(STR_DICT_DISABLED);
    case DictationSessionStatusFailureInternalError:
      return i18n_str(STR_DICT_INTERNAL);
    case DictationSessionStatusFailureRecognizerError:
      return i18n_str(STR_DICT_RECOGNIZER);
    default:
      return i18n_str(STR_DICT_GENERIC);
  }
}

static void prv_session_callback(DictationSession *session, DictationSessionStatus status,
                                 char *transcription, void *context) {
  if (status == DictationSessionStatusSuccess && transcription) {
    strncpy(s_buffer, transcription, sizeof(s_buffer) - 1);
    s_buffer[sizeof(s_buffer) - 1] = '\0';
    if (s_on_result) s_on_result(s_buffer, NULL);
  } else if (s_on_result) {
    s_on_result(NULL, prv_status_message(status));
  }
}

#endif

void dictation_init(void (*on_result)(const char *text, const char *error)) {
  s_on_result = on_result;
#if defined(PBL_MICROPHONE)
  s_session = dictation_session_create(sizeof(s_buffer), prv_session_callback, NULL);
#endif
}

void dictation_start(void) {
#if defined(PBL_MICROPHONE)
  if (s_session) dictation_session_start(s_session);
#endif
}

void dictation_deinit(void) {
#if defined(PBL_MICROPHONE)
  if (s_session) dictation_session_destroy(s_session);
  s_session = NULL;
#endif
}
