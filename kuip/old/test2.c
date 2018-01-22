#include <stdio.h>
#include <wiringPi.h>
#include <stdlib.h>
#include <string.h>
#include <iniparser.h>
#include <termios.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/time.h>
#include <dirent.h>
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <ctype.h>
#include <signal.h>

int main()
{
    int f=0, g=0;
    for(f=0; f<=20; f++)
    {
        printf("%d",f);
        pinMode(f, INPUT);
    }
    for(;;)
    {
        for(f=0; f<=20; f++)
        {
            g=digitalRead(f);
            printf("\npin #%d:%d", f, g);
        }
        printf("\033[2J");
        printf("\033[0;0f");
        sleep(1);
    }
    return 0;
}
