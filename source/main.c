/**
 * @file main.c
 * @brief BatteryHealth3DS - Creado por ElVichoPo
 *        Interfaz Gráfica 2D/3D (citro2d) + Asistente Animado "Bati Bot"
 *        + Canción de Fondo Relajante (con botón ON/OFF) + Sonido "Woupp!"
 *        + Intro 3D con Bati Bot + 10 Consejos + Multilenguaje + Despedida Positiva.
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <math.h>
#include <3ds.h>
#include <citro2d.h>

/* Dimensiones de las pantallas de 3DS */
#define TOP_WIDTH  400.0f
#define TOP_HEIGHT 240.0f
#define BOT_WIDTH  320.0f
#define BOT_HEIGHT 240.0f

/* Historial para la gráfica de voltaje en tiempo real */
#define GRAPH_HISTORY_LEN 60

/* Número total de consejos de Bati Bot */
#define TOTAL_BATI_TIPS 10

/* Paleta de colores estilo Dashboard Moderno (RGBA) */
#define COL_BG_DARK      C2D_Color32(11,  17,  29,  255)
#define COL_BG_HEADER    C2D_Color32(17,  26,  44,  255)
#define COL_CARD_BG      C2D_Color32(21,  32,  52,  255)
#define COL_CARD_ALT     C2D_Color32(27,  40,  64,  255)
#define COL_CARD_BORDER  C2D_Color32(42,  62,  96,  255)
#define COL_BUBBLE_BG    C2D_Color32(25,  42,  68,  255)
#define COL_TEXT_MAIN    C2D_Color32(240, 246, 255, 255)
#define COL_TEXT_MUTED   C2D_Color32(148, 168, 198, 255)
#define COL_ACCENT_CYAN  C2D_Color32(0,   214, 245, 255)
#define COL_CYAN_SHADOW  C2D_Color32(0,   88,  118, 255)
#define COL_GREEN_OK     C2D_Color32(24,  218, 126, 255)
#define COL_GREEN_DARK   C2D_Color32(12,  95,  56,  255)
#define COL_YELLOW_WARN  C2D_Color32(255, 196, 46,  255)
#define COL_YELLOW_DARK  C2D_Color32(110, 82,  16,  255)
#define COL_RED_CRIT     C2D_Color32(255, 76,  92,  255)
#define COL_RED_DARK     C2D_Color32(112, 26,  36,  255)
#define COL_GRAPH_GRID   C2D_Color32(34,  50,  78,  255)
#define COL_BOT_CHEEK    C2D_Color32(255, 120, 160, 255)

/* Idiomas soportados */
typedef enum {
    LANG_ES = 0,
    LANG_EN = 1,
    LANG_PT = 2,
    LANG_FR = 3,
    LANG_COUNT = 4
} AppLang;

/* Efectos de sonido del sistema */
typedef enum {
    SFX_WOUPP = 0,
    SFX_SUCCESS = 1,
    SFX_ALERT = 2,
    SFX_GOODBYE = 3,
    SFX_STARTUP = 4,
    SFX_COUNT = 5
} AssistantSfx;

/* Coordenadas de botones interactivos en la pantalla táctil (320x240) */
#define BTN_BGM_X    172.0f
#define BTN_BGM_Y    2.0f
#define BTN_BGM_W    72.0f
#define BTN_BGM_H    22.0f

#define BTN_LANG_X   248.0f
#define BTN_LANG_Y   2.0f
#define BTN_LANG_W   68.0f
#define BTN_LANG_H   22.0f

#define ASST_BOX_X   228.0f
#define ASST_BOX_Y   32.0f
#define ASST_BOX_W   84.0f
#define ASST_BOX_H   106.0f

#define BTN_TEST_X   10.0f
#define BTN_TEST_Y   192.0f
#define BTN_TEST_W   196.0f
#define BTN_TEST_H   40.0f

#define BTN_TIPS_X   214.0f
#define BTN_TIPS_Y   192.0f
#define BTN_TIPS_W   96.0f
#define BTN_TIPS_H   40.0f

/* Especificaciones de consola y batería original */
typedef struct {
    u8 modelId;
    bool isNew3DS;
    const char *consoleName;
    const char *codeName;
    const char *batteryModel;
    u32 capacityMah;
} ConsoleBatterySpec;

/* Estado instantáneo de lectura energética */
typedef struct {
    u8 ptmuBars;
    u8 isCharging;
    bool adapterPlugged;
    bool mcuAvailable;
    u8 mcuRawVoltage;
    u32 voltageMv;
    u8 mcuPercent;
    u8 mcuFwHigh;
    u8 mcuFwLow;
} PowerReading;

/* Resultado del Test de Degradación (Voltage Sag Test) */
typedef struct {
    bool completed;
    bool usedMcuMv;
    bool n3dsTurboUsed;
    u32 restMv;
    u32 minLoadMv;
    s32 sagMv;
    u8 restBars;
    u8 minLoadBars;
    u8 restPercent;
    u8 minLoadPercent;
    u32 iterations;
} SagTestResult;

/* Historial circular de voltajes en mV para graficar */
typedef struct {
    u32 samples[GRAPH_HISTORY_LEN];
    u32 count;
} VoltageGraph;

/* Subsistema de Audio Completo: Efectos + Canción de Fondo Relajante (Ambient Chiptune Lounge) */
#define AUDIO_SAMPLERATE 22050
#define SFX_SAMPLES      7717   /* ~0.35 segundos por efecto */
#define BGM_DURATION_SEC 8      /* Bucle musical armónico de 8 segundos */
#define BGM_SAMPLES      (AUDIO_SAMPLERATE * BGM_DURATION_SEC)

typedef struct {
    bool ndspActive;
    bool csndActive;
    bool bgmEnabled;
    s16 *sfxBuf[SFX_COUNT];
    ndspWaveBuf sfxWave[SFX_COUNT];
    s16 *bgmBuf;
    ndspWaveBuf bgmWave;
} AudioEngine;

/**
 * Sintetiza una canción de fondo (BGM) muy suave y agradable estilo Nintendo eShop / Mii Maker
 * (progresión de acordes Cmaj7 -> Am7 -> Fmaj7 -> G6 con arpegio cristalino suave).
 */
static void synthesizePleasantBgm(s16 *buf, u32 numSamples, bool stereo) {
    /* 16 notas de arpegio a lo largo de 8 segundos (0.5s cada paso) */
    const float arpNotes[16] = {
        523.25f, 659.25f, 783.99f, 987.77f, /* Cmaj7: C5, E5, G5, B5 */
        440.00f, 523.25f, 659.25f, 783.99f, /* Am7  : A4, C5, E5, G5 */
        349.23f, 440.00f, 523.25f, 659.25f, /* Fmaj7: F4, A4, C5, E5 */
        392.00f, 493.88f, 587.33f, 659.25f  /* G6   : G4, B4, D5, E5 */
    };
    const float bassNotes[4] = {
        130.81f, /* C3 */
        110.00f, /* A2 */
        174.61f, /* F3 */
        196.00f  /* G3 */
    };

    for (u32 i = 0; i < numSamples; i++) {
        float t = (float)i / (float)AUDIO_SAMPLERATE;
        int step = ((int)(t * 2.0f)) % 16;
        int chord = (step / 4) % 4;
        float noteTime = fmodf(t, 0.5f);

        float arpFreq  = arpNotes[step];
        float bassFreq = bassNotes[chord];

        /* Envolvente suave tipo campanita cálida para el arpegio */
        float arpEnv = expf(-noteTime * 5.2f) * (noteTime < 0.02f ? (noteTime / 0.02f) : 1.0f);
        float arpSig = 0.70f * sinf(2.0f * (float)M_PI * arpFreq * t) +
                       0.30f * sinf(2.0f * (float)M_PI * arpFreq * 2.0f * t);

        /* Pad cálido de fondo */
        float chordTime = fmodf(t, 2.0f);
        float padEnv = sinf((chordTime / 2.0f) * (float)M_PI);
        float padSig = 0.5f * sinf(2.0f * (float)M_PI * bassFreq * t) +
                       0.5f * sinf(2.0f * (float)M_PI * (bassFreq * 1.5f) * t);

        float mix = (0.55f * arpEnv * arpSig) + (0.35f * padEnv * padSig);
        s16 sample = (s16)(mix * 5200.0f); /* Volumen suave y relajante para no saturar */

        if (stereo) {
            buf[i * 2]     = sample;
            buf[i * 2 + 1] = sample;
        } else {
            buf[i] = sample;
        }
    }
}

static void synthesizeSfxBuffer(s16 *buf, u32 numSamples, AssistantSfx type, bool stereo) {
    for (u32 i = 0; i < numSamples; i++) {
        float t = (float)i / (float)AUDIO_SAMPLERATE;
        float progress = (float)i / (float)numSamples;
        float env = 1.0f;
        if (progress < 0.06f) env = progress / 0.06f;
        else if (progress > 0.68f) env = (1.0f - progress) / 0.32f;

        float phase = 0.0f;
        if (type == SFX_WOUPP) {
            /* Sonido "Woupp!" ascendente alegre al tocar con el lápiz (280 Hz -> 940 Hz) */
            float f0 = 280.0f;
            float f1 = 940.0f;
            float instFreq = f0 + (f1 - f0) * (progress * progress);
            phase = 2.0f * (float)M_PI * instFreq * t;
        } else if (type == SFX_STARTUP) {
            /* Jingle brillante y agradable al iniciar el programa */
            float freq = (progress < 0.25f) ? 523.25f :
                         (progress < 0.50f) ? 659.25f :
                         (progress < 0.75f) ? 783.99f : 1046.50f;
            phase = 2.0f * (float)M_PI * freq * t;
        } else if (type == SFX_SUCCESS) {
            float freq = (progress < 0.45f) ? 587.33f : 880.0f;
            phase = 2.0f * (float)M_PI * freq * t;
        } else if (type == SFX_ALERT) {
            float freq = (progress < 0.5f) ? 440.0f : 554.37f;
            phase = 2.0f * (float)M_PI * freq * t;
        } else {
            /* Melodía cálida de despedida */
            float freq = (progress < 0.33f) ? 523.25f : ((progress < 0.66f) ? 659.25f : 783.99f);
            phase = 2.0f * (float)M_PI * freq * t;
        }

        float wave = 0.75f * sinf(phase) + 0.25f * sinf(phase * 2.0f);
        s16 sample = (s16)(env * 16500.0f * wave);

        if (stereo) {
            buf[i * 2]     = sample;
            buf[i * 2 + 1] = sample;
        } else {
            buf[i] = sample;
        }
    }
}

static void audioEngineSetBgm(AudioEngine *audio, bool enable) {
    audio->bgmEnabled = enable;
    if (audio->ndspActive && audio->bgmBuf) {
        if (enable) {
            ndspChnWaveBufClear(1);
            ndspChnWaveBufAdd(1, &audio->bgmWave);
        } else {
            ndspChnWaveBufClear(1);
        }
    } else if (audio->csndActive && audio->bgmBuf) {
        if (enable) {
            csndPlaySound(9, SOUND_FORMAT_16BIT | SOUND_REPEAT, AUDIO_SAMPLERATE,
                          0.55f, 0.0f, audio->bgmBuf, audio->bgmBuf,
                          BGM_SAMPLES * sizeof(s16));
        } else {
            CSND_SetPlayState(9, 0);
            csndExecCmds(false);
        }
    }
}

