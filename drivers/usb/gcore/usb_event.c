/*
 * usb_event.c
 * 
 * Copyright 2005,2006,2009,2011,2013 Sony Corporation
 * 
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 * 
 */
 
/*-----------------------------------------------------------------------------
 * Include file
 *---------------------------------------------------------------------------*/
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/wait.h>
#include <linux/slab.h>
#include <linux/version.h>
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 39)
 #include <linux/sched.h>
#else
 #include <linux/smp_lock.h>
#endif
#include <linux/proc_fs.h>
#include <linux/moduleparam.h>
#include <asm/uaccess.h>
#include <linux/udif/cdev.h>
#include <mach/udif/devno.h>
#include <linux/usb/gcore/usb_event.h>

#include "usb_event_cfg.h"
#include "usb_event_pvt.h"

/*-----------------------------------------------------------------------------
 * Module infomation
 *---------------------------------------------------------------------------*/
MODULE_AUTHOR("Sony Corporation");
MODULE_DESCRIPTION(USBEVENT_NAME
                   " driver ver " USBEVENT_VERSION);
MODULE_LICENSE("GPL");

#define MYDRIVER_NAME    USBEVENT_NAME

/*-----------------------------------------------------------------------------
 * Type declaration
 *---------------------------------------------------------------------------*/
typedef struct _usb_event_queue {
        unsigned char *buf;             // �Хåե��ΰ�
        unsigned int buf_size;          // �Хåե��ΰ�Υ�����
        unsigned int data_size;         // �Хåե����ͭ���ʥǡ���������
        unsigned int read_p;            // Read�ݥ���
        unsigned int max_data_size;     // data_size�κ�����(procɽ����)
} usb_event_queue;

typedef struct _usb_event_hdllist {
        usb_hndl_t    *hdls;          // Handle����
        unsigned char num_element;    // Handle���Ǥο�
        unsigned char num_data;       // ͭ���ʥǡ����ο�
} usb_event_hdllist;

typedef struct _usb_event {
        usb_event_queue   queue;
        usb_event_hdllist hdl_list;
        wait_queue_head_t wait_queue;
} usb_event;

/*-----------------------------------------------------------------------------
 * Extern function declaration
 *---------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------
 * Function prototype declaration
 *---------------------------------------------------------------------------*/
static int usb_event_init_hdllist(usb_event_hdllist*);
static int usb_event_alloc_hdllist(usb_event_hdllist*, unsigned char);
static int usb_event_free_hdllist(usb_event_hdllist*);
static int find_data_from_hdllist(usb_event_hdllist*, usb_hndl_t);
static int set_data_to_hdllist(usb_event_hdllist*, usb_hndl_t);
static int delete_data_from_hdllist(usb_event_hdllist*, usb_hndl_t);

static int usb_event_init_queue(usb_event_queue*);
static int usb_event_alloc_queue(usb_event_queue*, unsigned int);
static int usb_event_free_queue(usb_event_queue*);
static int set_data_to_queue(usb_event_queue*, void*, unsigned int);
static unsigned int get_data_size_with_lock(usb_event_queue*);
static unsigned int get_data_from_queue(usb_event_queue*, void*, unsigned int);

static int usb_event_read_proc(char*, char**, off_t, int, int*, void*);
static int usb_event_install_proc(void);
static void usb_event_remove_proc(void);

static ssize_t usb_event_read(struct file*, char __user*, size_t, loff_t*);
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 36)
static long usb_event_ioctl(struct file*, unsigned int, unsigned long);
#else
static int usb_event_ioctl(struct inode*, struct file*, unsigned int, unsigned long);
#endif

static int usb_event_ioctl_enable(usb_hndl_t*);
static int usb_event_ioctl_disable(usb_hndl_t*);
static int usb_event_ioctl_stop(void);
static int usb_event_open(struct inode*, struct file*);
static int usb_event_release (struct inode*, struct file*);

int __init usb_event_module_init(void);
static void __exit usb_event_module_exit(void);


/*-----------------------------------------------------------------------------
 * Variable declaration
 *---------------------------------------------------------------------------*/
