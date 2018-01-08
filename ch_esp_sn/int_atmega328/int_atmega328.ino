#include <Adafruit_BMP085.h>
#include <Adafruit_Sensor.h>
#include <BH1750.h>
#include <DHT.h>
#include "../a1fl.c"
#include <Wire.h>
#include <limits.h>


#define FW_VER "1.0.3"

//определяем пины
#define MOSI_PIN 11
#define MISO_PIN 12
#define SCK_PIN 13
#define SS_PIN 10
#define A_MQV A0
#define A_MQ1 A1
#define A_MQ2 A2
#define A_VIN A3
#define DHT21_PIN 5

//инициализируем обьекты
DHT dht(DHT21_PIN, DHT21); //Инициализация библиотеки датчика температуры влажности и поределение пина
BH1750 lightMeter(0x23); //Инициализация библиотеки датчика освещённасти с указанием адреса на шине
Adafruit_BMP085 bmp; //Инициализация библиотеки датчика давления

//определяем переменную для получаемого байта
//byte recievedByte;

double GetTemp(void); //функция считование температуры МК
long readVcc(void); //функция считование напряжения МК
//double GetCharge(double voltage); 

bool bmp_f = 1;
double vcc = 0, MQ_v1 = 0, MQ_v5, lux = 0, hum_dht = 0, temp_dht = 0;
double vin = 0, pre_bmp = 0, temp_bmp = 0, mq1 = 0, mq2 = 0, mq2_5 = 0, mctmp = 0;
long ltmp[7], rnd = 0, loop_i = 0, mqrnd=0, mq5rnd=0;
bool loop_redy = 0, mqmode = 1;
unsigned long millis_pre, millis_new;
double dtmp[6];
double rdtmp[13][10];
String tempstr;


void bzero(void *mas, size_t bits){
	char *s =  (char*)mas;
    for(size_t u=0; u < bits; u++)
        s[u]='\0';
}

void setup() {
  //обнуляем регистр управления SPI
  // SPCR = B00000000;
  //разрешаем работу SPI
  // SPCR = (1<<SPE);
  Serial.begin(9600);
  Wire.begin();
  lightMeter.begin(BH1750_CONTINUOUS_HIGH_RES_MODE_2);
  if (!bmp.begin()) {
    bmp_f = 0;
  }
  dht.begin();
  //определяем пины для работы с SPI
  pinMode(MOSI_PIN, INPUT);
  pinMode(MISO_PIN, OUTPUT);
  pinMode(SCK_PIN, INPUT);
  pinMode(SS_PIN, INPUT);
  pinMode(9, OUTPUT); // ENABLE
  pinMode(6, OUTPUT); // RELAY CTR (HIGH 1.17 LOW 5)
  digitalWrite(9, HIGH);
  digitalWrite(6, HIGH);
  lzero(ltmp,7);
  dzero(dtmp,6);
  for(int r=0;r<10;r++){
	  bzero(rdtmp[r],10);
  }
  millis_pre= millis();
  
  Serial.println("STARTOK:1;");
}

