var Clay = require("@rebble/clay");
var clayConfig = require("./config");

// The config page serializes this function as-is: it must be self-contained.
// Adds the show/hide button BELOW the password box after the page is built
// (standard DOM, no reliance on Clay's internal library).
// The label shows the action that will run when clicked.
var clayCustomFn = function (m) {
  var clayConfig = this;
  clayConfig.on(clayConfig.EVENTS.AFTER_BUILD, function () {
    var input = document.querySelector('input[type="password"]');
    if (!input) return;
    var button = document.createElement("button");
    button.type = "button";
    button.textContent = "Ver / Show";
    button.style.cssText =
      "display:block;width:fit-content;margin:8px auto 12px;padding:10px 28px;font-size:15px;border-radius:6px;";
    var component = input.closest(".component-input") || input.parentNode;
    var description = component.querySelector(".description");
    component.insertBefore(button, description || null);
    button.addEventListener("click", function () {
      var visible = input.getAttribute("type") === "text";
      input.setAttribute("type", visible ? "password" : "text");
      button.textContent = visible ? "Ver / Show" : "Ocultar / Hide";
    });
    // Clay forces uppercase on headings (text-transform: uppercase);
    // the title must read exactly as "AImigo".
    var headings = document.querySelectorAll(
      ".component-heading h1, .component-heading h2, .component-heading h3, .component-heading h4"
    );
    for (var i = 0; i < headings.length; i++) {
      if (headings[i].textContent === "AImigo") {
        headings[i].style.textTransform = "none";
      }
    }
    addVoiceSampleButtons(input);
  });

  // Voice sample test: each voice option gets a play button that synthesizes
  // a short greeting with the NaN API using the API key typed on this page
  // (api.nan.builders sends access-control-allow-origin: *, so the settings
  // webview can call it directly; no audio is bundled or stored).
  function addVoiceSampleButtons(keyInput) {
    var SAMPLES = {
      ef_dora: ["ES · Dora (F)", "Hola, ¿en qué te puedo ayudar?"],
      em_alex: ["ES · Alex (M)", "Hola, ¿en qué te puedo ayudar?"],
      em_santa: ["ES · Santa (M)", "Hola, ¿en qué te puedo ayudar?"],
      af_heart: ["EN · Heart (F)", "Hi, how can I help you?"],
      am_michael: ["EN · Michael (M)", "Hi, how can I help you?"],
      ff_siwis: ["FR · Siwis (F)", "Bonjour, comment puis-je vous aider ?"],
      if_sara: ["IT · Sara (F)", "Ciao, come posso aiutarti?"],
      im_nicola: ["IT · Nicola (M)", "Ciao, come posso aiutarti?"],
      pf_dora: ["PT · Dora (F)", "Olá, como posso ajudar?"],
      pm_alex: ["PT · Alex (M)", "Olá, como posso ajudar?"]
    };
    var API_URL = "https://api.nan.builders/v1/audio/speech";
    var playing = null;
    var playingUrl = null;
    var playingButton = null;

    // ~0.1 s silent WAV, built at runtime: playing it inside the click
    // gesture "unlocks" audio in iOS/Android webviews, which only allow
    // play() within a user gesture; the same element then replays the
    // sample once it has been downloaded.
    function silenceWav() {
      var frames = 800;
      var buf = new ArrayBuffer(44 + frames * 2);
      var dv = new DataView(buf);
      function str(offset, text) {
        for (var i = 0; i < text.length; i++)
          dv.setUint8(offset + i, text.charCodeAt(i));
      }
      str(0, "RIFF");
      dv.setUint32(4, 36 + frames * 2, true);
      str(8, "WAVE");
      str(12, "fmt ");
      dv.setUint32(16, 16, true);
      dv.setUint16(20, 1, true);
      dv.setUint16(22, 1, true);
      dv.setUint32(24, 8000, true);
      dv.setUint32(28, 16000, true);
      dv.setUint16(32, 2, true);
      dv.setUint16(34, 16, true);
      str(36, "data");
      dv.setUint32(40, frames * 2, true);
      return buf;
    }

    function stopSample() {
      if (playing) {
        playing.pause();
        playing = null;
      }
      if (playingUrl) {
        URL.revokeObjectURL(playingUrl);
        playingUrl = null;
      }
      if (playingButton) {
        playingButton.textContent = "▶\uFE0E";
        playingButton = null;
      }
    }

    function requestSample(voice, format, callback) {
      var request = new XMLHttpRequest();
      request.open("POST", API_URL);
      request.timeout = 30000;
      request.setRequestHeader(
        "Authorization",
        "Bearer " + keyInput.value.trim()
      );
      request.setRequestHeader("Content-Type", "application/json");
      request.responseType = "arraybuffer";
      request.onload = function () {
        if (request.status >= 200 && request.status < 300) {
          callback(null, request.response);
        } else {
          var detail = "";
          try {
            if (typeof request.responseText === "string")
              detail = ": " + request.responseText.slice(0, 120);
          } catch (e) {}
          callback(new Error("HTTP " + request.status + detail));
        }
      };
      request.onerror = function () {
        callback(new Error("network error"));
      };
      request.ontimeout = function () {
        callback(new Error("timeout"));
      };
      request.send(
        JSON.stringify({
          model: "kokoro",
          voice: voice,
          input: SAMPLES[voice][1],
          response_format: format
        })
      );
    }

    function playSample(voice, button) {
      if (!keyInput || !keyInput.value.trim()) {
        alert(
          "Introduce tu API key de NaN arriba para probar las voces.\n" +
            "Enter your NaN API key above to test the voices."
        );
        return;
      }
      stopSample();
      button.textContent = "…";
      // Audio element unlocked by playing silence within this gesture.
      var audio = new Audio();
      var unlockUrl = URL.createObjectURL(
        new Blob([silenceWav()], { type: "audio/wav" })
      );
      playingUrl = unlockUrl;
      playing = audio;
      playingButton = button;
      try {
        var unlock = audio.play();
        if (unlock && unlock.catch) unlock.catch(function () {});
      } catch (e) {}
      var attempt = function (format, mime, failed) {
        requestSample(voice, format, function (err, buffer) {
          if (playing !== audio) return; // superseded by a newer request
          if (err) {
            if (!failed) return attempt("wav", "audio/wav", true);
            stopSample();
            alert("Error (" + SAMPLES[voice][0] + "): " + err.message);
            return;
          }
          try {
            if (unlockUrl) {
              URL.revokeObjectURL(unlockUrl);
              unlockUrl = null;
            }
            playingUrl = URL.createObjectURL(
              new Blob([buffer], { type: mime })
            );
            audio.onended = stopSample;
            audio.src = playingUrl;
            var playback = audio.play();
            if (playback && playback.catch)
              playback.catch(function () {
                stopSample();
                alert(
                  "No se pudo reproducir el audio en esta página.\n" +
                    "Audio could not be played on this page."
                );
              });
            // Plain ASCII "X": no emoji, renders identically everywhere.
            button.textContent = "X";
          } catch (e) {
            stopSample();
            alert("Error: " + e);
          }
        });
      };
      attempt("mp3", "audio/mpeg", false);
    }

    for (var voice in SAMPLES) {
      if (!SAMPLES.hasOwnProperty(voice)) continue;
      var radio = document.querySelector(
        '.component-radio input[type="radio"][value="' + voice + '"]'
      );
      if (!radio) continue;
      (function (voice, optionLabel) {
        // A <button> cannot be used here: it would become the label's
        // labeled control (first labelable element in tree order) and
        // steal every tap from the radio input. A <span role="button">
        // is not labelable, so taps on the row keep selecting the radio
        // and the span's own handler (with preventDefault) only tests
        // the voice.
        var playControl = document.createElement("span");
        playControl.setAttribute("role", "button");
        playControl.textContent = "▶\uFE0E";
        playControl.title = "Probar / Test";
        playControl.setAttribute("aria-label", "Probar voz " + optionLabel);
        playControl.style.cssText =
          "display:inline-flex;align-items:center;justify-content:center;" +
          "align-self:center;flex:0 0 auto;margin:0 8px 0 8px;width:28px;" +
          "height:28px;padding:0;font-size:12px;line-height:1;" +
          "border-radius:50%;cursor:pointer;background-color:#999;color:#fff;";
        playControl.addEventListener("click", function (event) {
          // Do not let the surrounding <label> select the radio option.
          event.preventDefault();
          event.stopPropagation();
          if (playControl.textContent === "X") {
            stopSample();
            return;
          }
          playSample(voice, playControl);
        });
        // Keep the span out of the "input ... <i>" chain so the
        // "input:checked + i" selector that paints the radio circle
        // keeps matching; inside the text span it also stays next to
        // the option name for any flex layout.
        var textSpan = radio.parentNode.querySelector(".label");
        textSpan.appendChild(playControl);
      })(voice, SAMPLES[voice][0]);
    }
  }
};

