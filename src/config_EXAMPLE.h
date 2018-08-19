// You can add as many remote control emulators as you want by adding elements to the "remotes" vector
// The id and mqtt_topic can have any value but must be unique
// default_rolling_code can be any unsigned int, usually leave it at 1

// Once the programe is uploaded on the ESP32:
// - Long-press the program button of YOUR ACTUAL REMOTE until your blind goes up and down slightly
// - send 'p' using MQTT on the corresponding topic
// - You can use the same remote control emulator for multiple blinds, just repeat these steps.
//
// Then:
// - u will make it go up
// - s make it stop
// - d will make it go down

//                                 id            mqtt_topic     default_rolling_code
std::vector<REMOTE> const remotes = {{0x113400, "smartHome/livingRoom/blinds", 1}
                                    ,{0x113500, "smartHome/office/blinds",     1}
                                    ,{0x113600, "smartHome/balcony/awning",    1}
                                    ,{0x140000, "smartHome/kitchen/blinds",    1}
                                    ,{0x113300, "smartHome/room1/blinds",      1}
                                    };

// Uncomment the following line to clear the rolling codes stored in the non-volatile storage
// The default_rolling_code will be used
//#define RESET_ROLLING_CODES

const char*        wifi_ssid = "myWIFISSID";
const char*    wifi_password = "superSecretPassword1234";

const char*      mqtt_server = "serverIpOrName";
const unsigned int mqtt_port = 1883;
const char*        mqtt_user = "username";
const char*    mqtt_password = "secretPassword5678";
const char*          mqtt_id = "esp32-somfy-remote";

#define PORT_TX 23 // Output data on pin 23 (can range from 0 to 31)