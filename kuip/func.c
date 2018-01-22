#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <iniparser.h>
#include <termios.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/time.h>
#include <dirent.h>
#include "def.h"
#include <sys/stat.h>

pthread_mutex_t mutex_log = PTHREAD_MUTEX_INITIALIZER;


float
	gettemp (char *sn)		//получение данных от датчика
{
	FILE *F;
	float tempc;
	int g = 0, i = 0;
	char buf, temp[6], path[50];
	bzero (temp, sizeof (buf));
	//bzero(path, 50);
	sprintf (path, "%s%s/w1_slave", TERM_DIR, sn);
	F = fopen (path, "rt");
	if (F == NULL)
	{
		//printf("Error!\nCold not open device\n");
		return 6513;
	}
	buf = fgetc (F);
	while (!feof (F))
	{
		if (buf == 'N')
		{
			//printf("\nCRC Error!!!\n");
			fclose (F);
			return 6514;
		}
		if (buf == 'Y')
		{
			while (!feof (F))
			{
				buf = fgetc (F);
				if (buf == 't')
				{
					buf = fgetc (F);
					if (buf == '=')
					{
						while (!feof (F))
						{
							temp[i] = fgetc (F);
							i++;
						}
						tempc = atof (temp);
						tempc = tempc / 1000;
						//printf("temp(char): %s \ntemp(float): %f\n",temp,tempc);
						fclose (F);
						return tempc;
					}
				}
			}
			fclose (F);
		}
		//printf("%c",buf);
		buf = fgetc (F);
	}
	fclose (F);
	return 6515;
}

void	clears (void)
{
	printf ("\033[2J");
	printf ("\033[0;0f");
}

int	iniparser_save (dictionary * inicon, char *ini_file_name)
{
	FILE *ini_file;
	ini_file = fopen (ini_file_name, "w+");
	if (ini_file == NULL)
	{
		printf ("\nCon't write ini file:%s", ini_file_name);
		perror (" \n");
		return 1;
	}
	iniparser_dump_ini (inicon, ini_file);
	fclose (ini_file);
	return 0;
}

void	ptolog (_Bool debug, FILE * file, const char *mtl, ...)
{
	pthread_mutex_lock (&mutex_log);
	char fp[100], timec[100], tmp[100];
	time_t t = time (NULL);
	struct tm *tm = localtime (&t);

	sprintf (timec, "\0");

	if (tm->tm_mday < 10)
		sprintf (tmp, "[0%d.", tm->tm_mday);
	else
		sprintf (tmp, "[%d_", tm->tm_mday);
	strcat (timec, tmp);

	if (tm->tm_mon + 1 < 10)
		sprintf (tmp, "0%d.", tm->tm_mon + 1);
	else
		sprintf (tmp, "%d_", tm->tm_mon + 1);
	strcat (timec, tmp);

	sprintf (tmp, "%d ", tm->tm_year + 1900);
	strcat (timec, tmp);

	if (tm->tm_hour < 10)
		sprintf (tmp, "0%d:", tm->tm_hour);
	else
		sprintf (tmp, "%d:", tm->tm_hour);
	strcat (timec, tmp);


	if (tm->tm_min < 10)
		sprintf (tmp, "0%d:", tm->tm_min);
	else
		sprintf (tmp, "%d:", tm->tm_min);
	strcat (timec, tmp);


	if (tm->tm_sec < 10)
		sprintf (tmp, "0%d]", tm->tm_sec);
	else
		sprintf (tmp, "%d]", tm->tm_sec);
	strcat (timec, tmp);


	va_list arglist;
	va_start (arglist, mtl);
	vsprintf (tmp, mtl, arglist);
	va_end (arglist);
	if (file == NULL)
	{
		debug = 1;
		perror ("\nLog error:");
		pthread_mutex_unlock (&mutex_log);
		return;
	}
	if (debug == 1)
		printf ("%s %s", timec, tmp);
	fprintf (file, "%s %s", timec, tmp);
	fflush (file);
	pthread_mutex_unlock (&mutex_log);
	return;
}

