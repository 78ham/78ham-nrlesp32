#include "web_config.h"

#include <WebServer.h>
#include <WiFi.h>
#include "state.h"
#include "storage.h"

static WebServer s_server(80);
WebConfigService WebConfig;

bool WebConfigService::begin(AppConfig *config, const String &apSsid, const String &apPassword) {
  config_ = config;
  saved_ = false;
  State.mode = DeviceMode::ConfigPortal;
  strlcpy(State.statusText, "Config portal", sizeof(State.statusText));
  WiFi.mode(WIFI_AP);
  WiFi.softAP(apSsid.c_str(), apPassword.c_str());
  s_server.on("/", [this]() { handleRoot(); });
  s_server.on("/save", HTTP_POST, [this]() { handleSave(); });
  s_server.begin();
  return true;
}

void WebConfigService::loop() {
  s_server.handleClient();
}

bool WebConfigService::saved() const {
  return saved_;
}

void WebConfigService::stop() {
  s_server.stop();
  WiFi.softAPdisconnect(true);
}

String WebConfigService::renderPage() const {
  const ServerConfig &server = activeServer(*config_);
  String html;
  html.reserve(6000);
  html += F("<!doctype html><html><head><meta charset='utf-8'><meta name='viewport' content='width=device-width,initial-scale=1'>");
  html += F("<title>78HAM ESP32</title><style>body{font-family:sans-serif;margin:16px;max-width:520px}input,button{width:100%;box-sizing:border-box;margin:5px 0 12px;padding:10px}button{background:#111;color:#fff;border:0}fieldset{border:1px solid #ccc;margin:12px 0}small{color:#666}</style></head><body>");
  html += F("<h3>78HAM ESP32</h3><form method='post' action='/save'>");
  html += F("<label>WiFi SSID</label><input name='wifi_ssid' value='");
  html += config_->wifi.ssid;
  html += F("'><label>WiFi Password</label><input name='wifi_password' type='password' value='");
  html += config_->wifi.password;
  html += F("'><label>Callsign</label><input name='callsign' maxlength='6' value='");
  html += config_->device.callsign;
  html += F("'><label>Callsign SSID</label><input name='callsign_ssid' type='number' min='1' max='99' value='");
  html += config_->device.ssid;
  html += F("'><fieldset><legend>Server</legend><label>Name</label><input name='server_name' maxlength='15' value='");
  html += server.name;
  html += F("'><label>Host</label><input name='server_host' maxlength='63' value='");
  html += server.host;
  html += F("'><label>Port</label><input name='server_port' type='number' value='");
  html += server.port;
  html += F("'></fieldset><fieldset><legend>Displayed Channels</legend><small>Only channels listed here appear on the device.</small>");
  for (size_t i = 0; i < 8; ++i) {
    html += F("<label>Channel ");
    html += String(i + 1);
    html += F(" Name</label><input name='ch_name_");
    html += String(i);
    html += F("' maxlength='15' value='");
    if (i < server.channelCount) {
      html += server.channels[i].name;
    }
    html += F("'><label>Channel ");
    html += String(i + 1);
    html += F(" Group ID</label><input name='ch_group_");
    html += String(i);
    html += F("' type='number' value='");
    if (i < server.channelCount) {
      html += server.channels[i].groupId;
    }
    html += F("'>");
  }
  html += F("</fieldset><button type='submit'>Save</button></form></body></html>");
  return html;
}

void WebConfigService::handleRoot() {
  s_server.send(200, "text/html", renderPage());
}

void WebConfigService::handleSave() {
  if (!config_) {
    s_server.send(500, "text/plain", "No config");
    return;
  }
  setStringField(config_->wifi.ssid, sizeof(config_->wifi.ssid), s_server.arg("wifi_ssid"));
  setStringField(config_->wifi.password, sizeof(config_->wifi.password), s_server.arg("wifi_password"));
  setStringField(config_->device.callsign, sizeof(config_->device.callsign), s_server.arg("callsign"));
  config_->device.ssid = constrain(s_server.arg("callsign_ssid").toInt(), 1, 99);

  config_->serverCount = 1;
  config_->currentServer = 0;
  ServerConfig &server = config_->servers[0];
  setStringField(server.name, sizeof(server.name), s_server.arg("server_name"));
  setStringField(server.host, sizeof(server.host), s_server.arg("server_host"));
  server.port = s_server.arg("server_port").toInt() > 0 ? s_server.arg("server_port").toInt() : kNrlDefaultPort;
  server.localPort = kNrlDefaultPort;
  server.channelCount = 0;

  for (size_t i = 0; i < 8 && server.channelCount < kMaxChannelsPerServer; ++i) {
    String name = s_server.arg(String("ch_name_") + String(i));
    uint32_t groupId = s_server.arg(String("ch_group_") + String(i)).toInt();
    name.trim();
    if (name.length() == 0 || groupId == 0) {
      continue;
    }
    ChannelConfig &channel = server.channels[server.channelCount++];
    setStringField(channel.name, sizeof(channel.name), name);
    channel.groupId = groupId;
  }
  if (server.channelCount == 0) {
    server.channelCount = 1;
    setStringField(server.channels[0].name, sizeof(server.channels[0].name), "Local");
    server.channels[0].groupId = 1;
  }
  config_->currentChannel = 0;
  Storage.save(*config_);
  saved_ = true;
  s_server.send(200, "text/html", "<html><body><h3>Saved</h3><p>Device will connect to WiFi.</p></body></html>");
}
