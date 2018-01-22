#include "ESP8266HTTPUpdateServer.h"
#include <ESP8266HTTPClient.h>
#include <ESP8266httpUpdate.h>
#include "FS.h"
#define ESP_CH
#include "a1fl.c" //Библиотека с прекладными функциями
#include <ArduinoJson.h>
#include <ESP8266WebServer.h>
#include <ESP8266WiFi.h>
#include <DHT.h>
#include <SPI.h>
#include <WiFiClient.h>
#include <WiFiUdp.h>

#include "ShiftRegLCD123.h"
#include <Ticker.h>

#pragma GCC push_options
#pragma GCC optimize ("O0")

#define SECS_PER_MIN  (60UL)
#define SECS_PER_HOUR (3600UL)
#define SECS_PER_DAY  (SECS_PER_HOUR * 24L)
#define BUF_SIZE 2512
#define RBUF_SIZE 512
#define DHT22_PIN 5
#define NAN -200
#define RCOL 30
#define TID 0
#define HID 1
#define S_MAX 100

#define USE_SERIAL srlcd

#define OFFSET 10                                           //LCD char offset

const char *HOST_NAME = "DISP_ESP";
const char *endl = "\n";
const int fw_ver = 31;
#define dataPin 12
#define clockPin 14
#define enablePin 13

#define numberOfSeconds(_time_) (_time_ % SECS_PER_MIN)  
#define numberOfMinutes(_time_) ((_time_ / SECS_PER_MIN) % SECS_PER_MIN) 
#define numberOfHours(_time_) (( _time_% SECS_PER_DAY) / SECS_PER_HOUR)
#define elapsedDays(_time_) ( _time_ / SECS_PER_DAY)  

ADC_MODE(ADC_VCC);

DHT dht(DHT22_PIN, DHT22);

Ticker data_collect, data_send_tic;

const char *password = "012345780";

ESP8266WebServer server(80);
ESP8266HTTPUpdateServer httpUpdater;

float mqv=0, mq7=0, mq9=0, vin=0, mc_vcc=0, mc_temp=0, lux=0, esp_vcc=0, tmp=0, mqv5=0, mq9_5=0;
float dht_temp=0, dht_hum=0, bmp_temp=0, bmp_pre=0, rdy=0, idht_temp=0, idht_hum=0, tidht_hum=0;
float sdht_temp[S_MAX], sdht_hum[S_MAX], tidht_temp=0;
unsigned int loop_i = 0, i=0, loop_u = 0, s_i=0;


unsigned long tfs = 0, timecor;

volatile bool bmp_ok=false, lux_ok=false, dht_ok=false, data_rec=false, idht_ok=false;
volatile bool ispmode = false, drq = false, send_data = false, repsend = false, s_redy=false;
volatile bool loop_en=true, selfup=false, lcdbackl=true, data_get=true, narodmon_send=false;

char cstr1[BUF_SIZE], replyb[RBUF_SIZE], nreplyb[RBUF_SIZE], ctmp='\0';
String wpass="84992434219", wname="A1 Net";
char mac[22];
double rdtmp[3][RCOL+2];

void getd();
bool loadConfig();
bool saveConfig();
bool lcdbacklset();
bool lcdbacklset(int);
unsigned int localPort = 2390;                              // local port to listen for UDP packets
IPAddress timeServerIP;
const char* ntpServerName = "ru.pool.ntp.org";

const int NTP_PACKET_SIZE = 48;                             // NTP time stamp is in the first 48 bytes of the message

byte packetBuffer[ NTP_PACKET_SIZE];                        //buffer to hold incoming and outgoing packets

// A UDP instance to let us send and receive packets over UDP
WiFiUDP udp;

WiFiClient client;

void httpRequest();
unsigned long sendNTPpacket(IPAddress&);
//int get_state(char *, unsigned int);
bool parse_A1DSP(char*);
bool parse_NAROD(char*);
bool A1_data_pr(char *, unsigned int);
bool NAROD_data_send(char *, short int);
void data_send_f();






long get_signal_qua(long rfrom, long rto){
    long rssi = WiFi.RSSI();
    if(rssi >= -40) 
    rssi = -40;
    else if(rssi <= -85) 
    rssi = -85;
    return map(rssi, -40, -85, rfrom, rto);
  }