static void audioEngineInit(AudioEngine *audio) {
    memset(audio, 0, sizeof(AudioEngine));
    audio->bgmEnabled = true;

    if (R_SUCCEEDED(ndspInit())) {
        audio->ndspActive = true;
        ndspSetOutputMode(NDSP_OUTPUT_STEREO);

        float mix[12];
        memset(mix, 0, sizeof(mix));
        mix[0] = 1.0f;
        mix[1] = 1.0f;

        /* Canal 0: Efectos de sonido (Woupp, Startup, etc.) */
        ndspChnSetInterp(0, NDSP_INTERP_LINEAR);
        ndspChnSetRate(0, AUDIO_SAMPLERATE);
        ndspChnSetFormat(0, NDSP_FORMAT_STEREO_PCM16);
        ndspChnSetMix(0, mix);

        /* Canal 1: Canción de fondo en bucle (BGM) */
        ndspChnSetInterp(1, NDSP_INTERP_LINEAR);
        ndspChnSetRate(1, AUDIO_SAMPLERATE);
        ndspChnSetFormat(1, NDSP_FORMAT_STEREO_PCM16);
        ndspChnSetMix(1, mix);

        for (int s = 0; s < SFX_COUNT; s++) {
            u32 byteSize = SFX_SAMPLES * 2 * sizeof(s16);
            audio->sfxBuf[s] = (s16 *)linearAlloc(byteSize);
            if (audio->sfxBuf[s]) {
                synthesizeSfxBuffer(audio->sfxBuf[s], SFX_SAMPLES, (AssistantSfx)s, true);
                DSP_FlushDataCache(audio->sfxBuf[s], byteSize);
                audio->sfxWave[s].data_vaddr = audio->sfxBuf[s];
                audio->sfxWave[s].nsamples   = SFX_SAMPLES;
                audio->sfxWave[s].looping    = false;
            }
        }

        u32 bgmBytes = BGM_SAMPLES * 2 * sizeof(s16);
        audio->bgmBuf = (s16 *)linearAlloc(bgmBytes);
        if (audio->bgmBuf) {
            synthesizePleasantBgm(audio->bgmBuf, BGM_SAMPLES, true);
            DSP_FlushDataCache(audio->bgmBuf, bgmBytes);
            audio->bgmWave.data_vaddr = audio->bgmBuf;
            audio->bgmWave.nsamples   = BGM_SAMPLES;
            audio->bgmWave.looping    = true;
            ndspChnWaveBufAdd(1, &audio->bgmWave);
        }
    } else if (R_SUCCEEDED(csndInit())) {
        audio->csndActive = true;
        for (int s = 0; s < SFX_COUNT; s++) {
            u32 byteSize = SFX_SAMPLES * sizeof(s16);
            audio->sfxBuf[s] = (s16 *)linearAlloc(byteSize);
            if (audio->sfxBuf[s]) {
                synthesizeSfxBuffer(audio->sfxBuf[s], SFX_SAMPLES, (AssistantSfx)s, false);
                GSPGPU_FlushDataCache(audio->sfxBuf[s], byteSize);
            }
        }

        u32 bgmBytes = BGM_SAMPLES * sizeof(s16);
        audio->bgmBuf = (s16 *)linearAlloc(bgmBytes);
        if (audio->bgmBuf) {
            synthesizePleasantBgm(audio->bgmBuf, BGM_SAMPLES, false);
            GSPGPU_FlushDataCache(audio->bgmBuf, bgmBytes);
            audioEngineSetBgm(audio, true);
        }
    }
}

static void audioEnginePlay(AudioEngine *audio, AssistantSfx sfx) {
    if (sfx >= SFX_COUNT || !audio->sfxBuf[sfx]) return;

    if (audio->ndspActive) {
        ndspChnWaveBufClear(0);
        ndspChnWaveBufAdd(0, &audio->sfxWave[sfx]);
    } else if (audio->csndActive) {
        csndPlaySound(8, SOUND_FORMAT_16BIT | SOUND_ONE_SHOT, AUDIO_SAMPLERATE,
                      1.0f, 0.0f, audio->sfxBuf[sfx], audio->sfxBuf[sfx],
                      SFX_SAMPLES * sizeof(s16));
    }
}

static void audioEngineExit(AudioEngine *audio) {
    for (int s = 0; s < SFX_COUNT; s++) {
        if (audio->sfxBuf[s]) {
            linearFree(audio->sfxBuf[s]);
            audio->sfxBuf[s] = NULL;
        }
    }
    if (audio->bgmBuf) {
        linearFree(audio->bgmBuf);
        audio->bgmBuf = NULL;
    }
    if (audio->ndspActive) {
        ndspExit();
    } else if (audio->csndActive) {
        csndExit();
    }
}

static AppLang detectSystemLanguage(void) {
    u8 sysLang = CFG_LANGUAGE_ES;
    if (R_SUCCEEDED(CFGU_GetSystemLanguage(&sysLang))) {
        switch (sysLang) {
            case CFG_LANGUAGE_ES: return LANG_ES;
            case CFG_LANGUAGE_PT: return LANG_PT;
            case CFG_LANGUAGE_FR: return LANG_FR;
            default:              return LANG_EN;
        }
    }
    return LANG_ES;
}

static const char *getLangCode(AppLang lang) {
    switch (lang) {
        case LANG_ES: return "ES";
        case LANG_EN: return "EN";
        case LANG_PT: return "PT";
        case LANG_FR: return "FR";
        default:      return "ES";
    }
}

static void graphPushSample(VoltageGraph *g, u32 mv) {
    if (g->count < GRAPH_HISTORY_LEN) {
        g->samples[g->count++] = mv;
    } else {
        memmove(&g->samples[0], &g->samples[1], sizeof(u32) * (GRAPH_HISTORY_LEN - 1));
        g->samples[GRAPH_HISTORY_LEN - 1] = mv;
    }
}

static ConsoleBatterySpec getConsoleBatterySpec(u8 model, bool isNew3DS) {
    ConsoleBatterySpec spec;
    spec.modelId = model;
    spec.isNew3DS = isNew3DS;

    switch (model) {
        case CFG_MODEL_3DS:
            spec.consoleName  = "Old Nintendo 3DS";
            spec.codeName     = "CTR";
            spec.batteryModel = "CTR-003 (1300 mAh / 3.7V)";
            spec.capacityMah  = 1300;
            break;
        case CFG_MODEL_3DSXL:
            spec.consoleName  = "Old Nintendo 3DS XL";
            spec.codeName     = "SPR";
            spec.batteryModel = "SPR-003 (1750 mAh / 3.7V)";
            spec.capacityMah  = 1750;
            break;
        case CFG_MODEL_N3DS:
            spec.consoleName  = "New Nintendo 3DS";
            spec.codeName     = "KTR";
            spec.batteryModel = "KTR-003 (1400 mAh / 3.7V)";
            spec.capacityMah  = 1400;
            break;
        case CFG_MODEL_2DS:
            spec.consoleName  = "Old Nintendo 2DS";
            spec.codeName     = "FTR";
            spec.batteryModel = "CTR-003 (1300 mAh / 3.7V)";
            spec.capacityMah  = 1300;
            break;
        case CFG_MODEL_N3DSXL:
            spec.consoleName  = "New Nintendo 3DS XL";
            spec.codeName     = "RED";
            spec.batteryModel = "SPR-003 (1750 mAh / 3.7V)";
            spec.capacityMah  = 1750;
            break;
        case CFG_MODEL_N2DSXL:
            spec.consoleName  = "New Nintendo 2DS XL";
            spec.codeName     = "JAN";
            spec.batteryModel = "CTR-003 (1300 mAh / 3.7V)";
            spec.capacityMah  = 1300;
            break;
        default:
            spec.consoleName  = "Nintendo 3DS Family";
            spec.codeName     = "3DS";
            spec.batteryModel = "CTR-003 (1300 mAh / 3.7V)";
            spec.capacityMah  = 1300;
            break;
    }
    return spec;
}

static u32 estimateMvFromPtmuBars(u8 bars) {
    switch (bars) {
        case 5:  return 4100;
        case 4:  return 3880;
        case 3:  return 3740;
        case 2:  return 3620;
        case 1:  return 3500;
        default: return 3400;
    }
}

static void samplePowerReading(PowerReading *out, bool mcuReady) {
    memset(out, 0, sizeof(PowerReading));

    u8 level = 0;
    u8 charging = 0;
    bool adapter = false;

    if (R_SUCCEEDED(PTMU_GetBatteryLevel(&level))) {
        out->ptmuBars = level;
    }
    if (R_SUCCEEDED(PTMU_GetBatteryChargeState(&charging))) {
        out->isCharging = charging;
    }
    if (R_SUCCEEDED(PTMU_GetAdapterState(&adapter))) {
        out->adapterPlugged = adapter;
    }

    if (mcuReady) {
        u8 rawVolt = 0;
        u8 pct = 0;
        Result rcVolt = MCUHWC_GetBatteryVoltage(&rawVolt);
        Result rcPct  = MCUHWC_GetBatteryLevel(&pct);

        if (R_SUCCEEDED(rcVolt)) {
            out->mcuAvailable  = true;
            out->mcuRawVoltage = rawVolt;
            out->voltageMv     = (u32)rawVolt * 20u;
        }
        if (R_SUCCEEDED(rcPct)) {
            out->mcuPercent = pct;
        }
        MCUHWC_GetFwVerHigh(&out->mcuFwHigh);
        MCUHWC_GetFwVerLow(&out->mcuFwLow);
    }

    if (!out->mcuAvailable) {
        out->voltageMv = estimateMvFromPtmuBars(out->ptmuBars);
        out->mcuPercent = (u8)(out->ptmuBars * 20u);
    }
}

/* Helpers de dibujo 2D y Texto con extrusión 3D */
static void drawCard(float x, float y, float w, float h, u32 bgCol, u32 borderCol) {
    C2D_DrawRectSolid(x, y, 0.0f, w, h, borderCol);
    C2D_DrawRectSolid(x + 1.5f, y + 1.5f, 0.0f, w - 3.0f, h - 3.0f, bgCol);
}

static void drawTextFmt(C2D_TextBuf buf, float x, float y, float scale, u32 color, u32 alignFlags, const char *fmt, ...) {
    char str[256];
    va_list args;
    va_start(args, fmt);
    vsnprintf(str, sizeof(str), fmt, args);
    va_end(args);

    C2D_Text text;
    C2D_TextParse(&text, buf, str);
    C2D_TextOptimize(&text);
    C2D_DrawText(&text, C2D_WithColor | alignFlags, x, y, 0.0f, scale, scale, color);
}

/**
 * Dibuja texto con efecto 3D multicapa (sombra profunda + extrusión lateral + frente brillante).
 */