void loop() {
  ltmp[0] += readVcc();
  
  if((millis()-millis_pre)<0)
	  millis_pre = millis();
  
  if(mqmode == 0) { //V5
    ltmp[1] += analogRead(A_MQV);
    ltmp[6] += analogRead(A_MQ2); //MQ9 LPG
	mq5rnd++;
  }
  else if(mqmode == 1) { //V1.4
    ltmp[5] += analogRead(A_MQV);
    ltmp[2] += analogRead(A_MQ1); //MQ7 CO
    ltmp[3] += analogRead(A_MQ2); //MQ9 CO
	mqrnd ++;
  }
  
  
  if((millis()-millis_pre) > 60000 && mqmode == 0)
  {
    mqmode =! mqmode;
    millis_pre=millis();
    digitalWrite(6, mqmode);
  }
  else if((millis()-millis_pre) > 90000 && mqmode == 1)
  {
    mqmode = !mqmode;
    millis_pre=millis();
    digitalWrite(6, mqmode);
  }
  
  ltmp[4] += analogRead(A_VIN);
  dtmp[0] += GetTemp();
  dtmp[1] += lightMeter.readLightLevel();
  dtmp[2] += dht.readHumidity();
  dtmp[3] += dht.readTemperature();
  
  if (bmp_f == 1) {
    dtmp[4] += bmp.readTemperature();
    dtmp[5] += bmp.readPressure() / 133.322;
  }
  
  delay(250);
  if (rnd >= 100) {
	rnd = rnd+1;
    if(mq5rnd >= 20) {
		rdtmp[1][loop_i] = ((ltmp[1] / mq5rnd) / 1023.0) * vcc; // MQVCC5
		rdtmp[11][loop_i] = ((ltmp[6] / mq5rnd) / 102.30) * vcc; // MQ2 V5
		ltmp[1]=0;
		ltmp[6]=0;
		mq5rnd=0;
    }
    if( mqrnd >= 20){
		rdtmp[12][loop_i] = ((ltmp[5] / mqrnd) / 1023.0) * vcc; // MQVCC
		rdtmp[2][loop_i]  = ((ltmp[2] / mqrnd) / 102.30) * vcc; // MQ1
		rdtmp[3][loop_i]  = ((ltmp[3] / mqrnd) / 1023.0) * vcc; // MQ2
		ltmp[2] = 0;
		ltmp[3] = 0;
		ltmp[5] = 0;
		mqrnd=0;
    }
    rdtmp[0][loop_i] = (ltmp[0] / rnd) / 1000.0;                        // MCVCC
    rdtmp[4][loop_i] = ((ltmp[4] / rnd) / 1023.0) * rdtmp[0][loop_i] * 10.5; // VIN
    rdtmp[5][loop_i] = (dtmp[0] / rnd);                       // MCTMP
    rdtmp[6][loop_i] = (dtmp[1] / rnd);                       // LUX
    rdtmp[7][loop_i] = (dtmp[2] / rnd);                       // DHUM
    rdtmp[8][loop_i] = (dtmp[3] / rnd);                       // DTMP
    if (bmp_f == 1) {
      rdtmp[9][loop_i] = dtmp[4] / rnd;  // BTMP
      rdtmp[10][loop_i] = dtmp[5] / rnd; // BPRE
    }
    ltmp[0] = 0;
    ltmp[4] = 0;
    dtmp[0] = 0;
    dtmp[1] = 0;
    dtmp[2] = 0;
    dtmp[3] = 0;
    dtmp[4] = 0;
    dtmp[5] = 0;
    rnd = 0;
    loop_i++;
    if (loop_i >= 10) {
      loop_redy = 1;
      loop_i = 0;
    }
    if(loop_redy==1){
    vcc = get_scsd(rdtmp[0], 10);}
  }
  else {
  rnd++;
  }
  
  if (Serial.available() > 0) {
	char ctmp = Serial.read();
    if (ctmp == '\n') {
      if (loop_redy == 1) {
        vcc = get_scsd(rdtmp[0], 10);
        MQ_v1 = get_scsd(rdtmp[12], 10);
        MQ_v5 = get_scsd(rdtmp[1], 10);
        mq1 = get_scsd(rdtmp[2], 10);
        mq2 = get_scsd(rdtmp[3], 10);
        mq2_5 = get_scsd(rdtmp[11], 10);
        vin = get_scsd(rdtmp[4], 10);
        mctmp = get_scsd(rdtmp[5], 10);
        temp_dht = get_scsd(rdtmp[8], 10);
        hum_dht = get_scsd(rdtmp[7], 10);
        lux = get_scsd(rdtmp[6], 10);
		if (bmp_f == 1) {
			temp_bmp = get_scsd(rdtmp[9], 10);
			pre_bmp = get_scsd(rdtmp[10], 10);
		}
		
        tempstr = String("MQV:") + MQ_v1 + String(" MQV5:") + MQ_v5;
		tempstr += String(" VMQ1:") + mq1 + String(" VMQ2:") + mq2; //CO
        tempstr += String(" VMQ25:") + mq2_5; //LPG
        Serial.print(tempstr);

        tempstr = String(" VIN:") + vin + String(" MCVCC:") + vcc + String(" MCTMP:") + mctmp;
        tempstr += String(" LUX:") + lux;

		if(hum_dht > 99.2) {
			hum_dht=100;
		}
        tempstr += String(" DHUM:") + hum_dht + String(" DTMP:") + temp_dht;
        Serial.print(tempstr);

        if (bmp_f == 1) {
          tempstr = String(" BPRE:") + pre_bmp + String(" BTMP:") + temp_bmp;
          Serial.print(tempstr);
        }
        tempstr += String(" RDY:1 DVG:1;\n");
        Serial.println(tempstr);
       } 
	else {
	 tempstr = String("MQV:") + ltmp[1]/mqrnd  + String(" VMQ1:") + ltmp[2]/mqrnd  + String(" VMQ2:") + ltmp[3]/mqrnd;
         Serial.print(tempstr);
 
         tempstr = String(" VIN:") + ltmp[4]/rnd  + String(" MCVCC:") + (ltmp[0]/rnd)/1000.0  + String(" MCTMP:") + dtmp[0]/rnd;
         Serial.print(tempstr);
         
         tempstr = String(" LUX:") + dtmp[1]/rnd;
         tempstr += String(" DHUM:") + dtmp[2]/rnd + String(" DTMP:") + dtmp[3]/rnd;
         Serial.print(tempstr);

        if (bmp_f == 1) {
           tempstr = String(" BPRE:") + dtmp[5]/rnd + String(" BTMP:") + dtmp[4]/rnd;
           Serial.print(tempstr);
         }
         tempstr += String(" RDY:0 DVG:0;\n");
         Serial.println(tempstr);
       }
    }
	
	else if (ctmp == 'D') {
	  Serial.print("FW VER: ");
	  Serial.println(FW_VER);
      Serial.println("Debug:");
		for(int i=0;i<13;i++){
		for(int j=0;j<10;j++){
			Serial.print(" rdtmp[");
			Serial.print(i);
			Serial.print("][");
			Serial.print(j);
			Serial.print("]=");
			Serial.println(rdtmp[i][j]);
		}
			Serial.println("");
		}
		for(int i=0;i<6;i++){
			Serial.print(" dtmp[");
			Serial.print(i);
			Serial.print("]=");
			Serial.println(dtmp[i]);
		}
		for(int i=0;i<7;i++){
			Serial.print(" ltmp[");
			Serial.print(i);
			Serial.print("]=");
			Serial.println(ltmp[i]);
		}
      tempstr = String("rnd: ") + (int)rnd + String("\n") + String("loop_i: ") + (int)loop_i + String("\n");
	  Serial.println(tempstr);
	}
	else if (ctmp == 'S') {
      byte error, address;
      int nDevices;
	  Serial.print("FW VER: ");
	  Serial.println(FW_VER);
	  Serial.println("Scanning...");
      nDevices = 0;
      for (address = 1; address < 127; address++) {
        // The i2c_scanner uses the return value of
        // the Write.endTransmisstion to see if
        // a device did acknowledge to the address.
        Wire.beginTransmission(address);
        error = Wire.endTransmission();

        if (error == 0) {
          Serial.print("I2C at 0x");
          if (address < 16)
            Serial.print("0");
          Serial.print(address, HEX);
          Serial.println("  !");

          nDevices++;
        } else if (error == 4) {
          Serial.print("U error at 0x");
          if (address < 16)
            Serial.print("0");
          Serial.println(address, HEX);
        }
      }
      if (nDevices == 0)
        Serial.println("No I2C devices found\n");
      else
        Serial.println("done\n");
    }
  }
}

