#include "state.h"

RuntimeState State;

const char *modeName(DeviceMode mode) {
  switch (mode) {
  case DeviceMode::Boot:
    return "BOOT";
  case DeviceMode::ConfigPortal:
    return "CONFIG";
  case DeviceMode::Connecting:
    return "CONNECT";
  case DeviceMode::Idle:
    return "IDLE";
  case DeviceMode::Tx:
    return "TX";
  case DeviceMode::Rx:
    return "RX";
  case DeviceMode::Offline:
    return "OFFLINE";
  case DeviceMode::Error:
    return "ERROR";
  }
  return "UNKNOWN";
}
