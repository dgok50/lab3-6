#include <stdio.h>
#include <stdlib.h>
#include <string.h>
//#include <stdarg.h>
#include <curl/curl.h>

struct string
{
  char *ptr;
  size_t len;
};

void init_string (struct string *s)
{
  s->len = 0;
  s->ptr = malloc (s->len + 1);
  if (s->ptr == NULL)
    {
      fprintf (stderr, "malloc() failed\n");
      exit (EXIT_FAILURE);
    }
  s->ptr[0] = '\0';
}

size_t writefunc (void *ptr, size_t size, size_t nmemb, struct string *s)
{
  size_t new_len = s->len + size * nmemb;
  s->ptr = realloc (s->ptr, new_len + 1);
  if (s->ptr == NULL)
    {
      fprintf (stderr, "realloc() failed\n");
      exit (EXIT_FAILURE);
    }
  memcpy (s->ptr + s->len, ptr, size * nmemb);
  s->ptr[new_len] = '\0';
  s->len = new_len;

  return size * nmemb;
}

int get_ups_data (double *vin, double *load, double *frqin, double *upsbat)
{

  CURL *curl;
  CURLcode res;
  //return 1;
  curl = curl_easy_init ();
  if (curl)
    {
      struct string s;
      init_string (&s);

      curl_easy_setopt (curl, CURLOPT_URL,
			"http://192.168.0.114:8000/ups.txt");
      curl_easy_setopt (curl, CURLOPT_WRITEFUNCTION, writefunc);
      curl_easy_setopt (curl, CURLOPT_WRITEDATA, &s);
      res = curl_easy_perform (curl);
      int n = 0;
      char *pch = s.ptr, *delim = "\n", *token;
      //syslog(LOG_NOTICE,"String: %s size: %d", s.ptr, s.len);
      n = 0;
      double tmp;
      while (token = strtok_r (pch, "\n#", &pch))
	{
	  tmp = atof (token);
	  //syslog(LOG_NOTICE,"Tmp: %d", tmp);
	  if (n == 0)
	    {
	      *vin = tmp + 0;
	    }
	  if (n == 2)
	    {
	      *load = tmp + 0;
	    }
	  if (n == 3)
	    {
	      *frqin = tmp + 0;
	    }
	  if (n == 5)
	    {
	      *upsbat = tmp + 0;
	    }
	  n++;
	}

/*    char * pch;
    fprintf(stderr,"String: %s\n size: %d \n", s.ptr, strlen(s.ptr));
	if(strlen(s.ptr) < 20) {
		return 1;
	}
    pch = strtok(s.ptr,"\n");
    int tmp = 0;
    while(pch != NULL) {
		tmp = atoi(pch);
		if(n==0) { vin = &tmp;}
		if(n==2) { load = &tmp;}
		if(n==3) { frqin = &tmp;}
		if(n==5) { upsbat = &tmp;}
		pch = strtok(NULL, "\n");
		n++;
    }*/
      if (n <= 5)
	return 1;
      //syslog(LOG_NOTICE,"vin=%fv load=%fprc frqin=%fhz upsbat=%fprc",*vin,*load,*frqin,*upsbat);
      free (s.ptr);

      /* always cleanup */
      curl_easy_cleanup (curl);
      return 0;
    }
  return 1;
}
