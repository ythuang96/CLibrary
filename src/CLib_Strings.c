#include "CLibrary.h"

/* Function to initialize a string structure */
void init_string(string_t *s) {
  /* allocate string */
  s->len = 0;
  s->ptr = malloc(s->len+1);
  /* Check allocation */
  if (s->ptr == NULL) {
    fprintf(stderr, "malloc() failed\n");
    exit(EXIT_FAILURE);
  }
  /* Set to empty string */
  s->ptr[0] = '\0';
  return;
}

/*
 * Function to write curl response to a string structure
 * It will delete any previously sorted data
 */
size_t writefunc(void *ptr, size_t size, size_t nmemb, string_t *s) {
  /* Reallocate with new size */
  /* size_t new_len = s->len + size*nmemb; */
  size_t new_len = size*nmemb;
  s->ptr = realloc(s->ptr, new_len+1);
  /* Check reallocation */
  if (s->ptr == NULL) {
    fprintf(stderr, "realloc() failed\n");
    exit(EXIT_FAILURE);
  }

  /* write to string */
  memcpy(s->ptr, ptr, size*nmemb);
  s->ptr[new_len] = '\0';
  s->len = new_len;

  return size*nmemb;
}

/*
 * Function to get the last octent of an IP address and return as an integer
 * Arguement:
 *   ip_add: [string, Input] the number-and-dot notation of the IP address
 * Return:
 *   digits: [int] the last octent as an integer
 */
int last_ip_digit( char ip_add[] ) {
  char *ptr;

  /* search for substring from the back */
  ptr = strrchr( ip_add, '.' );

  /* convert to integer and return */
  return atoi( ptr+1 );
}