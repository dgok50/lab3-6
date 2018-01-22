#include <sys/types.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <sys/stat.h>
#include <syslog.h>
#include <errno.h>
#include <math.h>
#include <string.h>
#include "ups_parse.c"
#include "usred.c"
//#include <wiringPiI2C.h>	//Used for I2C
#include <unistd.h>		//Used for UART
#include <fcntl.h>		//Used for UART
#include <termios.h>		//Used for UART
//#define RND 20
#define buffer_size 4096

#define VER "0.7"
#define HWVER "0.3"

//BMP085 Config
//set the oversampling value (OSS 0..3) - this affects the precision of pressure measurement and
//the current required to get one reading
// OSS value | +/- pressure (hPa) | +/- altitude (m) | av. current at 1 sample/sec (uA) | no. of samples
//     0     |       0.06         |       0.5        |               3                  |         1
//     1     |       0.05         |       0.4        |               5                  |         2
//     2     |       0.04         |       0.3        |               7                  |         4
//     3     |       0.03         |       0.25       |               12                 |         8
//#define OSS 3

int stop = 0;
void stop_all ();

void skeleton_daemon ()
{
  pid_t pid;

  /* Fork off the parent process */
  pid = fork ();

  /* An error occurred */
  if (pid < 0)
    exit (EXIT_FAILURE);

  /* Success: Let the parent terminate */
  if (pid > 0)
    exit (EXIT_SUCCESS);

  /* On success: The child process becomes session leader */
  if (setsid () < 0)
    exit (EXIT_FAILURE);

  /* Catch, ignore and handle signals */
  //TODO: Implement a working signal handler */
  signal (SIGCHLD, SIG_IGN);
  signal (SIGHUP, SIG_IGN);

  /* Fork off for the second time */
  pid = fork ();

  /* An error occurred */
  if (pid < 0)
    exit (EXIT_FAILURE);

  /* Success: Let the parent terminate */
  if (pid > 0)
    exit (EXIT_SUCCESS);

  /* Set new file permissions */
  umask (0);

  /* Change the working directory to the root directory */
  /* or another appropriated directory */
  //chdir ("/");

  /* Close all open file descriptors */
  int x;
  for (x = sysconf (_SC_OPEN_MAX); x > 0; x--)
    {
      close (x);
    }

}

short AC1, AC2, AC3;
unsigned short AC4, AC5, AC6;
short B1, B2, MB, MC, MD;
long B5, UP, UT, temp_bmp, pressure_bmp;
int fd;