var clay = new Clay(clayConfig, clayCustomFn);

var API_BASE = "https://api.nan.builders/v1";
var CHAT_MODEL = "gemma4";
var CHAT_MODELS = ["gemma4", "qwen3.6"];
var TARGET_RATE = 8000;
var CHUNK_SIZE = 460;
var SPEAKER_PLATFORMS = ["emery", "flint"];

var LANG_NAMES = {
  es: "español",
  en: "English",
  fr: "français",
  it: "italiano",
  pt: "português"
};
var VOICE_DEFAULTS = {
  es: "em_alex",
  en: "af_heart",
  fr: "ff_siwis",
  it: "if_sara",
  pt: "pf_dora"
};

var STRINGS = {
  es: {
    noKey: "Configura tu API key en Ajustes",
    consulting: "Consultando…",
    generating: "Generando voz…",
    ready: "SELECT: nueva pregunta",
    error: "Error: ",
    ttsError: "TTS: ",
    emptyAnswer: "Respuesta vacía",
    invalidAnswer: "Respuesta inválida",
    network: "Error de red",
    timeout: "Tiempo agotado",
    notBinary: "Motor JS sin arraybuffer",
    audioInvalid: "Audio ilegible",
    audioEmpty: "Audio vacío",
    transferFailed: "Transferencia de audio fallida",
    cantStart: "No se pudo iniciar el audio"
  },
  en: {
    noKey: "Set your API key in Settings",
    consulting: "Consulting…",
    generating: "Generating voice…",
    ready: "SELECT: new question",
    error: "Error: ",
    ttsError: "TTS: ",
    emptyAnswer: "Empty answer",
    invalidAnswer: "Invalid answer",
    network: "Network error",
    timeout: "Timed out",
    notBinary: "JS engine without arraybuffer",
    audioInvalid: "Unreadable audio",
    audioEmpty: "Empty audio",
    transferFailed: "Audio transfer failed",
    cantStart: "Could not start audio"
  },
  fr: {
    noKey: "Configurez votre clé API dans Réglages",
    consulting: "Consultation…",
    generating: "Génération de la voix…",
    ready: "SELECT : nouvelle question",
    error: "Erreur : ",
    ttsError: "TTS : ",
    emptyAnswer: "Réponse vide",
    invalidAnswer: "Réponse invalide",
    network: "Erreur réseau",
    timeout: "Délai dépassé",
    notBinary: "Moteur JS sans arraybuffer",
    audioInvalid: "Audio illisible",
    audioEmpty: "Audio vide",
    transferFailed: "Échec du transfert audio",
    cantStart: "Impossible de démarrer l’audio"
  },
  it: {
    noKey: "Configura la tua API key nelle Impostazioni",
    consulting: "Consultazione…",
    generating: "Generazione voce…",
    ready: "SELECT: nuova domanda",
    error: "Errore: ",
    ttsError: "TTS: ",
    emptyAnswer: "Risposta vuota",
    invalidAnswer: "Risposta non valida",
    network: "Errore di rete",
    timeout: "Tempo scaduto",
    notBinary: "Motore JS senza arraybuffer",
    audioInvalid: "Audio illeggibile",
    audioEmpty: "Audio vuoto",
    transferFailed: "Trasferimento audio non riuscito",
    cantStart: "Impossibile avviare l’audio"
  },
  pt: {
    noKey: "Configure sua API key nos Ajustes",
    consulting: "Consultando…",
    generating: "Gerando voz…",
    ready: "SELECT: nova pergunta",
    error: "Erro: ",
    ttsError: "TTS: ",
    emptyAnswer: "Resposta vazia",
    invalidAnswer: "Resposta inválida",
    network: "Erro de rede",
    timeout: "Tempo esgotado",
    notBinary: "Motor JS sem arraybuffer",
    audioInvalid: "Áudio ilegível",
    audioEmpty: "Áudio vazio",
    transferFailed: "Falha na transferência de áudio",
    cantStart: "Não foi possível iniciar o áudio"
  }
};

