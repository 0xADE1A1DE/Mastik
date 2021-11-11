/*
 * Copyright 2016 CSIRO
 *
 * This file is part of Mastik.
 *
 * Mastik is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Mastik is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Mastik.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __SYMBOL_H__
#define __SYMBOL_H__ 1

uint64_t sym_getsymboloffset(const char *file, const char *symbol);

uint64_t sym_loadersymboloffset(const char *file, const char *symbol);
uint64_t sym_debuglineoffset(const char *file, const char *src, int lineno);
uint64_t sym_addresstooffset(const char *file, uint64_t address);

#endif // __SYMBOL_H__
