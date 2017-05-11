#include <ESP8266WiFi.h>
#include <PubSubClient.h>

#define CHIME_PIN  D1
#define BUTTON_PIN D2

#define WIFI_SSID "*********"
#define WIFI_PASS "*********"

#define MQTT_SERVER "mqtt.example.com"
#define MQTT_PORT   1883
#define MQTT_USER   "*********"
#define MQTT_PASS   "*********"
#define PAYLOAD_OFF "OFF"
#define PAYLOAD_ON  "ON"
#define CMND_TOPIC  "doorbell/chime/cmnd"
#define STATE_TOPIC "doorbell/chime/state"
#define RING_TOPIC  "doorbell/ring"
#define RETAIN      true

// Enable the chime by default. If the enable/disable command is sent with the
// "retain" flag, this will be updated with the current setting as soon as we
// connect to MQTT.
bool doorbell_enabled = true;

WiFiClient espClient;
PubSubClient client(espClient);

void setup_wifi() {
  Serial.println("Connecting to WiFi");
  WiFi.hostname("doorbell");
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
  }
  Serial.println("Connected to WiFi. IP:");
  Serial.println(WiFi.localIP());
}

void callback(char* topic, byte* payload, unsigned int length) {
  // Turn the byte* payload into a char* we can work with
  char payloadBuf[length+1];
  memcpy(payloadBuf, payload, sizeof(payloadBuf));
  payloadBuf[sizeof(payloadBuf)-1] = 0;
  if (strcmp(payloadBuf, PAYLOAD_ON) == 0) {
    doorbell_enabled = true;
    Serial.println("Chime enabled");
    client.publish(STATE_TOPIC, PAYLOAD_ON, RETAIN);
  } else if (strcmp(payloadBuf, PAYLOAD_OFF) == 0) {
    doorbell_enabled = false;
    Serial.println("Chime disabled");
    client.publish(STATE_TOPIC, PAYLOAD_OFF, RETAIN);
  }
}

void setup() {
  Serial.begin(115200);
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(CHIME_PIN, OUTPUT);
  setup_wifi();
  client.setServer(MQTT_SERVER, MQTT_PORT);
  client.setCallback(callback);
}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    // Attempt to connect
    if (client.connect("doorbell", MQTT_USER, MQTT_PASS)) {
      client.subscribe(CMND_TOPIC);
    } else {
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void loop() {
  if (!client.connected()) {
    reconnect();
    Serial.println("Reconnecting MQTT");
  }
  if (digitalRead(BUTTON_PIN) == LOW) {
    Serial.println("Doorbell pressed.");
    client.publish(RING_TOPIC, "ring");
    if (doorbell_enabled) {
      // Play the chime
      digitalWrite(CHIME_PIN, HIGH);
      delay(500);
      digitalWrite(CHIME_PIN, LOW);
      Serial.println("Chime played.");
    } else {
      // Skip the chime
      Serial.println("Chime silenced.");
    }
    delay(1000); // Wait a second for the button to be released
  }
  client.loop();
}