static usb_event g_obj;                     /* USB Event ���֥������� */
static unsigned long g_lock = 0;            /* ��¾������ */
static unsigned long g_count;               /* ���ä�Event�ο� */
static unsigned long g_err_count;           /* �٤�ʤ��ä�Event�ο� */

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 39)
static DEFINE_SPINLOCK(lock);
#else
static spinlock_t lock = SPIN_LOCK_UNLOCKED;
#endif

/*=============================================================================
 *
 * Main function body
 *
 *===========================================================================*/
/*-------------------------------------------------------------------------*/
/*-------------------------------------------------------------------------*/
/* hdl_list���������� */
static int
usb_event_init_hdllist(usb_event_hdllist *list)
{
  /* hdl_list�ΰ��0�˽���� */
  memset(list->hdls, 0, (size_t)(list->num_element * sizeof(unsigned long)));
  
  /* �ǡ�����������0�˽���� */
  list->num_data = 0;
  
  return 0;
}

/*-------------------------------------------------------------------------*/
/* hdl_list�Ѥ��ΰ����ݤ��� */
static int
usb_event_alloc_hdllist(usb_event_hdllist *list, unsigned char num)
{
  unsigned long *tmp;
  
  /* �ΰ���� */
  tmp = (unsigned long *)kmalloc((size_t)(num * sizeof(usb_hndl_t)), GFP_KERNEL);
  if(tmp == NULL){
    list->num_element = 0;
    return -1;
  }
  
  list->hdls = tmp;
  list->num_element = num;
  
  return 0;
}

/*-------------------------------------------------------------------------*/
/* hdl_list�Ѥ��ΰ�������� */
static int
usb_event_free_hdllist(usb_event_hdllist *list)
{
  kfree((void*)list->hdls);
  
  return 0;
}

/*-------------------------------------------------------------------------*/
/* hdl_list����ǡ�����õ�� */
/* �֤���   -9 : �����۾�, -1 : ���Ĥ���ʤ�,   ����¾ : ��� */
static int
find_data_from_hdllist(usb_event_hdllist *list, usb_hndl_t data)
{
  unsigned char i;
  
  if(data == 0 || list == 0) return -9;
  
  for(i = 0; i < list->num_element; i++){
    if((list->hdls)[i] == data) break;
  }
  if(i == list->num_element) return -1;
  
  return i;
}

/*-------------------------------------------------------------------------*/
/* hdl_list�˥ǡ������ɲ� */
/* �֤���   -1 : �Хåե�������,   -2 : ���Ǥ����äƤ���, -9 : �����۾�, 
            ����¾ : �ɲä������ */
static int
set_data_to_hdllist(usb_event_hdllist *list, usb_hndl_t data)
{
  unsigned char i;
  
  if(data == 0 || list == 0) return -9;
  
  /* �Хåե��˶��������뤫��ǧ */
  if(list->num_data == list->num_element) return -1;
  
  /* ����Υǡ��������äƤ��뤫õ�� */
  if(find_data_from_hdllist(list, data) != -1) return -2;
  
  /* hdls��ζ����ΰ��õ�� */
  for(i = 0; i < list->num_element; i++){
    if((list->hdls)[i] == 0) break;
  }
  if(i == list->num_element) return -1;   /* ���פ��뤳�Ȥ�̵���Ϥ�������� */
  
  /* �����ΰ�˥ǡ��������� */
  (list->hdls)[i] = data;
  list->num_data++;
  
  return i;
}

/*-------------------------------------------------------------------------*/
/* hdl_list�������Υǡ������� */
/* �֤���   -1 : ���Ĥ���ʤ�,   ����¾ : ����Υǡ��������äƤ������ */
static int
delete_data_from_hdllist(usb_event_hdllist *list, usb_hndl_t data)
{
  int lo;
  
  /* ����Υǡ��������äƤ������õ�� */
  lo = find_data_from_hdllist(list, data);
  if(lo < 0) return -1;
  
  (list->hdls)[lo] = 0;
  list->num_data--;
  
  return lo;
}

