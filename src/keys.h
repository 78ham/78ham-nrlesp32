#pragma once

#include <Arduino.h>

enum class KeyId : uint8_t {
  Ptt,
  VolumeUp,
  VolumeDown,
  ChannelUp,
  ChannelDown,
  Power
};

using KeyHandler = void (*)(KeyId key, bool pressed, bool longPress);

class KeyService {
public:
  void begin();
  void loop();
  void setHandler(KeyHandler handler);

private:
  struct KeyState {
    int pin = -1;
    KeyId id = KeyId::Ptt;
    bool stable = false;
    bool lastRaw = false;
    bool longSent = false;
    uint32_t changedMs = 0;
    uint32_t pressedMs = 0;
  };

  void updateKey(KeyState &key, uint32_t now);

  KeyHandler handler_ = nullptr;
  KeyState keys_[6];
};

extern KeyService Keys;
