#pragma once

#include <Arduino.h>
#include "app_config.h"

class AudioService {
public:
  bool begin(AppConfig *config);
  void startTasks();
  void setPtt(bool pressed);
  void enqueueOpus(const uint8_t *payload, size_t len);

private:
  static void txTaskThunk(void *arg);
  static void rxTaskThunk(void *arg);
  void txTask();
  void rxTask();
  bool beginI2s();
  bool beginOpus();
  int encodeFrame(const int16_t *pcm, uint8_t *out, size_t outLen);
  int decodeFrame(const uint8_t *packet, size_t len, int16_t *pcm, size_t samples);

  AppConfig *config_ = nullptr;
  QueueHandle_t rxQueue_ = nullptr;
  volatile bool ptt_ = false;
  bool initialized_ = false;
  bool tasksStarted_ = false;
};

extern AudioService Audio;
