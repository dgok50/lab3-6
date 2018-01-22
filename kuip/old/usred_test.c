#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "usred.c"

struct usred test;


int main() {
    int i;
    double tmp=0, tsr=2;
    init_usred(&test, 10, 2);
    while(1) {
	printf("Enter [%u]:",test.data_index);
	scanf("%lf", &tmp);
	tsr = tmp;
	if(tmp == 219.0)
	    break;
	if(tmp == 322.0) {
	    //printf("reinit test");
	    free_usred(&test);
	    //while (1) 
	    //init_usred(&test, 10, 2);
	}
	write_usred(&test, 2, &tmp, &tsr);
	printf("Usred data tmp:%f data_redy:%d data_index: %u\n", tmp, test.data_redy, test.data_index);
	
    }
    free_usred(&test);
    return 0;
}