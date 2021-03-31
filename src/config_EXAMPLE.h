// You can add as many remote control emulators as you want by adding elements to the "remotes" vector
// The id and mqtt_topic can have any value but must be unique
// default_rolling_code can be any unsigned int, usually leave it at 1
// eeprom_address must be incremented by 4 for each remote
// cover_travel_time
//      is the amount in milliseconds the respective blind/shutter/cover needs to travel from
//      its top to bottom dead centers (or vice versa).

// Once the programe is uploaded on the ESP32:
// - Long-press the program button of YOUR ACTUAL REMOTE until your blind goes up and down slightly
// - send 'p' using MQTT on the corresponding topic
// - You can use the same remote control emulator for multiple blinds, just repeat these steps.
//
// Then:
// - u will make it go up
// - s make it stop
// - d will make it go down


//                                 id            mqtt_topic     default_rolling_code     eeprom_address   cover_travel_time
std::vector<REMOTE> const remotes = {{0x184623, "smartHome/livingRoom/blinds", 1,                0,       15000}
                                    ,{0x971547, "smartHome/office/blinds",     1,                4,       15000}
                                    ,{0x336124, "smartHome/balcony/awning",    1,                8,       15000}
                                    ,{0x187542, "smartHome/kitchen/blinds",    1,               12,       15000}
                                    ,{0x244412, "smartHome/room1/blinds",      1,               16,       15000}
                                    };

// Change reset_rolling_codes to true to clear the rolling codes stored in the non-volatile storage
// The default_rolling_code will be used

const bool reset_rolling_codes = false;

const char*        wifi_ssid = "myWIFISSID";
const char*    wifi_password = "superSecretPassword1234";

const char*      mqtt_server = "serverIpOrName";
const unsigned int mqtt_port = 1883;
const char*        mqtt_user = "username";
const char*    mqtt_password = "secretPassword5678";
const char*          mqtt_id = "esp32-somfy-remote";

const char*    state_topic_postfix = "/state"; // will be appended to the remote's mqtt_topic and report `opening`, `closing`, `stopped`, inspired by https://www.home-assistant.io/integrations/cover.mqtt/#configuration

const char*     status_topic = "smartHome/somfy-remote/status"; // Online / offline
const char*        ack_topic = "smartHome/somfy-remote/ack"; // Commands ack "id: 0x184623, cmd: u"

#define PORT_TX 23 // Is GPIO5 on D1 mini. constant is fetched from https://github.com/esp8266/Arduino/blob/5ac64ffa27461161401971796150ef7f2b26a77d/variants/d1_mini/pins_arduino.h#L38
