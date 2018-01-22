#include <stdio.h>
//#include <wiringPi.h>
#include <stdlib.h>
#include <string.h>
#include <iniparser.h>
#include <termios.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/time.h>
#include <dirent.h>
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <ctype.h>
#include <signal.h>

#include "func.c"
#include "def.h"


pthread_mutex_t mutex_sens = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_dev = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_count = PTHREAD_MUTEX_INITIALIZER;


char s_sn[256][17], s_name[256][17], s_pl[256][17], counter_sn[256][17], *db,
     *txt_dir, d_pl[256][17], d_name[256][17];
_Bool debug = 0;
char com2[30], com3[30], com1[30], confnd[256];
float temp[256];
int stop=0, i=0, d_col=0, d_type[256], d_port[256], counter_tmp[256],
    sens_trs=0, rrd_trs=0, dev_trs=0, count_trs=0, gpio_state[256];
unsigned long int counter_data[256];
FILE *LOG_F;


//Функция завершения работы
int stop_file(void) {
    int d;
    if(stop == 2) exit(10);
    if(stop != 1) {
        stop = 1;
        ptolog( debug,  LOG_F, "\nЗавершение работы.");
        for(;;)
        {
            if(rrd_trs == 1 || sens_trs == 1 || count_trs == 1 || dev_trs == 1)
            {
                ptolog( debug,  LOG_F, "\n");
                sleep(3);
                if(debug == 1)ptolog( debug,  LOG_F, "\nВсе потоки остановлены.\n");
                remove("term.pid");
                stop=2;
                break;
            }
        }
    }
}

//Поток мониторинга температур
void* sensor_thread(void *arg)
{
    //if(debug == 1)ptolog( debug,  LOG_F, "\n%s\n", rrdu1);
    int d=0;
    for (;;) {
        pthread_mutex_lock( &mutex_sens );
        d=0;
        while (i != d) {
            temp[d] = gettemp(s_sn[d]);
            if (temp[d] == 6515)
                ptolog( debug,  LOG_F, "Error!\n Con't read temp!\n");
            if (temp[d] == 6514)
                ptolog( debug,  LOG_F, "CRC Error!\n");
            if (temp[d] == 6513)
                ptolog( debug,  LOG_F, "Error! Device [%s] not found!\n",s_sn[d]);
            d++;
        }
        if(debug == 1)
        {
            for(d=0; d!=i; d++)
            {
                ptolog( debug,  LOG_F, "Sens thread\n");
                ptolog( debug,  LOG_F, "Temp on sensor #%d, %s: %f.3\n", d, s_name[d], temp[d]);

            }
        }
        pthread_mutex_unlock( &mutex_sens );
        sleep(1);
        if(stop != 0)
        {
            ptolog( debug,  LOG_F, ".");
            sens_trs = 1;
            break;
        }
    }
    return NULL;
}

//Поток записи данных в базу
void* rrdrec_thread(void *arg)
{
    char rrdu1[strlen(CONFIG_DIR) + strlen(db) + 32], rrdut[(i * 7) + 1], f_name[28];

    sprintf(rrdu1, "/usr/bin/rrdtool update %s/%s.rrd N", CONFIG_DIR, db);

    char rrduc[strlen(rrdu1) + (i * 7) + 1], gpioc_state[d_col][17];

    int m = 0;

    for(;;)
    {
        pthread_mutex_lock( &mutex_sens );
        bzero(rrdut, (i * 7) + 1);
        m = 0;
        while (i != m) {
            if(debug == 1)ptolog( debug,  LOG_F, "\nTemp %d in %s: %.3f\n", m, s_name[m], temp[m]);
            if (temp[m] < 6510)
                sprintf(rrdut, "%s:%.3f", rrdut, temp[m]);
            if (temp[m] > 6510) {
                sprintf(rrdut, "%s:", rrdut);
                ptolog( debug,  LOG_F, "\nНевозможжно получить данные с датчика %d,", m);
            }

            sprintf(f_name, "sens%d.txt", m);
            if (temp[m] != 6514)write_txt_file(txt_dir, f_name, "%.3f", temp[m]);
            m++;
        }
        m = 0;

        pthread_mutex_lock(&mutex_count);
        while (d_col != m)
        {
            if(d_type[m] == 2)
            {
                sprintf(f_name, "device%d.txt", m);
                if(d_col != 456)write_txt_file(txt_dir, f_name, "%u",counter_data[m]);
            }
            m++;
        }
        pthread_mutex_unlock(&mutex_count);
        m = 0;
        pthread_mutex_lock( &mutex_dev );
        while (d_col != m)
        {
            if(d_type[m] != 2)
            {
                sprintf(f_name, "device%d.txt", m);
                if(d_col != 456)write_txt_file(txt_dir, f_name, "%d", gpio_state[m]);
            }
            m++;
        }
        pthread_mutex_unlock( &mutex_dev );
        sprintf(rrduc, "%s%s", rrdu1, rrdut);
        if(debug == 1)ptolog( debug,  LOG_F, "\nКоманда внесения в базу:%s\n", rrduc);
        if (strlen(rrdut) != 0)
            system(rrduc);
        if(debug == 1)ptolog( debug,  LOG_F, "\nrrd thread\n");
        pthread_mutex_unlock( &mutex_sens );
        sleep(2);
        if(stop != 0)
        {
            ptolog( debug,  LOG_F, ".");
            rrd_trs = 1;
            break;
        }
    }
    return NULL;
}


