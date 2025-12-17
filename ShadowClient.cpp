#include "ShadowClient.h"

ShadowClient* ShadowClient::s_instance = nullptr;

ShadowClient::ShadowClient()
  : _net(), _mqtt(_net) {
  // build topic strings
  _updateTopic  = String("$aws/things/") + THING_NAME + "/shadow/update";
  _deltaTopic   = String("$aws/things/") + THING_NAME + "/shadow/update/delta";
  _commandTopic = commandTopic(); // from DeviceConfig
}

void ShadowClient::begin() {
  // configure TLS
  _net.setCACert(rootCA);
  _net.setCertificate(deviceCert);
  _net.setPrivateKey(privateKey);

  _mqtt.setServer(AWS_IOT_ENDPOINT, AWS_IOT_PORT);

  // register static callback which forwards to instance method
  s_instance = this;
  _mqtt.setCallback(ShadowClient::mqttCallbackStatic);

  // attempt initial connect
  connectIfNeeded();
}

bool ShadowClient::connected(){
  return _mqtt.connected();
}

void ShadowClient::connectIfNeeded() {
  if (_mqtt.connected()) return;
  Serial.printf("[ShadowClient] Connecting to AWS IoT as %s ...\n", CLIENT_ID);

  // attempt connect with CLIENT_ID
  if (_mqtt.connect(CLIENT_ID)) {
    Serial.println("[ShadowClient] MQTT connected");
    // subscribe to delta and command topics
    if (!_deltaTopic.isEmpty()) _mqtt.subscribe(_deltaTopic.c_str());
    if (!_commandTopic.isEmpty()) _mqtt.subscribe(_commandTopic.c_str());
    // also subscribe to accepted update ack if you want (optional)
    // String accepted = String("$aws/things/") + THING_NAME + "/shadow/update/accepted";
    // _mqtt.subscribe(accepted.c_str());
  } else {
    Serial.printf("[ShadowClient] MQTT connect failed, rc=%d\n", _mqtt.state());
  }
}

void ShadowClient::loop() {
  if (!_mqtt.connected()) {
    connectIfNeeded();
  }
  _mqtt.loop();
}

// user callback setters
void ShadowClient::onDesiredColor(std::function<void(const String&)> cb) {
  _cbDesiredColor = cb;
}
void ShadowClient::onScheduleUpdate(std::function<void(int,int)> cb) {
  _cbSchedule = cb;
}
void ShadowClient::onCommand(std::function<void(const String&)> cb) {
  _cbCommand = cb;
}

// static -> instance forwarder
void ShadowClient::mqttCallbackStatic(char* topic, byte* payload, unsigned int length) {
  if (!s_instance) return;
  s_instance->mqttCallback(topic, payload, length);
}

