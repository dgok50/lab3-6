#include "ESP8266HTTPUpdateServer.h"
#include <ESP8266HTTPClient.h>
#include <ESP8266httpUpdate.h>
#include "FS.h"
#define ESP_CH
#include "a1fl.c" //Библиотека с прекладными функциями
#include <ArduinoJson.h>
#include <string.h>
#include <stdio.h>
#include <ESP8266WebServer.h>
#include <ESP8266WiFi.h>
#include <SPI.h>
#include <WiFiClient.h>
#include <ESP8266AVRISP.h>
#include <Ticker.h>
#include <limits.h>
#include <float.h>

/*Обьявление макросов*/
#define SECS_PER_MIN  (60UL) //Кол секунд в минуту
#define SECS_PER_HOUR (3600UL) //Кол секунд в час
#define SECS_PER_DAY  (SECS_PER_HOUR * 24L) //Кол секунд в день 


#define BUF_SIZE 3512 //Размер буффера для формирование страниц
#define RBUF 256 //Размер буффера для ответов сервера
#define RDTIME 300 //Время задержки в микросекундах

#define MQ7_RL 10 //Сопротивление нагрузки датчика MQ7 в кОм
#define MQ9_RL 10 //Сопротивление нагрузки датчика MQ9 в кОм

#define MQ7_ROCLAIR 27 //Сопротивление датчика MQ7 кОм в чистом воздухе 
#define MQ9_ROCLAIR 9.8 //Сопротивление датчика MQ9 кОм в чистом воздухе

#define DEFHOSTNAME "CH_ESP_ST"

#define numberOfSeconds(_time_) (_time_ % SECS_PER_MIN)  //Перевод микросекунд в секунды 
#define numberOfMinutes(_time_) ((_time_ / SECS_PER_MIN) % SECS_PER_MIN) //Перевод микросекунд в минуты
#define numberOfHours(_time_) (( _time_% SECS_PER_DAY) / SECS_PER_HOUR) //Перевод микросекунд в часы
#define elapsedDays(_time_) ( _time_ / SECS_PER_DAY) //Перевод микросекунд в дни 


char hostname_load[20]; //Имя хоста
const int fw_ver = 135; //Версия прошивки

Ticker data_collect, data_send_tic; //Обьявление таймеров событий

ADC_MODE(ADC_VCC); //Установка АЦП в режим считывания питающиго напряжения

const char *password = "012345780"; //Пароль для аварийной сети

ESP8266AVRISP avrprog(328, 2); //Инициализация библиотеки ISP прогроматора
ESP8266WebServer server(80); //Инициализация библиотеки веб сервера, с установкой порта
ESP8266HTTPUpdateServer httpUpdater; //Инициализация библиотеки сервера самообновления
WiFiClient client;
  
/*Инициализация переменных*/
float mqv=0, mqv5=0, mq1=0, mq2=0, mq2_5=0, mq2_ro=0, mq2_5ro=0, mq1_ro=0;
unsigned long mq9LPGppm=0, mq9COppm=0, mq7COppm=0;
float vin=0, mc_vcc=0, mc_temp=0, lux=0, esp_vcc=0;
float dht_temp=0, dht_hum=0, bmp_temp=0, bmp_pre=0, rdy=0;
unsigned int loop_i = 0, i=0;
//unsigned long loop_time=0, loop_time_new=0;
unsigned long tfs = 0;
unsigned short int send_errors=0, rec_errors=0, t=0; 
volatile bool bmp_ok=false, lux_ok=false, dht_ok=false, data_rec=false, rsuc=true;
volatile bool ispmode = false, drq = false, send_data = false, repsend = false, selfup=false;
volatile bool data_get=true, narodmon_send=true, force_error=false;
char cstr1[BUF_SIZE], replyb[RBUF], rec_buff[RBUF]; //Инициализация временных массивов
char mac[20]; //Переменная для записи MAC адреса строкой

/*Прототипы функций*/
bool loadConfig(); //Функция загрузки конфигурации
bool saveConfig(); //Функция сохранения конфигурации
bool NAROD_data_send(char *, short int); //Функция компликтации пакета данных для отправки на народный мониторинг narodmon.ru
bool A1_data_pr(char *, unsigned int); //Функция компликтации пакета внутреннего протокола для обмена данными между устр 
void k_tdp(); //Функция утановления флага получения данных из мк 
int get_state(char *, unsigned int); //Функция компликтации XML файла статуса
bool parse_A1DSP(char*); //Функцияпарсинга внутреннего протакола обмена с мк
void data_send_f(); //Функция утановления флага отправки данных получения данных
bool sbufclean(); //Функция очистки буфера UART порта


long get_signal_qua(long rfrom, long rto){ //Функция ращёта качества приёма сигналана в % основе RSSI (Децибел на милливат)
	long rssi = WiFi.RSSI();
	return get_signal_qua(rfrom, rto, rssi);
}

//----------------------------------------------

//Функция расчёта сопротивление датчика
float calculateResistance(float vrl, float vcc, float RL) {
  //float vrl = rawAdc*(vcc / 1023);
  float rsAir=0;
  if(vrl!= 0){
  rsAir = (vcc - vrl)/vrl*RL;}
  return rsAir;
}

