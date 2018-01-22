/*
 * term_no_den.c
 *
 *  Created on: 13.02.2013
 *      Author: dgok50
 */

#include "term.c"
#include <stdio.h>



int main(int argc, char *argv[])
{   int exit=0, i=0;
    _Bool df;
    
    _Bool debug;
    _Bool dem;
    _Bool no_write;
    char *config_file;
    char *log_file;

    
    while(i!=argc)
    {
        printf("\narg[%d]:%s",i,argv[i]);
        i++;
        if(!strcmp(argv[i], "--debug") && !strcmp(argv[i], "-d") ) debug = 1;
        if(argv[i] == "--config") config_file = argv[i+1];
        if(argv[i] == "--log") log_file = argv[i+1];
        if(argv[i] == "--dem" && argv[i] == "-d") dem=1;
        if(argv[i] == "--no_db" && argv[i] == "-wd") no_write=1;
    }

    printf("\n debug:%d, config:%d, log:%d, dem:%d, wd:d",debug,config_file,log_file,dem,no_write);
    exit=termd(debug, dem, no_write, config_file, log_file);
    return exit;
}
