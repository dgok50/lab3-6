cd int_atmega328
make clean
make
echo "Trying to set avrisp"
curl -sS "http://192.168.0.61/set?avrisp_s=20"
sleep 2
echo "Trying to programm..."
avrdude -c arduino -p atmega328p -P net:192.168.0.61:328 -U ./build-uno/int_atmega328.hex
cp ./build-uno/int_atmega328.hex ~/rom/esp8266/ch_esp_sn/atmega.hex
cd ..
curl -sS "http://192.168.0.61/set?avr_reset=20"