long readVcc() {
// Чтение внутреннего опорного источника напряжения 1.1В 
// установка его в качестве опорного для замера питающего напряжения
#if defined(__AVR_ATmega32U4__) || defined(__AVR_ATmega1280__) ||              \
    defined(__AVR_ATmega2560__)
  ADMUX = _BV(REFS0) | _BV(MUX4) | _BV(MUX3) | _BV(MUX2) | _BV(MUX1);
#elif defined(__AVR_ATtiny24__) || defined(__AVR_ATtiny44__) ||                \
    defined(__AVR_ATtiny84__)
  ADMUX = _BV(MUX5) | _BV(MUX0);
#else
  ADMUX = _BV(REFS0) | _BV(MUX3) | _BV(MUX2) | _BV(MUX1);
#endif

  delay(2);            // Ждём пока напряжение стабилизируется
  ADCSRA |= _BV(ADSC); // Запускаем преобразование
  while (bit_is_set(ADCSRA, ADSC)); // замер по готовности

  //uint8_t low = ADCL;  // считываем нижные биты
  //uint8_t high = ADCH; // верхнии биты

  //long result = (high << 8) | low; 
  
  long result = ADCW; //считываем данные

  result = 1125300L / result; // Считываем напряжение питания (в мили вольтах); 1125300 = 1.1*1023*1000
  return result-307;              // Возвращаем напряжение из функции
}


double GetTemp(void) {
  unsigned int wADC;
  double t;
  /*
  Внутренний температурный датчик 
  работает на основе источника опорного напр 1.1В
  И так как 8 канал АЦП не может быть считан с использованием 
  функций среды ардуино, ручками устанавливаем регистры
  */
  
  //Настройка АДЦ и включение опорнонго источника
  ADMUX = (_BV(REFS1) | _BV(REFS0) | _BV(MUX3));
  ADCSRA |= _BV(ADEN); // включение АЦП

  delay(20); // ожидание стабилизации напряжения

  ADCSRA |= _BV(ADSC); // Запуск конверсии

  // Ожидание завершения
  while (bit_is_set(ADCSRA, ADSC));

  // Считываем данные
  wADC = ADCW;

  // Выполняем вычесления температуры, костанта 324,31 должна подстраиватся под конкретный мк
  t = (wADC - 324.31) / 1.22;

  // Взвращаем результат в градусах цельсия
  return (t+9);
}
