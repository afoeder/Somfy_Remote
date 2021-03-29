# Somfy Remote
An ESP32/ESP8266 program able to emulate a Somfy remote control.

Forked from

* https://github.com/marmotton/Somfy_Remote
  * https://github.com/Nickduino/Somfy_Remote

(see also Credits section below)

If you want to learn more about the Somfy RTS protocol, check out [Pushtack](https://pushstack.wordpress.com/somfy-rts-protocol/).


## How the hardware works
Connect a *433.42 MHz* RF transmitter to Pin 23 of the ESP32 (or change the pin in the config file). I ordered 433.42 MHz crystals to replace the one on a 433.92MHz transmitter.


## How the software works
Copy [src/config_EXAMPLE.h](src/config_EXAMPLE.h) to `config.h` to adapt to your location. The relevant code lines are remarked. You can add or remove remotes to your liking.

The ESP will subscribe to the configured MQTT topics. Watch what is happening on the serial port to make sure it is working.

Programming the blinds:
  1) Press the program button on your actual remote. The blinds will move slightly.
  2) Publish 'p' message on the corresponding MQTT topic. The blinds will move slightly.
  3) Done !

Simply publish these messages on the corresponding topics to control your blinds:
  - u (up)
  - d (down)
  - s (stop, my)

The rolling code value is stored in the EEPROM, so that you don't loose count of your rolling code after a reset. In case you'd like to replace the ESP, write down the current rolling codes which can be read using the serial terminal (and use them as default rolling codes in config.h).


## Use with Home Assistant
I added my blinds in the `configuration.yaml` in the following manner:

```yaml
cover:
  - name: "Living room blinds"
    platform: mqtt
    
    availability:
      - topic: "smartHome/somfy-remote/status"
        payload_available: "online"
        payload_not_available: "offline"

    command_topic: "smartHome/livingRoom/blinds"
    payload_open: "u"
    payload_close: "d"
    payload_stop: "s"
    
    state_topic: "smartHome/livingRoom/blinds/state"
    state_opening: "opening"
    state_closing: "closing"
    state_stopped: "stopped"

    optimistic: true

  - name: "Office blinds"
    platform: mqtt
    
    availability:
      - topic: "smartHome/somfy-remote/status"
        payload_available: "online"
        payload_not_available: "offline"

    command_topic: "smartHome/office/blinds"
    payload_open: "u"
    payload_close: "d"
    payload_stop: "s"
    
    state_topic: "smartHome/office/blinds"
    state_opening: "opening"
    state_closing: "closing"
    state_stopped: "stopped"

    optimistic: true
```

## Hardware I used

I basically followed [@marmotton](https://github.com/marmotton/)'s [original list](https://github.com/marmotton/Somfy_Remote/blob/789506d84c28794392c8eb38ed67748cf528c3aa/README.md#my-hardware), but this one here's adjusted for the German 🇩🇪 region:

ESP8266 board
:   [D1 Mini at BerryBase](https://www.berrybase.de/dev.-boards/esp8266-esp32-d1-mini/boards/d1-mini-esp8266-entwicklungsboard)

433 MHz RF transmitter
:   [FS1000A at BerryBase](https://www.berrybase.de/sensoren-module/funk-kommunikation/433mhz-sender-empf-228-nger-superregeneration-modul-fs1000a-xy-fst-xy-mk-5v)

433.42 MHz SAW oscillator
:   [at eBay from CN](https://www.ebay.de/itm/232574365405)

PCB:
:   _tbd_

## Credit

Credit is due to

* https://github.com/marmotton/Somfy_Remote
* https://github.com/Nickduino/Somfy_Remote

whose this project is forked from; besides I'd like to thank [@RoyOltmans](https://github.com/RoyOltmans) with his [somfy_esp8266_remote_arduino](https://github.com/RoyOltmans/somfy_esp8266_remote_arduino) repository: this one gave me some more advice and inspiration for using a nice [Fritzing](https://fritzing.org/) PCB. You can find mine at [assets/Somfy Remote Fritzing Sketch.fzz](assets/Somfy Remote Fritzing Sketch.fzz). Reviews and ideas are highly welcome.