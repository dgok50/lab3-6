/*
 * thread.c
 *
 *  Created on: 21.02.2013
 *      Author: dgok50
 */


#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <ctype.h>
#include <func.c>

static int x=0;

char* te;
char** ts;
int f=0;

void* aang112_thread(void *arg)
{

    while ( x==0 )
    {
        printf("I'm New Thread1!\n");
        sleep(1);
    }

    printf("Thread EXIT\n");
    return NULL;
    free(te);
}

void* test_thread(void *arg)
{
    int i;
    while ( x==0 )
    {
        printf("I'm New Thread2!\nTe:%s\n",te);
        sleep(3);
    }

    printf("Thread EXIT\n");
    return NULL;
    for(i=0; f < i; i++) free(ts[u]);
}

int main()
{
    pthread_attr_t attr;
    pthread_t info;
    pthread_attr_init(&attr);
    int t=0;
    printf("Enter leg:");
    scanf("%d",&t);
    te = (char *)malloc(t*sizeof(char));
    ts = (char **)malloc(t*sizeof(char))

         printf("Enter word:");
    scanf("%s",te);

    ts=(char *)malloc(t*sizeof(char));
    printf("Enter word:");
    scanf("%s",te);

    if (pthread_create(&info, NULL, aang112_thread, NULL) != 0)
    {
        printf("Unable to start thread\n");
    }

    if (pthread_create(&info, NULL, test_thread, NULL) != 0)
    {
        printf("Unable to start thread\n");
    }


    sleep(10);

    x=1;

    sleep(3);

    printf("MAIN EXIT\n");
    return 0;
}