//Функция калибровки датчиков газа
float calibrate(float resist, float RoClAir) {
  if(resist!=0){
  return resist/RoClAir; } //ro
  return 0;
}

//Считывание данных датчиков газов
float readRatio(float resist, float ro) {
  if(ro!= 0){
  return resist/ro;}
  return 0;
}

unsigned long readScaled(float a, float b, float ratio) {
  return exp((log(ratio)-b)/a);
}

//----------------------------------------------
  
void avrisp() { //Установка мк в режимпрошивки
  avrprog.setReset(false);
  avrprog.begin();
  ispmode = true;
}

const char *webPage = "<!DOCTYPE html>" //Главнаявеб страница
  "<html>\n"
  " <body>\n"
  "  <h1>ESP8266 Sensors Web</h1>\n"
  "  <a href= \"/update\">Update dialog</a><br>\n"
  "  <a href= \"/online_update\">Online Update</a><br>\n"
  "  <a href= \"/config.json\">Json Config info</a><br>\n"
  "  <a href= \"/xml.xml\">Info xml</a><br>\n"
  "  <a href= \"/a1pr\">A1_DSP</a><br>\n"
  "  <a href= \"/sysinfo.txt\">System info</a><br>\n"
  "  <a href= \"/set?restart=1\">Restart device</a><br>\n"
  "  <a href= \"/set?format=243\">Reset default st</a><br>\n"
  "  <a href= \"/set?reset_ro=243\">Reset MQ ro const</a><br>\n"
  "  <a href= \"/set?avrisp_s=1\">Go to isp</a><br>\n"
  "  <a href= \"/set?avr_reset=1\">Restart avr</a><br>\n"
  "  <a href= \"/i2c_scan.txt\">I2C Scan</a><br>\n"
  "  <a href= \"/serial_in.txt\">Get serial atmega data</a><br>\n"
  "  <a href= \"/parse.txt\">Parse atmega data</a>\n"
  " </body>\n"
  "</html>\n";

	
