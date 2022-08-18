#include "CLibrary.h"

void print_time(void) {
  time_t rawtime;
  struct tm * timeinfo;
  time ( &rawtime );
  timeinfo = localtime ( &rawtime );
  fprintf(error_log_, "%02d/%02d/%02d %02d:%02d:%02d  ", \
      (*timeinfo).tm_mon+1, (*timeinfo).tm_mday, \
      (*timeinfo).tm_year-100, (*timeinfo).tm_hour, \
      (*timeinfo).tm_min, (*timeinfo).tm_sec );
  return;
}


/* Sleep for nanoseconds */
void nsleep(uint64_t ns) {
	struct timespec req,rem;
	req.tv_sec = ns/1000000000;
	req.tv_nsec = ns%1000000000;
	/* loop untill nanosleep sets an error or finishes successfully */
	errno=0; /* reset errno to avoid false detection */
	while(nanosleep(&req, &rem) && errno==EINTR) {
		req.tv_sec = rem.tv_sec;
		req.tv_nsec = rem.tv_nsec;
	}
	return;
}


int current_time(void) {
  int timeint;
  time_t rawtime;
  struct tm * timeinfo;

  /* Get current time */
  time ( &rawtime );
  timeinfo = localtime ( &rawtime );
  /* Convert hour and minute */
  timeint = (*timeinfo).tm_hour*100 + (*timeinfo).tm_min;
  return timeint;
}

