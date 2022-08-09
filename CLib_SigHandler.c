#include "CLibrary.h"

void SigHandler(int dummy) {
  state_ = EXITING;
  print_time();
  fprintf(error_log_, "SIGINT Recieved, exiting code!\n");
  printf("\n");
  return;
}