void setup() { //Функция настройки запускается перед функцией главного цикла
  bzero(cstr1, BUF_SIZE); //Чистка буфера веб страницы
  bzero(rec_buff, RBUF); //Чистка буффера приёма данных из UART
  bzero(replyb, RBUF); //Чистка буфера ответа из narodmon
  bzero(hostname_load, 20);
  
  WiFi.begin(); //Инициализация функционала беспроводной сети
  delay(1000);
  Serial.begin(9600); //Инициализация UART порта
  // Serial.println();

  if (!SPIFFS.begin()) { //Инициализация встроенной фс, и формотирование в случае невозможности
    SPIFFS.format();
  }

  avrprog.setReset(false); //Даём мк возможность запустица (нога RESET подтягивается к питанию)

  WiFi.disconnect(); //Отсоеденение от сети в случае если подключение уже было выполнено до перезапуска
  delay(200);
  byte bmac[6]; //выбеление временной байтовой переменной под мак адрес 
  WiFi.macAddress(bmac); //считывание мак адреса модуля
  bzero(mac, 20); //Очистка строки
  sprintf(mac, "%X-%X-%X-%X-%X-%X", bmac[0], bmac[1], bmac[2], bmac[3], bmac[4], bmac[5]); //Запись мак адреса в строку
  /*Считывание последней причины перезапуска и установка флага принудительного обнавления в случае присуцтвия исключений*/
  selfup=!ESP.getResetS();
  if(selfup == false){ //В случаеотсутствия исключений попытказагрузки конфиг файла
   if (!loadConfig()) {
	strlcpy(hostname_load, DEFHOSTNAME, sizeof(hostname_load));
    SPIFFS.remove("/config.json"); //Удаление и пересозданияфайла в случаеневозможности загрузки
    if (!saveConfig()) { //В случае невозможности создания файла формотирование фс и перезапуск
		SPIFFS.format();
		ESP.restart();
    }
   } 
  }
  
  WiFi.mode(WIFI_OFF); //Повторный сброс беспроводнойсети тк WiFi.disconnect()иногда несрабатывает
  delay(100);
  WiFi.mode(WIFI_STA); //Устоновка режима клиента
  WiFi.disconnect(false); //
  delay(100);
  WiFi.hostname(hostname_load); //Установка имени хоста
  WiFi.begin("A1 Net", "84992434219"); //Подключение к тд
  WiFi.setAutoReconnect(true); //Включение режима автопереподключение
  
  t = 0;
  while (WiFi.status() != WL_CONNECTED) { //Ожудание установки соеденения
    delay(500);
    t++;
    if (t == 40) { //В случае невозможности
		selfup=false; //Отключение аварийного самообновления
		ispmode=false; //Отключение режима перепрошивки
		force_error=true; //Игнарирование всех ошибок
		WiFi.softAP(hostname_load, password); //запуск аварийной тд
		IPAddress myIP = WiFi.softAPIP(); //Выдача IP
    }
  }

  server.on("/", []() { //Инициализация обработчика главной страници
    server.send(200, "text/html", webPage); //Отправка данных клиенту
    delay(RDTIME);
  });

  server.on("/xml.xml", []() {  //Инициализация обработчика страници XML с данными датчиков
    get_state(cstr1, BUF_SIZE); //Формированиме XML
    server.send(200, "text/xml", cstr1); //Отправка данных клиенту
    delay(RDTIME);
  });
  server.on("/a1pr", []() {
    A1_data_pr(cstr1, BUF_SIZE); //формирование пакета с данными датчиков
    server.send(200, "text/plain", cstr1); //Отправка данных клиенту
    delay(RDTIME);
  });

  server.on("/config.json", []() {
    File file = SPIFFS.open("/config.json", "r"); //Открытие файла на чтение
    size_t sent = server.streamFile(file, "application/json"); //Отправка файла клиенту
    file.close(); //Закрытие файла
    delay(RDTIME);
  });

  server.on("/sysinfo.txt", []() { //Инфо страница статуса системы
	bzero(cstr1, BUF_SIZE);
	
	if(hostname_load[1]=='\0') {
		strlcpy(hostname_load,DEFHOSTNAME, sizeof(hostname_load));
		saveConfig();
	}
	/*Вывод имя хоста и версии прошивки*/
    sprintf(cstr1, "Hostname: %s\nFW Ver: %d.%d.%d\n", hostname_load, fw_ver/100, (fw_ver%100)/10, fw_ver%10);
	/*Вывод информации о компиляции*/
    sprintf(cstr1, "%sComp info, time: " __TIME__ ", date: " __DATE__ "\n", cstr1);
	/*Вывод мак адреса*/
	sprintf(cstr1, "%sMAC: %s\n", cstr1, mac);
	sprintf(cstr1, "%sSketchSize: %d\n", cstr1, ESP.getSketchSize()); //Размер скетча
    sprintf(cstr1, "%sSketchMD5: %s\n", cstr1, ESP.getSketchMD5().c_str()); //Хеш скетча
    sprintf(cstr1, "%sCPU Frq: %d MHz\n", cstr1, ESP.getCpuFreqMHz()); //Частота цп
    sprintf(cstr1, "%sFree Heap: %d\n", cstr1, ESP.getFreeHeap()); //Свободное ОЗУ
    sprintf(cstr1, "%sESP VCC: %.2f\n", cstr1, esp_vcc); //Наприжение питания
	tfs = millis()/1000; //Получение времени с момента старта
	sprintf(cstr1, "%sTime from start: %lu hours %lu minutes %lu Sec, and %lu days \n", cstr1, numberOfHours(tfs), numberOfMinutes(tfs), numberOfSeconds(tfs), elapsedDays(tfs));
    sprintf(cstr1, "%sESP Chip ID: %X\n", cstr1, ESP.getChipId()); //ID чипа
    sprintf(cstr1, "%sFlash real id: %X\n", cstr1, ESP.getFlashChipId()); //ID микрочипа флеш памяти
    sprintf(cstr1, "%sFlash real size: %d\n", cstr1, ESP.getFlashChipRealSize()); //Размер памяти по ID
    sprintf(cstr1, "%sFlash ide size: %d\n", cstr1, ESP.getFlashChipSize()); //Размер памяти по уст компилятора
    sprintf(cstr1, "%sFlash ide speed: %d\n", cstr1, ESP.getFlashChipSpeed()); //Скорость памяти
	/*Режим работы памяти*/
    sprintf(cstr1, "%sFlash ide mode: %s\n", cstr1, (ESP.getFlashChipMode() == FM_QIO ? "QIO" : ESP.getFlashChipMode() == FM_QOUT ? "QOUT" : ESP.getFlashChipMode() == FM_DIO ? "DIO" : ESP.getFlashChipMode() == FM_DOUT ? "DOUT" : "UNKNOWN"));
    sprintf(cstr1, "%sLast reset: %s\n", cstr1, ESP.getResetReason().c_str()); //Причина последней перезагрузки
	sprintf(cstr1, "%sReset info: %s\n", cstr1, ESP.getResetInfo().c_str()); //Описание
    sprintf(cstr1, "%sWiFi Status: %d\n", cstr1, WiFi.status()); //Статус соеденения
    sprintf(cstr1, "%sSSID: %s\n", cstr1, WiFi.SSID().c_str()); //Имя беспроводной сети
    sprintf(cstr1, "%sRSSI: %d dbm\n", cstr1, WiFi.RSSI()); //Уровень сигнала в дБм
    sprintf(cstr1, "%sSignal quality: %d %%\n", cstr1, get_signal_qua(100, 0)); //Уровень сигнала впроцентах
    sprintf(cstr1, "%sISP mode: %d\n", cstr1, ispmode); //Режим симулятора програматора
    sprintf(cstr1, "%sData recived success: %d\n", cstr1, rsuc); //Статус последнего считование данных с МК
    sprintf(cstr1, "%sData drq: %d\n", cstr1, drq); //Флаг считования данных
    sprintf(cstr1, "%sSelf Update: %d\n", cstr1, selfup); //Флаг обновления
    sprintf(cstr1, "%s\nRepet send: %d\n", cstr1, repsend); //Флаг переотправки данных
    sprintf(cstr1, "%sRecive errors: %d\n", cstr1, rec_errors); 
    sprintf(cstr1, "%sSend errors: %d\n", cstr1, send_errors); //Кол ошибок передачи
	sprintf(cstr1, "%sIgnore errors: %d\n", cstr1, force_error);
    sprintf(cstr1, "%sLast Reply: %s , fdite:%c\n", cstr1, replyb, replyb[0]); //Последний ответ сервера
    sprintf(cstr1, "%sRdy: %f\n", cstr1, rdy); //Статус готовности усреднённыхданных
    sprintf(cstr1, "%sMQ1_ro: %f\n", cstr1, mq1_ro); //Сопротивление датчика в чистом воздухе 1.5в
    sprintf(cstr1, "%sMQ2_ro: %f\n", cstr1, mq2_ro); //Сопротивление датчика в чистом воздухе 1.5в
    sprintf(cstr1, "%sMQ2_ro_5: %f\n", cstr1, mq2_5ro); //Сопротивление датчика в чистом воздухе 5в
    server.send(200, "text/xhtml", cstr1);
    delay(RDTIME);
  });
  server.on("/serial_in.txt", []() { //Вывод данных полученных от МК
    Serial.flush(); 
    Serial.println(""); //отправка запроса на данные
    sprintf(cstr1,"STR:1 ");
    delay(5000); //ожидание готовности
	i = strlen(cstr1);
    while (Serial.available() > 0) {
	  while (Serial.available() > 0) {
		  cstr1[i] = Serial.read(); //Считывание ответа
		  i++;
	  }
      delay(10);
    }
	cstr1[i]='\0';
    server.send(200, "text/plain", cstr1);
    delay(RDTIME);
  });
  server.on("/parse.txt", []() { //Принудительный запрос и парсинг данных
    Serial.flush();
    Serial.println("");
    delay(5000);
	i = 0;
    while (Serial.available() > 0) {
    yield();
	  while (Serial.available() > 0) {
		  rec_buff[i] = Serial.read();
		  i++;
	  }
      delay(10);
    }
	rec_buff[i]='\0';
	delay(10);
	server.sendContent(String("Recived: \n")+String(rec_buff)+String("\n Try split\n"));
	yield();
	int ret = parse_A1DSP(rec_buff);
	delay(10);
	server.sendContent(String("Ret: \n")+String(ret)+String("\n\n"));
    sprintf(cstr1, "%s\nParse state = %.2f\n", cstr1, rdy);
	sprintf(cstr1, "%sDate recived = %d\n", cstr1, data_rec);
	sprintf(cstr1, "%sbmp tmp = %.2f\n", cstr1, bmp_temp);
    server.send(200, "text/plain", cstr1);
    delay(RDTIME);
  });
  server.on("/i2c_scan.txt", []() { //Сканирование I2C шины
    Serial.print("S");
    delay(2000);
    int i=0;
    while (Serial.available() > 0) {
	  while (Serial.available() > 0) {
		  cstr1[i] = Serial.read();
		  i++;
	  }
      delay(100);
    }
	cstr1[i]='\0';
    server.send(200, "text/plain", cstr1);
    delay(RDTIME);
  });
  server.on("/debug", []() { //Вывод отладочных данных
    ESP.wdtDisable();
    server.sendContent(String("ESP8266: \n"));
    server.sendContent(String("\n mqv: ")+String(mqv)+String("\n mqv5: ")+String(mqv5)+String("\n mq1: ")+String(mq1));
    server.sendContent(String("\n mq2: ")+String(mq2)+String("\n vin: ")+String(vin)+String("\n mc_vcc: ")+String(mc_vcc));
    server.sendContent(String("\n mc_temp: ")+String(mc_temp)+String("\n lux: ")+String(lux)+String("\n esp_vcc: ")+String(esp_vcc));
    server.sendContent(String("\n dht_temp: ")+String(dht_temp)+String("\n dht_hum: ")+String(dht_hum)+String("\n bmp_temp: ")+String(bmp_temp));
    server.sendContent(String("\n bmp_pre: ")+String(bmp_pre)+String("\n rdy: ")+String(rdy)+String("\n loop_i: ")+String(loop_i));
    server.sendContent(String("\n i: ")+String(i)+String("\n send_errors: ")+String(send_errors)+String("\n tfs: ")+String(tfs));
    server.sendContent(String("\n data_get: ")+String(data_get)+String("\n narodmon_send: ")+String(narodmon_send));
    server.sendContent(String("\n cstr1: "));
    //server.sendContent(String(cstr1));
    server.sendContent(String("\n mac: "));
    //server.sendContent(String(mac));
    server.sendContent(String("\n replyb: "));
    //server.sendContent(String(replyb));
    server.sendContent(String("ATMEGA: \n"));
    Serial.print("D");
    delay(500);
	yield();
    while (Serial.available() > 0) {
      server.sendContent(Serial.readString());
      delay(200);
	  yield();
    }
    delay(RDTIME);
  });
  
  server.on("/online_update", []() { //Установка флага для самообновления
        server.send(200, "text/txt", "Try to selfupdate...\n");
		delay(RDTIME);
        selfup=true;
      });
	  
  server.on("/set", []() { 
    if (server.arg("reset_ro") != "") { //Сброс RO датчиков воздуха
		mq1_ro=0;
		mq2_ro=0;
		mq2_5ro=0;
    }    
	if (server.arg("restart") != "") { //Сброс модуля
      server.send(200, "text/xhtml", "RESTARTING...");
	  delay(RDTIME);
	  saveConfig();
      ESP.restart();
    }
    if (server.arg("avrisp_s") != "") { //Режим сетевого програматора (Включение)
      server.send(200, "text/xhtml", "Going to avrisp mode...");
	  delay(RDTIME);
      avrisp(); //Ввод в режим програматора
      // ESP.restart();
    }
    if (server.arg("data_get") != "") { //Смена режима получения данных
      if(data_get==0 && atoi(server.arg("data_get").c_str())) {
		data_collect.attach(60, k_tdp);
	  }
	  data_get=atoi(server.arg("data_get").c_str());
      server.send(200, "text/xhtml", "OK");
	  delay(RDTIME);
	  saveConfig();
    }
    if (server.arg("narodmon_send") != "") { //Смена режима отправки данных
	  narodmon_send = atoi(server.arg("narodmon_send").c_str());
      server.send(200, "text/xhtml", "OK");
	  delay(RDTIME);
	  saveConfig();
    }
    if (server.arg("format") != "") { //Форматирование встроенной фс
	  if(atoi(server.arg("pass").c_str()) == 243) {
		server.send(200, "text/xhtml", "Starting format...");
	    delay(RDTIME);
		SPIFFS.format();
		ESP.restart();
	  }
    }   
	
	if (server.arg("force_error") != "") { 
		force_error=tobool(server.arg("force_error").c_str());
		server.send(200, "text/xhtml", "set force_error flag");
		delay(RDTIME);
	    saveConfig();
    }
	
    if (server.arg("v_mode") != "") { //Принудительный переход в режим аварийной тд
      saveConfig();
      WiFi.disconnect(false);
      WiFi.begin("A1 Net", "84992434219");
    }
    if (server.arg("avr_reset") != "") { //Режим сетевого програматора (Jnrk.xtybt)
      avrprog.setReset(true); //Сброс МК
      delay(500);
      avrprog.setReset(false);
	  ispmode = false;//Установка флага работы програматтора
      server.send(200, "text/xhtml", "Reset avr...");
	  delay(RDTIME);
    }
  });

  server.begin();
  httpUpdater.setup(&server);
  data_collect.attach(60, k_tdp); //Установка таймера считывание данных 1 мин
  data_send_tic.attach(300, data_send_f); //Установка таймера отправки данных на 5мин
}