const char *webPage ="<!DOCTYPE html>"
"<html>\n"
" <body>\n"
"  <h1>ESP8266 DISP</h1>\n"
"  <a href= \"/update\">Update dialog</a><br>\n"
"  <a href= \"/online_update\">Online Update</a><br>\n"
"  <a href= \"/config.json\">Json Config info</a><br>\n"
"  <a href= \"/a1pr\">A1_DSP</a><br>\n"
"  <a href= \"/replyb.txt\">Reply buffer</a><br>\n"
"  <a href= \"/sysinfo.txt\">System info</a><br>\n"
"  <a href= \"/set?restart=1\">Restart device</a><br>\n"
"  <a href= \"/set?backlight=0\">Turn OFF backlight</a><br>\n"
"  <a href= \"/set?backlight=1\">Turn ON backlight</a><br>\n"
"  <a href= \"/set?format=243\">Reset default st</a><br>\n"
" </body>\n"
"</html>\n";

ShiftRegLCD123 srlcd(dataPin, clockPin, enablePin, SRLCD123);

void setup() {

    Serial.begin(9600);   
    Serial.println("A1 DISP_ESP_ST");
    pinMode(5, INPUT);
    yield();
    bzero(cstr1, BUF_SIZE);
    bzero(replyb, RBUF_SIZE);
    for(int r=0;r<RCOL;r++){
	  bzero(rdtmp[r],2);
    }
    srlcd.begin(20,2);
    srlcd.clear();
    srlcd.setCursor(0,0);
    srlcd.print(HOST_NAME);
    srlcd.setCursor(0,1);
    Serial.println("Starting esp.");
    srlcd.print("Версия ");    
	sprintf(cstr1, "%d.%d.%d", fw_ver/100, (fw_ver%100)/10, fw_ver%10);
    srlcd.print(cstr1);
    delay(1000);
    bzero(cstr1, BUF_SIZE);
    srlcd.setCursor(0,1);
    srlcd.print("От ");
    srlcd.print(__TIME__"  ");
    delay(2000);
    srlcd.setCursor(0,1);
    srlcd.print("Запуск.              ");
    srlcd.setCursor(OFFSET,0);
    srlcd.print("WiFi");
    Serial.println("WIFI");
    WiFi.begin();
    yield();
    srlcd.setCursor(7,1);
    srlcd.print(".");
    srlcd.backlightOn();
    srlcd.setCursor(OFFSET,0);
    srlcd.print("SPIFS");
    Serial.println("SPIFS");
    
    if(ESP.getResetS() == false || digitalRead(5) == LOW)
    {
        selfup=true;
        loop_en=false;
        srlcd.setCursor(0,1);
        srlcd.print("Ошибка посл выкл");
        delay(1000);
        srlcd.setCursor(0,1);
        srlcd.print("Принуд обн включено ");
        delay(1000);
    }
    if(selfup == false) {
    if (!SPIFFS.begin()) {                                      //|| !digitalRead(BUT)) {
        Serial.println("Failed to mount file system");
        SPIFFS.format();
        srlcd.setCursor(0,1);
        srlcd.print("Форматирование SPIFS");
      }

    srlcd.setCursor(OFFSET,0);
    srlcd.print("W СБРОС С1");
    yield();
    Serial.println("W СБРС");
    srlcd.setCursor(8,1);
    srlcd.print(".");
    yield();

    WiFi.disconnect();
    yield();
    delay(200);
    yield();

    srlcd.setCursor(OFFSET,0);
    srlcd.print("ЗАГР КОНФ ");
    Serial.println("CFG L");
    yield();
    srlcd.setCursor(9,1);
    srlcd.print(".");
	byte bmac[6];
	WiFi.macAddress(bmac);
	bzero(mac, 20);
	sprintf(mac, "%X-%X-%X-%X-%X-%X", bmac[0], bmac[1], bmac[2], bmac[3], bmac[4], bmac[5]);
    if (!loadConfig()) {
        srlcd.setCursor(0,1);
        srlcd.print("ОШИБКА, СБРОС");
        SPIFFS.remove("/config.json");
        if (!saveConfig()) {
    	    srlcd.setCursor(0,1);
    	    srlcd.print("НЕ ВОЗМОЖНО СОХР КОНФ");
            Serial.println("Failed to save config");
            SPIFFS.format();
			} 
		else {
    	      srlcd.setCursor(0,1);
    	      srlcd.print("УСП СОХР КОНФ");
              Serial.println("Config saved");
            }
        Serial.println("Failed to load config");
      } 

    srlcd.setCursor(OFFSET,0);
    Serial.println("W STA ");
    srlcd.print("W СБРОС С2");
    srlcd.setCursor(10,1);
    srlcd.print(".");
    }
    WiFi.mode(WIFI_OFF);
    delay(100);
    WiFi.mode(WIFI_STA);
    WiFi.disconnect(false);
    srlcd.setCursor(11,1);
    srlcd.print(".");
    WiFi.setAutoReconnect (true);
    delay(100);
    srlcd.setCursor(12,1);
    srlcd.print(".");
    srlcd.setCursor(OFFSET,0);
    srlcd.print("WAN КЛИЕНТ");
    srlcd.setCursor(0,1);
    srlcd.print("ПОПЫТКА ПОДКЛЮЧЕНИЯ ");
    WiFi.hostname(HOST_NAME);
    WiFi.begin("A1 Net", "84992434219");
    int t = 0;
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
        t++;
        if (t == 20) {
            srlcd.setCursor(OFFSET,0);
            srlcd.print("WAN ТД    ");
            srlcd.setCursor(0,1);
            srlcd.print("ПРЕРХОД В РЕЖИМ ТД  ");
            loop_en=false;
            WiFi.softAP(HOST_NAME, password);
            IPAddress myIP = WiFi.softAPIP();
          }
      }
    Serial.println("WiFi connected");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());

    srlcd.setCursor(OFFSET,0);
	dht.begin();
    srlcd.print("ЗАПУСК WEB");
    srlcd.setCursor(0,1);
    srlcd.print("Запуск сервера  ");
	
    server.on("/", []() {
        server.send(200, "text/html", webPage);
        Serial.printf("Stations connected = %d\n", WiFi.softAPgetStationNum());
      });

    server.on("/config.json", []() {
        File file = SPIFFS.open("/config.json", "r");
        size_t sent = server.streamFile(file, "application/json");
        file.close();
        delay(1000);
      });

    server.on("/sysinfo.txt", []() {
        bzero(cstr1, BUF_SIZE);
        
        tfs = millis()/1000;
		sprintf(cstr1, "Hostname: %s\nFW Ver: %d.%d.%d\n"
		"Comp info, time: " __TIME__ ", date: " __DATE__ "\n"
		"MAC: %s\n"
		 "SketchSize: %d\n"
         "SketchMD5: %s\n"
         "CPU Frq: %d MHz\n"
         "Free Heap: %d\n"

         "ESP VCC: %.2f\n"
         "Int DHT OK: %d\n"
         "Int DHT Hum: %.2f%c\n"
         "Int DHT Temp: %.2f%cC\n"
         "Int DHT Haetindex: %.2f%cC\n"
         "Int DHT S REDY: %d\n"
         "Int DHT Hum: %.2f%c\n"
         "Int DHT S Temp: %.2f%cC\n"
         "Int DHT S Haetindex: %.2f%cC\n"
         "Time from start: %lu hours %lu minutes %lu Sec, and %lu days \n"
         "ESP Chip ID: %X\n"
         "Flash id: %X\n"
         "Flash real size: %d\n"
         "Flash ide size: %d\n"
         "Flash ide speed: %d\n"
         "Flash ide mode: %s\n"
         "Last reset: %s(%d)\n"
         "Reset info: %s\n"
         "WiFi Status: %d\n"
         "SSID: %s\n"
         "RSSI: %d dbm\n"
         "data get: %d\n"
		 "Repet send: %d\n"
		 "Loop enable: %d\n"
         "Signal quality: %d %%\n"
         "Last Reply: %s\n", HOST_NAME, fw_ver/100, (fw_ver%100)/10, fw_ver%10,
		 mac, ESP.getSketchSize(), ESP.getSketchMD5().c_str(), ESP.getCpuFreqMHz(),
		 ESP.getFreeHeap(), esp_vcc, idht_ok, idht_hum, 0x25, idht_temp, 0xB0,
		 dht.computeHeatIndex(idht_temp, idht_hum, false), 0xB0, s_redy, tidht_hum,
		 0x25, tidht_temp, 0xB0, dht.computeHeatIndex(tidht_temp, tidht_hum, false),
		 0xB0, numberOfHours(tfs), numberOfMinutes(tfs), numberOfSeconds(tfs), elapsedDays(tfs),
		 ESP.getChipId(), ESP.getFlashChipId(), ESP.getFlashChipRealSize(), ESP.getFlashChipSize(), ESP.getFlashChipSpeed(),
		 (ESP.getFlashChipMode() == FM_QIO ? "QIO" : ESP.getFlashChipMode() == FM_QOUT ? "QOUT" : ESP.getFlashChipMode() == FM_DIO ? "DIO" : ESP.getFlashChipMode() == FM_DOUT ? "DOUT" : "UNKNOWN"),
		 ESP.getResetReason().c_str(), ESP.getResetS(), ESP.getResetInfo().c_str(), WiFi.status(),
		 WiFi.SSID().c_str(), WiFi.RSSI(), data_get, repsend, loop_en, get_signal_qua(100, 0), replyb);
        server.send(200, "text/xhtml", cstr1);
        delay(1000);
      });
    server.on("/nreplyb.txt", []() {
        server.send(200, "text/xhtml", nreplyb);
        delay(1000);
      });
    server.on("/replyb.txt", []() {
        server.send(200, "text/xhtml", replyb);
        delay(1000);
      });
	  
    server.on("/a1pr", []() {
		A1_data_pr(cstr1, BUF_SIZE);
		server.send(200, "text/plain", cstr1);
		delay(1000);
      });
	
    server.on("/online_update", []() {
        loop_en=false;
        server.send(200, "text/txt", "Try to selfupdate...\n");
        delay(1000);
        selfup=true;
      });

    server.on("/serial_in.txt", []() {
        Serial.flush();
        Serial.println("");
        sprintf(cstr1,"STR:1 ");
        delay(5000);
        i = strlen(cstr1);
        while (Serial.available() > 0) {
            while (Serial.available() > 0) {
                cstr1[i] = Serial.read();
                i++;
              }
            delay(10);
          }
        cstr1[i]='\0';
        server.send(200, "text/plain", cstr1);
        delay(1000);
      });
    server.on("/set", []() {
        if (server.arg("restart") != "") {
            server.send(200, "text/xhtml", "RESTARTING...\n");
            delay(1000);
            ESP.restart();
          }
        if (server.arg("format") != "") {
            if(atoi(server.arg("pass").c_str()) == 243) {
                server.send(200, "text/xhtml", "Starting format...\n");
                delay(1000);
                SPIFFS.format();
                ESP.restart();
              }
          }
        if (server.arg("v_mode") != "") {
            WiFi.disconnect(false);
            WiFi.begin(wname.c_str(), wpass.c_str());
          }
        if (server.arg("net_name") != "") {
        if (server.arg("pass") != "") {
              wname = server.arg("net_name");
              wpass = server.arg("pass");
              saveConfig();
              WiFi.disconnect(false);
              WiFi.begin(wname.c_str(), wpass.c_str());
            }
          }
        if (server.arg("backlight") != "") {
            lcdbacklset(atoi(server.arg("backlight").c_str()));
        }
        server.send(200, "text/html", webPage);
        delay(1000);
        saveConfig();
      });

    server.begin();
    httpUpdater.setup(&server);
    srlcd.setCursor(OFFSET,0);
    srlcd.print("НОРМ РЕЖИМ");
    srlcd.setCursor(0,1);
    srlcd.print("ПРЕХОД В НОРМ РЕЖИМ ");
    udp.begin(localPort);
    srlcd.setCursor(0,1);
    srlcd.print("Запуск ntp          ");

    WiFi.hostByName(ntpServerName, timeServerIP); 
    sendNTPpacket(timeServerIP);                                // send an NTP packet to a time server
    delay(1000);
    int cb = udp.parsePacket();
    if (cb) {
        Serial.println("no packet yet");
        udp.read(packetBuffer, NTP_PACKET_SIZE);
		
        unsigned long highWord = word(packetBuffer[40], packetBuffer[41]);
        unsigned long lowWord = word(packetBuffer[42], packetBuffer[43]);
        timecor = highWord << 16 | lowWord;
        timecor = timecor - (millis()/1000);
      }
    udp.stop();
    srlcd.clear();
	data_collect.attach(5, getd);
	data_send_tic.attach(300, data_send_f);
  }