/*-------------------------------------------------------------------------*/
/*-------------------------------------------------------------------------*/
/*-------------------------------------------------------------------------*/
/* queue��¤�Τ��������� */
static int
usb_event_init_queue(usb_event_queue *queue)
{
  /* Read�ݥ��󥿤���Ƭ���֤˽���� */
  queue->read_p = 0;
  
  /* �ǡ�����������0�˽���� */
  queue->data_size = 0;
  queue->max_data_size = 0;
  
  return 0;
}

/*-------------------------------------------------------------------------*/
/* queue�Ѥ��ΰ����ݤ��� */
static int
usb_event_alloc_queue(usb_event_queue *queue, unsigned int size)
{
  unsigned char *buf;
  
  /* �ΰ���� */
  buf = (unsigned char *) kmalloc((size_t)size, GFP_KERNEL);
  if(buf == NULL){
    queue->buf_size = 0;
    return -1;
  }
  
  queue->buf = buf;
  queue->buf_size = size;
  
  return 0;
}

/*-------------------------------------------------------------------------*/
/* queue�Ѥ��ΰ�������� */
static int
usb_event_free_queue(usb_event_queue *queue)
{
  kfree((void*)queue->buf);
  
  return 0;
}

/*-------------------------------------------------------------------------*/
/* queue�˥ǡ������Ѥ��write�ݥ��󥿤�ʤ�� */
/* �֤���  �ºݤ��Ѥ��������   -1�ʤ��queue��­��ʤ� */
static int
set_data_to_queue(usb_event_queue *queue, void *data, unsigned int data_size)
{
  unsigned char tmp_size;
  unsigned char *src_buf, *dest_buf;
  
  /* �Ѥ�ǡ����Υ�������0�ʤ�о�����ｪλ */
  if(data_size == 0){
    return 0;
  }
  
  /* �Ѥ�ǡ����Υ�������queue�λĤ����̰ʲ��Ǥ��뤳�Ȥ��ǧ���� */
  if(data_size > (queue->buf_size - queue->data_size)){
    return -1;
  }
  
  src_buf = (unsigned char*)data;
  dest_buf = queue->buf;
  
  if(data_size > (queue->buf_size - GET_WRITE_P(queue))){
    /** queue�κǸ���ޤ��֤�ȯ�������� **/
    
    /* �ޤ��ޤ��֤��ޤ�memcpy */
    tmp_size = queue->buf_size - GET_WRITE_P(queue);
    memcpy((void *)&(dest_buf[GET_WRITE_P(queue)]), (void*)src_buf, tmp_size);
    FORWARD_WRITE_P(queue, tmp_size);    /* queue->write_p == 0 �ˤʤ�Ϥ� */
    src_buf += tmp_size;
    data_size -= tmp_size;
    
    /* �Ĥ��memcpy */
    tmp_size = data_size;
    memcpy((void *)&(dest_buf[GET_WRITE_P(queue)]), (void*)src_buf, tmp_size);
    FORWARD_WRITE_P(queue, tmp_size);
    
  }else{
    /** queue�κǸ���ޤ��֤�ȯ�����ʤ���� **/
    memcpy((void *)&(dest_buf[GET_WRITE_P(queue)]), (void*)src_buf, data_size);
    FORWARD_WRITE_P(queue, data_size);
  }
  
  return data_size;
}

/*-------------------------------------------------------------------------*/
/* ���ߤ�queue��Υǡ��������������(��¾������) */
static unsigned int
get_data_size_with_lock(usb_event_queue *queue)
{
    unsigned long flags;
    unsigned int data_size;
    
    /** SPIN_LOCK */
    /**/spin_lock_irqsave(&lock, flags);
    /**/
    /**/data_size = queue->data_size;
    /**/
    /**/spin_unlock_irqrestore(&lock, flags);
    /** SPIN_UNLOCK */
    
    return data_size;
}