static void draw3DLogoText(C2D_TextBuf buf, float x, float y, float scale, u32 frontCol, u32 depthCol, const char *str) {
    C2D_Text text;
    C2D_TextParse(&text, buf, str);
    C2D_TextOptimize(&text);

    /* Capas de extrusión 3D */
    C2D_DrawText(&text, C2D_WithColor | C2D_AlignCenter, x + 3.5f, y + 3.5f, 0.0f, scale, scale, C2D_Color32(5, 10, 20, 255));
    C2D_DrawText(&text, C2D_WithColor | C2D_AlignCenter, x + 2.2f, y + 2.2f, 0.0f, scale, scale, depthCol);
    C2D_DrawText(&text, C2D_WithColor | C2D_AlignCenter, x + 1.1f, y + 1.1f, 0.0f, scale, scale, depthCol);
    C2D_DrawText(&text, C2D_WithColor | C2D_AlignCenter, x,        y,        0.0f, scale, scale, frontCol);
}

static void drawBatteryGauge(C2D_TextBuf buf, float x, float y, const PowerReading *pwr) {
    float bw = 112.0f;
    float bh = 54.0f;

    u32 mainCol = COL_GREEN_OK;
    if (pwr->isCharging) {
        mainCol = COL_ACCENT_CYAN;
    } else if (pwr->ptmuBars <= 1 || pwr->mcuPercent <= 15) {
        mainCol = COL_RED_CRIT;
    } else if (pwr->ptmuBars == 2 || pwr->mcuPercent <= 35) {
        mainCol = COL_YELLOW_WARN;
    }

    C2D_DrawRectSolid(x, y, 0.0f, bw, bh, COL_TEXT_MAIN);
    C2D_DrawRectSolid(x + 3.0f, y + 3.0f, 0.0f, bw - 6.0f, bh - 6.0f, COL_BG_DARK);
    C2D_DrawRectSolid(x + bw, y + 15.0f, 0.0f, 7.0f, 24.0f, COL_TEXT_MAIN);

    float fillPct = (float)(pwr->mcuPercent > 100 ? 100 : pwr->mcuPercent) / 100.0f;
    float maxFillW = bw - 12.0f;
    float fillW = maxFillW * fillPct;
    if (fillW < 4.0f && pwr->ptmuBars > 0) fillW = 4.0f;

    C2D_DrawRectSolid(x + 6.0f, y + 6.0f, 0.0f, fillW, bh - 12.0f, mainCol);

    for (int i = 1; i < 5; i++) {
        float sx = x + 6.0f + (maxFillW / 5.0f) * (float)i;
        C2D_DrawRectSolid(sx, y + 6.0f, 0.0f, 2.0f, bh - 12.0f, COL_BG_DARK);
    }

    if (pwr->mcuAvailable) {
        drawTextFmt(buf, x + bw * 0.5f, y + 14.0f, 0.78f, COL_TEXT_MAIN, C2D_AlignCenter,
                    "%u%%", pwr->mcuPercent);
    } else {
        drawTextFmt(buf, x + bw * 0.5f, y + 14.0f, 0.70f, COL_TEXT_MAIN, C2D_AlignCenter,
                    "%u/5", pwr->ptmuBars);
    }
}

/**
 * Dibuja a "Bati Bot" con animación viva en tiempo real:
 *  - Respiración suave
 *  - Parpadeo automático de ojos cada pocos segundos
 *  - Movimiento de pupilas mirando a los lados (izquierda, centro, derecha)
 *  - Expresión "¡Woupp!" cuando lo tocas con el lápiz
 */
static void drawBatiBot(C2D_TextBuf buf, float x, float y, int mood, float bounceOffset, u32 animTick, AppLang lang, bool showCard) {
    float breath = sinf((float)animTick * 0.07f) * 1.5f;
    float by = y + bounceOffset + breath;

    u32 bodyColor = COL_GREEN_OK;
    u32 sideShadow = COL_GREEN_DARK;
    if (mood == 1) {
        bodyColor = COL_YELLOW_WARN;
        sideShadow = COL_YELLOW_DARK;
    } else if (mood == 2) {
        bodyColor = COL_RED_CRIT;
        sideShadow = COL_RED_DARK;
    } else if (mood == 3) {
        bodyColor = COL_ACCENT_CYAN;
        sideShadow = COL_CYAN_SHADOW;
    }

    if (showCard) {
        drawCard(x, y, ASST_BOX_W, ASST_BOX_H, COL_CARD_ALT, bodyColor);
    }

    /* Sombra 3D proyectada detrás del cuerpo de Bati Bot */
    C2D_DrawRectSolid(x + 20.0f, by + 17.0f, 0.0f, 52.0f, 60.0f, sideShadow);

    /* Borne positivo (gorrito 3D) y antena con chispa animada */
    C2D_DrawRectSolid(x + 28.0f, by + 8.0f, 0.0f, 28.0f, 6.0f, COL_TEXT_MAIN);
    C2D_DrawRectSolid(x + 40.0f, by + 2.0f, 0.0f, 4.0f, 6.0f, COL_ACCENT_CYAN);
    if ((animTick / 20) % 2 == 0) {
        C2D_DrawRectSolid(x + 38.0f, by + 0.0f, 0.0f, 8.0f, 3.0f, COL_YELLOW_WARN);
    }

    /* Cuerpo principal de Bati Bot con bisel 3D */
    C2D_DrawRectSolid(x + 16.0f, by + 14.0f, 0.0f, 52.0f, 60.0f, COL_TEXT_MAIN);
    C2D_DrawRectSolid(x + 19.0f, by + 17.0f, 0.0f, 46.0f, 54.0f, COL_BG_DARK);
    C2D_DrawRectSolid(x + 21.0f, by + 24.0f, 0.0f, 42.0f, 45.0f, bodyColor);
    /* Bisel de profundidad derecho en el cuerpo */
    C2D_DrawRectSolid(x + 58.0f, by + 24.0f, 0.0f, 5.0f, 45.0f, sideShadow);

    /* Pantalla facial */
    C2D_DrawRectSolid(x + 23.0f, by + 22.0f, 0.0f, 38.0f, 30.0f, COL_BG_DARK);

    /* Lógica de parpadeo (cada ~3 segundos parpadea durante 6 frames) y mirada */
    bool isBlinking = ((animTick % 170) >= 164);
    float lookOffsetX = sinf((float)animTick * 0.035f) * 2.2f;
    float lookOffsetY = cosf((float)animTick * 0.05f) * 0.8f;

    if (mood == 3) {
        /* Ojos felices "^ ^" cuando hace Woupp! */
        C2D_DrawLine(x + 28.0f, by + 34.0f, COL_ACCENT_CYAN, x + 32.0f, by + 29.0f, COL_ACCENT_CYAN, 2.0f, 0.0f);
        C2D_DrawLine(x + 32.0f, by + 29.0f, COL_ACCENT_CYAN, x + 36.0f, by + 34.0f, COL_ACCENT_CYAN, 2.0f, 0.0f);
        C2D_DrawLine(x + 48.0f, by + 34.0f, COL_ACCENT_CYAN, x + 52.0f, by + 29.0f, COL_ACCENT_CYAN, 2.0f, 0.0f);
        C2D_DrawLine(x + 52.0f, by + 29.0f, COL_ACCENT_CYAN, x + 56.0f, by + 34.0f, COL_ACCENT_CYAN, 2.0f, 0.0f);
    } else if (isBlinking) {
        /* Ojos cerrados parpadeando (- -) */
        C2D_DrawRectSolid(x + 28.0f, by + 32.0f, 0.0f, 8.0f, 2.0f, COL_ACCENT_CYAN);
        C2D_DrawRectSolid(x + 48.0f, by + 32.0f, 0.0f, 8.0f, 2.0f, COL_ACCENT_CYAN);
    } else {
        /* Ojos abiertos con pupilas móviles que miran alrededor */
        C2D_DrawRectSolid(x + 28.0f, by + 28.0f, 0.0f, 7.5f, 8.5f, COL_TEXT_MAIN);
        C2D_DrawRectSolid(x + 48.0f, by + 28.0f, 0.0f, 7.5f, 8.5f, COL_TEXT_MAIN);

        /* Pupilas animadas */
        C2D_DrawRectSolid(x + 30.0f + lookOffsetX, by + 30.0f + lookOffsetY, 0.0f, 3.5f, 4.0f, COL_ACCENT_CYAN);
        C2D_DrawRectSolid(x + 50.0f + lookOffsetX, by + 30.0f + lookOffsetY, 0.0f, 3.5f, 4.0f, COL_ACCENT_CYAN);
    }

    /* Mejillas sonrojadas */
    C2D_DrawRectSolid(x + 25.0f, by + 38.0f, 0.0f, 5.0f, 3.0f, COL_BOT_CHEEK);
    C2D_DrawRectSolid(x + 54.0f, by + 38.0f, 0.0f, 5.0f, 3.0f, COL_BOT_CHEEK);

    /* Boca expresiva */
    if (mood == 3) {
        C2D_DrawRectSolid(x + 38.0f, by + 40.0f, 0.0f, 8.0f, 7.0f, COL_ACCENT_CYAN);
    } else if (mood == 0 || mood == 2) {
        C2D_DrawRectSolid(x + 36.0f, by + 43.0f, 0.0f, 12.0f, 3.0f, COL_GREEN_OK);
        C2D_DrawRectSolid(x + 34.0f, by + 41.0f, 0.0f, 3.0f, 3.0f, COL_GREEN_OK);
        C2D_DrawRectSolid(x + 47.0f, by + 41.0f, 0.0f, 3.0f, 3.0f, COL_GREEN_OK);
    } else {
        C2D_DrawRectSolid(x + 36.0f, by + 43.0f, 0.0f, 12.0f, 2.5f, COL_YELLOW_WARN);
    }

    if (showCard) {
        const char *tapLabel = "TOCAME!";
        if (lang == LANG_EN) tapLabel = "TAP ME!";
        else if (lang == LANG_PT) tapLabel = "TOQUE!";
        else if (lang == LANG_FR) tapLabel = "TOUCHE!";

        drawTextFmt(buf, x + ASST_BOX_W * 0.5f, y + 78.0f, 0.41f, COL_ACCENT_CYAN, C2D_AlignCenter,
                    "BATI BOT");
        drawTextFmt(buf, x + ASST_BOX_W * 0.5f, y + 91.0f, 0.36f, COL_TEXT_MAIN, C2D_AlignCenter,
                    "%s", tapLabel);
    }
}

/**
 * Pantalla de Presentación / Splash Inicial con Logo 3D y Bati Bot animado.
 */