int main ()
{
  skeleton_daemon ();

  /* Open the log file */
  openlog ("uart_to_html", LOG_PID, LOG_DAEMON);
  syslog (LOG_NOTICE, "Запуск сервиса");

  void (*TermSignal) (int);
  TermSignal = signal (SIGTERM, stop_all);

  void (*IntSignal) (int);
  IntSignal = signal (SIGINT, stop_all);

  void (*QuitSignal) (int);

  QuitSignal = signal (SIGQUIT, stop_all);

  int rc = 0, y = 0, c = 0, itc = 0;
  char **name_mas;
  char typec[10], strtmp[100];
  double *dat_mas;
  unsigned char rx[256];
  time_t itime;
  struct tm *Tm;

  struct usred sred;
  struct usred sred_main;

  int i = 0, rs = 255, uart0_filestream = -1;
  uart0_filestream = open ("/dev/ttyAMA0", O_RDWR | O_NOCTTY | O_NDELAY);	//Open in non blocking read/write mode
  if (uart0_filestream == -1)
    {
      //ERROR - CAN'T OPEN SERIAL PORT
      syslog (LOG_CRIT,
	      "Error - Unable to open UART.  Ensure it is not in use by another application\n");
      exit (1);
    }
  struct termios options;
  tcgetattr (uart0_filestream, &options);
  options.c_cflag = B9600 | CS8 | CLOCAL | CREAD;	//<Set Bbaud rate
  options.c_iflag = IGNPAR;
  options.c_oflag = 0;
  options.c_lflag = 0;
  tcflush (uart0_filestream, TCIFLUSH);
  tcsetattr (uart0_filestream, TCSANOW, &options);
  rc = readDA (uart0_filestream, rx);
  syslog (LOG_NOTICE, "pid: %d\n", getpid ());
  syslog (LOG_NOTICE, "rc:%d, rx:%s\n", rc, rx);

  syslog (LOG_NOTICE, "получено переменных:%d\n", rc);
  dat_mas = (double *) malloc (rc * sizeof (double));	//Выделение массива для паказаний датчиков
  name_mas = (char **) malloc (rc * sizeof (char *));	//Выделение массива указателей

  if (name_mas == NULL)		//Проверка выдиления
    {
      syslog (LOG_EMERG,
	      "Ошибка!\n Невозможно выдилить память.\n");
      exit (EXIT_FAILURE);
    }

  for (i = 0; i < rc; i++)
    {
      name_mas[i] = (char *) malloc (25 * sizeof (char));	//Выдиление массива char для каждого указателя(создание 2 мерного массива)

      if (name_mas[i] != NULL)
	{
	  memset (name_mas[i], '\0', 25 * sizeof (char));	//Заполнение нулями массива
	}
    }
  c = rc;
  double unixtime = 0, hum = -100, temp = -100, lux, in_temp, vcc =
    -100, pre, mctmp;
  double ups_v = -100, ups_load = -100, ups_frq = -100, ups_bat_stat =
    -100, ups_stat;
  int MSB, LSB, XLSB, timing, DEVICE_ADDRESS;
  DEVICE_ADDRESS = 0x77;

  fd = wiringPiI2CSetup (DEVICE_ADDRESS);
  read_calibration_values (fd);

  syslog (LOG_NOTICE,
	  "Данные успешно распознаны, сервис запущен.");
  //rc = rc - 1;
  FILE *F, *GIGASET_XML, *XML, *NAROD, *RTF;
  init_usred (&sred, 20, 4);
  init_usred (&sred_main, 20, 7);
  while (1)
    {
      //sleep(10);
      //syslog (LOG_NOTICE, "rec: %s\n", rx);
      //syslog (LOG_NOTICE, "col: %d\n", rc);
      if (stop == 1)
	break;
      if (readDA (uart0_filestream, rx) == rc)
	{
	  y = splint_rtoa (rx, rs, rc, name_mas, dat_mas);
	  temp = -100;
	  hum = -100;
	  mctmp = -100;
	  pre = -100;
	  in_temp = -100;
	  vcc = -100;
	  //ups_v= -100;
	  //ups_load= -100;
	  //ups_frq = -100;
	  //ups_bat_stat = -100;
	  ups_stat =
	    get_ups_data (&ups_v, &ups_load, &ups_frq, &ups_bat_stat);
	  if (ups_stat == 0)
	    {
	      ups_load = (ups_load / 100) * 375;
	      write_usred (&sred, 4, &ups_frq, &ups_v, &ups_load,
			   &ups_bat_stat);
	    }
	  if (ups_stat != 0)
	    syslog (LOG_WARNING,
		    "Не удаётса получить данные UPS");

	  unixtime = time (NULL) / 1000.0;

	  //BMP085 sec
	  //get raw temperature
	  wiringPiI2CWriteReg8 (fd, 0xF4, 0x2E);
	  usleep (4500);
	  MSB = wiringPiI2CReadReg8 (fd, 0xF6);
	  LSB = wiringPiI2CReadReg8 (fd, 0xF7);
	  UT = (MSB << 8) + LSB;

	  //get raw pressure
	  timing = get_timing ();
	  wiringPiI2CWriteReg8 (fd, 0xF4, 0x34 + (OSS << 6));
	  usleep (timing);
	  MSB = wiringPiI2CReadReg8 (fd, 0xF6);
	  LSB = wiringPiI2CReadReg8 (fd, 0xF7);
	  XLSB = wiringPiI2CReadReg8 (fd, 0xF8);
	  UP = ((MSB << 16) + (LSB << 8) + XLSB) >> (8 - OSS);

	  get_readings (UT, UP);
	  //BMP085 sec end

	  pre = pressure_bmp / 133.321995;
	  in_temp = temp_bmp / 10.0;

	  for (i = 0; i < rc; i++)
	    {
	      if (strcmp (name_mas[i], "OT") == 0)
		{
		  temp = dat_mas[i];
		}
	      else if (strcmp (name_mas[i], "Hum") == 0)
		{
		  hum = dat_mas[i];
		}
	      else if (strcmp (name_mas[i], "LUX") == 0)
		{
		  lux = dat_mas[i];
		}
	      else if (strcmp (name_mas[i], "MCTMP") == 0)
		{
		  mctmp = dat_mas[i];
		}
	      else if (strcmp (name_mas[i], "vcc") == 0)
		{
		  vcc = dat_mas[i];
		}
	    }
	  if (temp == -100 || hum == -100)
	    {
	      sleep (20);
	      syslog (LOG_WARNING,
		      "Не удаётса распознать строку %s, сброс...",
		      rx);
	      for (i = 0; i < rc; i++)
		{
		  syslog (LOG_WARNING, "Переменная %s = %f",
			  name_mas[i], dat_mas[i]);
		  bzero (name_mas[i], 25);
		}
	      bzero (rx, 255);
	      bzero (dat_mas, rc);
	      rc = readDA (uart0_filestream, rx);
	      continue;
	      //exit(1);
	    }
	  F = fopen ("/usr/share/nginx/www/tmp/arduino_raw.txt", "w+");
	  if (F == NULL)
	    {
	      syslog (LOG_WARNING, "Cannot Open arduino_raw.txt\n");
//            exit (1);
	    }
	  if (flock (fileno (F), LOCK_EX | LOCK_NB))
	    {
	      syslog (LOG_WARNING, "arduino_raw.txt file is blocking\n");
//            exit (1);
	    }
	  if (ups_stat == 0)
	    {
	      fprintf (F, "ups_frq:%f ups_v:%f ups_load:%f ups_bat_stat:%f ",
		       ups_frq, ups_v, ups_load, ups_bat_stat);
	    }
	  fprintf (F, "%stime:%f PRE:%f INTMP:%f;", rx, unixtime, pre,
		   in_temp);
	  //fseek (F, 0, SEEK_END);
	  flock (fileno (F), LOCK_UN);
	  fclose (F);



	  //syslog (LOG_NOTICE, "y=%d\n", y);
	  F = fopen ("/usr/share/nginx/www/tmp/arduino.html", "w+");
	  if (F == NULL)
	    {
	      //syslog (LOG_NOTICE, "Cannot Open\n");
	      exit (1);
	    }
	  if (flock (fileno (F), LOCK_EX | LOCK_NB))
	    {
	      //syslog (LOG_NOTICE, "file is blocking\n");
	      exit (1);
	    }
	  fprintf (F,
		   "<!DOCTYPE HTML PUBLIC " "-//W3C//DTD HTML 4.01//EN" " "
		   "http://www.w3.org/TR/html4/strict.dtd"
		   ">\n<html>\n <head>\n  <meta http-equiv=" "Content-Type"
		   " content=" "text/html; charset=utf-8"
		   ">\n  <meta http-equiv=" "refresh" " content=" "5;"
		   "> \n  <title>KUIP data</title>\n </head>\n <body>\n");
	  for (i = 0; i < rc; i++)
	    {
	      fprintf (F, "  <h1>sens_%d %s: %f </h1>\n", i, name_mas[i],
		       dat_mas[i]);
	    }
	  i++;
	  fprintf (F, "  <h1>sens_%d Pre: %f </h1>\n", i, pre);
	  i++;
	  fprintf (F, "  <h1>sens_%d bmp085_temp: %f </h1>\n", i, in_temp);
	  if (ups_stat == 0)
	    {
	      fprintf (F,
		       "<h1>now ups_frq:%f ups_v:%f ups_load:%f ups_bat_stat:%f data_redy:%d data_index:%u </h1>\n",
		       ups_frq, ups_v, ups_load, ups_bat_stat, sred.data_redy, sred.data_index);
	      /*fprintf (F,
		       "<h1>usr ups_frq:%f ups_v:%f ups_load:%f ups_bat_stat:%f data_redy:%d data_index: %u</h1>\n",
		       sred.data_usred[0], sred.data_usred[1],
		       sred.data_usred[2], sred.data_usred[3], sred.data_redy,
		       sred.data_index); */
	    }
	  fprintf (F, " </body>\n</html>");
	  //fseek (F, 0, SEEK_END);
	  flock (fileno (F), LOCK_UN);
	  fclose (F);

	  //Gigaset screen sever XHTML gen
	  GIGASET_XML = fopen ("/usr/share/nginx/www/tmp/arduino.xml", "w+");
	  if (GIGASET_XML == NULL)
	    {
	      syslog (LOG_WARNING, "Cannot Open xml\n");
	      exit (1);
	    }
	  if (flock (fileno (GIGASET_XML), LOCK_EX | LOCK_NB))
	    {
	      syslog (LOG_WARNING, "file is blocking\n");
	      exit (1);
	    }
	  fprintf (GIGASET_XML,
		   "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n<!DOCTYPE html PUBLIC \"-//OMA//DTD XHTML Mobile 1.2//EN\" \"http://www.openmobilealliance.org/tech/DTD/xhtmlmobile12.dtd\">\n<html xmlns=\"http://www.w3.org/1999/xhtml\">\n<head>\n <meta name=\"expires\" content=\"0.15\" /> \n <title>Out home</title>\n</head>\n<body>\n");
	  for (i = 1; i < (rc - 2); i++)
	    {
	      sprintf (typec, "\0");
	      if (name_mas[i][0] == 'O')
		{
		  //typec[0]=' ';
		  typec[0] = 'C';
		  typec[1] = '\0';
		  //sprintf (name_mas[i], "Temp");
		}
	      else if (name_mas[i][0] == 'H')
		{
		  //sprintf(typec," ");
		  typec[0] = 37;
		  typec[1] = '\0';
		}
	      //else
	      //    {sprintf(typec, "\0");}
	      fprintf (GIGASET_XML,	/*  <h1>sens_%d %s: %.2f </h1>\n  */
		       "  <p style=\"text-align:left\"> %s: %.2f  ",
		       name_mas[i], dat_mas[i]);
	      fprintf (GIGASET_XML, "%s", typec);
	      //syslog (LOG_NOTICE, "typec=%s", typec);
	      fprintf (GIGASET_XML, "</p>\n");
	      //if (i < (rc - 1))
	      //fprintf (GIGASET_XML, "  <br/> \n");
	    }
	  fprintf (GIGASET_XML,
		   " <p style=\"text-align:left\"> Pre: %.2f  </p>\n", pre);
	  fprintf (GIGASET_XML, " </body>\n</html>");
	  //fseek (F, 0, SEEK_END);
	  flock (fileno (GIGASET_XML), LOCK_UN);
	  fclose (GIGASET_XML);

	  if (temp > -40 && temp < 80 && hum > 0 && hum < 100)
	    {
	      NAROD = fopen ("/usr/share/nginx/www/tmp/arduino.txt", "w+");
	      if (NAROD == NULL)
		{
		  //syslog (LOG_NOTICE, "Cannot Open\n");
		  exit (1);
		}
	      if (flock (fileno (NAROD), LOCK_EX | LOCK_NB))
		{
		  //syslog (LOG_NOTICE, "file is blocking\n");
		  exit (1);
		}
	      write_usred (&sred_main, 7, &temp, &hum, &lux, &mctmp, &pre,
			   &in_temp, &vcc);
	      fprintf (NAROD,
		       "#b8-27-eb-8d-59-05#KUIP#55.7239#37.8174#176\n#Temp#%f#Температура (DHT21)\n#Hum#%f#Влажность\n#LUX#%f#Освещённость\n#MCTMP#%f#Темп ВБ1\n",
		       temp, hum, lux, mctmp);
	      fprintf (NAROD, "#PRE#%f#Давление (BMP180)\n", pre);
	      /*if(ups_stat == 0 && sred.data_redy == 1) {
	         fprintf (NAROD, "#UPSFRQ#%f#Частота сети\n", sred.data_usred[0]);
	         fprintf (NAROD, "#UPSV#%f#Напряжение сети\n", sred.data_usred[1]);
	         fprintf (NAROD, "#UPSLW#%f#Нагрузка на ИБП 2\n", sred.data_usred[2]);
	         fprintf (NAROD, "#UPSBAT#%f#Заряд батареи ИБП 2\n", sred.data_usred[3]);
	         } */
	      if (ups_stat == 0)
		{
		  fprintf (NAROD, "#UPSFRQ#%f#Частота сети\n",
			   ups_frq);
		  fprintf (NAROD, "#UPSV#%f#Напряжение сети\n",
			   ups_v);
		  fprintf (NAROD,
			   "#UPSLW#%f#Нагрузка на ИБП 2\n",
			   ups_load);
		  fprintf (NAROD,
			   "#UPSBAT#%f#Заряд батареи ИБП 2\n",
			   ups_bat_stat);
		}
	      fprintf (NAROD, "#INTEMP#%f#Темп ГБ\n", in_temp);
	      fprintf (NAROD, "#MCVCC#%f#Напр ВБ1\n", vcc);
	      //fprintf (NAROD, "#%s#%f#%u\n", name_mas[i], dat_mas[i], (unsigned)time(NULL));
	      fprintf (NAROD, "##");
	      //fseek (NAROD, 0, SEEK_END);
	      flock (fileno (NAROD), LOCK_UN);
	      fclose (NAROD);
	    }

	  //RTF Generator
	  RTF = fopen ("/usr/share/nginx/www/tmp/ar_report.rtf", "w+");
	  itime = time (NULL);
	  Tm = localtime (&itime);
	  fprintf (RTF,
		   "{\\rtf1\\ansi\\ansicpg1251\\deff0\\deflang1049{\\fonttbl{\\f0\\fnil\\fprq2\\fcharset0 DS-Terminal;}}\n");
	  fprintf (RTF,
		   "{\\*\\generator A1Template_base_gen 0.2 ;}\\viewkind4\\uc1\\pard\\qc\\lang1033\\f0\\fs28 KUIP sensor data report\\par\n");
	  fprintf (RTF, "%d/%d/%d %d:%d:%d\\par\n", Tm->tm_mday,
		   Tm->tm_mon + 1, Tm->tm_year + 1900, Tm->tm_hour,
		   Tm->tm_min, Tm->tm_sec);

	  fprintf (RTF, "\\pard Main module:\\par\n");
	  fprintf (RTF, " App ver: " VER "\\par\n");
	  fprintf (RTF, " HW ver: " HWVER "\\par\n");
	  char ds = '*';
	  fprintf (RTF, " Temp(BMP180): %.3f%cC\\par\n", in_temp, ds);
	  fprintf (RTF, " Pressure(BMP180): %.3fmm.Hg.\\par\n", pre);
	  fprintf (RTF, "\\par\n");

	  fprintf (RTF, "External module 1:\\par\n");
	  fprintf (RTF, " Module type: atmega328p-pu(base dev)\\par\n");
	  fprintf (RTF, " Module FW ver: 0.2\\par\n");
	  fprintf (RTF, " Module temp: %.3f%cC\\par\n", mctmp, ds);
	  fprintf (RTF, " Module vcc: %.3fV\\par\n", vcc);
	  fprintf (RTF, " Light sensor(BH1750): %.3f Lux\\par\n", lux);
	  fprintf (RTF, " Temp (DHT21): %.3f%cC\\par\n", temp, ds);
	  fprintf (RTF, " Humidity (DHT21): %.3f%c\\par\n\\par\n", hum, 37);
	  if (ups_stat == 0)
	    {
	      fprintf (RTF,
		       "Com server UPS:\\par\n Line Frequency: %.2fHz\\par\n",
		       ups_frq);
	      fprintf (RTF, " Line Voltage: %.2fV\\par\n", ups_v);
	      //fprintf (RTF, " Load: %f%c\\par\n",  ups_load, 37);
	      fprintf (RTF, " Power cons: %.3fW\\par\n", ups_load);
	      fprintf (RTF, " Bat charge: %.2f%c\\par\n\\par\n", ups_bat_stat,
		       37);
	    }
	  fprintf (RTF, "Timestamp: %u\\par\n", (unsigned) itime);
	  fprintf (RTF, "\\par\n");
	  fprintf (RTF, "\\par\n");
	  fprintf (RTF, "\\par\n");
	  fprintf (RTF, "}\n\n");

	  fclose (RTF);

	  //Web XML gen
	  XML = fopen ("/usr/share/nginx/www/tmp/arduino_web.xml", "w+");
	  if (XML == NULL)
	    {
	      syslog (LOG_NOTICE, "Cannot Open xml\n");
	      exit (1);
	    }
	  if (flock (fileno (XML), LOCK_EX | LOCK_NB))
	    {
	      syslog (LOG_NOTICE, "file is blocking\n");
	      exit (1);
	    }
	  fprintf (XML,
		   "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n<uart_kuip>\n <time>%u</time>\n <pre>%f</pre>\n <in_temp>%f</in_temp>\n <ups_frq>%f</ups_frq>\n <ups_v>%f</ups_v>\n <ups_load>%f</ups_load>\n <ups_bat_stat>%f</ups_bat_stat>\n",
		   (unsigned) time (NULL), pre, in_temp, ups_frq, ups_v,
		   ups_load, ups_bat_stat);
	  for (i = 0; i < rc; i++)
	    {
	      fprintf (XML,	/*  <h1>sens_%d %s: %.2f </h1>\n  */
		       " <%s>%.2f", name_mas[i], dat_mas[i]);
	      //syslog (LOG_NOTICE, "typec=%s", typec);B
	      fprintf (XML, "</%s>\n", name_mas[i]);
	    }
	  fprintf (XML, "</uart_kuip>");
	  //fseek (F, 0, SEEK_END);
	  flock (fileno (XML), LOCK_UN);
	  fclose (XML);
	  /*if(itc>=3) {
	     sprintf(strtmp,"/usr/bin/curl \"http://192.168.0.113:82/objects/?object=OutSide0&op=m&m=dataChange&h=%.2f&t=%.2f&ai=%.2f&vcc=%.2f\" -m 2", hum, temp, ai, vcc);
	     system(strtmp);
	     sprintf(strtmp,"/usr/bin/curl \"http://192.168.0.113:82/objects/?object=int_sens0&op=m&m=dataChange&p=%.2f&t=%.2f\" -m 2", pre, in_temp);
	     system(strtmp);
	     itc=0;
	     }
	     itc++; */
	  sleep (15);
	  //syslog (LOG_NOTICE, "\033[2J"); /* Clear the entire screen. */ 
	  //syslog (LOG_NOTICE, "\033[0;0f"); /* Move cursor to the top left hand corner*/
	}
    }
  syslog (LOG_NOTICE, "Закрытие uart порта.");
  close (uart0_filestream);
  //for (i = 0; i < y; i++)
  //{
  //      syslog (LOG_NOTICE, "sens_%d name: %s, data: %f\n", i, name_mas[i], dat_mas[i]);
  //}
  //syslog (LOG_NOTICE, "recev: %s",rx);
  syslog (LOG_NOTICE, "Высвобождение памяти.");
  for (i = 0; i < rc; i++)
    {
      free (name_mas[i]);
    }
  free_usred (&sred_main);
  free_usred (&sred);
  free (dat_mas);
  free (name_mas);
  syslog (LOG_NOTICE,
	  "Закрытие лога и завершение работы..");
  closelog ();
  return EXIT_SUCCESS;
}

