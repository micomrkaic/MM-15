#define _POSIX_C_SOURCE 200112L
#ifdef __APPLE__
#define _DARWIN_C_SOURCE
#include <sys/types.h>
#include <sys/sysctl.h>
#endif

#ifndef BUILD_STAMP
#define BUILD_STAMP __DATE__ " " __TIME__
#endif

#include <ctype.h>        // for isalnum
#include <stdio.h>        // for printf, snprintf, fclose, fgets, sscanf, fopen
#include <stdlib.h>       // for system
#include <string.h>       // for strstr, strchr, strlen, strncmp
#include <sys/utsname.h>  // for utsname, uname
#include <sys/wait.h>     // for WEXITSTATUS, WIFEXITED
#include <time.h>         // for ctime, time, time_t
#include <unistd.h>       // for NULL, gethostname
#include "splash.h"       // for splash_screen

static int read_cmd_line(const char *cmd, char *out, size_t outsz) {
  if (!out || outsz == 0) return -1;
  out[0] = '\0';

  FILE *p = popen(cmd, "r");
  if (!p) return -1;

  if (!fgets(out, (int)outsz, p)) {
    out[0] = '\0';
    int st = pclose(p);
    (void)st;
    return -1;
  }

  int status = pclose(p);
  if (status == -1) return -1;
  if (!WIFEXITED(status) || WEXITSTATUS(status) != 0) return -1;

  return 0;
}

static void chomp_newline(char *s) {
  if (!s) return;
  size_t n = strlen(s);
  while (n > 0 && (s[n-1] == '\n' || s[n-1] == '\r')) {
    s[--n] = '\0';
  }
}

static int run_cmd(const char *cmd) {
  int rc = system(cmd);
  if (rc == -1) return -1;
  if (WIFEXITED(rc) && WEXITSTATUS(rc) == 0) return 0;
  return -1;
}

static void print_machine_info(void) {
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

static void get_ip(char* buffer, size_t size) {
  if (run_cmd("curl -s https://api.ipify.org > /tmp/ip.txt") != 0) {
    snprintf(buffer, size, "ip: n/a");
    return;
  }

  FILE *f = fopen("/tmp/ip.txt", "r");
  if (!f) {
    snprintf(buffer, size, "ip: n/a");
    return;
  }
  
  if (!fgets(buffer, size, f)) {
    snprintf(buffer, size, "ip: n/a");
    fclose(f);
    return;
  }
  fclose(f);
}

static void get_location(const char *ip) {
    if (!ip || *ip == '\0') {
      printf("📍 Location: n/a\n");
        return;
    }

    // Basic sanity: reject obviously insane input to avoid shell injection.
    // Accept digits, dots, colons, hex (IPv6), and a few safe chars.
    for (const char *p = ip; *p; p++) {
        unsigned char c = (unsigned char)*p;
        if (!(isalnum(c) || c == '.' || c == ':' )) {
            printf("📍 Location: n/a\n");
            return;
        }
    }

    char cmd[512];
    // Use https; ip-api supports https on some endpoints; if yours requires http, switch back.
    // Add -f so curl fails on HTTP errors.
    snprintf(cmd, sizeof(cmd),
             "curl -fsS 'http://ip-api.com/json/%s?fields=city,regionName,country,status,message'",
             ip);

    char line[2048];
    if (read_cmd_line(cmd, line, sizeof(line)) != 0) {
        printf("📍 Location: n/a\n");
        return;
    }
    chomp_newline(line);

    char city[64] = {0}, region[64] = {0}, country[64] = {0};

    // Very lightweight JSON scraping; bounded to avoid overruns.
    // Note: ip-api uses "regionName" (camelCase) in its JSON.
    char *ptr;
    if ((ptr = strstr(line, "\"city\"")) != NULL) {
        (void)sscanf(ptr, "\"city\":\"%63[^\"]\"", city);
    }
    if ((ptr = strstr(line, "\"regionName\"")) != NULL) {
        (void)sscanf(ptr, "\"regionName\":\"%63[^\"]\"", region);
    }
    if ((ptr = strstr(line, "\"country\"")) != NULL) {
        (void)sscanf(ptr, "\"country\":\"%63[^\"]\"", country);
    }

    if (city[0] || region[0] || country[0]) {
        printf("📍 Location: %s%s%s%s%s\n",
               city[0] ? city : "",
               (city[0] && (region[0] || country[0])) ? ", " : "",
               region[0] ? region : "",
               (region[0] && country[0]) ? ", " : "",
               country[0] ? country : "");
    } else {
        fprintf(stderr, "📍 Location: n/a\n");
    }
}

static void get_weather(char *weather, size_t size) {
  if (!weather || size == 0) return;
  
  // -f: fail on HTTP errors, -sS: silent but still show errors to stderr
  // wttr.in returns a single line with format=3
  if (read_cmd_line("curl -fsS 'https://wttr.in?format=3'", weather, size) != 0) {
    snprintf(weather, size, "Weather: n/a");
    return;
  }
  chomp_newline(weather);
}

static void snazz(void) {
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
  printf("║             Version 1.0 (2026)               ║\n");
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
  print_machine_info();
  snazz();
  printf("\n");
}

