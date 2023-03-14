# JK-BMS Monitor using RS485

<img src="JK-BMS-and-Wemos.png"/>

---
Arduino code used to read voltages of cells, cell count, remain capacity, charging current and pack voltage.

There are two files;
1. Read BMS data and WifiManager, (Not required to hardcode Wifi credentials, WeMos module will be a AP at startup. Use mobile phone to connect and configure SSID and Password once connected to AP)
2. Read BMS data (include WifiManager code), BMS data will be publish to public MQTT broker.

# RS485 Module
<img src="rs485_module.jpeg"/>
`https://tronic.lk/product/max485-ttl-to-rs485-max485csa-converter-module-for-ardu`

# WeMos Module
<img src="WeMosD1Mini_pinout.png"/>
`https://tronic.lk/product/nodemcu-d1-mini-lua-wifi-wemos-4m-esp8266-module`

# JK-BMS RS485 adapter
<img src="JK RS485 adapter.jpg"/>
`https://tronic.lk/product/nodemcu-d1-mini-lua-wifi-wemos-4m-esp8266-module](https://www.aliexpress.com/item/1005002434675896.html?spm=a2g0o.productlist.main.9.42304b23PcXFgA&algo_pvid=6648b06e-1061-4b51-a98f-d319668948fe&algo_exp_id=6648b06e-1061-4b51-a98f-d319668948fe-4&pdp_ext_f=%7B%22sku_id%22%3A%2212000026766048709%22%7D&pdp_npi=3%40dis%21LKR%214878.89%212926.03%21%21%21%21%21%4021021b4716788047128274040d0701%2112000026766048709%21sea%21LK%210&curPageLogUid=YHo5EwrWTtl8)`

