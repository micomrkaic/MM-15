#ifndef MY_DATE_FUN_H
#define MY_DATE_FUN_H

int days_to_end_of_year(Stack* stack);
int make_date_string(Stack* stack);
int extract_day_month_year(Stack* stack);
int date_plus_days(Stack* stack);
int push_weekday_name_from_date_string(Stack* stack);
int push_today_date(Stack* stack);
int delta_days_strings(Stack* stack);
int push_now_time(Stack* stack);
int tz_offset(Stack* stack);

/* Helpers shared with my_astronomy.c */
int parse_date_relaxed(const char *s, int *d, int *m, int *y);
int zone_offset_hours(const char *zone, int d, int m, int y, double *hours_out);

#endif // MY_DATE_FUN_H