// instance MQTT callback - parse and dispatch
void ShadowClient::mqttCallback(char* topic, byte* payload, unsigned int length) {
  String t = String(topic);

  // collect payload as String
  String pl; pl.reserve(length + 1);
  for (unsigned int i = 0; i < length; ++i) pl += (char)payload[i];

  Serial.printf("[ShadowClient] Message on topic: %s\n", t.c_str());
  Serial.printf("[ShadowClient] Payload: %s\n", pl.c_str());

  // Delta topic -> configuration desired changes
  if (t == _deltaTopic) {
    handleDeltaPayload(pl);
    return;
  }

  // Command topic -> pass raw payload to command handler
  if (t == _commandTopic) {
    if (_cbCommand) _cbCommand(pl);
    else Serial.println("[ShadowClient] No command handler registered");
    return;
  }

  // else ignore
}
void ShadowClient::handleDeltaPayload(const String& payload) {
  StaticJsonDocument<512> doc;
  DeserializationError err = deserializeJson(doc, payload);
  if (err) {
    Serial.print("[ShadowClient] Delta JSON parse error: ");
    Serial.println(err.c_str());
    return;
  }
  JsonVariant state = doc["state"];
  if (state.isNull()) {
    Serial.println("[ShadowClient] Delta state missing");
    return;
  }

  // --- ONLY configuration via Shadow: pill_hour, pill_minute, buzzer_enabled, pill_name ---
  bool scheduleChanged = false;
  int h = -1, m = -1;
  if (state.containsKey("pill_hour")) {
    h = state["pill_hour"].as<int>();
    scheduleChanged = true;
  }
  if (state.containsKey("pill_minute")) {
    m = state["pill_minute"].as<int>();
    scheduleChanged = true;
  }
  if (scheduleChanged && _cbSchedule) {
    Serial.printf("[ShadowClient] Delta schedule -> %d:%d\n", h, m);
    _cbSchedule(h, m);
  }

  if (state.containsKey("buzzer_enabled")) {
    bool be = state["buzzer_enabled"].as<bool>();
    // notify via schedule callback with special marker or use separate callback (not shown)
    Serial.printf("[ShadowClient] Delta buzzer_enabled -> %d\n", be?1:0);
    // you may want a dedicated callback for buzzer - or include in schedule callback using global state
  }

  // Optional: support for config items like calibration values, wifi settings, etc.
  
}
// Publish reported configuration (minimize message size)
void ShadowClient::publishReportedConfig(int pillHour, int pillMinute, bool buzzerEnabled) {
  StaticJsonDocument<256> doc;
  JsonObject state = doc.createNestedObject("state");
  JsonObject reported = state.createNestedObject("reported");
  reported["pill_hour"] = pillHour;
  reported["pill_minute"] = pillMinute;
  reported["buzzer_enabled"] = buzzerEnabled;
  // include timestamp in Bolivia local epoch seconds
  reported["updated_at"] = (unsigned long)(time(nullptr) + BOLIVIA_OFFSET_SECONDS);

  char buf[512];
  size_t n = serializeJson(doc, buf);
  bool ok = _mqtt.publish(_updateTopic.c_str(), buf, n);
  Serial.printf("[ShadowClient] Published reported config (ok=%d)\n", ok ? 1 : 0);
}

// Publish dispense report: colors, angle, status and last_dispense (Bolivia local epoch)
void ShadowClient::publishDispenseReport(const String& dispensedColor,
                                        int dispensedAngle,
                                        const RGB& rgb,
                                        const String& status,
                                        unsigned long command_id) {
  StaticJsonDocument<384> doc;
  JsonObject state = doc.createNestedObject("state");
  JsonObject reported = state.createNestedObject("reported");

  reported["r"] = rgb.r;
  reported["g"] = rgb.g;
  reported["b"] = rgb.b;
  reported["dominant_color"] = dispensedColor;
  reported["dispensed_color"] = dispensedColor;
  reported["dispensed_angle"] = dispensedAngle;
  reported["dispense_status"] = status;
  reported["command_id"] = command_id;
  reported["last_dispense"] = (unsigned long)(time(nullptr) + BOLIVIA_OFFSET_SECONDS);

  char buf[512];
  size_t n = serializeJson(doc, buf);
  bool ok = _mqtt.publish(_updateTopic.c_str(), buf, n);
  Serial.printf("[ShadowClient] Published dispense report (ok=%d)\n", ok ? 1 : 0);
}

// Clear desired keys (set to null)
void ShadowClient::clearDesired() {
  StaticJsonDocument<128> doc;
  JsonObject state = doc.createNestedObject("state");
  JsonObject desired = state.createNestedObject("desired");
  desired["color"] = nullptr;
  desired["pill_hour"] = nullptr;
  desired["pill_minute"] = nullptr;
  desired["dispense_now"] = nullptr;
  char buf[128];
  size_t n = serializeJson(doc, buf);
  bool ok = _mqtt.publish(_updateTopic.c_str(), buf, n);
  Serial.printf("[ShadowClient] Published clearDesired (ok=%d)\n", ok ? 1 : 0);
}
