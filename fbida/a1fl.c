#ifndef __A1_FUNCTION_LIB
#define __A1_FUNCTION_LIB
#define __A1_FLIB_VER 1.2

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>


/*
bool A1_data_pr(char *s, unsigned int s_size) {
  bzero(s, s_size);
  sprintf(s,
		  "EVC:%f", esp_vcc);
	if(idht_ok == true) {
	sprintf(s,
			"%s DTMP:%f"
			  " DHUM:%f", s, idht_temp, idht_hum);
	}
  sprintf(s, "%s ;\n\0", s);
  return 0;
}

bool NAROD_data_send(byte *bmac, char *str, short int size) {
  WiFiClient client;
  bzero(str, size);
  char mac[20];
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
  else if(nreplyb[0]== '#') {
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
*/
//Get signal level limits
#define rssi_min -85
#define rssi_max -40

double get_scsd(double *, unsigned int);
float get_scsf(float *, unsigned int);
int get_scsi(int *, unsigned int);
long get_scsl(long *, unsigned int);
/*
#ifndef bzero
void bzero(void *, size_t);
#endif 
*/

void bbzero(void **, size_t, size_t);

void fzero(float *, size_t);
void dzero(double *, size_t);
void izero(int *, size_t);
void lzero(long *, size_t);

void bfoll(char *, size_t, size_t, char);

long get_signal_qua(long, long, long);
bool tobool(const char *);

int splint_narod(char const *, int, int, char **, int *);
int splint_rtoa(char const *, int, int, char **, float *);


double get_scsd(double *mas, unsigned int rr) { //функция усреднения массивов double
  double res = 0;
  for (int i = 0; i < rr; i++) {
    res += mas[i];
  }
  return res/rr;
}

float get_scsf(float *mas, unsigned int rr) { //функция усреднения массивов float
  float res = 0;
  for (unsigned int i = 0; i < rr; i++) {
    res += mas[i];
  }
  return res/rr;
}

int get_scsi(int *mas, unsigned int rr) { //функция усреднения массивов
  int res = 0;
  for (unsigned int i = 0; i < rr; i++) {
    res += mas[i];
  }
  return res/rr;
}

long get_scsl(long *mas, unsigned int rr) { //функция усреднения массивов
  long res = 0;
  for (unsigned int i = 0; i < rr; i++) {
    res += mas[i];
  }
  return res/rr;
}
/*
#ifndef bzero
void bzero(void *mas, size_t bits){
	char *s =  (char*)mas;
    for(size_t u=0; u < bits; u++)
        s[u]='\0';
}
#endif
*/
void bbzero(void **mas, size_t bits, size_t mcol){
	char **s =  (char**)mas;
    for(size_t u=0; u < mcol; u++)
        bzero(s[u], bits);
}

void fzero(float *s, size_t n){
    for(size_t i=0; i < n; i++)
        s[i]=0;
}

void dzero(double *s, size_t n)
{
  for (size_t i = 0; i < n; i++)
    s[i] = 0;
}

void lzero(long *s, size_t n)
{
  for (size_t i = 0; i < n; i++)
    s[i] = 0;
}

void izero(int *s, size_t n)
{
  for (size_t i = 0; i < n; i++)
    s[i] = 0;
}

void bfoll(char *mas, size_t start, size_t bits, char sym){
    for(size_t u=start; u < bits; u++)
        mas[u]=sym;
}

long get_signal_qua(long rfrom, long rto, long rssi){
	if(rssi >= rssi_max) 
		rssi = rssi_max;
	else if(rssi <= rssi_min) 
		rssi = rssi_min;
	return (rssi - (rssi_max)) * (rto - rfrom) / (rssi_min - rssi_max) + rfrom;
}

bool tobool(const char *str){
	if(strcmp(str, "true") == 0)
		return true;
	else if(strcmp(str, "TRUE") == 0)
		return true;
	else if(strcmp(str, "1") == 0)
		return true;
	else
		return false;
	
}

