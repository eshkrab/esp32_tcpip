# esp32_tcpip
TCP/IP stack component for esp32 esp-idf, made to work with WiFi or Ethernet, DHCP or static IP

This is a component for esp-idf projects that nicely wraps up TCP/IP functionality in ESP32. 
Use make menuconfig "TCP/IP Configuration" to set whether WiFi or Ethernet is used, and whether to request an IP address from DHCP or use a static one.

Tested/written for LAN8720 PHY chip and built-in WiFi.
