#include "audio.h"

#include <driver/i2s.h>
#include "network.h"
#include "pins.h"
#include "state.h"

#ifndef I2S_COMM_FORMAT_STAND_I2S
#define I2S_COMM_FORMAT_STAND_I2S I2S_COMM_FORMAT_I2S
#endif

#if __has_include(<opus.h>)
#include <opus.h>
#define NRL_HAS_OPUS 1
#else
#define NRL_HAS_OPUS 0
using OpusEncoder = void;
using OpusDecoder = void;
#endif

struct RxOpusFrame {
  uint16_t len = 0;
  uint8_t data[kOpusMaxPacketBytes];
};

AudioService Audio;

static OpusEncoder *s_encoder = nullptr;
static OpusDecoder *s_decoder = nullptr;

bool AudioService::begin(AppConfig *config) {
  if (initialized_) {
    config_ = config;
    return true;
  }
  config_ = config;
  rxQueue_ = xQueueCreate(10, sizeof(RxOpusFrame));
  initialized_ = rxQueue_ && beginI2s() && beginOpus();
  return initialized_;
}

bool AudioService::beginI2s() {
  i2s_config_t rxConfig = {};
  rxConfig.mode = static_cast<i2s_mode_t>(I2S_MODE_MASTER | I2S_MODE_RX);
  rxConfig.sample_rate = kOpusSampleRate;
  rxConfig.bits_per_sample = I2S_BITS_PER_SAMPLE_32BIT;
  rxConfig.channel_format = I2S_CHANNEL_FMT_ONLY_LEFT;
  rxConfig.communication_format = I2S_COMM_FORMAT_STAND_I2S;
  rxConfig.intr_alloc_flags = ESP_INTR_FLAG_LEVEL1;
  rxConfig.dma_buf_count = 6;
  rxConfig.dma_buf_len = 160;
  rxConfig.use_apll = false;
  rxConfig.tx_desc_auto_clear = false;
  rxConfig.fixed_mclk = 0;

  i2s_pin_config_t rxPins = {};
  rxPins.bck_io_num = PIN_MIC_BCLK;
  rxPins.ws_io_num = PIN_MIC_WS;
  rxPins.data_out_num = I2S_PIN_NO_CHANGE;
  rxPins.data_in_num = PIN_MIC_SD;

  i2s_config_t txConfig = {};
  txConfig.mode = static_cast<i2s_mode_t>(I2S_MODE_MASTER | I2S_MODE_TX);
  txConfig.sample_rate = kOpusSampleRate;
  txConfig.bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT;
  txConfig.channel_format = I2S_CHANNEL_FMT_ONLY_LEFT;
  txConfig.communication_format = I2S_COMM_FORMAT_STAND_I2S;
  txConfig.intr_alloc_flags = ESP_INTR_FLAG_LEVEL1;
  txConfig.dma_buf_count = 6;
  txConfig.dma_buf_len = 160;
  txConfig.use_apll = false;
  txConfig.tx_desc_auto_clear = true;
  txConfig.fixed_mclk = 0;

  i2s_pin_config_t txPins = {};
  txPins.bck_io_num = PIN_AMP_BCLK;
  txPins.ws_io_num = PIN_AMP_LRCLK;
  txPins.data_out_num = PIN_AMP_DIN;
  txPins.data_in_num = I2S_PIN_NO_CHANGE;

  pinMode(PIN_AMP_SD, OUTPUT);
  digitalWrite(PIN_AMP_SD, HIGH);

  esp_err_t rx = i2s_driver_install(I2S_NUM_0, &rxConfig, 0, nullptr);
  esp_err_t rxPin = i2s_set_pin(I2S_NUM_0, &rxPins);
  esp_err_t tx = i2s_driver_install(I2S_NUM_1, &txConfig, 0, nullptr);
  esp_err_t txPin = i2s_set_pin(I2S_NUM_1, &txPins);
  return rx == ESP_OK && rxPin == ESP_OK && tx == ESP_OK && txPin == ESP_OK;
}