void stop_all ()
{
  stop = 1;
  int g;
  syslog (LOG_NOTICE, "Остановка сервиса...");
}

int splint_rtoa (unsigned char *rx, int rs, int rc, char **name_mas,
	     double *dat_mas)
{
  int i, r, mix = 0;
  char tmp[20];
  //syslog (LOG_NOTICE, "butes num:%d\n", rs);

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
	  //syslog (LOG_NOTICE, "[splint]sens_%d %s dat:%f\n", i, name_mas[i], dat_mas[i]);
	}
    }
  //syslog (LOG_NOTICE, "end splint\n");
  return rc;
}

int readDA (int uart0_filestream, unsigned char *rx)
{
  int rc = 0;
  char no_dat = 0, no_byte = 0;
  if (uart0_filestream != -1)
    {
      int i = 0, r = 0, st = 0, rx_length;
      // Read up to 255 characters from the port if they are there
      unsigned char rx_buffer[buffer_size];
      while (i < 255)
	{
	  rx_length = read (uart0_filestream, (void *) rx_buffer, buffer_size);	//Filestream, buffer to store in, number of bytes to read (max)
	  if (rx_length < 0)
	    {
	      no_byte = 1;
	      sleep (2);
	      //An error occured (will occur if there are no bytes)
	    }
	  else if (rx_length == 0)
	    {
	      //No data waiting
	      no_dat = 1;
	      sleep (2);
	    }
	  else
	    {
	      //Bytes received
	      rx_buffer[rx_length] = '\0';
	      //syslog (LOG_NOTICE, "rx_buf: %s", rx_buffer);
	      for (r = 0; r < rx_length; r++)
		{
		  //syslog (LOG_NOTICE, "%d\n",r);
		  //r++;
		  if (rx_buffer[r] == 'v')
		    {
		      break;
		    }
		}
	      if (r >= rx_length)
		{
		  sleep (1);
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
			  //syslog (LOG_NOTICE, "rc++\n");
			}
		      rx[i] = rx_buffer[r];
		      i++;
		    }
		}
	      if (st == 1)
		break;
	      //syslog (LOG_NOTICE, "%i bytes read : %s\n", rx_length, rx_buffer);
	    }
	}

    }
  //syslog (LOG_NOTICE, "rx: %s\n", rx);
  if (no_dat == 1)
    {
      syslog (LOG_NOTICE, "No data\n");
    }
  if (no_byte == 1)
    {
      syslog (LOG_NOTICE, "no byte input\n");
    }
  return rc;
}