static void showStartupSplashScreen(C3D_RenderTarget *topTarget,
                                    C3D_RenderTarget *botTarget,
                                    C2D_TextBuf textBuf,
                                    AppLang lang,
                                    AudioEngine *audio) {
    audioEnginePlay(audio, SFX_STARTUP);

    for (u32 frame = 0; frame < 130 && aptMainLoop(); frame++) {
        hidScanInput();
        if (frame > 20 && (hidKeysDown() & (KEY_A | KEY_START | KEY_TOUCH))) {
            audioEnginePlay(audio, SFX_WOUPP);
            break;
        }

        C2D_TextBufClear(textBuf);
        C3D_FrameBegin(C3D_FRAME_SYNCDRAW);

        /* Pantalla Superior: Logo 3D + Modelo de Bati Bot */
        C2D_TargetClear(topTarget, COL_BG_DARK);
        C2D_SceneBegin(topTarget);

        /* Marco decorativo 3D */
        drawCard(18.0f, 14.0f, 364.0f, 212.0f, COL_CARD_BG, COL_ACCENT_CYAN);
        C2D_DrawRectSolid(24.0f, 20.0f, 0.0f, 352.0f, 4.0f, COL_GREEN_OK);

        float floatY = sinf((float)frame * 0.08f) * 3.0f;

        /* Logo 3D grande */
        draw3DLogoText(textBuf, TOP_WIDTH * 0.5f, 32.0f + floatY, 0.92f,
                       COL_ACCENT_CYAN, COL_CYAN_SHADOW, "BatteryHealth3DS");

        /* Modelo de Bati Bot en el centro de la pantalla de inicio */
        float botBounce = -4.0f * fabsf(sinf((float)frame * 0.12f));
        drawBatiBot(textBuf, 158.0f, 78.0f, (frame < 35) ? 3 : 0, botBounce, frame, lang, false);

        drawTextFmt(textBuf, TOP_WIDTH * 0.5f, 168.0f, 0.50f, COL_GREEN_OK, C2D_AlignCenter,
                    "Bati Bot - Smart Battery Analyzer");

        drawTextFmt(textBuf, TOP_WIDTH * 0.5f, 194.0f, 0.46f, COL_YELLOW_WARN, C2D_AlignCenter,
                    "Created by ElVichoPo");

        /* Pantalla Inferior: Bienvenida de Bati Bot */
        C2D_TargetClear(botTarget, COL_BG_DARK);
        C2D_SceneBegin(botTarget);

        drawCard(20.0f, 36.0f, 280.0f, 168.0f, COL_BUBBLE_BG, COL_CARD_BORDER);
        draw3DLogoText(textBuf, BOT_WIDTH * 0.5f, 56.0f, 0.68f,
                       COL_GREEN_OK, COL_GREEN_DARK, "¡Hola, soy Bati Bot!");

        const char *subMsg = (lang == LANG_EN) ? "Ready to check your 3DS / 2DS battery!" :
                             (lang == LANG_PT) ? "Pronto para cuidar da sua bateria 3DS / 2DS!" :
                             (lang == LANG_FR) ? "Pret a verifier la batterie de ta 3DS / 2DS !" :
                                                 "¡Listo para cuidar la bateria de tu consola!";
        drawTextFmt(textBuf, BOT_WIDTH * 0.5f, 102.0f, 0.45f, COL_TEXT_MAIN, C2D_AlignCenter,
                    "%s", subMsg);

        drawTextFmt(textBuf, BOT_WIDTH * 0.5f, 146.0f, 0.44f, COL_ACCENT_CYAN, C2D_AlignCenter,
                    "Desarrollado por: ElVichoPo");

        C3D_FrameEnd(0);
    }
}

/**
 * Obtiene el diálogo actual de Bati Bot (incluyendo 10 consejos completos de emergencia y cuidado).
 */
static void getBatiBotDialogue(AppLang lang,
                               const ConsoleBatterySpec *spec,
                               const PowerReading *pwr,
                               const SagTestResult *lastTest,
                               u32 tipIndex,
                               char *titleOut, size_t titleSz,
                               char *bodyOut, size_t bodySz,
                               int *moodOut) {
    *moodOut = 0;

    if (tipIndex > 0) {
        u32 idx = (tipIndex - 1) % TOTAL_BATI_TIPS;
        *moodOut = (idx == 0 || idx == 2) ? 1 : 0;

        if (lang == LANG_ES) {
            snprintf(titleOut, titleSz, "Consejo de Bati Bot #%lu/%u:", (unsigned long)(idx + 1), TOTAL_BATI_TIPS);
            switch (idx) {
                case 0:
                    snprintf(bodyOut, bodySz, "¡Emergencia! Si el LED rojo parpadea, baja el brillo al nivel 1, apaga el Wi-Fi y guarda tu partida de inmediato.");
                    break;
                case 1:
                    snprintf(bodyOut, bodySz, "Evita dejar tu %s al sol o en lugares calientes. El calor acelera la resistencia interna de tu bateria %s.", spec->codeName, spec->batteryModel);
                    break;
                case 2:
                    snprintf(bodyOut, bodySz, "Si algun dia notas que la tapa trasera se abomba (bateria hinchada), no la cargues: reemplazala con calma por seguridad.");
                    break;
                case 3:
                    snprintf(bodyOut, bodySz, "Si guardas tu consola por varios meses, dejala cargada entre 50%% y 60%%. ¡Asi la quimica de litio se conserva anos!");
                    break;
                case 4:
                    snprintf(bodyOut, bodySz, "Activar el Ahorro de Energia en el menu de la consola reduce el consumo de retroiluminacion y alarga cada carga.");
                    break;
                case 5:
                    snprintf(bodyOut, bodySz, "El modo StreetPass / espera con Wi-Fi consume bateria constante. Si no vas a salir, apaga la conexion inalambrica.");
                    break;
                case 6:
                    snprintf(bodyOut, bodySz, "Procura no agotar la bateria hasta el 0%% siempre; recargarla cuando le queda 20%%-30%% duplica sus ciclos de vida.");
                    break;
                case 7:
                    snprintf(bodyOut, bodySz, "Usa cargadores estables de 4.6V (o cable USB de buena calidad). Un voltaje limpio protege el chip de carga de tu consola.");
                    break;
                case 8:
                    snprintf(bodyOut, bodySz, "Jugar mientras cargas genera mas calor interno. Si haces sesiones largas, un descanso corto mantiene fresca la celda.");
                    break;
                default:
                    snprintf(bodyOut, bodySz, "Si cada 6 meses las barras parecen descalibradas, haz un ciclo completo (100%% a 5%% y de vuelta a 100%%) para recalibrar el MCU.");
                    break;
            }
        } else if (lang == LANG_EN) {
            snprintf(titleOut, titleSz, "Bati Bot Tip #%lu/%u:", (unsigned long)(idx + 1), TOTAL_BATI_TIPS);
            switch (idx) {
                case 0:
                    snprintf(bodyOut, bodySz, "Emergency! If the red LED blinks, set brightness to 1, turn off Wi-Fi, and save your game immediately!");
                    break;
                case 1:
                    snprintf(bodyOut, bodySz, "Keep your %s away from high heat. Excess heat increases internal resistance in %s batteries.", spec->codeName, spec->batteryModel);
                    break;
                case 2:
                    snprintf(bodyOut, bodySz, "If the back cover ever feels swollen or tight, stop charging and swap in a fresh replacement battery.");
                    break;
                case 3:
                    snprintf(bodyOut, bodySz, "Storing your console for months? Leave the battery around 50%%-60%% charge to preserve its lithium chemistry!");
                    break;
                case 4:
                    snprintf(bodyOut, bodySz, "Enabling Power-Saving Mode in HOME Menu settings reduces backlight draw and extends your playtime.");
                    break;
                case 5:
                    snprintf(bodyOut, bodySz, "Sleep Mode with Wi-Fi on drains battery searching for StreetPass. Turn off wireless when you don't need it!");
                    break;
                case 6:
                    snprintf(bodyOut, bodySz, "Avoid draining to 0%% every time. Recharging around 20%%-30%% greatly extends the total lifespan of your battery.");
                    break;
                case 7:
                    snprintf(bodyOut, bodySz, "Always use a reliable 4.6V adapter or quality USB cable to protect the internal charging IC.");
                    break;
                case 8:
                    snprintf(bodyOut, bodySz, "Playing heavy games while charging warms up the battery. Keeping it cool preserves maximum capacity!");
                    break;
                default:
                    snprintf(bodyOut, bodySz, "If the percentage ever feels inaccurate, do one full cycle (100%% down to 5%% and back to 100%%) to recalibrate.");
                    break;
            }
        } else if (lang == LANG_PT) {
            snprintf(titleOut, titleSz, "Dica do Bati Bot #%lu/%u:", (unsigned long)(idx + 1), TOTAL_BATI_TIPS);
            switch (idx) {
                case 0:
                    snprintf(bodyOut, bodySz, "Emergencia! Se o LED vermelho piscar, reduza o brilho para 1, desligue o Wi-Fi e salve seu jogo agora!");
                    break;
                case 1:
                    snprintf(bodyOut, bodySz, "Evite calor excessivo no seu %s. O calor aumenta a resistencia interna da bateria %s.", spec->codeName, spec->batteryModel);
                    break;
                case 2:
                    snprintf(bodyOut, bodySz, "Se notar a bateria estufada na tampa traseira, troque por uma nova com seguranca.");
                    break;
                case 3:
                    snprintf(bodyOut, bodySz, "Vai guardar o console por meses? Deixe a carga entre 50%% e 60%% para proteger a celula de litio!");
                    break;
                default:
                    snprintf(bodyOut, bodySz, "Recarregar quando estiver em 20%%-30%% e desligar o Wi-Fi quando nao usar aumenta muito a vida util da bateria!");
                    break;
            }
        } else {
            snprintf(titleOut, titleSz, "Astuce Bati Bot #%lu/%u:", (unsigned long)(idx + 1), TOTAL_BATI_TIPS);
            switch (idx) {
                case 0:
                    snprintf(bodyOut, bodySz, "Urgence ! Si la LED rouge clignote, baissez la luminosite a 1, coupez le Wi-Fi et sauvegardez vite !");
                    break;
                case 1:
                    snprintf(bodyOut, bodySz, "Evitez la chaleur sur votre %s. Elle augmente la resistance interne de la batterie %s.", spec->codeName, spec->batteryModel);
                    break;
                case 2:
                    snprintf(bodyOut, bodySz, "Si la batterie gonfle sous le capot, ne la chargez plus et remplacez-la facilement.");
                    break;
                case 3:
                    snprintf(bodyOut, bodySz, "Pour un stockage long, gardez la batterie entre 50%% et 60%% pour proteger sa chimie !");
                    break;
                default:
                    snprintf(bodyOut, bodySz, "Recharger vers 20%%-30%% et couper le Wi-Fi en veille prolonge enormement la duree de vie de ta batterie !");
                    break;
            }
        }
        return;
    }

    if (!lastTest->completed) {
        if (pwr->ptmuBars <= 1 || pwr->mcuPercent <= 15) {
            *moodOut = 1;
            if (lang == LANG_ES) {
                snprintf(titleOut, titleSz, "¡Bati Bot alerta: Bateria Baja!");
                snprintf(bodyOut, bodySz,
                         "Tienes %u%% (%lu mV). ¡Guarda tu partida pronto! Tocame con el lapiz para escuchar mis 10 consejos.",
                         pwr->mcuPercent, (unsigned long)pwr->voltageMv);
            } else if (lang == LANG_EN) {
                snprintf(titleOut, titleSz, "Bati Bot Alert: Low Battery!");
                snprintf(bodyOut, bodySz,
                         "You are at %u%% (%lu mV). Save your game soon! Tap me with the stylus for my 10 battery tips.",
                         pwr->mcuPercent, (unsigned long)pwr->voltageMv);
            } else if (lang == LANG_PT) {
                snprintf(titleOut, titleSz, "Alerta Bati Bot: Bateria Fraca!");
                snprintf(bodyOut, bodySz,
                         "Voce esta com %u%% (%lu mV). Salve seu jogo! Toque em mim com a stylus para dicas de emergencia.",
                         pwr->mcuPercent, (unsigned long)pwr->voltageMv);
            } else {
                snprintf(titleOut, titleSz, "Alerte Bati Bot : Batterie Faible !");
                snprintf(bodyOut, bodySz,
                         "Batterie a %u%% (%lu mV). Sauvegardez vite ! Touchez-moi avec le stylet pour mes conseils.",
                         pwr->mcuPercent, (unsigned long)pwr->voltageMv);
            }
            return;
        }

        *moodOut = 0;
        if (lang == LANG_ES) {
            snprintf(titleOut, titleSz, "¡Hola! Soy Bati Bot, tu guia");
            snprintf(bodyOut, bodySz,
                     "Tu %s usa la bateria %s. Presiona [A] o el boton verde para analizar su salud. ¡Tocame para darte 10 consejos!",
                     spec->consoleName, spec->batteryModel);
        } else if (lang == LANG_EN) {
            snprintf(titleOut, titleSz, "Hi! I'm Bati Bot, your guide");
            snprintf(bodyOut, bodySz,
                     "Your %s uses the %s battery. Press [A] or tap the green button to test its health. Tap me for 10 tips!",
                     spec->consoleName, spec->batteryModel);
        } else if (lang == LANG_PT) {
            snprintf(titleOut, titleSz, "Ola! Sou Bati Bot, seu guia");
            snprintf(bodyOut, bodySz,
                     "Seu %s usa a bateria %s. Aperte [A] ou toque no botao verde para testar a saude. Toque em mim para 10 dicas!",
                     spec->consoleName, spec->batteryModel);
        } else {
            snprintf(titleOut, titleSz, "Salut ! Je suis Bati Bot");
            snprintf(bodyOut, bodySz,
                     "Ta %s utilise une %s. Appuie sur [A] ou le bouton vert pour tester sa sante. Touche-moi pour 10 astuces !",
                     spec->consoleName, spec->batteryModel);
        }
        return;
    }

    bool barDropped = (lastTest->minLoadBars < lastTest->restBars);
    if (lastTest->sagMv <= 60 && !barDropped) {
        *moodOut = 0;
        if (lang == LANG_ES) {
            snprintf(titleOut, titleSz, "Bati Bot confirma: ¡SALUD EXCELENTE!");
            snprintf(bodyOut, bodySz,
                     "¡Woupp! Solo cayo %ld mV a 804 MHz. Tu bateria %s esta super sana y lista para largas horas de juego.",
                     (long)lastTest->sagMv, spec->batteryModel);
        } else if (lang == LANG_EN) {
            snprintf(titleOut, titleSz, "Bati Bot confirms: EXCELLENT!");
            snprintf(bodyOut, bodySz,
                     "Woupp! Only %ld mV drop under full load. Your %s battery is in top shape for long gaming sessions!",
                     (long)lastTest->sagMv, spec->batteryModel);
        } else if (lang == LANG_PT) {
            snprintf(titleOut, titleSz, "Bati Bot confirma: EXCELENTE!");
            snprintf(bodyOut, bodySz,
                     "Woupp! Caiu apenas %ld mV em carga maxima. Sua bateria %s esta perfeita para jogar muito!",
                     (long)lastTest->sagMv, spec->batteryModel);
        } else {
            snprintf(titleOut, titleSz, "Bati Bot confirme : EXCELLENTE !");
            snprintf(bodyOut, bodySz,
                     "Woupp ! Seulement %ld mV de chute. Ta batterie %s est en pleine forme pour jouer longtemps !",
                     (long)lastTest->sagMv, spec->batteryModel);
        }
    } else if (lastTest->sagMv <= 150 && !barDropped) {
        *moodOut = 1;
        if (lang == LANG_ES) {
            snprintf(titleOut, titleSz, "Bati Bot: DESGASTE MODERADO");
            snprintf(bodyOut, bodySz,
                     "Cayo %ld mV bajo carga. Tiene algo de uso pero sigue firme. ¡Baja un poco el brillo y te dara mucha diversion!",
                     (long)lastTest->sagMv);
        } else if (lang == LANG_EN) {
            snprintf(titleOut, titleSz, "Bati Bot: MODERATE WEAR");
            snprintf(bodyOut, bodySz,
                     "Dropped %ld mV under load. Normal aging, still reliable! Lower brightness a bit for extra playtime.",
                     (long)lastTest->sagMv);
        } else if (lang == LANG_PT) {
            snprintf(titleOut, titleSz, "Bati Bot: DESGASTE MODERADO");
            snprintf(bodyOut, bodySz,
                     "Caiu %ld mV sob carga. Desgaste normal, mas ainda firme! Reduza um pouco o brilho para durar mais.",
                     (long)lastTest->sagMv);
        } else {
            snprintf(titleOut, titleSz, "Bati Bot : USURE MODEREE");
            snprintf(bodyOut, bodySz,
                     "Chute de %ld mV sous charge. Usure normale mais stable ! Reduis un peu la luminosite pour jouer plus.",
                     (long)lastTest->sagMv);
        }
    } else {
        *moodOut = 2;
        if (lang == LANG_ES) {
            snprintf(titleOut, titleSz, "¡Animo! Hora de renovar bateria");
            snprintf(bodyOut, bodySz,
                     "Tu bateria dio todo (cayo %ld mV), ¡pero no te preocupes! Puedes reemplazarla por una %s nueva y tu consola quedara como nueva.",
                     (long)lastTest->sagMv, spec->batteryModel);
        } else if (lang == LANG_EN) {
            snprintf(titleOut, titleSz, "Cheer up! Easy battery refresh");
            snprintf(bodyOut, bodySz,
                     "Your battery worked hard (%ld mV drop), but don't worry! Swap in a new %s battery and your console will feel brand new!",
                     (long)lastTest->sagMv, spec->batteryModel);
        } else if (lang == LANG_PT) {
            snprintf(titleOut, titleSz, "Animo! Hora de renovar a bateria");
            snprintf(bodyOut, bodySz,
                     "Sua bateria deu o melhor (%ld mV), mas fique tranquilo! Voce pode trocar por uma %s nova e seu console ficara novo!",
                     (long)lastTest->sagMv, spec->batteryModel);
        } else {
            snprintf(titleOut, titleSz, "Courage ! Facile a remplacer");
            snprintf(bodyOut, bodySz,
                     "Ta batterie a bien servi (%ld mV), mais pas de souci ! Remplace-la par une %s neuve et ta console sera comme neuve !",
                     (long)lastTest->sagMv, spec->batteryModel);
        }
    }
}

