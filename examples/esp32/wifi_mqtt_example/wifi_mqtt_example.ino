#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <math.h>
#include <kymo_runtime.h>

// Edit credentials, endpoint and channels here. Do not commit real secrets.
static const char *WIFI_SSID = "YourWiFiSSID";
static const char *WIFI_PASSWORD = "YourWiFiPassword";
static const char *MQTT_BROKER = "192.168.1.100";
static const char *MQTT_TOPIC = "sensor/data";
static WiFiClient wifi;
static PubSubClient mqtt(wifi);
static KymoContext kymo;
static uint32_t last_connect;

/*******************************************************************************
 * clock_ms
 ******************************************************************************/
static uint32_t clock_ms(void *) { return millis(); }

/*******************************************************************************
 * sample
 ******************************************************************************/
static uint8_t sample(void *, uint8_t id, KymoSample *out) {
    float phase = (millis() % 2000u) / 2000.0f;
    out->value = id == 0 ? sinf(phase * 6.283185307f) : 2.0f * phase - 1.0f;
    return 1;
}

/*******************************************************************************
 * service
 ******************************************************************************/
static void service(void *) {
    uint32_t now = millis();
    // PubSubClient connect() is synchronous: not a hard real-time transport.
    if (WiFi.status() == WL_CONNECTED && !mqtt.connected() &&
        (uint32_t)(now - last_connect) >= 5000u) {
        last_connect = now;
        mqtt.connect("KymoECU"); // choose a unique ID for each device
    }
    mqtt.loop();
}

/*******************************************************************************
 * busy
 ******************************************************************************/
static uint8_t busy(void *) { return !mqtt.connected(); }

/*******************************************************************************
 * write_frame
 ******************************************************************************/
static uint8_t write_frame(void *, const uint8_t *bytes, uint8_t length) {
    // Message transport: accept the entire binary payload or retry later.
    return mqtt.publish(MQTT_TOPIC, bytes, length) ? length : 0;
}
static const KymoChannel channels[] = {
    {100, 0, KYMO_FLAG_TIMESTAMP}, {100, 1, KYMO_FLAG_TIMESTAMP}
};
static uint32_t last_sample_ms[2];
static const KymoConfig config = {
    channels, last_sample_ms, clock_ms, nullptr, sample, nullptr,
    write_frame, busy, service, nullptr, 2
};

/*******************************************************************************
 * setup
 ******************************************************************************/
void setup() {
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    mqtt.setServer(MQTT_BROKER, 1883);
    mqtt.setSocketTimeout(1);
    last_connect = millis() - 5000u;
    (void)Kymo_Init(&kymo, &config);
}

/*******************************************************************************
 * loop
 ******************************************************************************/
void loop() { (void)Kymo_Main(&kymo); }
