#ifndef BUILD_STAMP
#define BUILD_STAMP __DATE__ " " __TIME__
#endif

#include <stdio.h>        // for printf
#include <time.h>         // for ctime, time, time_t
#include "globals.h"      // for MM15_VERSION
#include "splash.h"       // for splash_screen

void splash_screen(void) {
  time_t now = time(NULL);
  char* started = ctime(&now);
  
  printf("\n");
  printf("╔══════════════════════════════════════════════╗\n");
  printf("║                                              ║\n");
  printf("║     Mico's Matrix & Scalar RPN Calculator    ║\n");
  printf("║             Version %s (2026)               ║\n", MM15_VERSION);
  printf("║                                              ║\n");
  printf("║  > Enter RPN expressions                     ║\n");
  printf("║  > Type 'help' for commands                  ║\n");
  printf("║  > Press 'q' or ctrl+d to quit               ║\n");
  printf("║                                              ║\n");
  printf("╚══════════════════════════════════════════════╝\n");

  printf("mm_15 git:     %s\n", VERSION);
  printf("Built:         %s\n", BUILD_STAMP);
  printf("Started:       %s", started);   // ctime() includes newline
  printf("\n");
}