int splint_rtoa(char const *rx, int rs, int rc, char **name_mas, float *dat_mas)
//rx входная строка, rs колличество символов в строке, rc количество параметров
{
	int i, r, mix = 0;
	char tmp[20];
	if(rs < strlen(rx)) {
	    rs=strlen(rx);}
	for (i = 0; i < rc; i++)
	{
		if (name_mas[i] != NULL)
		{
			for (r = mix; r < rs; r++)
			{
				if(r>=rs || rx[r] == ';' || rx[r] == '\0')
				{
					return i;
				}
				else if (rx[r] == ':')
				{
					name_mas[i][r-mix] = '\0';
					mix = r + 1;
					break;
				}
				else
				{
					name_mas[i][r - mix] = rx[r];
				}
			}
			for (int f=0; f<20; f++) {
			    tmp[f]='\0';}
			for (r = mix; r < rs; r++)
			{
				
				if(r>=rs || rx[r] == ';' || rx[r] == '\0')
				{
					return i;
				}
				else if (rx[r] == ' ' || r>=rs || rx[r] == ';' || rx[r] == '\0')
				{
					tmp[r - mix] = '\0';
					mix = r + 1;
					break;
				}
				else
				{
					tmp[r - mix] = rx[r];
				}
			}
			dat_mas[i] = atof(tmp);
			/*int tmi = 0;
			sscanf("%d.%d",tmp, data_mas[i], tmi);
			dat_mas[i]=dat_mas[i]+(tmi);*/
		}
	}
	return rc;
}

int splint_narod(char const *rx, int rs, int rc, char **name_mas, int *dat_mas){
//rx входная строка, rs колличество символов в строке, rc количество параметров
int i, r, mix = 0;
char tmp[20];
if(rs < strlen(rx)) {
    rs=strlen(rx);}
for (i = 0; i < rc; i++)
{
		for (r = mix; r < rs; r++)
		{
			if(r>=rs || rx[r] == '\0')
			{
				return i;
			}
			else if (rx[r] == '=')
			{
				name_mas[i][r-mix] = '\0';
				mix = r + 1;
				break;
			}
			else
			{
				name_mas[i][r - mix] = rx[r];
			}
		}
                for (int f=0; f<20; f++){
                tmp[f]='\0';}
		for (r = mix; r < rs; r++)
		{
			
			if(r>=rs || rx[r] == '\0')
			{
				return i;
			}
			else if (rx[r] == ',' || r>=rs || rx[r] == '\0')
			{
				tmp[r - mix] = '\0';
				mix = r + 1;
				break;
			}
			else
			{
				tmp[r - mix] = rx[r];
			}
		}
		dat_mas[i] = atoi(tmp);
}
return rc;
}
/*
bool parse_A1DSP(char* tempstr) {
    //rx входная строка, rs колличество символов в строке, rc количество параметров
	
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

	char **name_mas = (char **)malloc(st_col * sizeof(char *));
	for(i = 0; i < st_col; i++) {
		name_mas[i] = (char *)malloc(15 * sizeof(char));
	}
	
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
    
    //srlcd.setCursor(0,1);
    //srlcd.print(tempstr);
    int *dat_mas = (int *)malloc(st_col * sizeof(int));
	char **name_mas = (char **)malloc(st_col * sizeof(char *));
	for(i = 0; i < st_col; i++) {
		name_mas[i] = (char *)malloc(15 * sizeof(char));
	}
	
	for(int t = 0; t < (strlen(tempstr)-1); t++){
		tempstr[t]=tempstr[t+1];
	}
	//delay(1000);
	//srlcd.setCursor(0,1);
	//srlcd.print(tempstr);
	tempstr[strlen(tempstr)-1]='\0';
	splint_narod(tempstr, strlen(tempstr), col, name_mas, dat_mas);
	//return col;
	yield();
	if(col > 0) {
		data_rec=true;
		for(int ilp = 0; ilp < col; ilp++) {
			yield();
			//tempstr += String("\nName = ") + name_mas[ilp] + String(" data = ") + dat_mas[ilp];
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

*/
#endif