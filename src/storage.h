#pragma once

#include <Arduino.h>
#include "app_config.h"

class StorageService {
public:
  bool begin();
  bool load(AppConfig &config);
  bool save(const AppConfig &config);
  String apSsid();
  String apPassword();
  void resetConfig();

private:
  String ensureApPassword();
};

extern StorageService Storage;
