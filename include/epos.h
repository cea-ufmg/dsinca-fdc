/*
        epos.c - Maxxon motor EPOS module header
    Copyright (C) 2011  Dimas Abreu Dutra - dimasadutra@gmail.com

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef EPOS_H

#include <linux/types.h>

int epos_write_object(u16 index, u8 subindex, u8 nodeid, u32 data);
int epos_read_object(u16 index, u8 subindex, u8 nodeid);

#define EPOS_H


#endif//EPOS_H