function log(message) {
  console.log("[AImigo] " + message);
}

function getSettings() {
  // Clay persiste un objeto plano {messageKey: valor} bajo 'clay-settings'.
  try {
    var raw = localStorage.getItem("clay-settings");
    if (raw) return JSON.parse(raw) || {};
  } catch (e) {
    log("localStorage failed: " + e);
  }
  return {};
}

function lang() {
  var l = String(getSettings().Language || "es");
  return STRINGS[l] ? l : "en";
}

function t(key) {
  var s = STRINGS[lang()] || STRINGS.en;
  return s[key] || key;
}

function apiKey() {
  var key = getSettings().ApiKey;
  return typeof key === "string" ? key.trim() : "";
}

function sendStatus(text) {
  Pebble.sendAppMessage(
    { ResStatus: String(text).slice(0, 60) },
    null,
    function (e) {
      log("status nack: " + JSON.stringify(e));
    }
  );
}

function sendAnswer(text) {
  Pebble.sendAppMessage(
    { ResAnswer: String(text).slice(0, 480) },
    null,
    function (e) {
      log("answer nack: " + JSON.stringify(e));
    }
  );
}

function hasSpeaker() {
  try {
    var info = Pebble.getActiveWatchInfo();
    return SPEAKER_PLATFORMS.indexOf(info.platform) !== -1;
  } catch (e) {
    return false;
  }
}

