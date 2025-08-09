#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <time.h>
#include <math.h>  // for fabs()
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "stack.h"

int extract_day_month_year(Stack* stack) {
    if (stack->top < 0) {
        fprintf(stderr, "Error: Stack underflow\n");
        return 1;
    }

    stack_element date_elem = stack->items[stack->top--];

    if (date_elem.type != TYPE_STRING) {
        fprintf(stderr, "Error: Expected string date in DD.MM.YYYY format\n");
        return 1;
    }

    int day, month, year;
    if (sscanf(date_elem.string, "%d.%d.%d", &day, &month, &year) != 3) {
        fprintf(stderr, "Error: Invalid date format. Expected DD.MM.YYYY\n");
        return 1;
    }

    // Push year
    stack_element y = { .type = TYPE_REAL, .real = year };
    stack->items[++stack->top] = y;

    // Push month
    stack_element m = { .type = TYPE_REAL, .real = month };
    stack->items[++stack->top] = m;

    // Push day
    stack_element d = { .type = TYPE_REAL, .real = day };
    stack->items[++stack->top] = d;

    return 0;
}


int date_plus_days(Stack* stack) {
    if (stack->top < 1) {
        fprintf(stderr, "Error: Need a date string and number of days on stack\n");
        return 1;
    }

    stack_element days_elem = stack->items[stack->top--];
    stack_element date_elem = stack->items[stack->top--];

    if (date_elem.type != TYPE_STRING || (days_elem.type != TYPE_REAL && days_elem.type != TYPE_COMPLEX))
      {
        fprintf(stderr, "Error: Expected a string and a number\n");
        return 1;
      }

    int day, month, year;
    if (sscanf(date_elem.string, "%d.%d.%d", &day, &month, &year) != 3) {
        fprintf(stderr, "Error: Invalid date format. Expected DD.MM.YYYY\n");
        return 1;
    }

    struct tm date = {0};
    date.tm_mday = day;
    date.tm_mon = month - 1;
    date.tm_year = year - 1900;
    date.tm_hour = 12; // avoid DST issues

    time_t t = mktime(&date);
    if (t == (time_t)(-1)) {
        fprintf(stderr, "Error: mktime failed\n");
        return 1;
    }

    int delta = (int)(days_elem.type == TYPE_COMPLEX ? GSL_REAL(days_elem.complex_val) : days_elem.real);
    t += delta * 86400;  // seconds per day

    struct tm* new_date = localtime(&t);
    if (!new_date) {
        fprintf(stderr, "Error: localtime failed\n");
        return 1;
    }

    char buffer[64];
    snprintf(buffer, sizeof(buffer), "%02d.%02d.%04d",
             new_date->tm_mday,
             new_date->tm_mon + 1,
             new_date->tm_year + 1900);

    char* result = strdup(buffer);
    if (!result) {
        fprintf(stderr, "Error: Memory allocation failed\n");
        return 1;
    }

    stack_element out;
    out.type = TYPE_STRING;
    out.string = result;

    if (stack->top >= STACK_SIZE - 1) {
        fprintf(stderr, "Error: Stack overflow\n");
        free(result);
        return 1;
    }

    stack->items[++stack->top] = out;
    return 0;
}


