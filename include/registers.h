/*
 * This file is part of Mico's MM-15 Calculator
 *
 * Mico's MM-15 Calculator is free software: 
 * you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Mico's MM-15 Calculator is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Mico's MM-15 Calculator. If not, see <https://www.gnu.org/licenses/>.
 */

#include <stdbool.h>

#ifndef REGISTERS_H
#define REGISTERS_H

typedef struct {
    stack_element value;
    bool occupied;
} Register;

#define MAX_REG 64
extern Register registers[MAX_REG];  // Global register storage

stack_element copy_element(const stack_element* src);
void free_element(stack_element* el);
void store_to_register(Stack* stack);
void recall_from_register(Stack* stack);
void show_registers_status(void);
void init_registers(void);
void free_all_registers(void);
void load_registers_from_file(const char* filename);
void save_registers_to_file(const char* filename);
void find_first_free_register(Stack *stack);
#endif //REGISTERS_H

