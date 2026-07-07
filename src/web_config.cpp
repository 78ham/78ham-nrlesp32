#include "web_config.h"

#include <WebServer.h>
#include <WiFi.h>
#include "state.h"
#include "storage.h"

static WebServer s_server(80);
WebConfigService WebConfig;

static constexpr size_t kMaxWebChannels = 8;

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

static void appendServerFields(String &html, size_t index, const ServerConfig &server) {
  String idx(index);
  html += F("<fieldset><legend>Server ");
  html += idx;
  html += F("</legend><label>Name</label><input name='srv_");
  html += idx;
  html += F("_name' maxlength='15' value='");
  html += server.name;
  html += F("'><label>Host</label><input name='srv_");
  html += idx;
  html += F("_host' maxlength='63' value='");
  html += server.host;
  html += F("'><label>Port</label><input name='srv_");
  html += idx;
  html += F("_port' type='number' value='");
  html += server.port;
  html += F("'><label>Channels</label><small>Name and Group ID, empty rows are skipped.</small>");
  for (size_t c = 0; c < kMaxWebChannels; ++c) {
    html += F("<div style='display:flex;gap:8px'><input name='srv_");
    html += idx;
    html += F("_ch_");
    html += String(c);
    html += F("_name' maxlength='15' placeholder='Name' style='flex:1' value='");
    if (c < server.channelCount) {
      html += server.channels[c].name;
    }
    html += F("'><input name='srv_");
    html += idx;
    html += F("_ch_");
    html += String(c);
    html += F("_group' type='number' placeholder='Group ID' style='width:100px' value='");
    if (c < server.channelCount) {
      html += server.channels[c].groupId;
    }
    html += F("'></div>");
  }
  html += F("</fieldset>");
}