void loop() {
  server.handleClient(); //Обработка постпивших запросов клиентов
  delay(3);
  yield();
  if(selfup==true && ispmode == false) {
	    ispmode = true; //Установка флага програматора для избежания сробатований других функций
        ESP.wdtDisable();
        ESPhttpUpdate.rebootOnUpdate(false); //отключение перезапуска после обновления
        t_httpUpdate_return ret = ESPhttpUpdate.update("http://dev.a1mc.ru/rom/esp8266/ch_esp_sn/flash.bin");
        switch(ret) {
            case HTTP_UPDATE_FAILED:
                break;

            case HTTP_UPDATE_NO_UPDATES:
				break;

            case HTTP_UPDATE_OK:
                ESP.restart(); //В случае успешной перепрошивки перезапуск
                break;
        }
        selfup = false;
	    ispmode = false;
    }
  delay(1);
  yield();
  if (drq==true && ispmode == false){ //В случае установки флага получения данных, считование
    sbufclean(); //Очистка буфера компорта
    Serial.println(""); //Запрос данных
    bzero(rec_buff, RBUF); //Очистка буфера приёма
    delay(5000);
    esp_vcc=ESP.getVcc()/1000.0; //Считывание напряжение питания
    int is=0;
	bool stop = false;
    while (Serial.available() > 0) { //Ожидание данных
	  while (Serial.available() > 0) {
		  rec_buff[is] = Serial.read();
		  if(rec_buff[is] == ';') //В случяе обнаружения символа конца строки остановка считывания
		  {
			rsuc = true; //Установка флага успешного считования
			stop = true; //Установка флага остановки
		    break;
		  }
		  if(is>(RBUF-3)) //В случае подхода к концу буффера остановка
		  {
			rsuc = false; //Установка флага успешного считования
			stop = true; //Установка флага остановки
			break;
		  }
		  is++;
	  }
      delay(100);
	  if(stop == true)
		  break;
    }
	rec_buff[is+1]='\0'; //Кстановка символа конца строки
	rec_buff[is+2]='\0';
	if(rsuc==true) { //В случае успешного получения данных
		rsuc=parse_A1DSP(rec_buff); //Парсинг данных
	    if(mqv > 1.4 && mqv < 1.5 && mqv5 < 5.4 && mqv5 > 4.9){ //Проверка напряжения датчикови расчёт
			if(mq1_ro==0){
				mq1_ro=calibrate(calculateResistance(mq1/10, mc_vcc, MQ7_RL), MQ7_ROCLAIR);	  
			}
			else{
				mq7COppm= readScaled(-0.77, 3.38, readRatio(calculateResistance(mq1/10, mc_vcc, MQ7_RL), mq1_ro));
			}
			
			if(mq7COppm == 0) {
				mq1_ro=calibrate(calculateResistance(mq1/10, mc_vcc, MQ7_RL), MQ7_ROCLAIR);	
				saveConfig();
			}
	  
	  
			if(mq2_5ro==0){
				mq2_5ro=calibrate(calculateResistance(mq2_5/10, mc_vcc, MQ9_RL), MQ9_ROCLAIR);	
				mq2_ro=calibrate(calculateResistance(mq2/10, mc_vcc, MQ9_RL), MQ9_ROCLAIR);	  
				}
			else{
				mq9COppm=readScaled(-0.48, 3.10, readRatio(calculateResistance(mq2_5/10, mc_vcc, MQ9_RL), mq2_5ro));
				mq9LPGppm= readScaled(-0.48, 3.33, readRatio(calculateResistance(mq2_5/10, mc_vcc, MQ9_RL), mq2_5ro));
			}
			
			if(mq9COppm == 0 || mq9LPGppm == 0) {
				mq2_5ro=calibrate(calculateResistance(mq2_5/10, mc_vcc, MQ9_RL), MQ9_ROCLAIR);	
				mq2_ro=calibrate(calculateResistance(mq2/10, mc_vcc, MQ9_RL), MQ9_ROCLAIR);	  
				saveConfig();
			}
	    }
		if(repsend == true) { //В случае присутствия флагаповтора передачи
			if(send_errors > USHRT_MAX-20) //Проверка переполнения
			{
				send_errors=0; //Обнуление
			}
			send_errors++; //Увеличение счётчика ошибок
			if(send_errors > 20 && force_error != true) { //Перезапуск в случае привышения щётчика и отсутствие флага игнорирования
				ESP.restart();
			}
			send_data = true; //Установка флага отправки данных
		}
		rec_errors=0;
	}
	else {
		if(rec_errors > 20) { //В случае неполучение данных и привышения счётчика ошибок сброс МК
			avrprog.setReset(true);
			delay(1000);
			avrprog.setReset(false);
			rec_errors=0;
		}
		rec_errors++;
	}
	drq=false;
  }
  
  yield();
  if (send_data == true && ispmode == false) { //Отправка данных на народный мониторинг
	  send_data = false; //Сброс флага отправки
	  if(narodmon_send==1) {
	  repsend = !NAROD_data_send(cstr1, BUF_SIZE);}
  }
  
  yield();
  if (ispmode == true) { //Режи прошивки
    static AVRISPState_t last_state = AVRISP_STATE_IDLE;
    AVRISPState_t new_state = avrprog.update();
    if (last_state != new_state) {
      switch (new_state) {
      case AVRISP_STATE_IDLE: {
        break;
      }
      case AVRISP_STATE_PENDING: {
        break;
      }
      case AVRISP_STATE_ACTIVE: {
        break;
      }
      }
      last_state = new_state;
    }
    if (last_state != AVRISP_STATE_IDLE) {
      avrprog.serve();
    }
  }
}

