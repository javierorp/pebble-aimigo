module.exports = [
  {
    type: "heading",
    defaultValue: "AImigo"
  },
  {
    type: "text",
    defaultValue:
      "[ES] Asistente de voz con IA de NaN (nan.builders). Las respuestas usan un modelo de NaN (elígelo abajo): no tiene acceso a internet, solo conoce sus datos de entrenamiento."
  },
  {
    type: "text",
    defaultValue:
      "[EN] AI voice assistant powered by NaN (nan.builders). Answers use a NaN model (pick it below): it has no internet access, it only knows its training data."
  },
  {
    type: "section",
    items: [
      {
        type: "heading",
        defaultValue: "CONEXIÓN / CONNECTION"
      },
      {
        type: "input",
        messageKey: "ApiKey",
        label: "API key de NaN / NaN API key",
        defaultValue: "",
        attributes: { type: "password" }
      },
      {
        type: "heading",
        defaultValue: "MODELO / MODEL"
      },
      {
        type: "radiogroup",
        messageKey: "Model",
        label: "Modelo de las respuestas / Answer model",
        defaultValue: "gemma4",
        options: [
          { label: "Gemma 4 (recomendado/recommended)", value: "gemma4" },
          { label: "Qwen 3.6", value: "qwen3.6" }
        ]
      },
      {
        type: "heading",
        defaultValue: "IDIOMA / LANGUAGE"
      },
      {
        type: "radiogroup",
        messageKey: "Language",
        label: "Idioma de la app y las respuestas / App and answer language",
        defaultValue: "es",
        options: [
          { label: "Español", value: "es" },
          { label: "English", value: "en" },
          { label: "Français", value: "fr" },
          { label: "Italiano", value: "it" },
          { label: "Português", value: "pt" }
        ]
      },
      {
        type: "heading",
        defaultValue: "VOZ TTS / TTS VOICE"
      },
      {
        type: "radiogroup",
        messageKey: "Voice",
        label: "Voz del asistente / Assistant voice",
        defaultValue: "em_alex",
        description:
          "Pulsa ▶ para escuchar una muestra de cada voz en su idioma " +
          "(usa la API key de arriba).<br>" +
          "Press ▶ to listen to a sample of each voice in its language " +
          "(uses the API key above).",
        options: [
          { label: "ES · Dora (F)", value: "ef_dora" },
          { label: "ES · Alex (M)", value: "em_alex" },
          { label: "ES · Santa (M)", value: "em_santa" },
          { label: "EN · Heart (F)", value: "af_heart" },
          { label: "EN · Michael (M)", value: "am_michael" },
          { label: "FR · Siwis (F)", value: "ff_siwis" },
          { label: "IT · Sara (F)", value: "if_sara" },
          { label: "IT · Nicola (M)", value: "im_nicola" },
          { label: "PT · Dora (F)", value: "pf_dora" },
          { label: "PT · Alex (M)", value: "pm_alex" }
        ]
      }
    ]
  },
  {
    type: "submit",
    defaultValue: "Guardar / Save"
  }
];