/*-------------------------------------------------------------------------*/
/* queue����ǡ������ɤ��read�ݥ��󥿤�ʤ�� */
/* �֤��ͤϡ��ºݤ��ɤ���� */
/* ������buf�ϡ�user���֤Υ��ɥ쥹 */
static unsigned int
get_data_from_queue(usb_event_queue *queue, void *buf, unsigned int read_size)
{
  unsigned char *src_buf, *dest_buf;
  unsigned int tmp_size;
  unsigned long flags;
  
  /* �ºݤ��ɤ߹����̤���� */
  
  /** SPIN_LOCK */
  /**/spin_lock_irqsave(&lock, flags);
  /**/
  /**/read_size = min(read_size, queue->data_size);
  /**/
  /**/spin_unlock_irqrestore(&lock, flags);
  /** SPIN_UNLOCK */
  
  if(read_size == 0) return 0;
  
  src_buf = queue->buf;
  dest_buf = (unsigned char*)buf;
  
  if(read_size > (queue->buf_size - queue->read_p)){
    
    /** queue�κǸ���ޤ��֤�ȯ�������� **/
    
    /* �ޤ��ޤ��֤��ޤ�memcpy */
    tmp_size = queue->buf_size - queue->read_p;
    
    if(copy_to_user(dest_buf, (void*)&(src_buf[queue->read_p]), tmp_size) != 0){
        PERR("get_data_from_queue() failed\n");
        return 0;
    }
    
    dest_buf += tmp_size;
    
    /* �Ĥ��memcpy */
    tmp_size = read_size - tmp_size;
    if(copy_to_user(dest_buf, (void*)&(src_buf[0]), tmp_size) != 0){
        PERR("get_data_from_queue() failed\n");
        return 0;
    }
    
  }else{
    /** queue�κǸ���ޤ��֤�ȯ�����ʤ���� **/
    if(copy_to_user(dest_buf, (void*)&(src_buf[queue->read_p]), read_size) != 0){
        PERR("get_data_from_queue() failed\n");
        return 0;
    }
  }
    
  /** SPIN_LOCK */
  /**/ spin_lock_irqsave(&lock, flags);
  /**/
  /**/ FORWARD_READ_P(queue, read_size);
  /**/
  /**/ spin_unlock_irqrestore(&lock, flags);
  /** SPIN_UNLOCK */
  
  
  return read_size;
}