String WebConfigService::renderPage() const {
  String html;
  html.reserve(12000);
  html += F("<!doctype html><html><head><meta charset='utf-8'><meta name='viewport' content='width=device-width,initial-scale=1'>");
  html += F("<title>78HAM ESP32</title><style>body{font-family:sans-serif;margin:16px;max-width:600px}input,select,button{width:100%;box-sizing:border-box;margin:4px 0 10px;padding:10px}button{background:#111;color:#fff;border:0;font-size:16px}fieldset{border:1px solid #ccc;margin:14px 0;padding:12px}legend{font-weight:bold}small{color:#666;display:block;margin-bottom:6px}label{font-weight:bold}.radio-group{display:flex;gap:20px;margin:10px 0}.radio-group label{font-weight:normal}</style></head><body>");
  html += F("<h3>78HAM ESP32</h3><form method='post' action='/save'>");

  html += F("<fieldset><legend>WiFi</legend><label>SSID</label><input name='wifi_ssid' value='");
  html += config_->wifi.ssid;
  html += F("'><label>Password</label><input name='wifi_password' type='password' value='");
  html += config_->wifi.password;
  html += F("'></fieldset>");

  html += F("<fieldset><legend>Device</legend><label>Callsign</label><input name='callsign' maxlength='6' value='");
  html += config_->device.callsign;
  html += F("'><label>Callsign SSID</label><input name='callsign_ssid' type='number' min='1' max='99' value='");
  html += config_->device.ssid;
  html += F("'></fieldset>");

  for (size_t i = 0; i < kMaxServers; ++i) {
    const ServerConfig &server = (i < config_->serverCount) ? config_->servers[i] : ServerConfig{};
    appendServerFields(html, i, server);
  }

  html += F("<fieldset><legend>Active Server</legend><div class='radio-group'>");
  for (size_t i = 0; i < kMaxServers; ++i) {
    html += F("<label><input type='radio' name='current_server' value='");
    html += String(i);
    html += F("'");
    if (i == config_->currentServer) {
      html += F(" checked");
    }
    html += F("> Server ");
    html += String(i + 1);
    html += F("</label>");
  }
  html += F("</div></fieldset>");

  const ServerConfig &activeSrv = activeServer(*config_);
  html += F("<fieldset><legend>Startup Channel</legend><small>Select which channel to start on after boot.</small>");
  if (activeSrv.channelCount == 0) {
    html += F("<p style='color:#999'>No channels configured. Add channels in the server section above.</p>");
  }
  for (size_t c = 0; c < activeSrv.channelCount; ++c) {
    html += F("<label style='display:flex;align-items:center;gap:8px;margin:4px 0;font-weight:normal'>");
    html += F("<input type='radio' name='current_channel' value='");
    html += String(c);
    html += F("'");
    if (c == config_->currentChannel) {
      html += F(" checked");
    }
    html += F("> ");
    html += activeSrv.channels[c].name;
    html += F(" <span style='color:#999;font-size:13px'>(Group ");
    html += activeSrv.channels[c].groupId;
    html += F(")</span></label>");
  }
  html += F("</fieldset>");

  html += F("<button type='submit'>Save &amp; Connect</button></form></body></html>");
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

  config_->currentServer = constrain(s_server.arg("current_server").toInt(), 0, static_cast<int>(kMaxServers) - 1);

  config_->serverCount = 0;
  for (size_t i = 0; i < kMaxServers; ++i) {
    String srvName = s_server.arg(String("srv_") + String(i) + "_name");
    String srvHost = s_server.arg(String("srv_") + String(i) + "_host");
    srvName.trim();
    srvHost.trim();
    if (srvName.length() == 0 && srvHost.length() == 0) {
      continue;
    }
    ServerConfig &server = config_->servers[config_->serverCount];
    server = ServerConfig{};
    setStringField(server.name, sizeof(server.name), srvName.length() > 0 ? srvName : String("Main"));
    setStringField(server.host, sizeof(server.host), srvHost.length() > 0 ? srvHost : String("101.133.166.204"));
    server.port = s_server.arg(String("srv_") + String(i) + "_port").toInt();
    if (server.port == 0) {
      server.port = kNrlDefaultPort;
    }
    server.localPort = kNrlDefaultPort;
    server.channelCount = 0;

    for (size_t c = 0; c < kMaxWebChannels && server.channelCount < kMaxChannelsPerServer; ++c) {
      String chName = s_server.arg(String("srv_") + String(i) + "_ch_" + String(c) + "_name");
      uint32_t groupId = s_server.arg(String("srv_") + String(i) + "_ch_" + String(c) + "_group").toInt();
      chName.trim();
      if (chName.length() == 0 || groupId == 0) {
        continue;
      }
      ChannelConfig &channel = server.channels[server.channelCount++];
      setStringField(channel.name, sizeof(channel.name), chName);
      channel.groupId = groupId;
    }
    if (server.channelCount == 0) {
      server.channelCount = 1;
      setStringField(server.channels[0].name, sizeof(server.channels[0].name), "Local");
      server.channels[0].groupId = 1;
    }
    ++config_->serverCount;
  }

  if (config_->serverCount == 0) {
    config_->serverCount = 1;
    ServerConfig &server = config_->servers[0];
    setStringField(server.name, sizeof(server.name), "Main");
    setStringField(server.host, sizeof(server.host), "101.133.166.204");
    server.port = kNrlDefaultPort;
    server.localPort = kNrlDefaultPort;
    server.channelCount = 1;
    setStringField(server.channels[0].name, sizeof(server.channels[0].name), "Local");
    server.channels[0].groupId = 1;
  }

  if (config_->currentServer >= config_->serverCount) {
    config_->currentServer = 0;
  }
  {
    const ServerConfig &active = config_->servers[config_->currentServer];
    config_->currentChannel = constrain(s_server.arg("current_channel").toInt(), 0, static_cast<int>(active.channelCount) - 1);
  }
  Storage.save(*config_);
  saved_ = true;

  String html;
  html += F("<!doctype html><html><head><meta charset='utf-8'><meta name='viewport' content='width=device-width,initial-scale=1'>");
  html += F("<title>Saved</title><style>body{font-family:sans-serif;margin:16px;max-width:600px}h3{color:green}</style></head><body>");
  html += F("<h3>Configuration Saved</h3><p>Device will connect to WiFi now.</p>");
  html += F("<p><b>Active Server:</b> ");
  html += config_->servers[config_->currentServer].name;
  html += F(" (");
  html += config_->servers[config_->currentServer].host;
  html += F(":");
  html += config_->servers[config_->currentServer].port;
  html += F(")</p><p><b>Channels:</b></p><ul>");
  const ServerConfig &active = config_->servers[config_->currentServer];
  for (size_t c = 0; c < active.channelCount; ++c) {
    html += F("<li>");
    html += active.channels[c].name;
    html += F(" (Group ");
    html += active.channels[c].groupId;
    html += F(")</li>");
  }
  html += F("</ul></body></html>");
  s_server.send(200, "text/html", html);
}