int get_timing ()
{
  if (OSS == 0)
    return 4500;
  if (OSS == 1)
    return 7500;
  if (OSS == 2)
    return 13500;
  if (OSS == 3)
    return 25500;
  return 25500;
}

// reading factory calibration data - call this first
// 11 16 bit words from 2 x 8 bit registers each, MSB comes first
void read_calibration_values (int fd)
{
  AC1 = (wiringPiI2CReadReg8 (fd, 0xAA) << 8) + wiringPiI2CReadReg8 (fd, 0xAB);
  AC2 = (wiringPiI2CReadReg8 (fd, 0xAC) << 8) + wiringPiI2CReadReg8 (fd, 0xAD);
  AC3 = (wiringPiI2CReadReg8 (fd, 0xAE) << 8) + wiringPiI2CReadReg8 (fd, 0xAF);
  AC4 = (wiringPiI2CReadReg8 (fd, 0xB0) << 8) + wiringPiI2CReadReg8 (fd, 0xB1);
  AC5 = (wiringPiI2CReadReg8 (fd, 0xB2) << 8) + wiringPiI2CReadReg8 (fd, 0xB3);
  AC6 = (wiringPiI2CReadReg8 (fd, 0xB4) << 8) + wiringPiI2CReadReg8 (fd, 0xB5);
  B1 = (wiringPiI2CReadReg8 (fd, 0xB6) << 8) + wiringPiI2CReadReg8 (fd, 0xB7);
  B2 = (wiringPiI2CReadReg8 (fd, 0xB8) << 8) + wiringPiI2CReadReg8 (fd, 0xB9);
  MB = (wiringPiI2CReadReg8 (fd, 0xBA) << 8) + wiringPiI2CReadReg8 (fd, 0xBB);
  MC = (wiringPiI2CReadReg8 (fd, 0xBC) << 8) + wiringPiI2CReadReg8 (fd, 0xBD);
  MD = (wiringPiI2CReadReg8 (fd, 0xBE) << 8) + wiringPiI2CReadReg8 (fd, 0xBF);
  syslog (LOG_NOTICE,
	  "Калибровачные данные датчика давления BMP AC1 = %d AC2 = %d AC3 = %d AC4 = %d AC5 = %d AC6 = %d B1 = %d B2 = %d MB = %d MC = %d MD = %d \n",
	  AC1, AC2, AC3, AC4, AC5, AC6, B1, B2, MB, MC, MD);
}