int	write_txt_file (char *path, char *file_name, const char *input, ...)
{
	FILE *file;
	char fp[100], text[strlen (input)];
	sprintf (fp, "%s/%s", path, file_name);
	va_list arglist;
	va_start (arglist, input);
	vsprintf (text, input, arglist);
	va_end (arglist);
	file = fopen (fp, "w+");
	if (file == NULL)
	{
		printf ("\nCon't write text file:%s", file_name);
		perror (" \n");
		return 1;
	}
	fprintf (file, "%s", text);
	fclose (file);
	return 0;
}

int	getch ()
{
	struct termios oldt, newt;
	int ch;
	tcgetattr (STDIN_FILENO, &oldt);
	newt = oldt;
	newt.c_lflag &= ~(ICANON | ECHO);
	tcsetattr (STDIN_FILENO, TCSANOW, &newt);
	ch = getchar ();
	tcsetattr (STDIN_FILENO, TCSANOW, &oldt);
	return ch;
}

int	cre_conf (char *dir, char *name)
{
	DIR *sens;
	struct dirent *sen_n;
	dictionary *conf_d;
	char db[100], txt_dir[100], com1[30], uny[12], col[3], confnd[256],
		device_name[17], device_place[17];
	int i = 0, g = 0, v = 0, col2 = 0, type = 0, st = 2, tmp = 0, input[512];

	float temp;
	bzero (db, 100);
	bzero (txt_dir, 100);
	bzero (col, 3);
	sprintf (confnd, "%s/%s", dir, name);
	if (write_txt_file (dir, name, "[main]") == 1)
		return 1;
	conf_d = iniparser_load (confnd);
	iniparser_set (conf_d, "main", NULL);
	clears ();
	printf
		("Welcome to config setup ver: %s\nВведите название базы данных:",
		ver);
	scanf ("%s", db);
	iniparser_set (conf_d, "main:db_name", db);
	clears ();
	iniparser_set (conf_d, "t_sensors", NULL);
	iniparser_set (conf_d, "dev", NULL);
	int in, onewire = 0, uart = 0, sta = 0;
	char set[3][3], x[3] = { ' ', ' ', ' ' };
	for (sta = 0; sta < 4;)
	{
		if (sta > 4)
			break;
		if (sta > 0 && in == 'w')
			sta = sta - 1;
		if (sta < 2 && in == 's')
			sta = sta + 1;
		for (i = 0; i < 4; i++)
		{
			if (i == sta)
			{
				set[i][0] = '-';
				set[i][1] = '>';
				set[i][2] = '\0';
				if (in == ' ')
				{
					if (sta == 2)
						sta = 5;
					else if (x[i] == ' ')
						x[i] = 'x';
					else if (x[i] == 'x')
						x[i] = ' ';
					else
						x[i] = ' ';
				}
			}
			if (i != sta)
			{
				set[i][0] = ' ';
				set[i][1] = ' ';
				set[i][2] = '\0';
			}
		}
		clears ();
		printf
			("Выбирити типы двтчиков для настройки:\n");
		printf ("%s [%c]1-Wire\n", set[0], x[0]);
		printf ("%s [%c]Arduino UART()\n", set[1], x[1]);
		printf ("%sГотово\n", set[2]);
		in = getch ();
	}
	if (x[0] == 'x')
		onewire = 1;
	if (x[1] == 'x')
		uart = 1;
	if (x[1] != 'x' && x[0] != 'x')
	{
		printf("Не один из источников не был выбран!\nВыход...\n");
		exit (26);
	}
	i = 0;

	iniparser_set (conf_d, "main:onewire", onewire);
	iniparser_set (conf_d, "main:uart", uart);

	char **sc_sn, **sc_name, **sc_place;	//Обьявление указателей для последующего выделения памяти
	int uart0_filestream;
	int *sc_type;
	unsigned char rx[256];

	if (uart == 1)
	{
		v=readDA(uart0_filestream, rx);

	}

	if (onewire == 1)
	{
		printf ("Выберете датчики:");
		sens = opendir (TERM_DIR);
		if (sens == NULL)
		{
			printf ("\nSensors not found\n");
			perror ("");
			return 3;
		}
		while ((sen_n = readdir (sens)) != NULL)
		{
			v++;
		}
		closedir (sens);
	}

	sc_sn = (char **) malloc (v * sizeof (char *));
	sc_name = (char **) malloc (v * sizeof (char *));
	sc_place = (char **) malloc (v * sizeof (char *));
	sc_type = (int *) malloc (v * sizeof (int));
	for (i = 0; i < v; i++)
	{
		sc_sn[i] = (char *) malloc (25 * sizeof (char));	//Выдиление массива char для каждого указателя(создание 2 мерного массива)
		sc_name[i] = (char *) malloc (25 * sizeof (char));
		sc_place[i] = (char *) malloc (25 * sizeof (char));
		if (sc_place[i] != NULL)
		{
			memset (sc_sn[i], '\0', 25 * sizeof (char));
			memset (sc_name[i], '\0', 25 * sizeof (char));
			memset (sc_place[i], '\0', 25 * sizeof (char));
		}
	}
	/*if (uart == 1)
	{

	}
	*/
	if (onewire == 1)
	{
		//char sc_sn[v][17], sc_name[v][17], sc_place[v][17]; //Срочно переписать на malloc!
		sens = opendir (TERM_DIR);
		while ((sen_n = readdir (sens)) != NULL)
		{
			sprintf (sc_sn[i], "%s", sen_n->d_name);
			if (sen_n->d_name[0] == '.')
			{
				sen_n = readdir (sens);
				sen_n = readdir (sens);
				sprintf (sc_sn[i], "%s", sen_n->d_name);
			}

			if (gettemp (sc_sn[i]) == 6513)
			{
				sen_n = readdir (sens);
				sprintf (sc_sn[i], "%s", sen_n->d_name);
			}
			temp = gettemp (sc_sn[i]);
			printf
				("\nДатчик №%d\n SN:%s\n Температура:%.3f\n",
				i, sen_n->d_name, temp);
			sprintf (com1, "t_sensors:sens%d", i);
			printf ("Использовать(yes/no)?\n");
			scanf ("%3s", uny);
			sleep (1);
			if (uny[0] == 'y')
			{
				iniparser_set (conf_d, com1, sc_sn[i]);
				printf("Введтите название для датчика(максимум 15 символов):");
				bzero (sc_name[i], 17);
				scanf ("%15s", sc_name[i]);
				sprintf (com1, "t_sensors:sens%d_name", i);
				iniparser_set (conf_d, com1, sc_name[i]);
				printf("Введите расположение датчика:");
				scanf ("%15s", sc_place[i]);
				sprintf (com1, "t_sensors:sens%d_place", i);
				iniparser_set (conf_d, com1, sc_place[i]);
				sprintf (com1, "t_sensors:sens%d_type", i);
				iniparser_set (conf_d, com1, "1");
				i++;
			}
		}
		if (i == 0)
		{
			printf ("Не одного датчика не выбрано");
			exit (27);
		}

		v = 0;
	}

	/*
	while (g != i) {
	v = v + strlen(sc_name[g]);
	printf("\nName%d:%s\n SN:%s\n Len:%d\n", g, sc_name[g], sc_sn[g],
	strlen(sc_name[g]));
	g++;
	}
	*/
	//printf("\nobshaya dlinna:%d\n", v);

	//Формирование комманды для создания базы данных
	g = (1 + i) * 20;
	col2 = v + g;
	char rrd1[strlen (db) + strlen (CONFIG_DIR) + 40], rrd2[col2], rrdt[39],
		rrdp[115];
	bzero (rrd2, col2);
	sprintf (rrd1, "rrdtool create %s/%s.rrd --start N --step 60 ", CONFIG_DIR,
		db);
	g = 0;
	while (g != i)
	{
		sprintf (rrdt, "DS:%s:GAUGE:600:U:U ", sc_name[g]);
		sprintf (rrd2, "%s%s", rrd2, rrdt);
		g++;
	}

	sprintf (rrdp,
		"RRA:AVERAGE:0.5:1:12 RRA:AVERAGE:0.5:1:288 RRA:AVERAGE:0.5:12:168 RRA:AVERAGE:0.5:12:720 RRA:AVERAGE:0.5:288:365");
	char rrdc[strlen (rrd1) + strlen (rrd2) + 121];
	sprintf (rrdc, "%s%s%s", rrd1, rrd2, rrdp);
	system (rrdc);

	//printf("\n%s\n", rrdc);

	sprintf (col, "%d", i);
	iniparser_set (conf_d, "t_sensors:sens_col", col);
	closedir (sens);
	//clears();
	i = 0;
	printf
		("Хотите подключить доп. устрайства?\nyes/no\n");
	scanf ("%3s", uny);
	if (uny[0] == 'y')
		st = 0;
	while (1)
	{
		printf
			("Выберете тип устройства:\n 1.Реле\n 2.Счётчик\n 3.Обогреватель \n 4.Кондиционер\n 5.Слудующий шаг\n");
		scanf ("%d", &type);
		if (type >= 6 && type <= 0)
			printf ("Неверный тип.");
		if (type == 5)
			break;
		if (type <= 4 && type >= 1)
		{
			printf
				("Введите имя устройства(не более 15-ти символов):");
			scanf ("%15s", device_name);
			if (type == 2)
			{
				printf
					("Введите серийный номер(не более 15-ти символов):");
				scanf ("%15s", device_place);
				sprintf (com1, "dev:device%d_sn", i);
				iniparser_set (conf_d, com1, device_place);
			}
			printf
				("Введите расположение устройства:");
			scanf ("%15s", device_place);
			st = 2;
			while (st != 1)
			{
				printf
					("Введите номер порта GPIO к которому подключено устройство:");
				scanf ("%d", &input[i]);
				tmp = 0;
				while (tmp <= i)
				{

					//printf("tmp=%d,input=%d,i=%d,st=%d,input2=%d\n",tmp,input[i],i,st,input[tmp]);
					if (i == tmp)
						tmp++;
					if (input[i] == input[tmp])
					{
						printf
							("Порт занят, введите другой!\n");
						break;
					}
					if (input[i] != input[tmp])
						st = 1;
					tmp++;
				}
				//if (i = 0) break;

			}
			sprintf (com1, "dev:device%d_place", i);
			iniparser_set (conf_d, com1, device_place);
			sprintf (com1, "dev:device%d_name", i);
			iniparser_set (conf_d, com1, device_name);
			sprintf (com1, "dev:device%d_port", i);
			sprintf (col, "%d", input[i]);
			iniparser_set (conf_d, com1, col);
			sprintf (com1, "dev:device%d_type", i);
			sprintf (col, "%d", type);
			iniparser_set (conf_d, com1, col);
			i++;
		}
	}

	sprintf (col, "%d", i);
	iniparser_set (conf_d, "dev:device_col", col);
	while (1)
	{
		printf ("Введите пароль:");
		scanf ("%s", txt_dir);
		if (strlen (txt_dir) >= 4)
		{
			iniparser_set (conf_d, "main:key", txt_dir);
			break;
		}
		else
			printf
			("Минимальна длинна пароля 4 символа:");
	}

	printf
		("Введите путь для сохранения текстовых файлов(опционально):");
	scanf ("%s", txt_dir);
	iniparser_set (conf_d, "main:txt_dir", txt_dir);

	//printf("\nconf name:%s\n", name);

	if (iniparser_save (conf_d, confnd) == 1)
		return 1;
	iniparser_freedict (conf_d);
	cre_corn_sh ("term_xml_gen", confnd);
	return 0;
}