void loop() {
    yield();
    server.handleClient();
	
	if(narodmon_send==true) {
	  repsend = !NAROD_data_send(cstr1, BUF_SIZE);
	  narodmon_send=false;
	}
    if(loop_en == true) {
        esp_vcc = ESP.getVcc()/1000.0;
		if(data_get==true) {
			tmp=dht.readHumidity();
			if(tmp < 0 || tmp > 100 || tmp == NAN){
				idht_ok=false;
			}
			if(tmp > 0 && tmp < 100 && tmp != NAN){
				idht_temp = dht.readTemperature();;
				idht_hum = tmp;
				idht_ok=true;
			}
			data_get=false;
			if(idht_ok==1){
			sdht_hum[s_i]=idht_hum;
			sdht_temp[s_i]=idht_temp;
			if(s_i >= S_MAX) {
				s_redy=1;
				s_i=0;
			}
			else {
				s_i++;
			}
		}
		if(s_redy==1) {
				tidht_hum=get_scsf(sdht_hum,S_MAX);
				tidht_temp=get_scsf(sdht_temp,S_MAX);
			}
		else {
				tidht_hum=idht_hum;
				tidht_temp=idht_temp;
			}
		}
        if(client.available()) {
            bzero(replyb, RBUF_SIZE);
            for(i = 0;client.available() && i < RBUF_SIZE;i++)
            {
                replyb[i] = client.read();
            }
            if(i <= (RBUF_SIZE-2))
                replyb[i+1]='\0';
            else
                replyb[RBUF_SIZE-1]='\0';
            if(replyb[13]== 'O' && replyb[14]== 'K')
            {
				int y =0;
				for(i=0;i<(strlen(replyb)-4); i++)
				{
					if(replyb[i]== ';' && replyb[i+1]== 0x0a ) {
						break; }
					if(replyb[i]== 0x0d && replyb[i+1]== 0x0a && replyb[i+2]== 0x0d && replyb[i+3]== 0x0a)
					{
						y=i+4;
					}
				}
                for(i=y; i < strlen(replyb); i++)
                {
                    replyb[i-y] = replyb[i];
                    if(replyb[i] == ';')
                    {
                        if(i<(RBUF_SIZE-1))
                        {
                	    i++;
                            for(i = i-y; i < RBUF_SIZE; i++){
                               replyb[i]='\0';}
                            parse_A1DSP(replyb);
                        }
                        break;
                    }
                }
            }
          }
    }

    delay(1000);

    if(loop_i > 10){
        httpRequest();
        loop_i = 0;
        loop_u++;
      }
    yield();
    
    if(selfup==true) {
        ESP.wdtDisable();
        srlcd.clear();
        srlcd.setCursor(0,0);
        ESPhttpUpdate.rebootOnUpdate(false);
        srlcd.print("Запуск обновления... ");
        delay(2000);
        t_httpUpdate_return ret = ESPhttpUpdate.update("http://dev.a1mc.ru/rom/esp8266/disp/flash.bin");
        srlcd.setCursor(0,1);
        switch(ret) {
            case HTTP_UPDATE_FAILED:
                srlcd.print("Ошибка обновления:  ");
                delay(2000);
                srlcd.print(ESPhttpUpdate.getLastErrorString().c_str());
                break;

            case HTTP_UPDATE_NO_UPDATES:
                srlcd.print("Обн. не требуется   ");
                break;

            case HTTP_UPDATE_OK:
                srlcd.print("Обновлено успешно   ");
                delay(2000);
                srlcd.setCursor(0,1);
                srlcd.print("Перезагружаюсь...   ");
                delay(2000);
                ESP.restart();
                break;
        }
        selfup=false;
        delay(2000);
        srlcd.clear();
        loop_en = true;
    }
    if(loop_en == true) {
        if(dht_ok == 1){
            srlcd.setCursor(0,1);
            char sm[2] = {' ', ' '};
            srlcd.setCursor(0,1);
            yield();
            if(loop_u==1){
                if(dht_hum > 98){
                loop_u++;}
                sprintf(cstr1, "Влажн: %.2f", dht_hum);
                sm[0]=0x25;
              }
            else if(loop_u==2){
                if(lux == 0){
                loop_u++;}
                sprintf(cstr1, "Осв: %.2fлкс", lux);}
            else if(loop_u==3){
                sprintf(cstr1, "Темп: %.2f", dht_temp);
                sm[0]=0x99;
                sm[1]='C';
              }
			else if(loop_u==4 && idht_ok == true){
				sprintf(cstr1, "Вн темп: %.2f", tidht_temp);
				sm[0]=0x99;
				sm[1]='C';
			}
			else if(loop_u==5 && idht_ok == true){
				sprintf(cstr1, "Вн влажн: %.2f", tidht_hum);
				sm[0]=0x25;
			}
			
            else {
                sprintf(cstr1, "Давлен: %.2f%ммРтСт", bmp_pre);
                loop_u=0;
              }
            srlcd.print(cstr1);
            srlcd.writecode(sm[0]);
            srlcd.writecode(sm[1]);
            for( i=strlen(cstr1); i <= 22; i++)
			{
				srlcd.write(' ');
			}
          }
        yield();
        unsigned long secsSince1900 = timecor + (millis()/1000);
        srlcd.setCursor(18,0);
        switch(get_signal_qua(6, 0))
        {
            case 0:
            srlcd.writecode(0x80);
            srlcd.writecode(0x80);
                break;
            case 1:
            srlcd.writecode(0x81);
            srlcd.writecode(0x80);
                break;
            case 2:
            srlcd.writecode(0x82);
            srlcd.writecode(0x80);
                break;
            case 3:
            srlcd.writecode(0x83);
            srlcd.writecode(0x80);
                break;
            case 4:
            srlcd.writecode(0x83);
            srlcd.writecode(0x81);
                break;
            case 5:
            srlcd.writecode(0x83);
            srlcd.writecode(0x82);
                break;
            case 6:
            srlcd.writecode(0x83);
            srlcd.writecode(0x83);
                break;
        }
        // now convert NTP time into everyday time:
        // Unix time starts on Jan 1 1970. In seconds, that's 2208988800:
        const unsigned long seventyYears = 2208988800UL;
        // subtract seventy years:
        unsigned long epoch = secsSince1900 - seventyYears;
        srlcd.setCursor(0,0);
		srlcd.print(dht.computeHeatIndex(idht_temp, idht_hum, false));
        srlcd.setCursor(5,0);
		srlcd.print(" ");
        i=numberOfHours(epoch);
        i+=3;
        if(i>23) {
            i-=24;
          }
        if ( i < 10 ) {
            // In the first 10 minutes of each hour, we'll want a leading '0'
            srlcd.print('0');
          }
        srlcd.print(i);
        srlcd.print(':');
        if ( numberOfMinutes(epoch) < 10 ) {
            // In the first 10 minutes of each hour, we'll want a leading '0'
            srlcd.print('0');
          }
        srlcd.print(numberOfMinutes(epoch));
        srlcd.print(':');
        if ( numberOfSeconds(epoch) < 10 ) {
            // In the first 10 seconds of each minute, we'll want a leading '0'
            srlcd.print('0');
          }
        srlcd.print(numberOfSeconds(epoch));
      }
    loop_i++;
  }