/**
 * Dibuja la pantalla superior (400x240) con Título 3D (sin v3.0) y apartado del creador ElVichoPo.
 */
static void renderTopScreen2D(C3D_RenderTarget *topTarget,
                              C2D_TextBuf textBuf,
                              AppLang lang,
                              const ConsoleBatterySpec *spec,
                              const PowerReading *pwr,
                              const VoltageGraph *graph) {
    C2D_TargetClear(topTarget, COL_BG_DARK);
    C2D_SceneBegin(topTarget);

    C2D_DrawRectSolid(0.0f, 0.0f, 0.0f, TOP_WIDTH, 28.0f, COL_BG_HEADER);
    C2D_DrawRectSolid(0.0f, 27.0f, 0.0f, TOP_WIDTH, 1.5f, COL_ACCENT_CYAN);

    /* Título con relieve 3D sin número de versión */
    draw3DLogoText(textBuf, 94.0f, 4.0f, 0.58f, COL_ACCENT_CYAN, COL_CYAN_SHADOW, "BatteryHealth3DS");

    /* Apartado destacado del Creador en el Header */
    drawCard(188.0f, 3.0f, 108.0f, 21.0f, COL_CARD_ALT, COL_CARD_BORDER);
    drawTextFmt(textBuf, 242.0f, 6.0f, 0.40f, COL_YELLOW_WARN, C2D_AlignCenter,
                "By: ElVichoPo");

    if (pwr->mcuAvailable) {
        drawTextFmt(textBuf, 392.0f, 6.0f, 0.42f, COL_GREEN_OK, C2D_AlignRight,
                    "MCU %u.%u OK", pwr->mcuFwHigh, pwr->mcuFwLow);
    } else {
        drawTextFmt(textBuf, 392.0f, 6.0f, 0.42f, COL_YELLOW_WARN, C2D_AlignRight,
                    "PTMU MODE");
    }

    /* Tarjeta Superior Izquierda: Hardware & Batería de Fábrica */
    drawCard(8.0f, 34.0f, 232.0f, 94.0f, COL_CARD_BG, COL_CARD_BORDER);

    const char *hwHdr = (lang == LANG_EN) ? "CONSOLE & FACTORY BATTERY" :
                        (lang == LANG_PT) ? "CONSOLE E BATERIA DE FABRICA" :
                        (lang == LANG_FR) ? "CONSOLE ET BATTERIE D'ORIGINE" :
                                            "CONSOLA Y BATERIA DE FABRICA";
    drawTextFmt(textBuf, 16.0f, 38.0f, 0.44f, COL_ACCENT_CYAN, C2D_AlignLeft, "%s", hwHdr);
    drawTextFmt(textBuf, 16.0f, 55.0f, 0.52f, COL_TEXT_MAIN, C2D_AlignLeft,
                "%s (%s)", spec->consoleName, spec->codeName);
    drawTextFmt(textBuf, 16.0f, 74.0f, 0.43f, COL_TEXT_MUTED, C2D_AlignLeft,
                "SoC: %s", spec->isNew3DS ? "New ARM11 4-Core @ 804 MHz" : "Old ARM11 2-Core @ 268 MHz");
    drawTextFmt(textBuf, 16.0f, 91.0f, 0.46f, COL_GREEN_OK, C2D_AlignLeft,
                "Bat: %s", spec->batteryModel);
    drawTextFmt(textBuf, 16.0f, 109.0f, 0.40f, COL_YELLOW_WARN, C2D_AlignLeft,
                "Creador / Dev: ElVichoPo");

    /* Tarjeta Superior Derecha: Medidor visual de Batería */
    drawCard(246.0f, 34.0f, 146.0f, 94.0f, COL_CARD_BG, COL_CARD_BORDER);
    drawBatteryGauge(textBuf, 259.0f, 43.0f, pwr);

    const char *chgText = "EN DESCARGA";
    if (lang == LANG_EN) chgText = pwr->isCharging ? "CHARGING AC" : (pwr->adapterPlugged ? "AC PLUGGED" : "DISCHARGING");
    else if (lang == LANG_PT) chgText = pwr->isCharging ? "CARREGANDO" : (pwr->adapterPlugged ? "AC CONECTADO" : "DESCARREGANDO");
    else if (lang == LANG_FR) chgText = pwr->isCharging ? "EN CHARGE" : (pwr->adapterPlugged ? "SECTEUR AC" : "EN DECHARGE");
    else chgText = pwr->isCharging ? "CARGANDO AC" : (pwr->adapterPlugged ? "AC CONECTADO" : "EN DESCARGA");

    u32 chgCol = (pwr->isCharging || pwr->adapterPlugged) ? COL_YELLOW_WARN : COL_GREEN_OK;
    drawTextFmt(textBuf, 319.0f, 103.0f, 0.44f, chgCol, C2D_AlignCenter,
                "%s (%u/5)", chgText, pwr->ptmuBars);

    /* Tarjeta Inferior en Pantalla Superior: Telemetría de Voltaje + Gráfica en Vivo */
    drawCard(8.0f, 134.0f, 384.0f, 98.0f, COL_CARD_BG, COL_CARD_BORDER);

    const char *vHdr = (lang == LANG_EN) ? "LIVE VOLTAGE TELEMETRY" :
                       (lang == LANG_PT) ? "TENSAO EM TEMPO REAL" :
                       (lang == LANG_FR) ? "TENSION EN TEMPS REEL" :
                                           "VOLTAJE EN TIEMPO REAL";
    drawTextFmt(textBuf, 16.0f, 140.0f, 0.45f, COL_ACCENT_CYAN, C2D_AlignLeft, "%s", vHdr);

    u32 vInt  = pwr->voltageMv / 1000;
    u32 vFrac = pwr->voltageMv % 1000;
    drawTextFmt(textBuf, 16.0f, 158.0f, 0.78f, COL_TEXT_MAIN, C2D_AlignLeft,
                "%lu.%03lu V", (unsigned long)vInt, (unsigned long)vFrac);
    drawTextFmt(textBuf, 16.0f, 186.0f, 0.50f, COL_GREEN_OK, C2D_AlignLeft,
                "%4lu mV %s", (unsigned long)pwr->voltageMv,
                pwr->mcuAvailable ? "(MCU I2C)" : "(PTMU)");
    drawTextFmt(textBuf, 16.0f, 206.0f, 0.42f, COL_TEXT_MUTED, C2D_AlignLeft,
                "Reg 0x0B: 0x%02X (x20mV)", pwr->mcuRawVoltage);

    float gx = 166.0f, gy = 144.0f, gw = 216.0f, gh = 78.0f;
    C2D_DrawRectSolid(gx, gy, 0.0f, gw, gh, COL_BG_DARK);

    for (int i = 0; i <= 2; i++) {
        float ly = gy + (gh * 0.5f) * (float)i;
        C2D_DrawLine(gx, ly, COL_GRAPH_GRID, gx + gw, ly, COL_GRAPH_GRID, 1.0f, 0.0f);
    }
    drawTextFmt(textBuf, gx + 4.0f, gy + 1.0f, 0.36f, COL_TEXT_MUTED, C2D_AlignLeft, "4.2V");
    drawTextFmt(textBuf, gx + 4.0f, gy + gh - 13.0f, 0.36f, COL_TEXT_MUTED, C2D_AlignLeft, "3.3V");

    if (graph->count > 1) {
        float stepX = gw / (float)(GRAPH_HISTORY_LEN - 1);
        for (u32 i = 1; i < graph->count; i++) {
            float v0 = (float)graph->samples[i - 1];
            float v1 = (float)graph->samples[i];
            if (v0 < 3300.0f) v0 = 3300.0f;
            if (v0 > 4250.0f) v0 = 4250.0f;
            if (v1 < 3300.0f) v1 = 3300.0f;
            if (v1 > 4250.0f) v1 = 4250.0f;

            float x0 = gx + (float)(i - 1) * stepX;
            float y0 = gy + gh - ((v0 - 3300.0f) / 950.0f) * gh;
            float x1 = gx + (float)i * stepX;
            float y1 = gy + gh - ((v1 - 3300.0f) / 950.0f) * gh;

            C2D_DrawLine(x0, y0, COL_ACCENT_CYAN, x1, y1, COL_ACCENT_CYAN, 2.0f, 0.0f);
        }
    }
}

