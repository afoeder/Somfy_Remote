/*
 * Somfy-Remote
 *
 * Authors/Credits:
 *     originally from https://github.com/Nickduino/Somfy_Remote, CC-BY-SA 4.0
 *     Adapted to run on ESP32 by https://github.com/marmotton/Somfy_Remote
 *     and further adjusted to custom needs by https://github.com/afoeder/Somfy_Remote (CC-BY-SA)
 *
 * This program allows you to emulate a Somfy RTS or Simu HZ remote.
 * If you want to learn more about the Somfy RTS protocol, check out https://pushstack.wordpress.com/somfy-rts-protocol/
 *
 * The rolling code will be stored in non-volatile storage (Preferences), so that you can power the Arduino off.
 *
 * Serial communication of the original code is replaced by MQTT over WiFi.
 *
 * Modifications should only be needed in config.h.
 *
**/

#include <Arduino.h>
#include <vector>
#include <TaskManagerIO.h>

// Wifi and MQTT
#ifdef ESP32
    #include <WiFi.h>
#elif ESP8266
    #include <ESP8266WiFi.h>
#endif

#include <PubSubClient.h>

// GPIO macros
#ifdef ESP32
    #define SIG_HIGH GPIO.out_w1ts = 1 << PORT_TX
    #define SIG_LOW  GPIO.out_w1tc = 1 << PORT_TX
#elif ESP8266
    #define SIG_HIGH GPIO_REG_WRITE(GPIO_OUT_W1TS_ADDRESS, 1 << PORT_TX)
    #define SIG_LOW  GPIO_REG_WRITE(GPIO_OUT_W1TC_ADDRESS, 1 << PORT_TX)
#endif

// Store the rolling codes in NVS
#ifdef ESP32
    #include <Preferences.h>
    Preferences preferences;
#elif ESP8266
    #include <EEPROM.h>
#endif

#define HAUT 0x2
#define STOP 0x1
#define BAS 0x4
#define PROG 0x8

byte frame[7];

class REMOTE;

void BuildFrame(byte *frame, byte button, REMOTE &remote);
void SendCommand(byte *frame, byte sync);

class REMOTE {

private:
    unsigned int id;
    char const* mqtt_topic;
    unsigned int default_rolling_code;
    uint32_t eeprom_address;
    uint32_t cover_travel_time;
    uint32_t stateTaskId;

    void sendCommand(PubSubClient &aPubSubClient, const char* anAckTopic, const char* anAckMessage) {

        // Send the MQTT ack message
        String ackString = "id: 0x";
        ackString.concat( String(this->getId(), HEX) );
        ackString.concat(", cmd: ");
        ackString.concat(anAckMessage);
        aPubSubClient.publish(anAckMessage, ackString.c_str());

        // send the actual radio command
        SendCommand(frame, 2);
        for ( int i = 0; i<2; i++ ) {
            SendCommand(frame, 7);
        }
    }


public:
    REMOTE(
        unsigned int id,
        const char *mqttTopic,
        unsigned int defaultRollingCode,
        uint32_t eepromAddress,
        uint32_t coverTravelTime
    ) :
        id(id),
        mqtt_topic(mqttTopic),
        default_rolling_code(defaultRollingCode),
        eeprom_address(eepromAddress),
        cover_travel_time(coverTravelTime)
    { /* intentionally no body, just ctor member init */ }

    void moveUp(PubSubClient &aPubSubClient, const char *anAckTopic, const char *aTopicPostfix) {
        BuildFrame(frame, HAUT, *this);
        this->sendCommand(aPubSubClient, anAckTopic, "u");
        this->publishState(aPubSubClient, aTopicPostfix, "opening");
    };

    void moveDown(PubSubClient &aPubSubClient, const char *anAckTopic, const char *aTopicPostfix) {
        BuildFrame(frame, BAS, *this);
        this->sendCommand(aPubSubClient, anAckTopic, "c");
        this->publishState(aPubSubClient, aTopicPostfix, "closing");

        taskManager.cancelTask(stateTaskId);

        stateTaskId =
            taskManager
                .scheduleOnce(
                    this->cover_travel_time,
                    [this, aPubSubClient, aTopicPostfix] {
                        this->publishState(aPubSubClient, aTopicPostfix, "closed");
                    });
    };

    void stop(PubSubClient &aPubSubClient, const char *anAckTopic, const char *aTopicPostfix) {
        BuildFrame(frame, STOP, *this);
        this->sendCommand(aPubSubClient, anAckTopic, "s");
        this->publishState(aPubSubClient, aTopicPostfix, "stopped");
        taskManager.cancelTask(stateTaskId);
    };