bool parse_A1DSP(char* tempstr) {
    //rx входная строка, rs колличество символов в строке, rc количество параметров
	
    /*srlcd.setCursor(OFFSET,0);
    srlcd.print("ПАРС А1ПРО");
	delay(1000);*/
    bmp_ok=false;
    lux_ok=false;
    dht_ok=false;
	int st_col = 0;
	for(int i =0; i<strlen(tempstr); i++)
	{
		if(tempstr[i] == ':') {
			st_col++; }
	}
    if(st_col<1 || st_col > 25)
		return false;
	
	yield();
	int col=st_col;
	st_col++;
    int i = 0;
    float *dat_mas = (float *)malloc(st_col * sizeof(float));
/*    for(i=0 ; i < st_col; i ++)
    {
	dat_mas[i]=0;
    }*/
	char **name_mas = (char **)malloc(st_col * sizeof(char *));
	for(i = 0; i < st_col; i++) {
		name_mas[i] = (char *)malloc(15 * sizeof(char));
	}
	
    /*srlcd.setCursor(OFFSET,0);
    srlcd.print("РАЗД А1ПРО");
	delay(1000);*/
	splint_rtoa(tempstr, strlen(tempstr), col, name_mas, dat_mas);
	//return col;
	yield();
	if(col > 0) {
		data_rec=true;
		for(int ilp = 0; ilp < col; ilp++) {
			yield();
			//tempstr += String("\nName = ") + name_mas[ilp] + String(" data = ") + dat_mas[ilp];
			if (strcmp(name_mas[ilp], "RDY") == 0) {
				rdy=dat_mas[ilp];
			}
			else if (strcmp(name_mas[ilp], "MQV5") == 0)  {
				mqv5=dat_mas[ilp];
			}
			else if (strcmp(name_mas[ilp], "MQV") == 0)  {
				mqv=dat_mas[ilp];
			}
			else if (strcmp(name_mas[ilp], "VMQ7") == 0) {
				mq7=dat_mas[ilp];
			}
			else if (strcmp(name_mas[ilp], "VMQ9") == 0) {
				mq9=dat_mas[ilp];
			}
			else if (strcmp(name_mas[ilp], "VMQ9_5") == 0) {
				mq9_5=dat_mas[ilp];
			}
			else if (strcmp(name_mas[ilp], "VIN") == 0)  {
				vin=dat_mas[ilp];
			}
			else if (strcmp(name_mas[ilp], "MCVCC") == 0){
				mc_vcc=dat_mas[ilp];
			}
			else if (strcmp(name_mas[ilp], "MCTMP") == 0){
				mc_temp=dat_mas[ilp];
			}
			else if (strcmp(name_mas[ilp], "LUX") == 0)  {
				lux=dat_mas[ilp];
				lux_ok=true;
			}
			else if (strcmp(name_mas[ilp], "DHUM") == 0) {
				dht_hum=dat_mas[ilp];
				dht_ok=true;
			}
			else if (strcmp(name_mas[ilp], "DTMP") == 0) {
				dht_temp=dat_mas[ilp];
				dht_ok=true;
			}
			else if (strcmp(name_mas[ilp], "BPRE") == 0) {
				bmp_pre=dat_mas[ilp];
				bmp_ok=true;
			}
			else if (strcmp(name_mas[ilp], "RFWVER") == 0) {
				if(dat_mas[ilp] > fw_ver){
					srlcd.setCursor(0,1);
					srlcd.print(dat_mas[ilp]);
					srlcd.print(">");
					srlcd.print(fw_ver);
					delay(2000);
					selfup=true;}
			}
			else if (strcmp(name_mas[ilp], "BTMP") == 0) {
				bmp_temp=dat_mas[ilp];
				bmp_ok=true;
			}
		}	
	}
	else {
		data_rec=false;
	}
	
	free(dat_mas);
	for(i=0; i < st_col; i++) {
		free(name_mas[i]);
	}
	free(name_mas);
	
	return data_rec;
	
}

