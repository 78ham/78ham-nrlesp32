#pragma once

#include <Arduino.h>

enum class DeviceMode : uint8_t {
  Boot,
  ConfigPortal,
  Connecting,
  Idle,
  Tx,
  Rx,
  Offline,
  Error
};

struct RuntimeState {
  DeviceMode mode = DeviceMode::Boot;
  bool wifiConnected = false;
  bool udpReady = false;
  bool pttPressed = false;
  int rssi = 0;
  IPAddress ip;
  char remoteCallsign[8] = "";
  uint8_t remoteSsid = 0;
  uint32_t lastRxMs = 0;
  uint32_t lastTxMs = 0;
  char statusText[32] = "Booting";
};

extern RuntimeState State;
const char *modeName(DeviceMode mode);
