#pragma once

#include <Arduino.h>
#include "app_config.h"

class UiService {
public:
  bool begin(AppConfig *config);
  void loop();
  void showConfigPortal(const String &ssid, const String &password);

private:
  void drawMain();
  void drawHeader();
  void drawStatus();
  void drawFooter();

  AppConfig *config_ = nullptr;
  uint32_t lastDrawMs_ = 0;
  String portalSsid_;
  String portalPassword_;
};

extern UiService UI;
