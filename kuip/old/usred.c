#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

struct usred {
  double **data;		//Указатель на массив текущих данных
  double *data_usred;		//Указатель для массива усреднённых данных
  //double *data_accuracy;        //Указатель на массив допустимых погрешностей
  unsigned long data_usred_col;	//Количество переменных для усреднения
  unsigned long data_col;	//Количество переменных для текущих данных
  unsigned long data_index;	//Индекс массива текущих данных
  int data_redy;                //Флаг готовности усреднённых данных
  //unsigned seconds_to_usred=0; //Временной интервал для усреднения
  //unsigned old_sec = 0;
};


int init_usred (struct usred *sred, unsigned long usred_ciclas,
        unsigned long args) { //Функция выделения массивов под данные и результаты усреднения
  unsigned long tmp_ind = 0;
  //if(sred->data_usred != NULL && sred->data != NULL)	{ //Проверка существования структуры
    //fprintf(stderr, "[usred_init] Структура уже инициализированна.\n[usred_init] Повторная инецилизация не возможна!\n");
    //return 2;
  //}
  sred->data_usred_col = args; 
  sred->data_col = usred_ciclas;
  sred->data_index = 0; //Обнуление индекса
  sred->data_redy = 0; //Установление флага готовности в false
  sred->data_usred = malloc ((sred->data_usred_col + 1) * sizeof (double)); //Выделение памяти под усреднённые данные
  sred->data = malloc ((sred->data_col + 1) * sizeof (double *)); //Выделения массива указателей на массивы данных для усреднения, 1 изм
  if(sred->data == NULL || sred->data_usred == NULL) {
    fprintf(stderr, "[usred_init] Ошибка выделения памяти!");
    return 1;
  }
  unsigned long i, j;
  for (i = 0; i < (sred->data_col); i++) {
    sred->data[i] = malloc ((sred->data_usred_col + 1) * sizeof (double)); //Выделение памяти для массива данных для усред, 2 изм
	if(sred->data[i] == NULL) {
	    fprintf(stderr, "[usred_init] Ошибка выделения памяти!");
	    return 1;
	}
  }
  for (i = 0; i < (sred->data_usred_col); i++) { 
    for (j = 0; j < (sred->data_col); j++) {
     sred->data[j][i] = 0; //Обнуление массива данных для усреднения (для нормальной работы до полного заполнения)
    }
   sred->data_usred[i] = 0; //Обнуления массива для средних(чисто для приличия)
  }
  return 0;
}

int write_usred (struct usred *sred, int args, ...) //Функция внесения данных и возврата усреднённых на их место
{
  unsigned long i, tmp_col = 0;
  if(sred->data_usred == NULL) {
    fprintf(stderr,"[usred_write] Ошибка структура усреднения не инециализированна\n");
    return 3;
  }
  if (args > sred->data_usred_col) //Выход при превышение количества переменных для входа
    return 1;
  va_list ap; //Создание списка арг

  va_start (ap, args); //Начало разбора списка аргументов
  /*TODO:
    Сделать авто reloc при увеличение входных данных
    Сделать распознавание количества аргументов(через NULL ptr?..)*/
  for (i = 0; i < args; i++) {
      double *data_ptr = va_arg (ap, double *); //Выделение временного указателя
      if(*data_ptr != 0)
	sred->data[sred->data_index][i] = *data_ptr; //Запись данных в массив для усреднения
      if (sred->data_redy == 1 && *data_ptr != 0) //В случае готовности усреднённых данных
        *data_ptr = sred->data_usred[i]; //Вывод их на место входных
    }
  va_end (ap); //Окончание разбора аргументов

  unsigned long j;
  double tmp;
  for (i = 0; i < (sred->data_usred_col); i++) { //Проход алгоритмом усреднения по массиву i номер аргумента
      tmp = 0;
      for (j = 0; j < (sred->data_col); j++) { //Проход по подмассиву входных данных j индекс входных данных
       tmp += sred->data[j][i];
      }
      sred->data_usred[i] = tmp / sred->data_col; //Подсчёт среднего для i аргумента
    }
  sred->data_index++; //Увеличение индекса
  if ((sred->data_index + 1) > sred->data_col) { //В случае обнаружения окончания массива
      sred->data_index = 0; //Возврат индекса к 0 элементу для обеспечения цикличности
      sred->data_redy = 1; //Установления флага готовности данных в true
    }
}

void free_usred (struct usred *sred) { //Функция очистки памяти от структуры усреднения
  if(sred->data_usred == NULL) {
    fprintf(stderr, "[usred_free] Структура уже очищена!\n");
    return;}
  free (sred->data_usred); //Очистка массива усреднённых данных
  unsigned long i;
  for (i = 0; i < (sred->data_col); i++) { //Чистка массива усредняемых данных:
      free (sred->data[i]); //чистка массива аргументов
    }
  free (sred->data); //чистка массива указателей
  sred->data_usred = NULL;
  sred->data = NULL;
  sred->data_index = 0;
}