// config_path.h (single-file helper; just #include it in one .c)
#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#include <unistd.h>
#include <pwd.h>

#ifndef APP_NAME
#define APP_NAME "mm_15"
#endif

static int mkdir_p(const char *dir) {
    char tmp[PATH_MAX];
    size_t len = strnlen(dir, PATH_MAX);
    if (len == 0 || len >= PATH_MAX) return -1;
    memcpy(tmp, dir, len + 1);
    for (char *p = tmp + 1; *p; ++p) {
        if (*p == '/') { *p = '\0'; if (mkdir(tmp, 0700) && errno != EEXIST) return -1; *p = '/'; }
    }
    if (mkdir(tmp, 0700) && errno != EEXIST) return -1;
    return 0;
}

static const char *home_dir(void) {
    const char *h = getenv("HOME");
    if (h && *h) return h;
    struct passwd *pw = getpwuid(getuid());
    return (pw && pw->pw_dir) ? pw->pw_dir : NULL;
}

// Fill out with ~/.config/mm_15/config.txt or $XDG_CONFIG_HOME/mm_15/config.txt
static int resolve_config_path(char out[PATH_MAX]) {
    const char *xdg = getenv("XDG_CONFIG_HOME");
    if (xdg && *xdg) {
        if (snprintf(out, PATH_MAX, "%s/%s/config.txt", xdg, APP_NAME) >= PATH_MAX) return -1;
        return 0;
    }
    const char *home = home_dir();
    if (!home) return -1;
    if (snprintf(out, PATH_MAX, "%s/.config/%s/config.txt", home, APP_NAME) >= PATH_MAX) return -1;
    return 0;
}

// Ensure the parent dir exists (mkdir -p)
static int ensure_parent_dir(const char *path) {
    char dir[PATH_MAX];
    size_t n = strnlen(path, PATH_MAX);
    if (n == 0 || n >= PATH_MAX) return -1;
    memcpy(dir, path, n + 1);
    char *slash = strrchr(dir, '/');
    if (!slash) return 0;
    *slash = '\0';
    return mkdir_p(dir);
}
