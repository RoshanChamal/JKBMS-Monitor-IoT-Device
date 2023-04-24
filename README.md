# JK-BMS Monitor using RS485

Arduino code used to read voltages of cells, cell count, remain capacity, charging current and pack voltage. This code used to monitor 8 cells of LiFEPo4, 24 Battery pack using JK-B1A8S20P

There are two files;
1. Read BMS data and WifiManager, Not required to hardcode Wifi credentials, WeMos module will be a AP at startup. Use mobile phone to connect and configure SSID and Password once connected to AP - this will apear as "Solar.JKBMS".
2. Read BMS data (include WifiManager code), BMS data will be publish to public MQTT broker.
In addition, it will monitor battery capacity and turn on/off a 30A 5VDC 230VAC Relay.

There are two Schematics:
1. RS-485 module work by supply 5V 
2. RS-485 module work by supply 3.3V

(Note: JK-BMS RS485 adapter GND and RS-485 module GND need to connect)
# Schematics 5V
<img src="JK-BMS-and-Wemos-with-logic_level_shifter.jpg"/>

# Schematics 3.3V
<img src="BMS-and-Wemos-with-3v3.JPG"/>

# RS485 Module
<img src="rs485_module.jpeg"/>
`https://tronic.lk/product/max485-ttl-to-rs485-max485csa-converter-module-for-ardu`

# WeMos Module
<img src="WeMosD1Mini_pinout.png"/>
`https://tronic.lk/product/nodemcu-d1-mini-lua-wifi-wemos-4m-esp8266-module`

# JK-BMS RS485 adapter
<img src="JK RS485 adapter.jpg"/>

# 3.3V 5V Bi-Directional Logic Level Shifter Module
<img src="logic_level_shifter.jpg"/>
`https://tronic.lk/product/4-channel-3-3v-5v-bi-directional-logic-level-shifter-mo`

## References

* https://www.mischianti.org/2020/05/11/interface-arduino-esp8266-esp32-rs-485/
* https://github.com/dj-nitehawk/jk-bms-test
* https://github.com/dj-nitehawk/Hybrid-Inverter-Monitor
