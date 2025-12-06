#define _POSIX_C_SOURCE 200112L
#ifdef __APPLE__
#define _DARWIN_C_SOURCE
#include <sys/types.h>
#include <sys/sysctl.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/utsname.h>
#include <time.h>
#include "globals.h"


void whose_place(void) {
  printf("Your place or mine?\n");
  return;
}

void print_machine_info(void) {
  char hostname[256];
  struct utsname sysinfo;

  // Get hostname
  if (gethostname(hostname, sizeof(hostname)) == 0) {
    printf("🖥️ Hostname: %s\n", hostname);
  }

  // Get uname info
  if (uname(&sysinfo) == 0) {
    printf("📀 OS: %s %s\n", sysinfo.sysname, sysinfo.release);
    printf("💾 Arch: %s\n", sysinfo.machine);
  }

#if defined(__linux__)
  // Linux: read from /proc/cpuinfo
  FILE* f = fopen("/proc/cpuinfo", "r");
  if (f) {
    char line[256];
    while (fgets(line, sizeof(line), f)) {
      if (strncmp(line, "model name", 10) == 0) {
	char* model = strchr(line, ':');
	if (model) {
	  model++;
	  while (*model == ' ') model++;
	  printf("⚙️ CPU: %s", model); // already includes \n
	  break;
	}
      }
    }
    fclose(f);
  }
#elif defined(__APPLE__)
  // mac_os: use sysctl

  char cpu_model[256];
  size_t size = sizeof(cpu_model);
  if (sysctlbyname("machdep.cpu.brand_string", &cpu_model, &size, NULL, 0) == 0) {
    printf("⚙️ CPU: %s\n", cpu_model);
  }
#endif
}

void get_ip(char* buffer, size_t size) {
  system("curl -s https://api.ipify.org > /tmp/ip.txt");
  FILE* f = fopen("/tmp/ip.txt", "r");
  if (f) {
    fgets(buffer, size, f);
    fclose(f);
  }
}

void get_location(const char* ip) {
  char cmd[1024];
  snprintf(cmd, sizeof(cmd), "curl -s http://ip-api.com/json/%s > /tmp/location.json", ip);
  system(cmd);

  FILE* f = fopen("/tmp/location.json", "r");
  if (!f) return;

  char line[1024];
  fgets(line, sizeof(line), f);  // Just read the one JSON line

  char city[64] = {0}, region[64] = {0}, country[64] = {0};

  char *ptr;

  if ((ptr = strstr(line, "\"city\""))) {
    sscanf(ptr, "\"city\":\"%[^\"]\"", city);
  }
  if ((ptr = strstr(line, "\"region_name\""))) {
    sscanf(ptr, "\"region_name\":\"%[^\"]\"", region);
  }
  if ((ptr = strstr(line, "\"country\""))) {
    sscanf(ptr, "\"country\":\"%[^\"]\"", country);
  }
    
  printf("📍 Location: %s, %s, %s\n", city, region, country);

  fclose(f);
}

void get_weather(char* weather, size_t size) {
  system("curl -s wttr.in?format=3 > /tmp/weather.txt");
  FILE* f = fopen("/tmp/weather.txt", "r");
  if (f) {
    fgets(weather, size, f);
    fclose(f);
  }
}

void snazz(void) {
  char ip[64] = {0};
  char weather[128] = {0};

  get_ip(ip, sizeof(ip));
  get_location(ip);
  get_weather(weather, sizeof(weather));

  printf("🌐 IP: %s\n", ip);
  printf("☁️ Weather: %s", weather);
}


void splash_screen(void) {
  time_t now = time(NULL);
  char* started = ctime(&now);
  
  printf("\n");
  printf("╔══════════════════════════════════════════════╗\n");
  printf("║                                              ║\n");
  printf("║     Mico's Matrix & Scalar RPN Calculator    ║\n");
  printf("║          Version beta 0.2  (2025)            ║\n");
  printf("║        mm_15 version: %s          ║\n", VERSION);
  printf("║                                              ║\n");
  printf("║  > Enter RPN expressions                     ║\n");
  printf("║  > Type 'help' for commands                  ║\n");
  printf("║  > Press 'q' or ctrl+d to quit               ║\n");
  printf("║                                              ║\n");
  printf("╚══════════════════════════════════════════════╝\n");
  printf("         Started on: %s", started);  // already has newline
  printf("\n");
  print_machine_info();
  snazz();
  printf("\n");
}