int	cre_corn_sh (char *sh_name, char *ini_name)
{
	dictionary *conf;
	char com2[30], com3[30], com1[30], confnd[256];
	int i = 0, d = 0;
	FILE *SH;
	bzero (confnd, 256);
	sprintf (confnd, "%s/%s", CONFIG_DIR, CONFIG_FILE);
	conf = iniparser_load (confnd);
	if (iniparser_getstring (conf, "t_sensors", "error") == "error")
	{
		printf
			("\nПроблема, некоректный файл конфигурации, пытаюсь пересоздать.\n");
		sleep (3);
		sprintf (com1, "rm -f %s", CONFIG_FILE);
		system (com1);
		iniparser_freedict (conf);
		if (cre_conf (CONFIG_DIR, CONFIG_FILE) != 0)
		{
			perror
				("\n Ошибка создания файла конфигурации.");
			exit (2);
		}
		conf = iniparser_load (confnd);
	}
	i = iniparser_getint (conf, "t_sensors:sens_col", 456);
	if (i == 456)
	{
		perror ("Not found sensors!\n");
		exit (3);
	}
	//Загрузка данных в память
	char sc_name[i][17], tempc[i][11], sc_sn[i][17], rrdut[(i * 7) + 1], *db,
		*txt_dir;
	float temp[i];
	d = 0;
	txt_dir = iniparser_getstring (conf, "main:txt_dir", "/tmp");

	db = iniparser_getstring (conf, "main:db_name", "none");
	char sh[strlen (CONFIG_DIR) + strlen (sh_name) + 1];
	sprintf (sh, "%s/%s.sh", CONFIG_DIR, sh_name);
	SH = fopen (sh, "w");

	//printf("\n%s\n", rrdu1);
	while (i != d)
	{
		sprintf (com3, "t_sensors:sens%d", d);
		sprintf (com2, "t_sensors:sens%d_name", d);
		sprintf (sc_sn[d], "%s", iniparser_getstring (conf, com3, "none"));
		sprintf (sc_name[d], "%s", iniparser_getstring (conf, com2, "none"));
		//printf("\nSensor№%d\n SN:%s \n Name:%s \n", d, sc_sn[d], sc_name[d]);
		d++;
	}
	d = 0;
	fprintf (SH,
		"#!/bin/bash\n#Скрипт создания xml файлов для highcharts\nDIR=\"%s\"\n",
		CONFIG_DIR);
	fprintf (SH, "XMLDIR=\"%s\"\nDBNAME=\"%s\"\n\n", txt_dir, db);
	//За 4 часа
	fprintf (SH, "#hourly\nrrdtool xport -s now-4h -e now");
	while (i != d)
	{
		fprintf (SH, " \\\n");
		fprintf (SH, "DEF:%s=$DIR/$DBNAME.rrd:%s:AVERAGE", sc_name[d],
			sc_name[d]);
		d++;
	}
	d = 0;
	while (i != d)
	{
		fprintf (SH, " \\\n");
		fprintf (SH, "XPORT:%s:\"%s\"", sc_name[d], sc_name[d]);
		d++;
	}
	fprintf (SH, " > $XMLDIR/temp4h.xml\n\n");

	//За 2 деня
	fprintf (SH, "#daily\nrrdtool xport -s now-48h  -e now");
	d = 0;
	while (i != d)
	{
		fprintf (SH, " \\\n");
		fprintf (SH, "DEF:%s=$DIR/$DBNAME.rrd:%s:AVERAGE", sc_name[d],
			sc_name[d]);
		d++;
	}
	d = 0;
	while (i != d)
	{
		fprintf (SH, " \\\n");
		fprintf (SH, "XPORT:%s:\"%s\"", sc_name[d], sc_name[d]);
		d++;
	}
	fprintf (SH, " > $XMLDIR/temp48h.xml\n\n");

	//За 1 неделю
	fprintf (SH, "#weekly\nrrdtool xport -s now-1w  -e now");
	d = 0;
	while (i != d)
	{
		fprintf (SH, " \\\n");
		fprintf (SH, "DEF:%s=$DIR/$DBNAME.rrd:%s:AVERAGE", sc_name[d],
			sc_name[d]);
		d++;
	}
	d = 0;
	while (i != d)
	{
		fprintf (SH, " \\\n");
		fprintf (SH, "XPORT:%s:\"%s\"", sc_name[d], sc_name[d]);
		d++;
	}
	fprintf (SH, " > $XMLDIR/temp1w.xml\n\n");

	//За 1 месяц
	fprintf (SH, "#monthly\nrrdtool xport -s now-1month  -e now");
	d = 0;
	while (i != d)
	{
		fprintf (SH, " \\\n");
		fprintf (SH, "DEF:%s=$DIR/$DBNAME.rrd:%s:AVERAGE", sc_name[d],
			sc_name[d]);
		d++;
	}
	d = 0;
	while (i != d)
	{
		fprintf (SH, " \\\n");
		fprintf (SH, "XPORT:%s:\"%s\"", sc_name[d], sc_name[d]);
		d++;
	}
	fprintf (SH, " > $XMLDIR/temp1m.xml\n\n");

	//За 1 год
	fprintf (SH, "#yearly\nrrdtool xport -s now-1y  -e now");
	d = 0;
	while (i != d)
	{
		fprintf (SH, " \\\n");
		fprintf (SH, "DEF:%s=$DIR/$DBNAME.rrd:%s:AVERAGE", sc_name[d],
			sc_name[d]);
		d++;
	}
	d = 0;
	while (i != d)
	{
		fprintf (SH, " \\\n");
		fprintf (SH, "XPORT:%s:\"%s\"", sc_name[d], sc_name[d]);
		d++;
	}
	fprintf (SH, " > $XMLDIR/temp1y.xml\n\n");

	fclose (SH);
	chmod (sh, S_IRWXU | S_IRGRP | S_IXGRP | S_IXOTH);
	return 0;
}

