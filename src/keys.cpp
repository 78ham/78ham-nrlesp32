#include "keys.h"

#include "pins.h"

static constexpr uint32_t kDebounceMs = 30;
static constexpr uint32_t kLongPressMs = 2500;

KeyService Keys;

void KeyService::begin() {
  keys_[0] = {PIN_KEY_PTT, KeyId::Ptt};
  keys_[1] = {PIN_KEY_VOL_UP, KeyId::VolumeUp};
  keys_[2] = {PIN_KEY_VOL_DOWN, KeyId::VolumeDown};
  keys_[3] = {PIN_KEY_UP, KeyId::ChannelUp};
  keys_[4] = {PIN_KEY_DOWN, KeyId::ChannelDown};
  keys_[5] = {PIN_KEY_POWER, KeyId::Power};

  for (auto &key : keys_) {
    pinMode(key.pin, INPUT_PULLUP);
    key.lastRaw = digitalRead(key.pin) == LOW;
    key.stable = key.lastRaw;
    key.changedMs = millis();
  }
}

void KeyService::loop() {
  uint32_t now = millis();
  for (auto &key : keys_) {
    updateKey(key, now);
  }
}

void KeyService::setHandler(KeyHandler handler) {
  handler_ = handler;
}

void KeyService::updateKey(KeyState &key, uint32_t now) {
  bool raw = digitalRead(key.pin) == LOW;
  if (raw != key.lastRaw) {
    key.lastRaw = raw;
    key.changedMs = now;
  }

  if (now - key.changedMs >= kDebounceMs && raw != key.stable) {
    key.stable = raw;
    key.longSent = false;
    if (raw) {
      key.pressedMs = now;
    }
    if (handler_) {
      handler_(key.id, key.stable, false);
    }
  }

  if (key.stable && !key.longSent && now - key.pressedMs >= kLongPressMs) {
    key.longSent = true;
    if (handler_) {
      handler_(key.id, true, true);
    }
  }
}
