make -f ~/makeEspArduino/makeEspArduino.mk clean
rm -R -f /tmp/mkESP/disp_esp_sn_d1_mini/
make -f ~/makeEspArduino/makeEspArduino.mk
#mkdir ~/rom/esp8266/disp_esp_sn
rm -f *.bin
rm -f ~/rom/esp8266/disp_esp_sn/flash.bin
cp /tmp/mkESP/disp_esp_sn_d1_mini/disp_esp_sn.bin ~/rom/esp8266/disp/flash.bin
cp /tmp/mkESP/disp_esp_sn_d1_mini/disp_esp_sn.bin ./flash.bin
sleep 2
curl -sS "http://192.168.0.106/online_update"
sleep 2