function xhr(method, url, headers, body, responseType, timeoutMs, callback) {
  var request = new XMLHttpRequest();
  request.open(method, url);
  request.timeout = timeoutMs;
  if (headers) {
    for (var name in headers) {
      if (headers.hasOwnProperty(name))
        request.setRequestHeader(name, headers[name]);
    }
  }
  try {
    request.responseType = responseType;
  } catch (e) {
    log("responseType not supported: " + e);
  }
  request.onload = function () {
    if (request.status >= 200 && request.status < 300) {
      callback(null, request.response);
    } else {
      var detail = "";
      try {
        if (typeof request.responseText === "string")
          detail = ": " + request.responseText.slice(0, 80);
      } catch (e) {}
      callback(new Error("HTTP " + request.status + detail));
    }
  };
  request.onerror = function () {
    callback(new Error(t("network")));
  };
  request.ontimeout = function () {
    callback(new Error(t("timeout")));
  };
  if (body) {
    request.send(body);
  } else {
    request.send();
  }
}

function sanitize(text) {
  var value = String(text || "");
  value = value
    .replace(/[*#_`>]/g, "")
    .replace(/\s+/g, " ")
    .trim();
  return value.slice(0, 280);
}

function chatModel() {
  var m = getSettings().Model;
  return CHAT_MODELS.indexOf(m) !== -1 ? m : CHAT_MODEL;
}

function chat(question, key, callback) {
  var body = JSON.stringify({
    model: chatModel(),
    messages: [
      {
        role: "system",
        content:
          "You are AImigo, an assistant on a Pebble watch. Always respond in " +
          LANG_NAMES[lang()] +
          " briefly and directly: maximum 20 words. No markdown, no emojis, no lists. " +
          "If you do not know the answer, say so in a few words."
      },
      { role: "user", content: question }
    ],
    max_tokens: 100,
    reasoning_effort: "none"
  });
  xhr(
    "POST",
    API_BASE + "/chat/completions",
    {
      Authorization: "Bearer " + key,
      "Content-Type": "application/json"
    },
    body,
    "text",
    20000,
    function (err, res) {
      if (err) return callback(err);
      try {
        var data = JSON.parse(res);
        var choice = data && data.choices && data.choices[0];
        var content = choice && choice.message && choice.message.content;
        if (!content) throw new Error(t("emptyAnswer"));
        callback(null, sanitize(content));
      } catch (e) {
        callback(new Error(t("invalidAnswer")));
      }
    }
  );
}

function ttsFormat(text, voice, key, format, callback) {
  var body = JSON.stringify({
    model: "kokoro",
    voice: voice,
    input: text,
    response_format: format,
    speed: 1.0
  });
  xhr(
    "POST",
    API_BASE + "/audio/speech",
    {
      Authorization: "Bearer " + key,
      "Content-Type": "application/json"
    },
    body,
    "arraybuffer",
    30000,
    function (err, res) {
      if (err) return callback(err);
      var buf = res instanceof ArrayBuffer ? res : null;
      if (!buf && typeof res === "string") {
        // Engine without arraybuffer: binary corrupted by UTF-8; unrecoverable.
        return callback(new Error(t("notBinary")));
      }
      callback(null, buf);
    }
  );
}

function tts(text, voice, key, callback) {
  ttsFormat(text, voice, key, "wav", function (err, buf) {
    if (err) {
      log("wav failed (" + err.message + "), trying pcm");
      ttsFormat(text, voice, key, "pcm", callback);
    } else {
      callback(null, buf);
    }
  });
}

function decodeWav(buf) {
  var dv = new DataView(buf);
  if (buf.byteLength < 44) return null;
  if (dv.getUint32(0, true) !== 0x52494646) return null; // 'RIFF'
  if (dv.getUint32(8, true) !== 0x57415645) return null; // 'WAVE'
  var offset = 12;
  var fmt = null;
  var dataOffset = -1;
  var dataSize = 0;
  while (offset + 8 <= buf.byteLength) {
    var id = dv.getUint32(offset, true);
    var size = dv.getUint32(offset + 4, true);
    if (id === 0x666d7420) {
      // 'fmt '
      fmt = {
        channels: dv.getUint16(offset + 10, true),
        sampleRate: dv.getUint32(offset + 12, true),
        bits: dv.getUint16(offset + 22, true)
      };
    } else if (id === 0x64617461) {
      // 'data'
      dataOffset = offset + 8;
      dataSize = size;
      break;
    }
    offset += 8 + size + (size % 2);
  }
  if (!fmt || dataOffset < 0 || fmt.bits !== 16) return null;
  var frames = Math.floor(dataSize / (2 * fmt.channels));
  return {
    dv: dv,
    channels: fmt.channels,
    sampleRate: fmt.sampleRate,
    dataOffset: dataOffset,
    numSamples: frames
  };
}

function downsample(wav, buf) {
  var dv = wav ? wav.dv : new DataView(buf);
  var channels = wav ? wav.channels : 1;
  var sourceRate = wav ? wav.sampleRate : 24000;
  var total = wav ? wav.numSamples * channels : Math.floor(buf.byteLength / 2);
  var factor = Math.max(1, Math.round(sourceRate / TARGET_RATE));
  var frames = Math.floor(total / channels);
  var count = Math.floor(frames / factor);
  var sampleAt = wav
    ? function (i) {
        return dv.getInt16(wav.dataOffset + i * 2, true);
      }
    : function (i) {
        return dv.getInt16(i * 2, true);
      };
  var mono = new Int16Array(count);
  for (var o = 0, s = 0; o < count; o++, s += factor) {
    var acc = 0;
    for (var j = 0; j < factor; j++) {
      var idx = (s + j) * channels;
      if (idx + channels > total) idx = total - channels;
      if (idx < 0) idx = 0;
      acc += sampleAt(idx);
    }
    var m = Math.round(acc / factor);
    if (m > 32767) m = 32767;
    if (m < -32768) m = -32768;
    mono[o] = m;
  }
  // Maximum volume: normalize the peak (up to 8x) and apply a strong
  // boost (~+8 dB) with soft limiting to avoid hard clipping.
  var peak = 0;
  for (o = 0; o < count; o++) {
    var a = mono[o] < 0 ? -mono[o] : mono[o];
    if (a > peak) peak = a;
  }
  var gain = peak > 0 ? Math.min(32767 / peak, 8) : 1;
  var BOOST = 2.5;
  var KNEE = 12000;
  var out = new Uint8Array(count);
  for (o = 0; o < count; o++) {
    var v = mono[o] * gain * BOOST;
    if (v > KNEE) v = KNEE + (v - KNEE) * 0.25;
    else if (v < -KNEE) v = -KNEE + (v + KNEE) * 0.25;
    if (v > 32767) v = 32767;
    else if (v < -32767) v = -32767;
    var s8 = Math.round(v / 256);
    if (s8 > 127) s8 = 127;
    if (s8 < -128) s8 = -128;
    out[o] = s8 & 0xff;
  }
  return out;
}

function sliceToArray(bytes, start, end) {
  var arr = new Array(end - start);
  for (var k = start; k < end; k++) arr[k - start] = bytes[k];
  return arr;
}

function streamAudio(pcm) {
  var total = Math.ceil(pcm.length / CHUNK_SIZE);
  log("audio ready: " + pcm.length + " bytes in " + total + " chunks");

  Pebble.sendAppMessage(
    { AudioMeta: pcm.length },
    function () {
      sendChunk(0, 0);
    },
    function (e) {
      log("meta nack: " + JSON.stringify(e));
      sendStatus(t("cantStart"));
    }
  );

  function sendChunk(index, retries) {
    if (index >= total) {
      Pebble.sendAppMessage({ AudioEnd: total }, null, function (e) {
        Pebble.sendAppMessage({ AudioEnd: total }, null, null);
      });
      return;
    }
    var start = index * CHUNK_SIZE;
    var end = Math.min(start + CHUNK_SIZE, pcm.length);
    var payload = {
      AudioIndex: index,
      AudioChunk: sliceToArray(pcm, start, end)
    };
    Pebble.sendAppMessage(
      payload,
      function () {
        sendChunk(index + 1, 0);
      },
      function (e) {
        log("chunk " + index + " nack: " + JSON.stringify(e));
        if (retries >= 3) {
          sendStatus(t("transferFailed"));
          return;
        }
        sendChunk(index, retries + 1);
      }
    );
  }
}

function handleQuestion(question) {
  var key = apiKey();
  if (!key) {
    sendStatus(t("noKey"));
    return;
  }
  sendStatus(t("consulting"));
  chat(question, key, function (err, answer) {
    if (err) {
      sendStatus(t("error") + err.message);
      return;
    }
    sendAnswer(answer);
    if (!hasSpeaker()) {
      sendStatus(t("ready"));
      return;
    }
    sendStatus(t("generating"));
    var voice = getSettings().Voice || VOICE_DEFAULTS[lang()] || "ef_dora";
    tts(answer, voice, key, function (err2, buf) {
      if (err2) {
        sendStatus(t("ttsError") + err2.message);
        return;
      }
      var pcm = null;
      try {
        var wav = decodeWav(buf);
        pcm = downsample(wav, buf);
      } catch (e3) {
        sendStatus(t("audioInvalid"));
        return;
      }
      if (!pcm || pcm.length === 0) {
        sendStatus(t("audioEmpty"));
        return;
      }
      streamAudio(pcm);
    });
  });
}

Pebble.addEventListener("appmessage", function (e) {
  var payload = e.payload || {};
  var question = payload.ReqQuestion;
  if (typeof question === "string" && question.length > 0) {
    handleQuestion(question);
  }
});

log("started");