/**
 * Dibuja la pantalla inferior (320x240) con Bati Bot animado, interruptor de Música ON/OFF y Selector de Idioma.
 */
static void renderBottomScreen2D(C3D_RenderTarget *botTarget,
                                 C2D_TextBuf textBuf,
                                 AppLang lang,
                                 const ConsoleBatterySpec *spec,
                                 const PowerReading *pwr,
                                 const SagTestResult *lastTest,
                                 u32 tipIndex,
                                 u32 wouppAnimFrames,
                                 u32 animTick,
                                 bool bgmEnabled) {
    C2D_TargetClear(botTarget, COL_BG_DARK);
    C2D_SceneBegin(botTarget);

    /* Header inferior + Botón Música (ON/OFF) + Botón Idioma */
    C2D_DrawRectSolid(0.0f, 0.0f, 0.0f, BOT_WIDTH, 26.0f, COL_BG_HEADER);
    drawTextFmt(textBuf, 6.0f, 5.0f, 0.45f, COL_ACCENT_CYAN, C2D_AlignLeft, "BATI BOT - BY ELVICHOPO");

    /* Botón para encender/apagar la canción de fondo (Táctil o botón Y) */
    drawCard(BTN_BGM_X, BTN_BGM_Y, BTN_BGM_W, BTN_BGM_H,
             bgmEnabled ? COL_GREEN_DARK : COL_CARD_ALT,
             bgmEnabled ? COL_GREEN_OK : COL_CARD_BORDER);
    drawTextFmt(textBuf, BTN_BGM_X + BTN_BGM_W * 0.5f, BTN_BGM_Y + 3.0f, 0.40f,
                bgmEnabled ? COL_GREEN_OK : COL_TEXT_MUTED, C2D_AlignCenter,
                "♪ %s [Y]", bgmEnabled ? "ON" : "OFF");

    /* Botón de cambio de idioma (Táctil o SELECT) */
    drawCard(BTN_LANG_X, BTN_LANG_Y, BTN_LANG_W, BTN_LANG_H, COL_CARD_ALT, COL_ACCENT_CYAN);
    drawTextFmt(textBuf, BTN_LANG_X + BTN_LANG_W * 0.5f, BTN_LANG_Y + 3.0f, 0.40f, COL_ACCENT_CYAN, C2D_AlignCenter,
                "%s [SEL]", getLangCode(lang));

    /* Obtener diálogo y estado de ánimo de Bati Bot */
    char dlgTitle[96];
    char dlgBody[256];
    int mood = 0;
    getBatiBotDialogue(lang, spec, pwr, lastTest, tipIndex,
                       dlgTitle, sizeof(dlgTitle), dlgBody, sizeof(dlgBody), &mood);

    float bounce = 0.0f;
    if (wouppAnimFrames > 0) {
        mood = 3;
        bounce = -4.0f * fabsf(sinf((float)wouppAnimFrames * 0.35f));
    }

    /* Globo de diálogo a la izquierda (x=8..220, y=32..138) */
    drawCard(8.0f, 32.0f, 214.0f, 106.0f, COL_BUBBLE_BG,
             (wouppAnimFrames > 0) ? COL_ACCENT_CYAN : COL_CARD_BORDER);

    C2D_DrawTriangle(222.0f, 68.0f, COL_BUBBLE_BG,
                     228.0f, 74.0f, COL_BUBBLE_BG,
                     222.0f, 80.0f, COL_BUBBLE_BG, 0.0f);

    if (wouppAnimFrames > 10) {
        drawTextFmt(textBuf, 214.0f, 36.0f, 0.40f, COL_YELLOW_WARN, C2D_AlignRight, "Woupp! ♪");
    }

    drawTextFmt(textBuf, 15.0f, 37.0f, 0.43f, COL_ACCENT_CYAN, C2D_AlignLeft, "%s", dlgTitle);

    C2D_Text bodyText;
    C2D_TextParse(&bodyText, textBuf, dlgBody);
    C2D_TextOptimize(&bodyText);
    C2D_DrawText(&bodyText, C2D_WithColor | C2D_WordWrap, 15.0f, 55.0f, 0.0f, 0.41f, 0.41f, COL_TEXT_MAIN, 198.0f);

    /* Dibuja a Bati Bot animado en la esquina derecha */
    drawBatiBot(textBuf, ASST_BOX_X, ASST_BOX_Y, mood, bounce, animTick, lang, true);

    /* Banda de resumen técnico del último test (y=143..186) */
    if (lastTest->completed) {
        bool barDropped = (lastTest->minLoadBars < lastTest->restBars);
        u32 verdBg = COL_GREEN_DARK;
        u32 verdBorder = COL_GREEN_OK;
        const char *verdLabel = (lang == LANG_EN) ? "EXCELLENT" :
                                (lang == LANG_PT) ? "EXCELENTE" :
                                (lang == LANG_FR) ? "EXCELLENTE" : "EXCELENTE";

        if (lastTest->sagMv > 150 || barDropped) {
            verdBg = COL_RED_DARK;
            verdBorder = COL_RED_CRIT;
            verdLabel = (lang == LANG_EN) ? "REPLACE BAT" :
                        (lang == LANG_PT) ? "TROCAR BAT." :
                        (lang == LANG_FR) ? "A REMPLACER" : "CAMBIAR BAT";
        } else if (lastTest->sagMv > 60) {
            verdBg = COL_YELLOW_DARK;
            verdBorder = COL_YELLOW_WARN;
            verdLabel = (lang == LANG_EN) ? "MODERATE" :
                        (lang == LANG_PT) ? "MODERADA" :
                        (lang == LANG_FR) ? "MODEREE" : "MODERADA";
        }

        drawCard(8.0f, 143.0f, 304.0f, 43.0f, verdBg, verdBorder);
        drawTextFmt(textBuf, 16.0f, 147.0f, 0.45f, verdBorder, C2D_AlignLeft,
                    "[%s]  Delta V: %ld mV (%s)",
                    verdLabel, (long)lastTest->sagMv,
                    lastTest->n3dsTurboUsed ? "804 MHz" : "268 MHz");
        drawTextFmt(textBuf, 16.0f, 166.0f, 0.41f, COL_TEXT_MAIN, C2D_AlignLeft,
                    "V_rest: %lu.%03luV -> V_min: %lu.%03luV",
                    (unsigned long)(lastTest->restMv / 1000), (unsigned long)(lastTest->restMv % 1000),
                    (unsigned long)(lastTest->minLoadMv / 1000), (unsigned long)(lastTest->minLoadMv % 1000));
    } else {
        drawCard(8.0f, 143.0f, 304.0f, 43.0f, COL_CARD_BG, COL_CARD_BORDER);
        const char *readyLine1 = (lang == LANG_EN) ? "Ready for Internal Resistance Sag Test (5s)" :
                                 (lang == LANG_PT) ? "Pronto para Teste de Resistencia Interna (5s)" :
                                 (lang == LANG_FR) ? "Pret pour le Test de Resistance Interne (5s)" :
                                                     "Listo para Test de Resistencia Interna (5s)";
        const char *readyLine2 = (lang == LANG_EN) ? "<=60mV: Excellent | 60-150mV: Good | >150mV: Replace" :
                                 (lang == LANG_PT) ? "<=60mV: Excelente | 60-150mV: Bom | >150mV: Trocar" :
                                 (lang == LANG_FR) ? "<=60mV: Excellent | 60-150mV: Bon | >150mV: Changer" :
                                                     "<=60mV: Excelente | 60-150mV: Normal | >150mV: Cambiar";
        drawTextFmt(textBuf, 16.0f, 148.0f, 0.43f, COL_ACCENT_CYAN, C2D_AlignLeft, "%s", readyLine1);
        drawTextFmt(textBuf, 16.0f, 166.0f, 0.39f, COL_TEXT_MUTED, C2D_AlignLeft, "%s", readyLine2);
    }

    /* Botón Izquierdo: Iniciar Voltage Sag Test */
    bool blockedByCharger = (pwr->adapterPlugged || pwr->isCharging);
    u32 btnBg     = blockedByCharger ? COL_CARD_ALT : COL_GREEN_DARK;
    u32 btnBorder = blockedByCharger ? COL_YELLOW_WARN : COL_GREEN_OK;

    drawCard(BTN_TEST_X, BTN_TEST_Y, BTN_TEST_W, BTN_TEST_H, btnBg, btnBorder);
    if (blockedByCharger) {
        const char *unplugMsg = (lang == LANG_EN) ? "UNPLUG AC CHARGER" :
                                (lang == LANG_PT) ? "DESCONECTE O AC" :
                                (lang == LANG_FR) ? "DEBRANCHEZ LE CHARGEUR" :
                                                    "DESCONECTA EL CARGADOR";
        drawTextFmt(textBuf, BTN_TEST_X + BTN_TEST_W * 0.5f, BTN_TEST_Y + 12.0f, 0.44f, COL_YELLOW_WARN, C2D_AlignCenter,
                    "%s", unplugMsg);
    } else {
        const char *startMsg = (lang == LANG_EN) ? "[A] START SAG TEST" :
                               (lang == LANG_PT) ? "[A] INICIAR TESTE" :
                               (lang == LANG_FR) ? "[A] LANCER LE TEST" :
                                                   "[A] INICIAR SAG TEST";
        drawTextFmt(textBuf, BTN_TEST_X + BTN_TEST_W * 0.5f, BTN_TEST_Y + 6.0f, 0.47f, COL_GREEN_OK, C2D_AlignCenter,
                    "%s", startMsg);
        drawTextFmt(textBuf, BTN_TEST_X + BTN_TEST_W * 0.5f, BTN_TEST_Y + 23.0f, 0.37f, COL_TEXT_MAIN, C2D_AlignCenter,
                    "START: Exit / Salir");
    }

    /* Botón Derecho: 10 Consejos de Bati Bot ([X] o Táctil) */
    drawCard(BTN_TIPS_X, BTN_TIPS_Y, BTN_TIPS_W, BTN_TIPS_H, COL_CARD_ALT, COL_ACCENT_CYAN);
    const char *tipsBtn1 = (lang == LANG_EN) ? "[X] 10 TIPS" :
                           (lang == LANG_PT) ? "[X] 10 DICAS" :
                           (lang == LANG_FR) ? "[X] 10 ASTUCES" :
                                               "[X] CONSEJOS";
    const char *tipsBtn2 = (lang == LANG_EN) ? "Bati Bot SOS" :
                           (lang == LANG_PT) ? "Bati Bot SOS" :
                           (lang == LANG_FR) ? "Bati Bot SOS" :
                                               "Bati Bot (10)";
    drawTextFmt(textBuf, BTN_TIPS_X + BTN_TIPS_W * 0.5f, BTN_TIPS_Y + 6.0f, 0.44f, COL_ACCENT_CYAN, C2D_AlignCenter,
                "%s", tipsBtn1);
    drawTextFmt(textBuf, BTN_TIPS_X + BTN_TIPS_W * 0.5f, BTN_TIPS_Y + 23.0f, 0.38f, COL_TEXT_MAIN, C2D_AlignCenter,
                "%s", tipsBtn2);
}

