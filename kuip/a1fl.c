/*Для устраннения возможной колизии,
 проверяем не подключенна ли библиотека*/
#ifndef __A1_FUNCTION_LIB
#define __A1_FUNCTION_LIB
#define __A1_FLIB_VER 1.2

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>


//Лимиты для максимального и минимального уровня сигнала 
#define rssi_min -90
#define rssi_max -20

/*Функции усреднения массивов
 для различных типов*/
double get_scsd(double *, unsigned int);
float get_scsf(float *, unsigned int);
int get_scsi(int *, unsigned int);
long get_scsl(long *, unsigned int);

#ifndef ESP_CH
void bzero(void *, size_t);
#endif 

//Заполнение нулями 2-у мерного массива
void bbzero(void **, size_t, size_t);

/*Функции для обнуления массивов
 различных типов*/
void fzero(float *, size_t);
void dzero(double *, size_t);
void izero(int *, size_t);
void lzero(long *, size_t);

/*Функция заполнения массива указанным символом*/
void bfoll(char *, size_t, size_t, char);


long get_signal_qua(long, long, long);
/*Функция преобразование строки
 в байтовою переменную, 
 принимает на вход цифренном виде 1/0 
 и в строковом true/false
 в случае невозможности преобразования
 возвращает false*/
bool tobool(const char *);

/*Функция разбора протокола ответа народного мониторинга*/
int splint_narod(char const *, int, int, char **, int *); 
/*Возвращают данные в виде двух парных массивов имя - значение
  в массивы указанные в аргументах*/
/*Функция разбора протокола для внутреннего обмена*/
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

int get_scsi(int *mas, unsigned int rr) { //функция усреднения массивов int
  int res = 0;
  for (unsigned int i = 0; i < rr; i++) {
    res += mas[i];
  }
  return res/rr;
}

long get_scsl(long *mas, unsigned int rr) { //функция усреднения массивов long
  long res = 0;
  for (unsigned int i = 0; i < rr; i++) {
    res += mas[i];
  }
  return res/rr;
}

#ifndef ESP_CH
void bzero(void *mas, size_t bits){
	char *s =  (char*)mas;
    for(size_t u=0; u < bits; u++)
        s[u]='\0';
}
#endif

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
#endif