bool loadConfig() { //Функция загрузки конфигурации
  File configFile = SPIFFS.open("/config.json", "r");
  if (!configFile) {
    return false;
  }

  if (configFile.size() > 1024) {
    return false;
  }

  StaticJsonBuffer<1024> jsonBuffer;
  
  JsonObject &rootp = jsonBuffer.parseObject(configFile);


  if (!rootp.success()) {
    return false;
  }

  // const char* serverName = rootp["serverName"];
  // const char* accessToken = rootp["accessToken"];
  if(rootp["serial_data_read"]==NULL || rootp["mac"]==NULL) {
	  return false; }
  
  if(atoi(rootp["fw_ver"]) >= fw_ver) {
	  return false; }
  
  
  data_get = tobool(rootp["serial_data_read"]);
  
  strlcpy(mac, rootp["mac"], sizeof(mac));
 
  strlcpy(hostname_load, rootp["HOSTNAME"] | DEFHOSTNAME, sizeof(hostname_load));
  
  narodmon_send = tobool(rootp["narodmon_send"]);
  mq1_ro = atof(rootp["mq1_ro"]);
  mq2_ro = atof(rootp["mq2_ro"]);
  mq2_5ro = atof(rootp["mq2_5ro"]);
  force_error = atof(rootp["force_error"]);
  
  configFile.close();
  return true;
}

