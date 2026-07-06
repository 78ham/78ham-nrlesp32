#pragma once

#include <Arduino.h>
#include "app_config.h"

class WebConfigService {
public:
  bool begin(AppConfig *config, const String &apSsid, const String &apPassword);
  void loop();
  bool saved() const;
  void stop();

private:
  String renderPage() const;
  void handleRoot();
  void handleSave();

  AppConfig *config_ = nullptr;
  bool saved_ = false;
};

extern WebConfigService WebConfig;