unsigned long sendNTPpacket(IPAddress& address){
    Serial.println("[NTP]sending NTP packet...");
    // set all bytes in the buffer to 0
    memset(packetBuffer, 0, NTP_PACKET_SIZE);
    // Initialize values needed to form NTP request
    // (see URL above for details on the packets)
    packetBuffer[0] = 0b11100011;                               // LI, Version, Mode
    packetBuffer[1] = 0;                                        // Stratum, or type of clock
    packetBuffer[2] = 6;                                        // Polling Interval
    packetBuffer[3] = 0xEC;                                     // Peer Clock Precision
    // 8 bytes of zero for Root Delay & Root Dispersion
    packetBuffer[12]  = 49;
    packetBuffer[13]  = 0x4E;
    packetBuffer[14]  = 49;
    packetBuffer[15]  = 52;

    // all NTP fields have been given values, now
    // you can send a packet requesting a timestamp:
    udp.beginPacket(address, 123);                              //NTP requests are to port 123
    udp.write(packetBuffer, NTP_PACKET_SIZE);
    udp.endPacket();
}

void httpRequest() {
    // close any connection before send a new request.
    // This will free the socket on the WiFi shield
    client.stop();

    // if there's a successful connection:
    if (client.connect("dev.a1mc.ru", 80)) {
        //Serial.println("connecting...");
        // send the HTTP GET request:
        client.println("GET /kd1.php HTTP/1.1");
        client.println("Host: dev.a1mc.ru");
        client.println("User-Agent: ESP_DISP/1.1");
        client.println("Connection: close");
        client.println();
      } else {
          // if you couldn't make a connection:
          //Serial.println("connection failed");
        }
  }