// the pressure_bmp measurement depends on the temp_bmperature measurement
// hence it's done in the same method
void get_readings (long UT, long UP)
{
  long X1, X2, X3, B3, B6, B7, p;
  unsigned long B4;
  X1 = ((UT - AC6) * AC5) >> 15;
  X2 = (MC << 11) / (X1 + MD);
  B5 = X1 + X2;
  temp_bmp = (B5 + 8) >> 4;

  B6 = B5 - 4000;
  X1 = (B2 * ((B6 * B6) >> 12)) >> 11;
  X2 = (AC2 * B6) >> 11;
  X3 = X1 + X2;
  B3 = (((AC1 * 4 + X3) << OSS) + 2) >> 2;
  X1 = (AC3 * B6) >> 13;
  X2 = (B1 * (B6 * B6 >> 12)) >> 16;
  X3 = ((X1 + X2) + 2) >> 2;
  B4 = (AC4 * (unsigned long) (X3 + 32768)) >> 15;
  B7 = ((unsigned long) UP - B3) * (50000 >> OSS);
  p = B7 < 0x80000000 ? (B7 * 2) / B4 : (B7 / B4) * 2;

  X1 = (p >> 8) * (p >> 8);
  X1 = (X1 * 3038) >> 16;
  X2 = (-7357 * p) >> 16;
  pressure_bmp = p + ((X1 + X2 + 3791) >> 4);
}
