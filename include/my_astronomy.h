#ifndef MY_ASTRONOMY_H
#define MY_ASTRONOMY_H

#include "stack.h"

/*
 * Stack interface for sunrise/sunset in mm_15.
 *
 * Stack contract (top-first, RPN style):
 *
 *   [ ... , "YYYY-MM-DD", latitude_deg, longitude_deg, utc_offset_h ]
 *                                         ^ top
 *
 * Types:
 *   - "YYYY-MM-DD"      : TYPE_STRING
 *   - latitude_deg      : TYPE_REAL (north +, south -)
 *   - longitude_deg     : TYPE_REAL (east +, west -)
 *   - utc_offset_h      : TYPE_REAL (e.g. -5.0 for EST, 1.0 for CET)
 *
 * After SUNRISE:
 *   consumes 4 items and pushes "HH:MM" (TYPE_STRING) local sunrise time.
 *
 * After SUNSET:
 *   consumes 4 items and pushes "HH:MM" (TYPE_STRING) local sunset time.
 *
 * Returns:
 *   1 on success, 0 on error (insufficient items, type mismatch, invalid
 *   date, or sun never rises/sets). On error, the stack is left unchanged.
 */

int sunrise(Stack *stack);
int sunset(Stack *stack);
int dawn(Stack *stack);
int dusk(Stack *stack);

#endif /* MY_ASTRONOMY_H */