    void program(PubSubClient &aPubSubClient, const char *anAckTopic, const char *aTopicPostfix) {
        BuildFrame(frame, PROG, *this);
        this->sendCommand(aPubSubClient, anAckTopic, "p");
        this->publishState(aPubSubClient, aTopicPostfix, "programming");
    };

    void dummy() {};

    void publishState(
            PubSubClient &aPublisher,
            const char *aTopicPostfix,
            const char *aShutterState) {

        String stateTopicName =
            (String(mqtt_topic) + aTopicPostfix);

        aPublisher.publish(
            stateTopicName.c_str(),
            aShutterState);
    }

    unsigned int getId() const { return id; }
    const char *getMqttTopic() const { return mqtt_topic; }
    unsigned int getDefaultRollingCode() const { return default_rolling_code; }
    uint32_t getEepromAddress() const { return eeprom_address; }
    uint32_t getCoverTravelTime() const { return cover_travel_time; }
};

#include "config.h"

WiFiClient wifiClient;
PubSubClient mqtt(wifiClient);

// Buttons
#define SYMBOL 640

void SendCommand(byte *frame, byte sync);
void receivedCallback(char* topic, byte* payload, unsigned int length);
void mqttconnect();

void setup() {
    // USB serial port
    Serial.begin(74880);

    // Output to 433.42MHz transmitter
    pinMode(PORT_TX, OUTPUT);
    SIG_LOW;

    // Open storage for storing the rolling codes
    #ifdef ESP32
        preferences.begin("somfy-remote", false);
    #elif ESP8266
        EEPROM.begin(1024);
    #endif

    // Clear all the stored rolling codes (not used during normal operation). Only ESP32 here (ESP8266 further below).
    #ifdef ESP32
        if ( reset_rolling_codes ) {
                preferences.clear();
        }
    #endif

    // Print out all the configured remotes.
    // Also reset the rolling codes for ESP8266 if needed.
    for ( REMOTE remote : remotes ) {
        Serial.print("Simulated remote number : ");
        Serial.println(remote.getId(), HEX);
        Serial.print("Current rolling code    : ");
        unsigned int current_code;

        #ifdef ESP32
            current_code = preferences.getUInt( (String(remote.getId()) + "rolling").c_str(), remote.getDefaultRollingCode());
        #elif ESP8266
            if ( reset_rolling_codes ) {
                EEPROM.put( remote.getEepromAddress(), remote.getDefaultRollingCode() );
                EEPROM.commit();
            }
            EEPROM.get( remote.getEepromAddress(), current_code );
        #endif

        Serial.println( current_code );
    }
    Serial.println();

    // Connect to WiFi
    Serial.print("Connecting to ");
    Serial.println(wifi_ssid);

    WiFi.begin(wifi_ssid, wifi_password);

    #ifdef ESP32
        WiFi.setHostname("ESP32-somfy");
    #elif ESP8266
        WiFi.hostname("ESP8266-somfy");
    #endif

    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }

    Serial.println("");
    Serial.println("WiFi connected");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());

    // Configure MQTT
    mqtt.setServer(mqtt_server, mqtt_port);
    mqtt.setCallback(receivedCallback);

}

void loop() {
    // Reconnect MQTT if needed
    if ( !mqtt.connected() ) {
        mqttconnect();
    }

    mqtt.loop();

    taskManager.runLoop();

    delay(100);
}

void mqttconnect() {
    // Loop until reconnected
    while ( !mqtt.connected() ) {
        Serial.print("MQTT connecting ...");

        // Connect to MQTT, with retained last will message "offline"
        if (mqtt.connect(mqtt_id, mqtt_user, mqtt_password, status_topic, 1, 1, "offline")) {
            Serial.println("connected");

            // Subscribe to the topic of each remote with QoS 1
            for ( REMOTE remote : remotes ) {
                mqtt.subscribe(remote.getMqttTopic(), 1);
                Serial.print("Subscribed to topic: ");
                Serial.println(remote.getMqttTopic());
            }

            // Update status, message is retained
            mqtt.publish(status_topic, "online", true);
        }
        else {
            Serial.print("failed, status code =");
            Serial.print(mqtt.state());
            Serial.println("try again in 5 seconds");
            // Wait 5 seconds before retrying
            delay(5000);
        }
    }
}