bool loadConfig() {
    File configFile = SPIFFS.open("/config.json", "r");
    if (!configFile) {
        return false;
      }

    size_t size = configFile.size();
    if (size > 512) {
		configFile.close();
        return false;
      }


    StaticJsonBuffer<512> jsonBuffer;
    JsonObject &json = jsonBuffer.parseObject(configFile);
	
	

    if (!json.success()) {
        return false;
      }

    if(json["fw_ver"]==NULL) {
        return false;}
    if(atoi(json["fw_ver"]) != fw_ver){
		return false;
    }
	
    lcdbacklset(tobool(json["lcdbackl"]));

	 
    return true;
  }

bool saveConfig() {
    StaticJsonBuffer<512> jsonBuffer;
    JsonObject &json = jsonBuffer.createObject();
    File configFile = SPIFFS.open("/config.json", "w");
    if (!configFile) {
        return false;
    }
    json["fw_ver"] = fw_ver;
    json["lcdbackl"] = lcdbacklset();
    json["wname"] = wname;
    json["wpass"] = wpass;
    json.printTo(configFile);
	configFile.close();
    return true;
  }
  
void getd()	{
	data_get=true;
	return;
}

bool A1_data_pr(char *s, unsigned int s_size) {
  bzero(s, s_size);
  sprintf(s,
		  "EVC:%f", esp_vcc);
	if(idht_ok == true) {
	sprintf(s,
			"%s DTMP:%f"
			  " DHUM:%f", s, tidht_temp, tidht_hum);
	}
  sprintf(s, "%s ;\n\0", s);
  return 0;
}