bool saveConfig() {  
  File configFile = SPIFFS.open("/config.json", "w+");
  if (!configFile) {
    return false;
  }
  StaticJsonBuffer<256> jsonBuffer;
  JsonObject &rootp = jsonBuffer.createObject();
  rootp["fw_ver"]=fw_ver;
  rootp["mac"]=mac;
  rootp["force_error"]=force_error;
  rootp["HOSTNAME"]=hostname_load;
  rootp["serial_data_read"] = data_get;
  rootp["narodmon_send"] = narodmon_send;
  rootp["mq1_ro"] = mq1_ro;
  rootp["mq2_ro"] = mq2_ro;
  rootp["mq2_5ro"] = mq2_5ro;

  rootp.printTo(configFile);
  configFile.close();
  return true;
}

bool parse_A1DSP(char* tempstr) { //Парсинг данных МК
	//rx входная строка, rs колличество символов в строке, rc количество параметров
	bmp_ok=false;//Обнуление флагов считывание датчиков
	lux_ok=false;
	dht_ok=false;
	int st_col = 0;
	for(int i =0; i<strlen(tempstr); i++) //Вычесление количества переменных в строке
	{
		if(tempstr[i] == ':') {
			st_col++; }
	}
    if(st_col<1 || st_col > 25) //в случае выхода за пределы возможностей или недостаточности данных
		return false; //Выход из функции с ошибкой
	
	yield();
	int ilp=0, col=st_col;
    int i = 0;
	//выделение памяти под временные парные массивы имя данные
    float *dat_mas = (float *)malloc(st_col * sizeof(float));
	char **name_mas = (char **)malloc(st_col * sizeof(char *));
	for(i = 0; i < st_col; i++) {
		name_mas[i] = (char *)malloc(15 * sizeof(char));
	}
	
	col = splint_rtoa(tempstr, strlen(tempstr), col, name_mas, dat_mas); //парсинг данных
	
	yield();
	if(col > 0) { //вроверка успешного разбора
		data_rec=true;
		for(ilp = 0; ilp <= col; ilp++) { //поиск известных ключей
			yield(); //даём поработать беспроводной подсистеме
			if (strcmp(name_mas[ilp], "RDY") == 0) {
				rdy=dat_mas[ilp];
			}
			else if (strcmp(name_mas[ilp], "MQV5") == 0)  {
				mqv5=dat_mas[ilp];
			}
			else if (strcmp(name_mas[ilp], "MQV") == 0)  {
				mqv=dat_mas[ilp];
			}
			else if (strcmp(name_mas[ilp], "VMQ1") == 0) {
				mq1=dat_mas[ilp];
			}
			else if (strcmp(name_mas[ilp], "VMQ2") == 0) {
				mq2=dat_mas[ilp];
			}
			else if (strcmp(name_mas[ilp], "VMQ25") == 0) {
				mq2_5=dat_mas[ilp];
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
	free(name_mas); //Очистка памяти
	
	return data_rec;
}

int get_state(char *s, unsigned int s_size) { //Формирование XML на основе шаблона
  sprintf(s,
          "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n"
		  "<esp>\n"
		  " <fw>\n"
		  "  <b_time>" __TIME__ "</b_time>\n"
		  "  <b_date>" __DATE__ "</b_date>\n"
		  "  <ver>%d</ver>\n"
		  " </fw>\n"
		  " <status>\n"
		  "  <esp_vcc>%f</esp_vcc>\n"
		  "  <mc_vcc>%f</mc_vcc>\n"
		  "  <mc_tmp>%f</mc_tmp>\n"
		  "  <data_recived>%d</data_recived>\n", fw_ver, esp_vcc, mc_vcc, mc_temp, data_rec);
  if(data_rec == true) {
  sprintf(s, "%s  <data_redy>%f</data_redy>\n", s, rdy);}
  sprintf(s, "%s </status>\n", s);  
  if(data_rec == true) {
  sprintf(s, "%s <sensors>\n", s);
  
  sprintf(s, "%s  <bmp_r>%d</bmp_r>\n", s, bmp_ok);
  if(bmp_ok == true) {
	sprintf(s, "%s  <bmp>\n",s);
	sprintf(s,
			"%s   <temp>%f</temp>\n"
			"   <pre>%f</pre>\n", s, bmp_temp, bmp_pre);
	sprintf(s, "%s  </bmp>\n",s);
  }
    
  sprintf(s, "%s  <dht_r>%d</dht_r>\n", s, dht_ok);
  if(dht_ok == true) {
	sprintf(s, "%s  <dht>\n",s);
	sprintf(s,
			"%s   <temp>%f</temp>\n"
			  "   <hum>%f</hum>\n", s, dht_temp, dht_hum);
	sprintf(s, "%s  </dht>\n",s);
  }
     
  sprintf(s, "%s  <lux_r>%d</lux_r>\n", s, lux_ok);
  if(lux_ok == true) {
	sprintf(s, "%s  <lux>%f</lux>\n", s, lux);
  }
    
  sprintf(s, "%s  <else_r>%f</else_r>\n", s, rdy);
  if(rdy == 1) {
	sprintf(s, "%s  <else>\n",s);
	sprintf(s, "%s   <mq7co>%lu</mq7co>\n", s, mq7COppm);
	sprintf(s, "%s   <mq9co>%lu</mq9co>\n", s, mq9COppm);
	sprintf(s, "%s   <mq9LPG>%lu</mq9LPG>\n", s, mq9LPGppm);
	sprintf(s,
			"%s   <vin>%f</vin>\n"
			  "   <mqv>%f</mqv>\n"
			  "   <mqv5>%f</mqv5>\n"
			  "   <mq7>%f</mq7>\n"
			  "   <mq9>%f</mq9>\n"
			  "   <mq9_5>%f</mq9_5>\n", s, vin, mqv, mqv5, mq1, mq2, mq2_5);
	sprintf(s, "%s  </else>\n",s);
  }
  
  sprintf(s, "%s </sensors>\n", s);
  }
  sprintf(s, "%s</esp>\n", s);
  
  return 0;
}

bool NAROD_data_send(char *str,short int size) { //Отправка данных на народны мониторинг
  bzero(str, size);
  sprintf(str,
          "#%s#%s_v%d.%d.%d\n"
		  "#FHED#%d#Free ram in byte\n", mac, hostname_load, fw_ver/100, (fw_ver%100)/10, fw_ver%10, ESP.getFreeHeap());
  if(data_rec == true) { 
  sprintf(str,
		  "%s#MVC#%.2f\n"
		  "#MTMP#%.2f\n", str, mc_vcc, mc_temp);
  if(bmp_ok == true) {
	sprintf(str,
			"%s#BTMP#%.2f\n"
			"#BPRE#%.2f\n", str, bmp_temp, bmp_pre);
  }
  if(dht_ok == true) {
	sprintf(str,
			"%s#DTMP#%.2f\n"
			  "#DHUM#%.2f\n", str, dht_temp, dht_hum);
  }
  if(lux_ok == true) {
	sprintf(str, "%s#LUX#%.2f\n", str, lux);
  }
    
  if(rdy == 1) {
	  
	  if(mqv > 1.4 && mqv < 1.5 && mqv5 < 5.4 && mqv5 > 4.9){
	   if(mq1_ro!=0){
		  mq7COppm= readScaled(-0.77, 3.38, readRatio(calculateResistance(mq1/10, mc_vcc, MQ7_RL), mq1_ro));
		  sprintf(str, "%s#MQ7#%lu#MQ7 CO ppm\n", str, mq7COppm);
	   }
	   if(mq2_5ro!=0){
		  mq9COppm=readScaled(-0.48, 3.10, readRatio(calculateResistance(mq2_5/10, mc_vcc, MQ9_RL), mq2_5ro));
		  sprintf(str, "%s#MQ9#%lu#MQ9 CO ppm\n", str, mq9COppm);
		  
		  mq9LPGppm= readScaled(-0.48, 3.33, readRatio(calculateResistance(mq2_5/10, mc_vcc, MQ9_RL), mq2_5ro));
		  sprintf(str, "%s#MQ9L#%lu#MQ9 LPG ppm\n", str, mq9LPGppm);
	   }
	  }
	sprintf(str,
			"%s#VIN#%.2f\n",
			str, vin);
	
  }
  }
  sprintf(str, "%s##\n\0", str);
  
  if (!client.connect("narodmon.ru", 8283)) {	//Соеденение с сервером (открытие сокета)
	  return false;
  }
  client.print(str);  //Отправка пакета
  
  unsigned long timeout = millis();

  while (client.available() == 0) { //Ожидание ответа
    if (millis() - timeout > 1000) { //В слечае ожидания более 1 сек
      client.stop(); //закрытие сокета
	  return false; //возврат кода ошибки
    }
  }
  
  for(i = 0; i < (RBUF - 10); i++) //получение буфера
  {
	  replyb[i]=client.read();
	  if(replyb[i] == '\r' || replyb[i] == '\0' || replyb[i] == '\n') //В случае конца строки остановка
		  break;
  }
  replyb[i]='\0'; //Установка символа конца строки
  if(replyb[0] == 'O' && replyb[1] == 'K') { //В случае успеха
	return true;
  }
  else if(replyb[0] == '#') { //В случае получение удалённой команды игнорируем
	return true;
  }
  
  char tmp = replyb[7]; //Обрезка до 7 символа (сохранения)
  replyb[7]='\0';
  if(i > 8 && strcmp(replyb,"INTERVAL")){ //В случае ошибки по привышению частоты отправки, защитываем отправку как успешную для предотвращения повторной
    replyb[7]=tmp;
	return true;  
  }
  replyb[7]=tmp;	
  return false;
}

bool A1_data_pr(char *s, unsigned int s_size) { //Формирование пакета протокола локального обменаданными
  bzero(s, s_size);
  sprintf(s,
		  "EVC:%f", esp_vcc);
  if(data_rec == true) { 
  sprintf(s,
		  "%s MVC:%f"
		  " MTMP:%f", s, mc_vcc, mc_temp);
  if(bmp_ok == true) {
	sprintf(s,
			"%s BTMP:%f"
			" BPRE:%f", s, bmp_temp, bmp_pre);
  }
  if(dht_ok == true) {
	sprintf(s,
			"%s DTMP:%f"
			  " DHUM:%f", s, dht_temp, dht_hum);
  }
  if(lux_ok == true) {
	sprintf(s, "%s LUX:%f", s, lux);
  }
    
  if(rdy == 1) {
	sprintf(s,
			"%s VIN:%f"
			  " MQV:%f"
			  " MQV5:%f"
			  " MQ7:%f"
			  " MQ9:%f"
			  " MQ9_5:%f", s, vin, mqv, mqv5, mq1, mq2, mq2_5);
  }
  }
  sprintf(s, "%s ;\n\0", s);
  return 0;
}

void data_send_f() { //Установка флага отправки данных
	if(data_get == true && narodmon_send == true)
		send_data=true;
	return;
}

void k_tdp()	{ //Установка флага полученияданных
	if(data_get == true)
		drq=true;
	return;
}

bool sbufclean() { //Спец функция (костыль) для чистки буфера, т.к. flush почти некогда не работает
	int i = 0;
	Serial.flush(); //Скрещеваем пальцы и надеемся что на этот раз повезёт
	while(Serial.available() > 0) {//Считываем всё что есть в буфере(очищая его) в /dev/null
	  while (Serial.available() > 0) {
		  Serial.read();
	  }	
	  if(i > 6000)
		return false;
	  i++;
      delay(100);
	}
	return true;
}