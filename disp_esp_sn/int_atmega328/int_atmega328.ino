#include <Adafruit_BMP085.h>
#include <Adafruit_Sensor.h>
#include <BH1750.h>
#include <DHT.h>
#include <Wire.h>

#define FW_VER 1.0.1

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
DHT dht(DHT21_PIN, DHT21);
BH1750 lightMeter(0x23);
Adafruit_BMP085 bmp;

//определяем переменную для получаемого байта
//byte recievedByte;

double GetTemp(void); //функция считование температуры МК
long readVcc(void); //функция считование напряжения МК
double GetCharge(double voltage);

double get_scs(double *mas, int rr) { //функция усреднения массивов
  double res = 0;
  for (int i = 0; i < rr; i++) {
    res += mas[i];
  }
  return res/rr;
}

bool bmp_f = 1;
double vcc = 0, MQ_vcc = 0, lux = 0, hum_dht = 0, temp_dht = 0, vin = 0, pre_bmp = 0, temp_bmp = 0, mq1 = 0, mq2 = 0, mctmp = 0, data_ok=0;
long ltmp[5], rnd = 0, loop_i = 0;
bool loop_redy = 0;
double dtmp[7];
double rdtmp[12][10];
String tempstr;

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

  ltmp[0] = 0;
  ltmp[1] = 0;
  ltmp[2] = 0;
  ltmp[3] = 0;
  ltmp[4] = 0;
  dtmp[0] = 0;
  dtmp[1] = 0;
  dtmp[2] = 0;
  dtmp[3] = 0;
  dtmp[4] = 0;
  dtmp[5] = 0;
  dtmp[6] = 0;
  
  
  Serial.println("STARTOK:1;");
}