bool NAROD_data_send(char *str,short int size) {
  WiFiClient client;
  bzero(str, size);
  
  byte bmac[6];
  WiFi.macAddress(bmac);
  bzero(mac, 20);
  sprintf(mac, "%X-%X-%X-%X-%X-%X", bmac[0], bmac[1], bmac[2], bmac[3], bmac[4], bmac[5]);
  sprintf(str,
          "#%X-%X-%X-%X-%X-%X#%s_v%d.%d.%d\n"
		  "#FHED#%d#Free ram in byte\n"
		  "#lcdbackl#%d\n", bmac[0], bmac[1], bmac[2], bmac[3], bmac[4], bmac[5], HOST_NAME, fw_ver/100, (fw_ver%100)/10, fw_ver%10, ESP.getFreeHeap(),lcdbacklset());
  if(idht_ok == true) {
	sprintf(str,
			"%s#DTMP#%.2f\n"
			  "#DHUM#%.2f\n", str, tidht_temp, tidht_hum);
  }
  sprintf(str, "%s##\n\0", str);
  
  client.connect("narodmon.ru", 8283);											
  client.print(str);  
  
  unsigned long timeout = millis();
  while (client.available() == 0) {
    if (millis() - timeout > 1000) {
      client.stop();
	  return false;
    }
  }
  
  for(i = 0; i < (RBUF_SIZE - 10); i++)
  {
	  nreplyb[i]=client.read();
	  if(nreplyb[i] == '\r' || nreplyb[i] == '\0' || nreplyb[i] == '\n')
		  break;
  }
  nreplyb[i]='\0';
  if(nreplyb[0] == 'O' && nreplyb[1] == 'K') {
	return true;
  }
  else if(nreplyb[0] == '#') {
	  parse_NAROD(nreplyb);
		return true;  
  }
  
  
  char tmp = nreplyb[7];
  nreplyb[7]='\0';
  if(i > 8 && strcmp(nreplyb,"INTERVAL")){
    nreplyb[7]=tmp;
	return true;  
  }
  nreplyb[7]=tmp;
  if(nreplyb[0]== '#') {
	  parse_NAROD(nreplyb);
		return true;  
  }
  
  return false;
}

