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
#include "function_list.h"
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

#define BOLD      "\033[1m"
#define ITALIC    "\033[3m"
#define UNDERLINE "\033[4m"
#define RESET     "\033[0m"

#define title(s) printf(BOLD s RESET "\n")
#define subtitle(s) printf(UNDERLINE s RESET "\n")

void help_menu(void) {
  //  printf("\n\n_mico's toy Matrix and Scalar RPN Calculator\n");
  printf("\n");
  title("RPN Calculator for real and complex scalars and matrices");
  subtitle("Quick Start and Entering data");
  printf("    All inputs are case sensitive. Enter strings as \"string\".\n");
  printf("    Enter complex numbers as in: (1,3) or (-1.2e-4, 0.7e2).\n");
  printf("    Enter inline matrices as in J language [#rows #cols $ values]. \n");
  printf("    Example: [2 2 $ -1 2 5 1]. Matrix entries can be real or complex.\n");
  printf("    Read matrix from file as [#rows, #cols, \"filename\"].\n");
  printf("    You can undo the last line entry with undo.\n");
  subtitle("Stack manipulations");
  printf("    drop, dup, swap, clst, nip, tuck, roll, over\n");
  subtitle("Math functions");
  printf("    Math functions work on scalars and matrices wherever possible. \n");
  printf("    Basic stuff: +, -, *, /, ^,  ln, exp, log, chs, inv, pct, pctchg \n");
  printf("    Trigonometry: sin, cos, tan, asin, acos, atan\n");
  printf("    Hyperbolic: sinh, cosh, tanh, asinh, acosh, atanh\n");
  printf("    Polynomials: evaluation and zeros\n");
  printf("    Normal distribution: npdf, ncdf, nquant {quantiles}\n");
  printf("    Special functions: gamma, ln_gamma, beta, ln_beta\n");
  subtitle("Comparison and logic functions");
  printf("    eq, leq, lt, gt, geq, neq, and,  or, not\n");
  subtitle("Complex numbers");
  printf("    re, im, abs, arg, re2c, split_c, j2r {join 2 reals into complex}\n");
  subtitle("Constants");
  printf("    pi, e, gravity, inf, nan\n");
  subtitle("Matrix functions");
  printf("    Get individual matrix elements with get_aij; set them with set_aij.\n");
  printf("    Print the matrix on top of the stack with pm \n");  
  printf("    Special matrices: eye, ones, rand, randn, rrange.\n");  
  printf("    Manipulation: reshape, diag, to_diag, split_mat, join_h, join_v \n");
  printf("    Cummulative sums and products: cumsum_r, cumsum_c, cumprod_r, cumprod_c \n");  
  printf("    Basic matrix statistics: csum, rsum, cmean, rmean, cvar, rvar\n");  
  printf("    Matrix min and max: cmin, rmin, cmax, rmax\n");  
  printf("    Linear algebra: tran, {also '}, det, minv, pinv, chol, eig, svd\n");  
  subtitle("Register functions");
  printf("    sto, rcl, pr {print registers}, save, load, ffr {1st free register} \n");
  subtitle("String functions");
  printf("    scon, s2u, s2l, slen, srev, int2str, eval {evaluate string}\n");
  subtitle("Financial and date functions");
  printf("    npv, irr, ddays, dateplus, today, dow, num2date \n");
  subtitle("Output format options");
  printf("    setprec {set print precision}, sfs {fix<->sci}\n");
  subtitle("Help and utilities");
  printf("    listfcns {list built in functions}\n");
  printf("    listmacros {list predefined macros}\n");
  printf("    listwords {list user-defined words}\n");
  printf("    new words start with : end with ;\n");
  printf("    Example to compute square : sq dup * ;\n");
  printf("\n");
  skip_stack_printing = true;
}

void list_all_functions(void) {
  printf("Built-in functions:\n\n");
  int count = 0;
  for (int i = 0; function_names[i] != NULL; ++i) {
    printf("%-16s", function_names[i]);  // left-align in 16-char width
    count++;
    if (count % 4 == 0)
      printf("\n");
  }
  if (count % 4 != 0)
    printf("\n");  // final newline if last line wasn't complete
}

// Compare function for qsort
int compare_strings(const void* a, const void* b) {
  const char* sa = *(const char**)a;
  const char* sb = *(const char**)b;
  return strcmp(sa, sb);
}

void list_all_functions_sorted(void) {
  // Step 1: Count functions
  int count = 0;
  while (function_names[count] != NULL) count++;

  // Step 2: Copy to a temporary array
  const char** sorted = malloc(count * sizeof(char*));
  if (!sorted) {
    fprintf(stderr, "Memory allocation failed\n");
    return;
  }
  for (int i = 0; i < count; ++i)
    sorted[i] = function_names[i];

  // Step 3: Sort
  qsort(sorted, count, sizeof(char*), compare_strings);

  // Step 4: Print 6 per row
  printf("Built-in functions:\n\n");
  for (int i = 0; i < count; ++i) {
    printf("%-16s", sorted[i]);
    if ((i + 1) % 6 == 0)
      printf("\n");
  }
  if (count % 6 != 0)  printf("\n");
  printf("\n");

  // Cleanup
  free(sorted);
}