/*-------------------------------------------------------------------------*/
/*-------------------------------------------------------------------------*/
/*-------------------------------------------------------------------------*/
int usb_event_add_queue(unsigned char priority,
                        usb_event_callback cb,
                        usb_hndl_t handle,
                        usb_kevent_id_t id,
                        unsigned char size,
                        void* data)
{
    unsigned long flags;
    int result;
    unsigned int old_data_size, padding_size;
    
    /** �����Ǥϡ�priority ��̵�� */
    
    PDEBUG("usb_event_add_queue() call\n");
    PVERBOSE("  handle:%08lu sizeof(handle):%d\n", handle, sizeof(handle));
    PVERBOSE("  cb:%02x sizeof(cb):%d\n", (unsigned int)cb, sizeof(cb));
    PVERBOSE("  id:%02x sizeof(id):%d\n", id, sizeof(id));
    PVERBOSE("  size:%02x sizeof(size):%d\n", size, sizeof(size));
    PVERBOSE("  &data:%08x size:%d\n", (unsigned int)data, size);
    
    /* Callback�褬NULL �ޤ��� handle��0 �ʤ�� ���顼��λ */
    if(cb == NULL || handle == 0){
        PWARN("cb or handle is 0\n");
        return -EINVAL;
    }
    
    /* open����Ƥ��ʤ���С�queue�ΰ褬¸�ߤ��ʤ��Τǡ�����˽�λ */
    if(test_bit(USB_EVENT_ATOMIC_FD_LOCK, &g_lock) == 0){
        PWARN("usb_event_add_queue() called before open\n");
        return 0;
    }
    
    result = 0;
    
    /********** SPIN_LOCK **********/
    spin_lock_irqsave(&lock, flags);
    
    /* hdl_list �� handle�����뤫��ǧ */
    if(find_data_from_hdllist(&(g_obj.hdl_list), handle) < 0){
        PINFO("handle not found in hdl_list\n");
        /* ���ｪλ */
        goto comp;
    }
    
    /* ���ߤΥǡ����Υ���������¸���Ƥ��� */
    old_data_size = GET_DATA_SIZE(&(g_obj.queue));
    
    /* queue��handle���ɲ� */
    if(set_data_to_queue(&(g_obj.queue), &handle, sizeof(handle)) < 0){
        result = -ENOBUFS;
        goto comp;
    }
    
    /* queue��cb���ɲ� */
    if(set_data_to_queue(&(g_obj.queue), &cb, sizeof(cb)) < 0){
        result = -ENOBUFS;
        goto comp;
    }
    
    /* queue��id���ɲ� */
    if(set_data_to_queue(&(g_obj.queue), &id, sizeof(id)) < 0){
        result = -ENOBUFS;
        goto comp;
    }
    
    /* queue��size���ɲ� */
    if(set_data_to_queue(&(g_obj.queue), &size, sizeof(size)) < 0){
        result = -ENOBUFS;
        goto comp;
    }
    
    /* ALIGNMENTĴ�� padding���ɲ� */
    padding_size = PADDING_SIZE((GET_DATA_SIZE(&(g_obj.queue)) - old_data_size), 4);
    PDEBUG(" padding_size1 : %d\n", padding_size);
    if(set_data_to_queue(&(g_obj.queue), &handle, padding_size) < 0){
        result = -ENOBUFS;
        goto comp;
    }
    
    /* queue�˥ǡ������ɲ� */
    if(set_data_to_queue(&(g_obj.queue), data, size) < 0){
        result = -ENOBUFS;
        goto comp;
    }
    
    /* ALIGNMENTĴ�� padding���ɲ� */
    padding_size = PADDING_SIZE((GET_DATA_SIZE(&(g_obj.queue)) - old_data_size), 4);
    PDEBUG(" padding_size2 : %d\n", padding_size);
    if(set_data_to_queue(&(g_obj.queue), &handle, padding_size) < 0){
        result = -ENOBUFS;
        goto comp;
    }
    
comp:
    
    /* queue�ζ������̤�­��ʤ�������Ǽ��Ԥ�����硢
       add_queue() ��Ϥ������ data_size ���᤹ */
    if(result == -ENOBUFS){
        PWARN("set_data_to_queue() : -ENOBUFS\n");
        PDEBUG(" data_size set to %d -> %d\n", GET_DATA_SIZE(&g_obj.queue), old_data_size);
        SET_DATA_SIZE(&(g_obj.queue), old_data_size);
        g_err_count++;
    }else{
        g_count++;
    }
    
    
    /********** SPIN_UNLOCK **********/
    spin_unlock_irqrestore(&lock, flags);
    
    if(result == 0){
        /* �Ԥ���������äƤ���ץ�����ư */
        wake_up_interruptible(&(g_obj.wait_queue));
    }
    
    PVERBOSE("usb_event_add_queue() success\n");
    
    return result;
}

/*-------------------------------------------------------------------------*/
/*-------------------------------------------------------------------------*/
/*-------------------------------------------------------------------------*/
static int
usb_event_read_proc(char *buf, char **start, off_t offset, int count,
                   int *eof, void *data)
{
    int len, i;
    
    if(offset > 0){
        len = 0;
        return 0;
    }
    
    len = 0;
    
    len += snprintf(buf+len, PAGE_SIZE-len, "count                : %ld\n", g_count);
    len += snprintf(buf+len, PAGE_SIZE-len, "err_count            : %ld\n\n", g_err_count);
    
    len += snprintf(buf+len, PAGE_SIZE-len, "queue.buf_size       : %d\n", g_obj.queue.buf_size);
    len += snprintf(buf+len, PAGE_SIZE-len, "queue.data_size      : %d\n", g_obj.queue.data_size);
    len += snprintf(buf+len, PAGE_SIZE-len, "queue.max_data_size  : %d\n\n", g_obj.queue.max_data_size);
    
    len += snprintf(buf+len, PAGE_SIZE-len, "hdl_list.num_element : %d\n", g_obj.hdl_list.num_element);
    len += snprintf(buf+len, PAGE_SIZE-len, "hdl_list.num_data    : %d\n", g_obj.hdl_list.num_data);
    for(i = 0; i < g_obj.hdl_list.num_element; i++){
        if(g_obj.hdl_list.hdls[i]){
            len += snprintf(buf+len, PAGE_SIZE-len, " handle[%2d]:%08d\n", i, (int)g_obj.hdl_list.hdls[i]);
        }
        if(len > (count - 80)){
            len += snprintf(buf+len, PAGE_SIZE-len, "[more]\n");
        }
    }
    
    *eof = 1;
    
    return len;
}

