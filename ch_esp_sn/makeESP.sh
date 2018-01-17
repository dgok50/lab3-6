make -f ~/makeEspArduino/makeEspArduino.mk clean
rm -R -f /tmp/mkESP/ch_esp_sn_d1_mini/
make -f ~/makeEspArduino/makeEspArduino.mk all
#make -f ~/makeEspArduino/makeEspArduino.mk http
#mkdir ~/rom/esp8266/ch_esp_sn
rm -f ~/rom/esp8266/ch_esp_sn/flash.bin
cp /tmp/mkESP/ch_esp_sn_d1_mini/ch_esp_sn.bin ~/rom/esp8266/ch_esp_sn/flash.bin
cp /tmp/mkESP/ch_esp_sn_d1_mini/ch_esp_sn.bin ./flash/flash.bin
sleep 2
#curl -sS "http://192.168.0.61/online_update"
sleep 2