bool AudioService::beginOpus() {
#if NRL_HAS_OPUS
  int err = OPUS_OK;
  s_encoder = opus_encoder_create(kOpusSampleRate, kOpusChannels, OPUS_APPLICATION_VOIP, &err);
  if (err != OPUS_OK || !s_encoder) {
    return false;
  }
  opus_encoder_ctl(s_encoder, OPUS_SET_BITRATE(kOpusBitrate));
  opus_encoder_ctl(s_encoder, OPUS_SET_COMPLEXITY(kOpusComplexity));
  opus_encoder_ctl(s_encoder, OPUS_SET_VBR(1));
  opus_encoder_ctl(s_encoder, OPUS_SET_SIGNAL(OPUS_SIGNAL_VOICE));

  s_decoder = opus_decoder_create(kOpusSampleRate, kOpusChannels, &err);
  return err == OPUS_OK && s_decoder;
#else
  Serial.println("Opus headers not found. Check PlatformIO lib_deps.");
  return false;
#endif
}

void AudioService::startTasks() {
  if (tasksStarted_) {
    return;
  }
  xTaskCreatePinnedToCore(txTaskThunk, "audio_tx", 12288, this, 5, nullptr, 1);
  xTaskCreatePinnedToCore(rxTaskThunk, "audio_rx", 12288, this, 5, nullptr, 1);
  tasksStarted_ = true;
}

void AudioService::setPtt(bool pressed) {
  ptt_ = pressed;
  State.pttPressed = pressed;
  if (pressed) {
    State.mode = DeviceMode::Tx;
  } else if (State.mode == DeviceMode::Tx) {
    State.mode = DeviceMode::Idle;
  }
}

void AudioService::enqueueOpus(const uint8_t *payload, size_t len) {
  if (!payload || len == 0 || len > kOpusMaxPacketBytes || !rxQueue_) {
    return;
  }
  RxOpusFrame frame;
  frame.len = len;
  memcpy(frame.data, payload, len);
  xQueueSend(rxQueue_, &frame, 0);
}

void AudioService::txTaskThunk(void *arg) {
  static_cast<AudioService *>(arg)->txTask();
}

void AudioService::rxTaskThunk(void *arg) {
  static_cast<AudioService *>(arg)->rxTask();
}

int AudioService::encodeFrame(const int16_t *pcm, uint8_t *out, size_t outLen) {
#if NRL_HAS_OPUS
  return opus_encode(s_encoder, pcm, kOpusFrameSamples, out, outLen);
#else
  return -1;
#endif
}

int AudioService::decodeFrame(const uint8_t *packet, size_t len, int16_t *pcm, size_t samples) {
#if NRL_HAS_OPUS
  return opus_decode(s_decoder, packet, len, pcm, samples, 0);
#else
  return -1;
#endif
}

void AudioService::txTask() {
  int32_t raw[kOpusFrameSamples];
  int16_t pcm[kOpusFrameSamples];
  uint8_t opus[kOpusMaxPacketBytes];

  for (;;) {
    size_t bytesRead = 0;
    i2s_read(I2S_NUM_0, raw, sizeof(raw), &bytesRead, portMAX_DELAY);
    size_t samples = min(bytesRead / sizeof(int32_t), static_cast<size_t>(kOpusFrameSamples));
    for (size_t i = 0; i < samples; ++i) {
      int32_t sample = raw[i] >> 14;
      sample = constrain(sample, -32768, 32767);
      pcm[i] = static_cast<int16_t>(sample);
    }
    if (samples < kOpusFrameSamples) {
      memset(pcm + samples, 0, (kOpusFrameSamples - samples) * sizeof(int16_t));
    }

    if (ptt_) {
      int len = encodeFrame(pcm, opus, sizeof(opus));
      if (len > 0) {
        Network.sendVoiceOpus(opus, len);
      }
    }
  }
}

void AudioService::rxTask() {
  RxOpusFrame frame;
  int16_t pcm[kOpusFrameSamples];
  for (;;) {
    if (xQueueReceive(rxQueue_, &frame, portMAX_DELAY) == pdTRUE) {
      int samples = decodeFrame(frame.data, frame.len, pcm, kOpusFrameSamples);
      if (samples > 0) {
        size_t bytesWritten = 0;
        i2s_write(I2S_NUM_1, pcm, samples * sizeof(int16_t), &bytesWritten, portMAX_DELAY);
      }
    }
  }
}