int	init_uart ()
{
	int uart0_filestream = open ("/dev/ttyAMA0", O_RDWR | O_NOCTTY | O_NDELAY);	//Open in non blocking read/write mode
	if (uart0_filestream == -1)
	{
		//ERROR - CAN'T OPEN SERIAL PORT
		printf
			("Error - Unable to open UART.  Ensure it is not in use by another application\n");
	}
	struct termios options;
	tcgetattr (uart0_filestream, &options);
	options.c_cflag = B9600 | CS8 | CLOCAL | CREAD;	//<Set baud rate
	options.c_iflag = IGNPAR;
	options.c_oflag = 0;
	options.c_lflag = 0;
	tcflush (uart0_filestream, TCIFLUSH);
	tcsetattr (uart0_filestream, TCSANOW, &options);
	return uart0_filestream;
}


int splint_rtoa (unsigned char *rx, int rs, int rc, char **name_mas, float *dat_mas)
{
	int i, r, mix = 0;
	char tmp[20];
	printf ("butes num:%d\n", rs);

	for (i = 0; i < rc; i++)
	{
		if (name_mas[i] != NULL)
		{
			for (r = mix; r < rs; r++)
			{
				if (rx[r] == ':')
				{
					mix = r + 1;
					break;
				}
				else
				{
					name_mas[i][r - mix] = rx[r];
				}
			}
			for (r = mix; r < rs; r++)
			{
				if (rx[r] == ' ')
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
			dat_mas[i] = atof (tmp);
			printf ("[splint]sens_%d %s dat:%f\n", i, name_mas[i], dat_mas[i]);
		}
	}
	printf ("end splint\n");
	return rc;
}

int readDA (int uart0_filestream, unsigned char *rx)
{
	int rc = 0;
	if (uart0_filestream != -1)
	{
		int i = 0, r = 0, st = 0, rx_length;
		// Read up to 255 characters from the port if they are there
		unsigned char rx_buffer[256];
		while (i < 255)
		{
			rx_length = read (uart0_filestream, (void *) rx_buffer, 128);	//Filestream, buffer to store in, number of bytes to read (max)
			if (rx_length < 0)
			{
				//printf("no byte\n");
				//An error occured (will occur if there are no bytes)
			}
			else if (rx_length == 0)
			{
				//No data waiting
				printf ("No data\n");
			}
			else
			{
				//Bytes received
				rx_buffer[rx_length] = '\0';
				//printf("rx_buf: %s", rx_buffer);
				for(r=0;r < rx_length; r++)
				{
					//printf("%d\n",r);
					//r++;
					if(rx_buffer[r] == 'v')
					{
						break;
					}
				}
				if(r >= rx_length)
				{
					sleep(1);
					continue;
				}
				//r=r-1;
				for (; r < rx_length; r++)
				{
					if (rx_buffer[r] == ';')
					{
						rx[i] = '\0';
						st = 1;
						break;
					}
					else
					{
						if (rx_buffer[r] == ' ')
						{
							rc++;
							//printf ("rc++\n");
						}
						rx[i] = rx_buffer[r];
						i++;
					}
				}
				if (st == 1)
					break;
				//printf("%i bytes read : %s\n", rx_length, rx_buffer);
			}
		}  

	}
	printf ("rx: %s\n", rx);
	return rc;
}