int push_weekday_name_from_date_string(Stack* stack) {
    if (stack->top < 0) {
        fprintf(stderr, "Error: Stack underflow\n");
        return 1;
    }

    stack_element elem = stack->items[stack->top--];

    if (elem.type != TYPE_STRING) {
        fprintf(stderr, "Error: Expected string date in DD.MM.YYYY format\n");
        return 1;
    }

    int day, month, year;
    if (sscanf(elem.string, "%d.%d.%d", &day, &month, &year) != 3) {
        fprintf(stderr, "Error: Invalid date format. Expected DD.MM.YYYY\n");
        return 1;
    }

    struct tm date = {0};
    date.tm_mday = day;
    date.tm_mon = month - 1;       // tm_mon is 0-based
    date.tm_year = year - 1900;    // tm_year is years since 1900
    date.tm_hour = 12;             // avoid DST ambiguity

    if (mktime(&date) == (time_t)(-1)) {
        fprintf(stderr, "Error: mktime failed\n");
        return 1;
    }

    static const char* weekdays[] = {
        "Sunday", "Monday", "Tuesday", "Wednesday",
        "Thursday", "Friday", "Saturday"
    };

    if (date.tm_wday < 0 || date.tm_wday > 6) {
        fprintf(stderr, "Error: Invalid weekday\n");
        return 1;
    }

    const char* weekday_name = weekdays[date.tm_wday];
    char* result = strdup(weekday_name);
    if (!result) {
        fprintf(stderr, "Error: Memory allocation failed\n");
        return 1;
    }

    stack_element out;
    out.type = TYPE_STRING;
    out.string = result;

    if (stack->top >= STACK_SIZE - 1) {
        fprintf(stderr, "Error: Stack overflow\n");
        free(result);
        return 1;
    }

    stack->items[++stack->top] = out;
    return 0;
}



int push_today_date(Stack* stack) {
    time_t now = time(NULL);
    if (now == (time_t)(-1)) {
        fprintf(stderr, "Error: could not get current time\n");
        return 1;
    }

    struct tm* tm_now = localtime(&now);
    if (!tm_now) {
        fprintf(stderr, "Error: could not convert time to local time\n");
        return 1;
    }

    char buffer[64];  // enough for "DD.MM.YYYY\0"
    snprintf(buffer, sizeof(buffer), "%02d.%02d.%04d",
             tm_now->tm_mday,
             tm_now->tm_mon + 1,
             tm_now->tm_year + 1900);

    // Duplicate the string before pushing
    char* date_str = strdup(buffer);
    if (!date_str) {
        fprintf(stderr, "Error: memory allocation failed\n");
        return 1;
    }

    stack_element elem;
    elem.type = TYPE_STRING;
    elem.string = date_str;

    if (stack->top >= STACK_SIZE - 1) {
        fprintf(stderr, "Error: stack overflow\n");
        free(date_str);
        return 1;
    }

    stack->items[++stack->top] = elem;
    return 0;
}


int delta_days_strings(Stack* stack) {
    if (stack->top < 1) {
        fprintf(stderr, "Error: delta_days requires two values on the stack\n");
        return 1;
    }

    stack_element b = stack->items[stack->top--];
    stack_element a = stack->items[stack->top--];

    if (a.type != TYPE_STRING || b.type != TYPE_STRING) {
        fprintf(stderr, "Error: delta_days requires two strings in DD.MM.YYYY format\n");
        return 1;
    }

    struct tm tm1 = {0}, tm2 = {0};
    if (sscanf(a.string, "%d.%d.%d", &tm1.tm_mday, &tm1.tm_mon, &tm1.tm_year) != 3 ||
        sscanf(b.string, "%d.%d.%d", &tm2.tm_mday, &tm2.tm_mon, &tm2.tm_year) != 3) {
        fprintf(stderr, "Error: invalid date format. Use DD.MM.YYYY\n");
        return 1;
    }

    // Normalize to struct tm
    tm1.tm_mon -= 1;  // months since January
    tm1.tm_year -= 1900;
    tm1.tm_hour = 12; // avoid DST edge cases
    tm2.tm_mon -= 1;
    tm2.tm_year -= 1900;
    tm2.tm_hour = 12;

    time_t t1 = mktime(&tm1);
    time_t t2 = mktime(&tm2);

    if (t1 == (time_t)-1 || t2 == (time_t)-1) {
        fprintf(stderr, "Error: failed to convert date to time_t\n");
        return 1;
    }

    int days = (int) difftime(t2, t1) / (60 * 60 * 24);
    push_real(stack, (double) days);  // or push_int if you have it

    return 0;
}
