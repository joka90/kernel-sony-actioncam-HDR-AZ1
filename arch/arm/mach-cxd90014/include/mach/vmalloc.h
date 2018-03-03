/*
 *  linux/include/asm-arm/arch-realview/vmalloc.h
 *
 *  Copyright (C) 2003 ARM Limited
 *  Copyright (C) 2000 Russell King.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
#define VMALLOC_OFFSET		(8*1024*1024)
#ifdef CONFIG_QEMU
#define VMALLOC_START		((unsigned long)(0xc8000000 + VMALLOC_OFFSET))
#else
#define VMALLOC_START		((unsigned long)(0xc2000000 + VMALLOC_OFFSET))
#endif
#define VMALLOC_END		((unsigned long)(0xf0000000 - VMALLOC_OFFSET))
