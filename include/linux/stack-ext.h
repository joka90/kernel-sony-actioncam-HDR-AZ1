/* 2013-03-21: File added and changed by Sony Corporation */
/*
 *  Copyright 2012 Sony, Inc.  All rights reserved.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

#ifndef __LINUX_STACK_EXT_H__
#define __LINUX_STACK_EXT_H__

#ifdef CONFIG_STACK_EXTENSIONS
extern int extend_stack(void);
extern void *unextend_stack(void);
extern void __handle_irq(unsigned int irq);
extern void __handle_irq_possibly_extending_stack(unsigned int irq);
#else
#define extend_stack()		do {} while (0)
#define unextend_stack()	do {} while (0)
#define __handle_irq_possibly_extending_stack(irq) __handle_irq(irq)
#endif /* CONFIG_STACK_EXTENSIONS */

#endif /* __LINUX_STACK_EXT_H__ */