static void renderBottomScreenTesting2D(C3D_RenderTarget *botTarget,
                                        C2D_TextBuf textBuf,
                                        AppLang lang,
                                        const char *phaseTitle,
                                        u32 progressPct,
                                        u32 baselineMv,
                                        u32 currentMv,
                                        u32 minLoadMv,
                                        bool isNewTurbo) {
    C2D_TargetClear(botTarget, COL_BG_DARK);
    C2D_SceneBegin(botTarget);

    C2D_DrawRectSolid(0.0f, 0.0f, 0.0f, BOT_WIDTH, 26.0f, COL_BG_HEADER);
    drawTextFmt(textBuf, BOT_WIDTH * 0.5f, 4.0f, 0.50f, COL_YELLOW_WARN, C2D_AlignCenter,
                "%s", (lang == LANG_EN) ? "RUNNING VOLTAGE SAG TEST..." : "EJECUTANDO VOLTAGE SAG TEST...");

    drawCard(14.0f, 36.0f, 292.0f, 190.0f, COL_CARD_BG, COL_ACCENT_CYAN);

    drawTextFmt(textBuf, BOT_WIDTH * 0.5f, 46.0f, 0.50f, COL_ACCENT_CYAN, C2D_AlignCenter,
                "%s", phaseTitle);

    drawTextFmt(textBuf, BOT_WIDTH * 0.5f, 68.0f, 0.44f, COL_TEXT_MUTED, C2D_AlignCenter,
                "Clock CPU: %s", isNewTurbo ? "804 MHz (New 3DS Turbo)" : "268 MHz (Standard)");

    float barX = 28.0f, barY = 94.0f, barW = 264.0f, barH = 24.0f;
    C2D_DrawRectSolid(barX, barY, 0.0f, barW, barH, COL_BG_DARK);
    float fillW = (barW - 4.0f) * ((float)(progressPct > 100 ? 100 : progressPct) / 100.0f);
    C2D_DrawRectSolid(barX + 2.0f, barY + 2.0f, 0.0f, fillW, barH - 4.0f, COL_ACCENT_CYAN);
    drawTextFmt(textBuf, BOT_WIDTH * 0.5f, barY + 4.0f, 0.48f, COL_TEXT_MAIN, C2D_AlignCenter,
                "%lu%%", (unsigned long)progressPct);

    s32 deltaMv = (s32)baselineMv - (s32)minLoadMv;
    if (deltaMv < 0) deltaMv = 0;

    drawTextFmt(textBuf, 30.0f, 132.0f, 0.48f, COL_TEXT_MAIN, C2D_AlignLeft,
                "V_rest : %lu.%03lu V (%lu mV)",
                (unsigned long)(baselineMv / 1000), (unsigned long)(baselineMv % 1000), (unsigned long)baselineMv);
    drawTextFmt(textBuf, 30.0f, 154.0f, 0.48f, COL_GREEN_OK, C2D_AlignLeft,
                "V_curr : %lu.%03lu V (%lu mV)",
                (unsigned long)(currentMv / 1000), (unsigned long)(currentMv % 1000), (unsigned long)currentMv);
    drawTextFmt(textBuf, 30.0f, 176.0f, 0.48f, COL_YELLOW_WARN, C2D_AlignLeft,
                "V_min  : %lu.%03lu V (%lu mV)",
                (unsigned long)(minLoadMv / 1000), (unsigned long)(minLoadMv % 1000), (unsigned long)minLoadMv);
    drawTextFmt(textBuf, 30.0f, 198.0f, 0.52f, COL_RED_CRIT, C2D_AlignLeft,
                "Delta V: %ld mV", (long)deltaMv);
}

/**
 * Pantalla interactiva de despedida positiva de Bati Bot al salir con START.
 */
static void showPositiveFarewellScreen(C3D_RenderTarget *topTarget,
                                       C3D_RenderTarget *botTarget,
                                       C2D_TextBuf textBuf,
                                       AppLang lang,
                                       const ConsoleBatterySpec *spec,
                                       const PowerReading *pwr,
                                       const SagTestResult *lastTest,
                                       const VoltageGraph *graph,
                                       AudioEngine *audio) {
    audioEngineSetBgm(audio, false);
    audioEnginePlay(audio, SFX_GOODBYE);

    bool needsReplacement = (lastTest->completed && (lastTest->sagMv > 150 || lastTest->minLoadBars < lastTest->restBars));

    for (u32 frame = 0; frame < 90; frame++) {
        C2D_TextBufClear(textBuf);
        C3D_FrameBegin(C3D_FRAME_SYNCDRAW);

        renderTopScreen2D(topTarget, textBuf, lang, spec, pwr, graph);

        C2D_TargetClear(botTarget, COL_BG_DARK);
        C2D_SceneBegin(botTarget);

        drawCard(14.0f, 16.0f, 292.0f, 208.0f, COL_BUBBLE_BG, COL_ACCENT_CYAN);

        float bounce = -5.0f * fabsf(sinf((float)frame * 0.22f));
        drawBatiBot(textBuf, 118.0f, 22.0f, 3, bounce, frame, lang, false);

        const char *byeTitle = "¡Bati Bot te desea felices partidas!";
        const char *byeBody  = "¡Disfruta mucho tu consola! Gracias por usar BatteryHealth3DS creado por ElVichoPo.";

        if (needsReplacement) {
            if (lang == LANG_ES) {
                byeTitle = "¡Animo y disfruta mucho tu consola!";
                byeBody  = "Recuerda que cambiar la bateria por una nueva es muy sencillo y dejara tu consola como nueva. ¡Saludos de Bati Bot y ElVichoPo!";
            } else if (lang == LANG_EN) {
                byeTitle = "See you soon! Enjoy your console!";
                byeBody  = "Remember that swapping in a new battery is super easy and will make your console feel brand new. By ElVichoPo!";
            } else if (lang == LANG_PT) {
                byeTitle = "Ate logo! Aproveite seu console!";
                byeBody  = "Lembre-se que trocar a bateria por uma nova e super facil e deixara seu console novinho em folha! By ElVichoPo.";
            } else {
                byeTitle = "A bientot ! Profite de ta console !";
                byeBody  = "N'oublie pas que remplacer la batterie est tres simple et rendra ta console comme neuve. Par ElVichoPo !";
            }
        } else {
            if (lang == LANG_EN) {
                byeTitle = "Bati Bot says: Enjoy your console!";
                byeBody  = "Your battery is ready for adventure! Have an amazing time playing your favorite games! Created by ElVichoPo.";
            } else if (lang == LANG_PT) {
                byeTitle = "Bati Bot diz: Aproveite seu console!";
                byeBody  = "Sua bateria esta pronta para a aventura! Divirta-se muito jogando seus jogos favoritos! Por ElVichoPo.";
            } else if (lang == LANG_FR) {
                byeTitle = "Bati Bot : Profite de ta console !";
                byeBody  = "Ta batterie est prete pour l'aventure ! Passe un excellent moment sur tes jeux preferes ! Par ElVichoPo.";
            }
        }

        drawTextFmt(textBuf, BOT_WIDTH * 0.5f, 112.0f, 0.48f, COL_GREEN_OK, C2D_AlignCenter,
                    "%s", byeTitle);

        C2D_Text msgText;
        C2D_TextParse(&msgText, textBuf, byeBody);
        C2D_TextOptimize(&msgText);
        C2D_DrawText(&msgText, C2D_WithColor | C2D_WordWrap | C2D_AlignCenter,
                     BOT_WIDTH * 0.5f, 136.0f, 0.0f, 0.41f, 0.41f, COL_TEXT_MAIN, 266.0f);

        C3D_FrameEnd(0);
    }
}