void receivedCallback(char* topic, byte* payload, unsigned int length) {
    char command = *payload; // 1st byte of payload

    Serial.print("MQTT message received: ");
    Serial.println(topic);

    Serial.print("Payload: ");
    for (unsigned int i = 0; i < length; i++) {
        Serial.print((char)payload[i]);
    }
    Serial.println();

    // Command is valid if the payload contains one of the chars below
    if (length != 1 || !(command == 'u' || command == 's' || command == 'd' || command == 'p')) {
        return;
    }

    auto
        responsibleRemoteIterator =
            std::find_if(
                begin(remotes),
                end(remotes),
                [topic] (REMOTE const& remote) {
                    return
                        strcmp(remote.getMqttTopic(), topic) == 0;
                });

    if (responsibleRemoteIterator == end(remotes)) {
        return;
    }

    if ( command == 'u' ) {
        Serial.println("Monte"); // Somfy is a French company, after all.
        responsibleRemoteIterator->moveUp(mqtt, ack_topic, state_topic_postfix);
    }
    else if ( command == 's' ) {
        Serial.println("Stop");
        responsibleRemoteIterator->stop(mqtt, ack_topic, state_topic_postfix);
    }
    else if ( command == 'd' ) {
        Serial.println("Descend");
        responsibleRemoteIterator->moveDown(mqtt, ack_topic, state_topic_postfix);
    }
    else if ( command == 'p' ) {
        Serial.println("Prog");
        responsibleRemoteIterator->program(mqtt, ack_topic, state_topic_postfix);
    }
}

void BuildFrame(byte *frame, byte button, REMOTE &remote) {
    unsigned int code;

    #ifdef ESP32
        code = preferences.getUInt( (String(remote.getId()) + "rolling").c_str(), remote->getDefaultRollingCode());
    #elif ESP8266
        EEPROM.get( remote.getEepromAddress(), code );
    #endif

    frame[0] = 0xA7;            // Encryption key. Doesn't matter much
    frame[1] = button << 4;     // Which button did  you press? The 4 LSB will be the checksum
    frame[2] = code >> 8;       // Rolling code (big endian)
    frame[3] = code;            // Rolling code
    frame[4] = remote.getId() >> 16; // Remote address
    frame[5] = remote.getId() >>  8; // Remote address
    frame[6] = remote.getId();       // Remote address

    Serial.print("Frame         : ");
    for(byte i = 0; i < 7; i++) {
        if(frame[i] >> 4 == 0) { //  Displays leading zero in case the most significant nibble is a 0.
            Serial.print("0");
        }
        Serial.print(frame[i],HEX); Serial.print(" ");
    }

    // Checksum calculation: a XOR of all the nibbles
    byte checksum = 0;
    for(byte i = 0; i < 7; i++) {
        checksum = checksum ^ frame[i] ^ (frame[i] >> 4);
    }
    checksum &= 0b1111; // We keep the last 4 bits only


    // Checksum integration
    frame[1] |= checksum; //  If a XOR of all the nibbles is equal to 0, the blinds will consider the checksum ok.

    Serial.println(""); Serial.print("With checksum : ");
    for(byte i = 0; i < 7; i++) {
        if(frame[i] >> 4 == 0) {
            Serial.print("0");
        }
        Serial.print(frame[i],HEX); Serial.print(" ");
    }


    // Obfuscation: a XOR of all the bytes
    for(byte i = 1; i < 7; i++) {
        frame[i] ^= frame[i-1];
    }

    Serial.println(""); Serial.print("Obfuscated    : ");
    for(byte i = 0; i < 7; i++) {
        if(frame[i] >> 4 == 0) {
            Serial.print("0");
        }
        Serial.print(frame[i],HEX); Serial.print(" ");
    }
    Serial.println("");
    Serial.print("Rolling Code  : ");
    Serial.println(code);

    #ifdef ESP32
        preferences.putUInt( (String(remote.getId()) + "rolling").c_str(), code + 1); // Increment and store the rolling code
    #elif ESP8266
        EEPROM.put( remote.getEepromAddress(), code + 1 );
        EEPROM.commit();
    #endif
}

void SendCommand(byte *frame, byte sync) {
    if(sync == 2) { // Only with the first frame.
        //Wake-up pulse & Silence
        SIG_HIGH;
        delayMicroseconds(9415);
        SIG_LOW;
        delayMicroseconds(89565);
    }

    // Hardware sync: two sync for the first frame, seven for the following ones.
    for (int i = 0; i < sync; i++) {
        SIG_HIGH;
        delayMicroseconds(4*SYMBOL);
        SIG_LOW;
        delayMicroseconds(4*SYMBOL);
    }

    // Software sync
    SIG_HIGH;
    delayMicroseconds(4550);
    SIG_LOW;
    delayMicroseconds(SYMBOL);

    //Data: bits are sent one by one, starting with the MSB.
    for(byte i = 0; i < 56; i++) {
        if(((frame[i/8] >> (7 - (i%8))) & 1) == 1) {
            SIG_LOW;
            delayMicroseconds(SYMBOL);
            SIG_HIGH;
            delayMicroseconds(SYMBOL);
        }
        else {
            SIG_HIGH;
            delayMicroseconds(SYMBOL);
            SIG_LOW;
            delayMicroseconds(SYMBOL);
        }
    }

    SIG_LOW;
    delayMicroseconds(30415); // Inter-frame silence
}