void data_send_f() {
	narodmon_send = true;
	return;
}

bool parse_NAROD(char* tempstr) {
    bool data_rec=false;
 	int st_col = 0;
 	
 	for(int i =0; i<strlen(tempstr); i++)
	{
		if(tempstr[i] == '=') {
			st_col++; }
	}
    if(st_col<0 || st_col > 25)
		return false;
	
	yield();
	int col=st_col;
    int i = 0;
    
    int *dat_mas = (int *)malloc(st_col * sizeof(int));
	char **name_mas = (char **)malloc(st_col * sizeof(char *));
	for(i = 0; i < st_col; i++) {
		name_mas[i] = (char *)malloc(15 * sizeof(char));
	}
	
	for(int t = 0; t < (strlen(tempstr)-1); t++){
		tempstr[t]=tempstr[t+1];
	}
	tempstr[strlen(tempstr)-1]='\0';
	splint_narod(tempstr, strlen(tempstr), col, name_mas, dat_mas);
	yield();
	if(col > 0) {
		data_rec=true;
		for(int ilp = 0; ilp < col; ilp++) {
			yield();
			if (strcmp(name_mas[ilp], "lcdbackl") == 0) {
				lcdbacklset(dat_mas[ilp]);
				saveConfig();
			}		
		}	
	}
	else {
		data_rec=false;
	}
	free(dat_mas);
	for(i=0; i < st_col; i++) {
		free(name_mas[i]);
	}
	free(name_mas);

	return data_rec;
}

bool lcdbacklset(int bkl){
 switch (bkl) {
	case 1:
		lcdbackl=true;
		srlcd.backlightOn();
		break;
       case 0:
		srlcd.backlightOff();
		lcdbackl=false;
		break;
		}
 return lcdbackl;
}

bool lcdbacklset(){
	return lcdbackl;
}
#pragma GCC pop_options