//Поток подсчета воды
void* con_thread(void *arg)
{
    int cory[d_col], g, c_col;
    c_col=d_col;
    for(;;)
    {

        pthread_mutex_lock(&mutex_count);
        if(debug == 1)ptolog( debug,  LOG_F, "Count thread");
        for(g=0; g!=c_col; g++)
        {
            if(d_type[g] == 2)
            {
                //counter_tmp[g]=digitalRead(g);
                if(debug == 1)ptolog( debug,  LOG_F, "Cor:%u, Now:%u\n", cory[g], counter_tmp[g]);
                if(counter_tmp[g] == 1 & counter_tmp[g] != cory[g])
                {
                    counter_data[g]++;
                    cory[g]=counter_tmp[g];
                }
                if(debug == 1) ptolog( debug,  LOG_F, "Текущии данные, холодная вода(%s):%u\n", counter_sn[g], counter_data[g]);
                if(counter_tmp[g] == 0 & counter_tmp[g] != cory[g]) cory[g]=counter_tmp[g];
            }
        }
        pthread_mutex_unlock(&mutex_count);
        if(stop != 0)
        {
            ptolog( debug,  LOG_F, ".");
            count_trs = 1;
            break;
        }
        sleep(1);
    }
    return NULL;
}

//Поток мониторинга состояния устройств
void* dev_thread(void *arg)
{
    int d;
    for(;;)
    {
        sleep(1);
        pthread_mutex_lock(&mutex_dev);
        for(d=0; d!=d_col; d++)
        {
            if(d_type[d] != 2)
            {   
                //gpio_state[d]=digitalRead(d);
	      if(debug==1)ptolog( debug,  LOG_F, "gpio_state=%d\n",gpio_state[d]);
            }
        }
        pthread_mutex_unlock(&mutex_dev);
        if(stop != 0)
        {
            ptolog( debug,  LOG_F, ".");
            dev_trs = 1;
            break;
        }
    }
    return NULL;
}