void loop() {
  ltmp[0] += readVcc();
  ltmp[1] += analogRead(A_MQV);
  ltmp[2] += analogRead(A_MQ1);
  ltmp[3] += analogRead(A_MQ2);
  ltmp[4] += analogRead(A_VIN);
  dtmp[0] += GetTemp();
  dtmp[1] += lightMeter.readLightLevel();
  dtmp[2] += dht.readHumidity();
  dtmp[3] += dht.readTemperature();
  if (bmp_f == 1) {
    dtmp[4] += bmp.readTemperature();
    dtmp[5] += bmp.readPressure() / 133.322;
  }
  dtmp[6] += 100;
  
  delay(250);
  if (rnd >= 100) {
	  rnd = rnd+1;
    rdtmp[0][loop_i] = (ltmp[0] / rnd) / 1000.0;                        // MCVCC
    rdtmp[1][loop_i] = ((ltmp[1] / rnd) / 1023.0) * rdtmp[0][loop_i]; // MQVCC
    rdtmp[2][loop_i] = ((ltmp[2] / rnd) / 1023.0) * rdtmp[0][loop_i]; // MQ1
    rdtmp[3][loop_i] = ((ltmp[3] / rnd) / 1023.0) * rdtmp[0][loop_i]; // MQ2
    rdtmp[4][loop_i] = ((ltmp[4] / rnd) / 1023.0) * rdtmp[0][loop_i] * 10.2; // VIN
    rdtmp[5][loop_i] = (dtmp[0] / rnd);                       // MCTMP
    rdtmp[6][loop_i] = (dtmp[1] / rnd);                       // LUX
    rdtmp[7][loop_i] = (dtmp[2] / rnd);                       // DHUM
    rdtmp[8][loop_i] = (dtmp[3] / rnd);                       // DTMP
    if (bmp_f == 1) {
      rdtmp[9][loop_i] = dtmp[4] / rnd;  // BTMP
      rdtmp[10][loop_i] = dtmp[5] / rnd; // BPRE
    }  
	rdtmp[11][loop_i] = dtmp[6] / rnd; // BPRE
    ltmp[0] = 0;
    ltmp[1] = 0;
    ltmp[2] = 0;
    ltmp[3] = 0;
    ltmp[4] = 0;
    dtmp[0] = 0;
    dtmp[1] = 0;
    dtmp[2] = 0;
    dtmp[3] = 0;
    dtmp[4] = 0;
    dtmp[5] = 0;
    dtmp[6] = 0;
    rnd = 0;
    loop_i++;
    if (loop_i >= 10) {
      loop_redy = 1;
      loop_i = 0;
    }
  }
  else {
  rnd++;
  }
  
  if (Serial.available() > 0) {
	char ctmp = Serial.read();
    if (ctmp == '\n') {
      if (loop_redy == 1) {
        vcc = get_scs(rdtmp[0], 10);
        MQ_vcc = get_scs(rdtmp[1], 10);
        mq1 = get_scs(rdtmp[2], 10);
        mq2 = get_scs(rdtmp[3], 10);
        vin = get_scs(rdtmp[4], 10);
        mctmp = get_scs(rdtmp[5], 10);
		data_ok = get_scs(rdtmp[11], 10);
		
        tempstr = String("MQV:") + MQ_vcc + String(" VMQ1:") + mq1 + String(" VMQ2:") + mq2;
        Serial.print(tempstr);

        tempstr = String(" VIN:") + vin + String(" MCVCC:") + vcc + String(" MCTMP:") + mctmp;
        Serial.print(tempstr);

        lux = get_scs(rdtmp[6], 10);
        tempstr = String(" LUX:") + lux;

        hum_dht = get_scs(rdtmp[7], 10);
		if(hum_dht > 100) {
			hum_dht=100;
		}
        temp_dht = get_scs(rdtmp[8], 10);
        tempstr += String(" DHUM:") + hum_dht + String(" DTMP:") + temp_dht;
        Serial.print(tempstr);

        if (bmp_f == 1) {
          temp_bmp = get_scs(rdtmp[9], 10);
          pre_bmp = get_scs(rdtmp[10], 10);
          tempstr = String(" BPRE:") + pre_bmp + String(" BTMP:") + temp_bmp;
          Serial.print(tempstr);
        }
		tempstr = String(" DQ:") + data_ok;
        tempstr += String(" RDY:1;\n");
        Serial.println(tempstr);
       } 
	else {
	 tempstr = String("MQV:") + ltmp[1]/rnd  + String(" VMQ1:") + ltmp[2]/rnd  + String(" VMQ2:") + ltmp[3]/rnd;
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
		 tempstr = String(" DQ:") + dtmp[6]/rnd;
         tempstr += String(" RDY:0;\n");
         Serial.println(tempstr);
         //Serial.println("RDY:0;");
       }
    }
	
	else if (ctmp == 'D') {
      Serial.println("Debug:");
		for(int i=0;i<12;i++){
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
		for(int i=0;i<7;i++){
			Serial.print(" dtmp[");
			Serial.print(i);
			Serial.print("]=");
			Serial.println(dtmp[i]);
		}
		for(int i=0;i<4;i++){
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
      Serial.println("\nStatus:");
      tempstr = String("MCVCC: ") + ltmp[0]/rnd + String("\n") + String("MQVCC: ") + ltmp[1]/rnd + String("\n");
      tempstr += String("MQ1: ") + ltmp[2]/rnd + String("\n") + String("MQ2: ") + ltmp[3]/rnd + String("\n");
      tempstr += String("VIN: ") + ltmp[4]/rnd + String("\n") + String("MCTMP: ") + dtmp[0]/rnd + String("\n");
      tempstr += String("LUX: ") + dtmp[1]/rnd + String("\n") + String("DHT_HUM: ") + dtmp[2]/rnd + String("\n");
      tempstr += String("DHT_TMP: ") + dtmp[3]/rnd + String("\n") + String("BMP_TMP: ") + dtmp[4]/rnd + String("\n");
      tempstr += String("BMP_PRE: ") + dtmp[5]/rnd + String("\n");
      tempstr += String("rund: ") + (int)rnd + String("\n") + String("loop_i: ") + (int)loop_i + String("\n");
	  ltmp[0] = 0;
	  ltmp[1] = 0;
	  ltmp[2] = 0;
	  ltmp[3] = 0;
	  ltmp[4] = 0;
	  dtmp[0] = 0;
	  dtmp[1] = 0;
	  dtmp[2] = 0;
	  dtmp[3] = 0;
	  dtmp[4] = 0;
	  dtmp[5] = 0;
	  rnd = 0;
	  Serial.println(tempstr);
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

/*
void loop() {
  //пока пин slave select опущен
  while (digitalRead(SS_PIN)==LOW){
    //принимаем байт и записываем его в переменную
    recievedByte=spi_receive();
    //смотрим в мониторе полученный байт
    Serial.println(recievedByte,BIN);
    //зажигаем светодиоды, которые соответствуют единицам в полученном байте
    digitalWrite(2,recievedByte & (1 << 0));
    digitalWrite(3,recievedByte & (1 << 1));
    digitalWrite(4,recievedByte & (1 << 2));
    digitalWrite(5,recievedByte & (1 << 3));
    digitalWrite(6,recievedByte & (1 << 4));
    digitalWrite(7,recievedByte & (1 << 5));
    digitalWrite(8,recievedByte & (1 << 6));
    digitalWrite(9,recievedByte & (1 << 7));
  }
}
*/
//функция для приема байта
byte spi_receive() {
  //пока не выставлен флаг окончания передачи, принимаем биты
  while (!(SPSR & (1 << SPIF))) {
  };
  //позвращяем содержимое регистра данных SPI
  return SPDR;
}

long readVcc() {
// Read 1.1V reference against AVcc
// set the reference to Vcc and the measurement to the internal 1.1V reference
#if defined(__AVR_ATmega32U4__) || defined(__AVR_ATmega1280__) ||              \
    defined(__AVR_ATmega2560__)
  ADMUX = _BV(REFS0) | _BV(MUX4) | _BV(MUX3) | _BV(MUX2) | _BV(MUX1);
#elif defined(__AVR_ATtiny24__) || defined(__AVR_ATtiny44__) ||                \
    defined(__AVR_ATtiny84__)
  ADMUX = _BV(MUX5) | _BV(MUX0);
#else
  ADMUX = _BV(REFS0) | _BV(MUX3) | _BV(MUX2) | _BV(MUX1);
#endif

  delay(2);            // Wait for Vref to settle
  ADCSRA |= _BV(ADSC); // Start conversion
  while (bit_is_set(ADCSRA, ADSC))
    ; // measuring

  uint8_t low = ADCL;  // must read ADCL first - it then locks ADCH
  uint8_t high = ADCH; // unlocks both

  long result = (high << 8) | low;

  result = 1125300L / result; // Calculate Vcc (in mV); 1125300 = 1.1*1023*1000
  return result;              // Vcc in millivolts
}

double GetCharge(double voltage) {
  double charge = 0;
  if (voltage > 13.2)
    return 101;
  if (voltage > 11) {
    voltage = voltage - 11;
    charge = (voltage / 1.2) * 100;
  }
  return charge;
}

double GetTemp(void) {
  unsigned int wADC;
  double t;

  // The internal temperature has to be used
  // with the internal reference of 1.1V.
  // Channel 8 can not be selected with
  // the analogRead function yet.

  // Set the internal reference and mux.
  ADMUX = (_BV(REFS1) | _BV(REFS0) | _BV(MUX3));
  ADCSRA |= _BV(ADEN); // enable the ADC

  delay(20); // wait for voltages to become stable.

  ADCSRA |= _BV(ADSC); // Start the ADC

  // Detect end-of-conversion
  while (bit_is_set(ADCSRA, ADSC))
    ;

  // Reading register "ADCW" takes care of how to read ADCL and ADCH.
  wADC = ADCW;

  // The offset of 324.31 could be wrong. It is just an indication.
  t = (wADC - 324.31) / 1.22;

  // The returned temperature is in degrees Celsius.
  return (t);
}