static void runVoltageSagTest2D(C3D_RenderTarget *topTarget,
                                C3D_RenderTarget *botTarget,
                                C2D_TextBuf textBuf,
                                AppLang lang,
                                const ConsoleBatterySpec *spec,
                                bool mcuReady,
                                VoltageGraph *graph,
                                SagTestResult *result,
                                AudioEngine *audio) {
    PowerReading current;
    samplePowerReading(&current, mcuReady);

    if (current.adapterPlugged || current.isCharging) {
        return;
    }

    audioEnginePlay(audio, SFX_WOUPP);

    bool isNew = false;
    APT_CheckNew3DS(&isNew);

    u64 startRestMs = osGetTime();
    u32 restSumMv   = 0;
    u32 restSamples = 0;
    u8  restBars    = current.ptmuBars;
    u8  restPct     = current.mcuPercent;

    while (aptMainLoop() && (osGetTime() - startRestMs) < 1000) {
        u64 elapsed = osGetTime() - startRestMs;
        samplePowerReading(&current, mcuReady);
        restSumMv += current.voltageMv;
        restSamples++;
        restBars = current.ptmuBars;
        restPct  = current.mcuPercent;
        graphPushSample(graph, current.voltageMv);

        u32 avgMv = restSumMv / restSamples;
        C2D_TextBufClear(textBuf);
        C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
        renderTopScreen2D(topTarget, textBuf, lang, spec, &current, graph);
        renderBottomScreenTesting2D(botTarget, textBuf, lang,
                                    (lang == LANG_EN) ? "Step 1/2: Measuring Rest Voltage (1s)"
                                                      : "Fase 1/2: Midiendo V_reposo (1s)",
                                    (u32)((elapsed * 100) / 1000),
                                    avgMv, current.voltageMv, avgMv, false);
        C3D_FrameEnd(0);
        svcSleepThread(40000000ULL);
    }

    u32 baselineMv = (restSamples > 0) ? (restSumMv / restSamples) : current.voltageMv;

    if (isNew) {
        osSetSpeedupEnable(true);
    }

    u32 minLoadMv   = baselineMv;
    u8  minLoadBars = restBars;
    u8  minLoadPct  = restPct;
    u32 totalIters  = 0;

    u64 startLoadMs  = osGetTime();
    u64 lastSampleMs = startLoadMs;
    volatile double sink = 1.000001;

    while (aptMainLoop()) {
        u64 nowMs = osGetTime();
        u64 elapsedMs = nowMs - startLoadMs;
        if (elapsedMs >= 5000) {
            break;
        }

        double acc = sink;
        for (int i = 1; i < 25000; i++) {
            acc += sqrt((double)i * 3.1415926535) * sin((double)i * 0.0174532925);
            acc = fmod(acc, 100000.0);
        }
        sink = acc;
        totalIters++;

        if ((nowMs - lastSampleMs) >= 100) {
            lastSampleMs = nowMs;
            samplePowerReading(&current, mcuReady);
            graphPushSample(graph, current.voltageMv);

            if (current.voltageMv < minLoadMv) {
                minLoadMv = current.voltageMv;
            }
            if (current.ptmuBars < minLoadBars) {
                minLoadBars = current.ptmuBars;
            }
            if (current.mcuPercent < minLoadPct) {
                minLoadPct = current.mcuPercent;
            }

            u32 pctDone = (u32)((elapsedMs * 100) / 5000);

            C2D_TextBufClear(textBuf);
            C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
            renderTopScreen2D(topTarget, textBuf, lang, spec, &current, graph);
            renderBottomScreenTesting2D(botTarget, textBuf, lang,
                                        (lang == LANG_EN) ? "Step 2/2: CPU/FPU Stress Load (5s)"
                                                          : "Fase 2/2: Estres CPU/FPU (5s)",
                                        pctDone,
                                        baselineMv, current.voltageMv, minLoadMv, isNew);
            C3D_FrameEnd(0);
        }
    }

    if (isNew) {
        osSetSpeedupEnable(false);
    }

    result->completed      = true;
    result->usedMcuMv      = current.mcuAvailable;
    result->n3dsTurboUsed  = isNew;
    result->restMv         = baselineMv;
    result->minLoadMv      = minLoadMv;
    result->sagMv          = (s32)baselineMv - (s32)minLoadMv;
    if (result->sagMv < 0) {
        result->sagMv = 0;
    }
    result->restBars       = restBars;
    result->minLoadBars    = minLoadBars;
    result->restPercent    = restPct;
    result->minLoadPercent = minLoadPct;
    result->iterations     = totalIters;

    audioEnginePlay(audio, SFX_SUCCESS);
}

int main(int argc, char **argv) {
    (void)argc;
    (void)argv;

    gfxInitDefault();
    C3D_Init(C3D_DEFAULT_CMDBUF_SIZE);
    C2D_Init(C2D_DEFAULT_MAX_OBJECTS);
    C2D_Prepare();

    C3D_RenderTarget *topTarget = C2D_CreateScreenTarget(GFX_TOP, GFX_LEFT);
    C3D_RenderTarget *botTarget = C2D_CreateScreenTarget(GFX_BOTTOM, GFX_LEFT);
    C2D_TextBuf textBuf = C2D_TextBufNew(4096);

    Result cfguRes  = cfguInit();
    Result ptmuRes  = ptmuInit();
    Result mcuRes   = mcuHwcInit();
    bool   mcuReady = R_SUCCEEDED(mcuRes);

    AudioEngine audio;
    audioEngineInit(&audio);

    AppLang currentLang = detectSystemLanguage();

    u8 modelId = CFG_MODEL_N2DSXL;
    if (R_SUCCEEDED(cfguRes)) {
        CFGU_GetSystemModel(&modelId);
    }

    bool isNew3DS = false;
    APT_CheckNew3DS(&isNew3DS);

    ConsoleBatterySpec spec = getConsoleBatterySpec(modelId, isNew3DS);

    SagTestResult lastTest;
    memset(&lastTest, 0, sizeof(lastTest));

    VoltageGraph graph;
    memset(&graph, 0, sizeof(graph));

    PowerReading power;
    samplePowerReading(&power, mcuReady);
    graphPushSample(&graph, power.voltageMv);

    /* Mostrar la pantalla de presentación 3D con Bati Bot y sonido agradable al ejecutar */
    showStartupSplashScreen(topTarget, botTarget, textBuf, currentLang, &audio);

    u32 tipIndex = 0;
    u32 wouppAnimFrames = 18;
    u32 animTick = 0;

    u64 lastRefreshMs = osGetTime();

    while (aptMainLoop()) {
        hidScanInput();
        u32 kDown = hidKeysDown();
        animTick++;

        if (kDown & KEY_START) {
            showPositiveFarewellScreen(topTarget, botTarget, textBuf, currentLang,
                                       &spec, &power, &lastTest, &graph, &audio);
            break;
        }

        /* Cambiar idioma con SELECT */
        if (kDown & KEY_SELECT) {
            currentLang = (AppLang)((currentLang + 1) % LANG_COUNT);
            wouppAnimFrames = 18;
            audioEnginePlay(&audio, SFX_WOUPP);
        }

        /* Encender / Apagar canción de fondo con botón Y */
        if (kDown & KEY_Y) {
            audioEngineSetBgm(&audio, !audio.bgmEnabled);
            audioEnginePlay(&audio, SFX_WOUPP);
        }

        /* Siguiente consejo de Bati Bot con botón X */
        if (kDown & KEY_X) {
            tipIndex = (tipIndex + 1) % (TOTAL_BATI_TIPS + 1);
            wouppAnimFrames = 22;
            audioEnginePlay(&audio, SFX_WOUPP);
        }

        /* Interacción con el lápiz táctil (Stylus) */
        bool touchStartTest = false;
        if (kDown & KEY_TOUCH) {
            touchPosition touch;
            hidTouchRead(&touch);

            /* 1. Tocar a Bati Bot o su globo de diálogo -> ¡Sonido "Woupp!" + siguiente consejo! */
            if (touch.px >= 8 && touch.px <= (ASST_BOX_X + ASST_BOX_W) &&
                touch.py >= ASST_BOX_Y && touch.py <= (ASST_BOX_Y + ASST_BOX_H)) {
                tipIndex = (tipIndex + 1) % (TOTAL_BATI_TIPS + 1);
                wouppAnimFrames = 24;
                audioEnginePlay(&audio, SFX_WOUPP);
            }
            /* 2. Tocar el botón de 10 Consejos de Bati Bot */
            else if (touch.px >= BTN_TIPS_X && touch.px <= (BTN_TIPS_X + BTN_TIPS_W) &&
                     touch.py >= BTN_TIPS_Y && touch.py <= (BTN_TIPS_Y + BTN_TIPS_H)) {
                tipIndex = (tipIndex == 0) ? 1 : ((tipIndex % TOTAL_BATI_TIPS) + 1);
                wouppAnimFrames = 24;
                audioEnginePlay(&audio, SFX_WOUPP);
            }
            /* 3. Tocar el botón de Música ON/OFF en la barra superior */
            else if (touch.px >= BTN_BGM_X && touch.px <= (BTN_BGM_X + BTN_BGM_W) &&
                     touch.py >= BTN_BGM_Y && touch.py <= (BTN_BGM_Y + BTN_BGM_H + 4)) {
                audioEngineSetBgm(&audio, !audio.bgmEnabled);
                audioEnginePlay(&audio, SFX_WOUPP);
            }
            /* 4. Tocar el selector de idioma */
            else if (touch.px >= BTN_LANG_X && touch.px <= (BTN_LANG_X + BTN_LANG_W) &&
                     touch.py >= BTN_LANG_Y && touch.py <= (BTN_LANG_Y + BTN_LANG_H + 4)) {
                currentLang = (AppLang)((currentLang + 1) % LANG_COUNT);
                wouppAnimFrames = 18;
                audioEnginePlay(&audio, SFX_WOUPP);
            }
            /* 5. Tocar el botón de Iniciar Voltage Sag Test */
            else if (touch.px >= BTN_TEST_X && touch.px <= (BTN_TEST_X + BTN_TEST_W) &&
                     touch.py >= BTN_TEST_Y && touch.py <= (BTN_TEST_Y + BTN_TEST_H)) {
                touchStartTest = true;
            }
        }

        if ((kDown & KEY_A) || touchStartTest) {
            samplePowerReading(&power, mcuReady);
            if (!power.adapterPlugged && !power.isCharging) {
                tipIndex = 0;
                runVoltageSagTest2D(topTarget, botTarget, textBuf, currentLang,
                                    &spec, mcuReady, &graph, &lastTest, &audio);
                wouppAnimFrames = 25;
                samplePowerReading(&power, mcuReady);
                graphPushSample(&graph, power.voltageMv);
                lastRefreshMs = osGetTime();
            } else {
                wouppAnimFrames = 15;
                audioEnginePlay(&audio, SFX_ALERT);
            }
        }

        if (wouppAnimFrames > 0) {
            wouppAnimFrames--;
        }

        u64 nowMs = osGetTime();
        if ((nowMs - lastRefreshMs) >= 250) {
            lastRefreshMs = nowMs;
            samplePowerReading(&power, mcuReady);
            graphPushSample(&graph, power.voltageMv);
        }

        C2D_TextBufClear(textBuf);
        C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
        renderTopScreen2D(topTarget, textBuf, currentLang, &spec, &power, &graph);
        renderBottomScreen2D(botTarget, textBuf, currentLang, &spec, &power,
                             &lastTest, tipIndex, wouppAnimFrames, animTick, audio.bgmEnabled);
        C3D_FrameEnd(0);
    }

    audioEngineExit(&audio);
    C2D_TextBufDelete(textBuf);
    C2D_Fini();
    C3D_Fini();

    if (mcuReady) {
        mcuHwcExit();
    }
    if (R_SUCCEEDED(ptmuRes)) {
        ptmuExit();
    }
    if (R_SUCCEEDED(cfguRes)) {
        cfguExit();
    }

    gfxExit();
    return 0;
}
