make clean
sleep 2
make
sleep 2
service slide stop
sleep 2
rm -f /root/fbi
cp /root/fbida-2.09/fbi /root/fbi
sleep 2
service slide start