/*-------------------------------------------------------------------------*/
static int
usb_event_install_proc(void)
{
    struct proc_dir_entry *proc = NULL;
    
    proc = create_proc_entry(MYDRIVER_NAME, S_IFREG | S_IRUGO, NULL);
    
    if(proc != 0){
        proc->read_proc = usb_event_read_proc;
        return 0;
    }
    
    return -1;
}

/*-------------------------------------------------------------------------*/
static void
usb_event_remove_proc(void)
{
    remove_proc_entry(MYDRIVER_NAME, NULL);
}


/*-------------------------------------------------------------------------*/
/*-------------------------------------------------------------------------*/
/*-------------------------------------------------------------------------*/
static ssize_t
usb_event_read(struct file *filp, char __user *buf, size_t len, loff_t *ptr)
{
    PDEBUG("usb_event_read() call\n");
    
    /* Queue�˥ǡ���������Τ��Ԥ� */
    if(wait_event_interruptible(g_obj.wait_queue, 
                                (get_data_size_with_lock(&(g_obj.queue)) != 0 
                                 || test_bit(USB_EVENT_ATOMIC_WAKEUP, &g_lock)))){
        PWARN("usb_event: usb_event_read() signal interrupt\n");
        return -ERESTARTSYS;
    }
    
    clear_bit(USB_EVENT_ATOMIC_WAKEUP, &g_lock);
    
    len = get_data_from_queue(&(g_obj.queue), buf, len);
    
    PVERBOSE("usb_event_read() success\n");
    
    return len;
}

/*-------------------------------------------------------------------------*/
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 36)
static long
usb_event_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
#else
static int
usb_event_ioctl(struct inode *inode, struct file *filp, unsigned int cmd, unsigned long arg)
#endif
{
    int result;
    
    switch(cmd){
      /*----------------------------------------*/
      case USB_IOC_EVENT_ENABLE:
        result = usb_event_ioctl_enable((usb_hndl_t*)arg);
        break;
      
      /*----------------------------------------*/
      case USB_IOC_EVENT_DISABLE:
        result = usb_event_ioctl_disable((usb_hndl_t*)arg);
        break;
      
      /*----------------------------------------*/
      case USB_IOC_EVENT_STOP:
        result = usb_event_ioctl_stop();
        break;
        
      /*----------------------------------------*/
      default:
        PWARN("usb_event_ioctl(none) call\n");
        result = -EINVAL;
    }
    
    return result;
}

/*-------------------------------------------------------------------*/
static int
usb_event_ioctl_enable(usb_hndl_t *arg)
{
    usb_hndl_t handle;
    unsigned long flags;
    int result;
    
    PDEBUG("usb_event_ioctl(ENABLE) call\n");
    
    /* Enable�ˤ���Handle����Ф� */
    if(get_user(handle, arg) != 0){
        PERR("usb_event_ioctl(ENABLE) fail\n");
        return -EFAULT;
    }
    PVERBOSE("  handle : %08lu\n", handle);
    
    result = 0;
    
    /********** SPIN_LOCK **********/
    spin_lock_irqsave(&lock, flags);
    
    /* ���ꤵ�줿Handle�����hdl_list���ɲ� */
    if(set_data_to_hdllist(&(g_obj.hdl_list), handle) < 0){
        PWARN("usb_event_ioctl(ENABLE) fail\n");
        result = -EFAULT;
    }
    
    /********** SPIN_UNLOCK **********/
    spin_unlock_irqrestore(&lock, flags);
    
    if(result == 0){
        PVERBOSE("usb_event_ioctl(ENABLE) success\n");
    }
    
    return result;
}