int termd(_Bool gdebug,
    _Bool dem,
    _Bool no_write,
    char *config_file,
    char *log_file)
{

    dictionary * conf;
    int d = 0, chk = 0, pid = 0;
    char flog[100];

    pthread_attr_t attr;
    pthread_t info;
    pthread_attr_init(&attr);

    void (*TermSignal)(int);
    TermSignal = signal(SIGTERM, stop_file);

    void (*IntSignal)(int);
    IntSignal = signal(SIGINT, stop_file);
    
    void (*QuitSignal)(int);

    QuitSignal = signal(SIGQUIT, stop_file);

    
    sprintf(flog, "%s/%s", CONFIG_DIR, LOG_FILE);
   
    LOG_F = fopen( flog, "at");
    
    /*if (wiringPiSetup() == -1)
    {
        ptolog( debug,  LOG_F, "Не обнаружина библиотека WiringPi или невозможно её иницилизировать.\nВыход...\n");
        exit (1);
    }*/
    pid = getpid();
    ptolog( debug,  LOG_F,"\nPid:%d\n",pid);
    write_txt_file(CONFIG_DIR, "term.pid", "%d", pid);
    sprintf(confnd, "%s/%s", CONFIG_DIR, CONFIG_FILE);
    conf = iniparser_load(confnd);
    debug = iniparser_getint(conf, "main:debug", 0);
    if(gdebug == 1) debug = 1;
    if(iniparser_getstring(conf, "t_sensors", "error") == "error") chk = 1;
    if(iniparser_getstring(conf, "main", "error") == "error") chk = 1;
    if ( chk != 0 ) {
        ptolog( debug,  LOG_F, "\nПроблема, некоректный файл конфигурации.\n");
        sleep(3);
        return 20;
        sprintf(com1, "rm -f %s", CONFIG_FILE);
        system(com1);
        iniparser_freedict(conf);
        if (cre_conf(CONFIG_DIR, CONFIG_FILE) != 0) {
            perror("\n Ошибка создания файла конфигурации");
            exit(2);
        }
        conf = iniparser_load(confnd);
    }
    i = iniparser_getint(conf, "t_sensors:sens_col", 456); //Загрузка количества датчиков
    if (i == 456) {
        perror("Not found sensors!\n");
        exit(3);
    }
    //Загрузка данных в память
    d = 0;

    txt_dir = iniparser_getstring(conf, "main:txt_dir", "/tmp/");

    db = iniparser_getstring(conf, "main:db_name", "none");

    while (i != d) {
        sprintf(com3, "t_sensors:sens%d", d);
        sprintf(s_sn[d], "%s", iniparser_getstring(conf, com3, "none"));
        sprintf(com3, "t_sensors:sens%d_name", d);
        sprintf(s_name[d], "%s", iniparser_getstring(conf, com3, "none"));
        sprintf(com3, "t_sensors:sens%d_place", d);
        sprintf(s_pl[d], "%s", iniparser_getstring(conf, com3, "none"));
        if(debug == 1)ptolog( debug,  LOG_F, "\nSensor№%d\n SN:%s \n Name:%s \n", d, s_sn[d] , s_name[d] );
        d++;
    }

    d_col = iniparser_getint(conf, "dev:device_col", 456); //Загрузка количества устройств
    if (d_col == 456) {
        ptolog( debug,  LOG_F, "Not found device!\n");
        //exit(3);
    }
    d=0;
    while (d_col != d) {
        sprintf(com3, "dev:device%d_name", d);
        sprintf(d_name[d], "%s", iniparser_getstring(conf, com3, "none"));
        sprintf(com3, "dev:device%d_place", d);
        sprintf(d_pl[d], "%s", iniparser_getstring(conf, com3, "none"));
        sprintf(com3, "dev:device%d_port", d);
        d_port[d] = iniparser_getint(conf, com3, 456);
        sprintf(com3, "dev:device%d_type", d);
        d_type[d] = iniparser_getint(conf, com3, 456);
        sprintf(com3, "dev:device%d_sn", d);

        if(d_port[d]<= 20)
        {
            if(d_type != 2)
            {
                sprintf(com3,"echo \"%d\" > /sys/class/gpio/export", d);
                system(com3);
                sprintf(com3,"echo \"out\" > /sys/class/gpio/gpio%d/direction", d);
                system(com3);
            }
            //else pinMode(d_port[d], INPUT);
        }

        sprintf(counter_sn[d], iniparser_getstring(conf, com3, "none"));
        sprintf(com3, "count:counter%d", d);
        counter_data[d] = atol(iniparser_getstring(conf, com3, "456"));
        d++;
    }
    //Запуск потоков
    if (pthread_create(&info, NULL, sensor_thread, NULL) != 0)
    {
        ptolog( debug,  LOG_F, "Unable to start sensors thread\n");
        exit(3);
    }
    if (pthread_create(&info, NULL, rrdrec_thread, NULL) != 0)
    {
        ptolog( debug,  LOG_F, "Unable to start RRD Update thread\n");
        exit(2);
    }
    if (pthread_create(&info, NULL, con_thread, NULL) != 0)
    {
        ptolog( debug,  LOG_F, "Unable to start counter thread\n");
        exit(2);
    }
    if (pthread_create(&info, NULL, dev_thread, NULL) != 0)
    {
        ptolog( debug,  LOG_F, "Unable to start device thread\n");
        exit(2);
    }

    //Главный цикл
    while(stop == 0)
    {
        sleep(1);
    }
    sleep(30);

    pthread_mutex_lock( &mutex_count );
    iniparser_set(conf, "count", NULL);
    for(d=0; d != d_col; d++)
    {
        sprintf(com2, "%u", counter_data[d]);
        sprintf(com1, "count:counter%d", d);
        if(debug == 1)ptolog( debug,  LOG_F, "#%d=%u\n",d , counter_data[d]);
        iniparser_set(conf, com1, com2);
    }
    iniparser_save(conf, confnd);
    iniparser_freedict(conf);

    pthread_mutex_unlock( &mutex_count );
    return 0;
}
