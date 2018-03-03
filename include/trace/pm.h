/* 2013-08-20: File added by Sony Corporation */
#ifndef _TRACE_PM_H
#define _TRACE_PM_H

#include <linux/tracepoint.h>

DECLARE_TRACE_NOARGS(pm_idle_entry);
DECLARE_TRACE_NOARGS(pm_idle_exit);
DECLARE_TRACE_NOARGS(pm_suspend_entry);
DECLARE_TRACE_NOARGS(pm_suspend_exit);

#endif