/*-------------------------------------------------------------------*/
static int
usb_event_ioctl_disable(usb_hndl_t *arg)
{
    usb_hndl_t handle;
    unsigned long flags;
    int result;
    
    PDEBUG("usb_event_ioctl(DISABLE) call\n");
    
    /* Disable�ˤ���Handle����Ф� */
    if(get_user(handle, arg) != 0){
        PERR("usb_event_ioctl(DISABLE) fail\n");
        return -EFAULT;
    }
    PVERBOSE("  handle : %08lu\n", handle);
    
    result = 0;
    
    /********** SPIN_LOCK **********/
    spin_lock_irqsave(&lock, flags);
    
    /* ���ꤵ�줿Handle�����hdl_list������ */
    if(delete_data_from_hdllist(&(g_obj.hdl_list), handle) < 0){
        PWARN("usb_event_ioctl(DISABLE) fail\n");
        result = -EFAULT;
    }
    
    /********** SPIN_UNLOCK **********/
    spin_unlock_irqrestore(&lock, flags);
    
    if(result == 0){
        
        set_bit(USB_EVENT_ATOMIC_WAKEUP, &g_lock);
        
        /* �Ԥ���������äƤ���ץ�����ư */
        wake_up_interruptible(&(g_obj.wait_queue));
        
        PVERBOSE("usb_event_ioctl(DISABLE) success\n");
    }
    
    return result;
}

/*-------------------------------------------------------------------*/
static int
usb_event_ioctl_stop(void)
{
    PDEBUG("usb_event_ioctl(STOP) call\n");
    
    set_bit(USB_EVENT_ATOMIC_WAKEUP, &g_lock);
    
    /* �Ԥ���������äƤ���ץ�����ư */
    wake_up_interruptible(&(g_obj.wait_queue));
    
    return 0;
}

/*-------------------------------------------------------------------------*/
static int
usb_event_open(struct inode *inode, struct file *filp)
{
    /* ¾��process��usb_event��ȤäƤ��ʤ�����ǧ */
    if(test_and_set_bit(USB_EVENT_ATOMIC_FD_LOCK, &g_lock) != 0){
        PWARN("usb_event_open() failed\n");
        return -EBUSY;
    }
    
    /* queue�Ѥ��ΰ����� */
    if(usb_event_alloc_queue(&(g_obj.queue), USB_EVENT_DEFAULT_QUEUE_SIZE) != 0){
        PERR(" err:usb_event_alloc_queue()\n");
        
        return -ENOMEM;
    }
    
    /* queue�ν���� */
    if(usb_event_init_queue(&(g_obj.queue)) != 0){
        PERR(" err:usb_event_init_queue()\n");
        
        /* queue�Ѥ��ΰ���� */
        usb_event_free_queue(&(g_obj.queue));
        
        return -ENOMEM;
    }
    
    /* hdl_list���ΰ���� */
    if(usb_event_alloc_hdllist(&(g_obj.hdl_list), USB_EVENT_DEFAULT_NUM_HDL_LIST) != 0){
        PERR(" err:usb_event_alloc_hdllist()\n");
        
        /* queue�Ѥ��ΰ���� */
        usb_event_free_queue(&(g_obj.queue));
        
        return -ENOMEM;
    }
    
    /* hdl_list�ν���� */
    if(usb_event_init_hdllist(&(g_obj.hdl_list)) != 0){
        PERR(" err:usb_event_alloc_hdllist()\n");
        
        /* queue�Ѥ��ΰ���� */
        usb_event_free_queue(&(g_obj.queue));
        
        /* hdl_list���ΰ賫�� */
        usb_event_free_hdllist(&(g_obj.hdl_list));
        
        return -ENOMEM;
    }
    
    /* wait_queue �ν���� */
    init_waitqueue_head(&(g_obj.wait_queue));
    
    clear_bit(USB_EVENT_ATOMIC_WAKEUP, &g_lock);
    
    g_count = 0; g_err_count = 0;
    
    PDEBUG("usb_event_open() success\n");
    
    return 0;
}

