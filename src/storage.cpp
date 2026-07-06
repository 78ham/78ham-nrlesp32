#include "storage.h"

#include <ArduinoJson.h>
#include <LittleFS.h>
#include <Preferences.h>
#include <WiFi.h>

static constexpr const char *kConfigPath = "/config.json";
static constexpr const char *kPrefsNamespace = "78ham";

StorageService Storage;

static String randomReadablePassword() {
  static constexpr char alphabet[] = "23456789ABCDEFGHJKLMNPQRSTUVWXYZ";
  String password;
  for (int i = 0; i < 10; ++i) {
    password += alphabet[esp_random() % (sizeof(alphabet) - 1)];
  }
  return password;
}

bool StorageService::begin() {
  return LittleFS.begin(true);
}

bool StorageService::load(AppConfig &config) {
  if (!LittleFS.exists(kConfigPath)) {
    return false;
  }

  File file = LittleFS.open(kConfigPath, "r");
  if (!file) {
    return false;
  }

  JsonDocument doc;
  DeserializationError err = deserializeJson(doc, file);
  file.close();
  if (err) {
    return false;
  }

  config.version = doc["version"] | 1;
  setStringField(config.device.callsign, sizeof(config.device.callsign), doc["device"]["callsign"] | "NOCALL");
  config.device.ssid = doc["device"]["ssid"] | kDefaultCallsignSsid;
  config.device.devModel = doc["device"]["dev_model"] | kDefaultDeviceModel;
  config.device.volume = doc["device"]["volume"] | 70;
  config.device.micGain = doc["device"]["mic_gain"] | 100;
  String codec = doc["device"]["voice_codec"] | "opus";
  config.device.opusEnabled = codec != "g711";
  setStringField(config.wifi.ssid, sizeof(config.wifi.ssid), doc["wifi"]["ssid"] | "");
  setStringField(config.wifi.password, sizeof(config.wifi.password), doc["wifi"]["password"] | "");

  JsonArray servers = doc["servers"].as<JsonArray>();
  config.serverCount = min(static_cast<size_t>(servers.size()), kMaxServers);
  if (config.serverCount == 0) {
    config.serverCount = 1;
  }

  for (size_t i = 0; i < config.serverCount; ++i) {
    JsonObject src = servers[i];
    ServerConfig &server = config.servers[i];
    setStringField(server.name, sizeof(server.name), src["name"] | "Main");
    setStringField(server.host, sizeof(server.host), src["host"] | "101.133.166.204");
    server.port = src["port"] | kNrlDefaultPort;
    server.localPort = src["local_port"] | kNrlDefaultPort;

    JsonArray channels = src["channels"].as<JsonArray>();
    server.channelCount = min(static_cast<size_t>(channels.size()), kMaxChannelsPerServer);
    if (server.channelCount == 0) {
      server.channelCount = 1;
    }
    for (size_t c = 0; c < server.channelCount; ++c) {
      setStringField(server.channels[c].name, sizeof(server.channels[c].name), channels[c]["name"] | "Local");
      server.channels[c].groupId = channels[c]["group_id"] | 1;
    }
  }

  config.currentServer = doc["current_server"] | 0;
  config.currentChannel = doc["current_channel"] | 0;
  activeChannel(config);
  return true;
}

bool StorageService::save(const AppConfig &config) {
  JsonDocument doc;
  doc["version"] = config.version;
  doc["device"]["callsign"] = config.device.callsign;
  doc["device"]["ssid"] = config.device.ssid;
  doc["device"]["dev_model"] = config.device.devModel;
  doc["device"]["volume"] = config.device.volume;
  doc["device"]["mic_gain"] = config.device.micGain;
  doc["device"]["voice_codec"] = config.device.opusEnabled ? "opus" : "g711";
  doc["wifi"]["ssid"] = config.wifi.ssid;
  doc["wifi"]["password"] = config.wifi.password;
  doc["current_server"] = config.currentServer;
  doc["current_channel"] = config.currentChannel;

  JsonArray servers = doc["servers"].to<JsonArray>();
  for (size_t i = 0; i < config.serverCount && i < kMaxServers; ++i) {
    const ServerConfig &server = config.servers[i];
    JsonObject dst = servers.add<JsonObject>();
    dst["name"] = server.name;
    dst["host"] = server.host;
    dst["port"] = server.port;
    dst["local_port"] = server.localPort;
    JsonArray channels = dst["channels"].to<JsonArray>();
    for (size_t c = 0; c < server.channelCount && c < kMaxChannelsPerServer; ++c) {
      JsonObject ch = channels.add<JsonObject>();
      ch["name"] = server.channels[c].name;
      ch["group_id"] = server.channels[c].groupId;
    }
  }

  File file = LittleFS.open(kConfigPath, "w");
  if (!file) {
    return false;
  }
  bool ok = serializeJsonPretty(doc, file) > 0;
  file.close();
  return ok;
}

String StorageService::apSsid() {
  uint64_t mac = ESP.getEfuseMac();
  char suffix[7];
  snprintf(suffix, sizeof(suffix), "%06X", static_cast<unsigned int>(mac & 0xFFFFFF));
  return String("78HAM-ESP32-") + suffix;
}

String StorageService::ensureApPassword() {
  Preferences prefs;
  prefs.begin(kPrefsNamespace, false);
  String password = prefs.getString("ap_pass", "");
  if (password.length() != 10) {
    password = randomReadablePassword();
    prefs.putString("ap_pass", password);
  }
  prefs.end();
  return password;
}

String StorageService::apPassword() {
  return ensureApPassword();
}

void StorageService::resetConfig() {
  LittleFS.remove(kConfigPath);
}