/*-------------------------------------------------------------------------*/
static int
usb_event_release (struct inode *inode, struct file *filp)
{
    /* queue�Ѥ��ΰ���� */
    if(usb_event_free_queue(&(g_obj.queue)) != 0){
        PERR(" err:usb_event_free_queue()\n");
    }
    
    /* hdl_list���ΰ賫�� */
    if(usb_event_free_hdllist(&(g_obj.hdl_list)) != 0){
        PERR(" err:usb_event_free_hdllist()\n");
    }
    
    PDEBUG("usb_event_release() success!!\n");
    
    /* usb_event��������� */
    if(test_and_clear_bit(USB_EVENT_ATOMIC_FD_LOCK, &g_lock) == 0){
        PERR("usb_event_release() fatal error\n");
        /* ���ꤨ�ʤ����Ȥ�����ȯ�����Ƥ⤳���ǽ�λ���ʤ� */
    }
    
    return 0;
}

/*-------------------------------------------------------------------------*/
static struct file_operations usb_event_fops ={
    .owner =    THIS_MODULE,
    .open =     usb_event_open,
    .release =  usb_event_release,
    .read =     usb_event_read,
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 36)
    .unlocked_ioctl = usb_event_ioctl,
#else
    .ioctl = usb_event_ioctl,
#endif

};

struct cdev usb_event_device;


/*-------------------------------------------------------------------------*/
int __init usb_event_module_init(void)
{
    
    UDIF_DEVNODE *node;
    UDIF_DEVNO devno;
    int res;
    
    PINFO("USB Event driver ver " USBEVENT_VERSION "\n");
    
    /* lock����� */
    clear_bit(USB_EVENT_ATOMIC_FD_LOCK, &g_lock);
    
    // CharacterDevice����Ͽ
    node = udif_device_node(UDIF_NODE_USB_EVENT);
    devno = UDIF_MKDEV(node->major, node->first_minor);
    
    res = register_chrdev_region(devno, node->nr_minor, node->name);
    if(res){
        PINFO(" -->fail!!\n");
        PERR("register_chrdev_region() failed\n");
        return res;
    }
    
    cdev_init(&usb_event_device, &usb_event_fops);
    
    res = cdev_add(&usb_event_device, devno, node->nr_minor);
    if(res){
        PINFO(" -->fail!!\n");
        PERR("cdev_add() failed\n");
        unregister_chrdev_region(devno, node->nr_minor);
        return res;
    }
    
    /* proc�ե��������Ͽ */
    usb_event_install_proc();
    
    PINFO(" --- start --- \n");
    
    PDEBUG("usb_event_module_init() success\n");
    
    return 0;
}

/*-------------------------------------------------------------------------*/
static void __exit usb_event_module_exit(void)
{
    
    UDIF_DEVNODE *node;
    UDIF_DEVNO devno;
    int res;
    
    // CharacterDevice����Ͽ���
    node = udif_device_node(UDIF_NODE_USB_EVENT);
    devno = UDIF_MKDEV(node->major, node->first_minor);
    
    cdev_del(&usb_event_device);
    unregister_chrdev_region(devno, node->nr_minor);
    
    /* proc�ե��������Ͽ���� */
    usb_event_remove_proc();
    
    PINFO(" --- stop --- \n");
    
    if(res == 0){
        PDEBUG("usb_event_module_exit() success\n");
    }else{
        PDEBUG("usb_event_module_exit() success\n");
    }
}

/*=============================================================================
 * Export symbols
 *===========================================================================*/
EXPORT_SYMBOL(usb_event_add_queue);

module_exit(usb_event_module_exit);
