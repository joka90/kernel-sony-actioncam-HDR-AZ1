/* 2012-10-03: File added by Sony Corporation */
/*
 * L2Cacheflush: Flush the specified cache range in L2Cache.
 *
 * Sony CONFIDENTIAL
 *
 * Copyright 2011 Sony Corporation
 *
 * DO NOT COPY AND/OR REDISTRIBUTE WITHOUT PERMISSION.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/stat.h>
#include <linux/kernel.h>
#include <linux/proc_fs.h>
#include <linux/version.h>
#include <linux/err.h>
#include <linux/string.h>
#include <linux/ctype.h>
#include <asm/uaccess.h>
#include <asm/outercache.h>
#include <asm/cacheflush.h>

unsigned long physicaladdress;
unsigned long pagesize;
unsigned long cacheflush;
unsigned long invalid;
unsigned long cacheclean;

typedef struct {
        char *name;
        struct proc_dir_entry *entry;
} procdir_entry_t;

typedef struct {
        char *name;
        struct proc_dir_entry *entry;
	int (*read)(char *page, char **start, off_t off,
		    int count, int *eof, void *data);
	int(*write)(struct file *file, const char *buffer,
			unsigned long count, void *data);
	struct file_operations fops;
} procfile_entry_t;

/* Proc dir Entry */
#define PROC_DIR_L2CACHEFLUSH	"l2cacheflush"

/* Proc file Entries */
#define PROC_PHYSICAL_ADDRESS  "physical_addr"
#define PROC_SIZE              "size"
#define PROC_FLUSH             "flush"
#define PROC_INVALIDATE        "invalidate"
#define PROC_CLEAN             "clean"

static int check_input_addr(const char *buffer, unsigned long *addr,
                            unsigned long count, int check_word_alignment)
{
        unsigned int word_mask;

        /* addr_str can hold max of 64-bit address */
        char addr_str[19];

        /* Check string length */
        if (count >= sizeof(addr_str))
                return -EINVAL;

        /* Get the address */
        if (copy_from_user(addr_str, buffer, count))
                return -EFAULT;
        addr_str[count] = '\0';

        /* Convert the input string to unsigned long  */
        *addr = simple_strtoul(addr_str, NULL, 0);

        /* Check whether the address is word aligned */
        if (check_word_alignment) {
                word_mask = sizeof(long) - 1;
                if (*addr & word_mask) {
                        printk("Input address is not word aligned\n");
                        return -EINVAL;
                }
        }

        return 0;
}

static int proc_clean_read(char *page, char **start, off_t off,
                           int count, int *eof, void *data)
{
        int ret;

        ret = scnprintf(page, 64, "%lu \n",cacheclean);

        return ret;
}

static int proc_clean_l2cache(struct file *file, const char *buffer,
                                unsigned long count, void *data)
{
        int ret;

        ret = check_input_addr(buffer, &cacheclean, count, 0);

#ifdef CONFIG_ARM
        if (physicaladdress && pagesize) {
		dmac_flush_range(__va(physicaladdress), __va(physicaladdress+pagesize));
                outer_clean_range(physicaladdress,(physicaladdress+pagesize));
                physicaladdress = 0;
                pagesize        = 0;
		cacheclean	= 0;
        }
	else {
		printk("physical address and page size are not set to clean l2cache \n");
	}
#else
        printk("l2cache clean is not needed \n");
#endif

        return count;
}

static int proc_invalid_read(char *page, char **start, off_t off,
                           int count, int *eof, void *data)
{
        int ret;

        ret = scnprintf(page, 64, "%lu\n",invalid);

        return ret;
}

static int proc_invalidate_l2cache(struct file *file, const char *buffer,
                                unsigned long count, void *data)
{
        int ret;

        ret = check_input_addr(buffer, &invalid, count, 0);

#ifdef CONFIG_ARM
        if (physicaladdress && pagesize) {
                outer_inv_range(physicaladdress,(physicaladdress+pagesize));
                physicaladdress = 0;
                pagesize        = 0;
		invalid		= 0;
        }
	else {
		printk("physical address and page size are not set to invalidate l2cache \n");
	}
#else
        printk("l2cache invalidation is not needed \n");
#endif

        return count;
}

static int proc_flush_read(char *page, char **start, off_t off,
                           int count, int *eof, void *data)
{
        int ret;

        ret = scnprintf(page, 64, "%lu \n",cacheflush);

        return ret;
}

static int proc_flush_l2cache(struct file *file, const char *buffer,
				unsigned long count, void *data)
{
        int ret;

        ret = check_input_addr(buffer, &cacheflush, count, 0);

#ifdef CONFIG_ARM
	if (cacheflush &&  physicaladdress && pagesize) {
		outer_flush_range(physicaladdress,(physicaladdress+pagesize));
		physicaladdress = 0;
		pagesize	= 0;
		cacheflush	= 0;
	}
	else {
		printk("physical address and page size are not set to flush l2cache \n");
	}
#else
	printk("l2cache flush is not needed \n");
#endif
	return count;
}

static int proc_size_read(char *page, char **start, off_t off,
                           int count, int *eof, void *data)
{
        int ret;

        ret = scnprintf(page, 256, "0x%lx\n",pagesize);

        return ret;
}

static int proc_size_strtoul(struct file *file, const char *buffer,
                     unsigned long count, void *data)
{
        int ret;

        ret = check_input_addr(buffer, &pagesize, count, 0);

        return count;
}

static int proc_paddr_read(char *page, char **start, off_t off,
			   int count, int *eof, void *data)
{
	int ret;

	ret = scnprintf(page, 256, "0x%lx\n",physicaladdress);

	return ret;
}

static int proc_paddr_strtoul(struct file *file, const char *buffer,
                     unsigned long count, void *data)
{
        int ret;

        ret = check_input_addr(buffer, &physicaladdress, count, 0);

        return count;
}

static procdir_entry_t procdir_entry = {PROC_DIR_L2CACHEFLUSH, NULL};

static procfile_entry_t procfile_entry_data[] = {
	{PROC_PHYSICAL_ADDRESS, NULL, proc_paddr_read, proc_paddr_strtoul, {}},
        {PROC_SIZE, NULL, proc_size_read, proc_size_strtoul, {}},
        {PROC_FLUSH, NULL, proc_flush_read, proc_flush_l2cache, {}},
        {PROC_INVALIDATE, NULL, proc_invalid_read, proc_invalidate_l2cache, {}},
        {PROC_CLEAN, NULL, proc_clean_read, proc_clean_l2cache, {}},
};

/* Create proc entries */
static int create_proc_entries(void)
{
	int len, i;

	/* Create proc directory */
	procdir_entry.entry = proc_mkdir(procdir_entry.name, NULL);
	if (procdir_entry.entry == NULL) {
		printk(KERN_ALERT
			"Error: Could not create directory /proc/%s\n",
			procdir_entry.name);
		return -ENOMEM;
	}

	/* Create proc files */
	len = sizeof(procfile_entry_data) / sizeof(procfile_entry_t);
	for (i = 0; i < len; i++) {
		procfile_entry_t pf = procfile_entry_data[i];
		pf.entry = create_proc_entry(pf.name, 0777, procdir_entry.entry);
		if (pf.entry == NULL) {
			printk(KERN_ALERT
                  	       "Error: Could not create /proc/%s/%s\n",procdir_entry.name, pf.name);
			return -ENOMEM;
		}
		procfile_entry_data[i].entry = pf.entry;

		if (pf.read)
			pf.entry->read_proc = pf.read;

		if (pf.write)
			pf.entry->write_proc = pf.write;

		if (!(pf.read || pf.write))
			pf.entry->proc_fops = &(pf.fops);
	}

	return 0;
}

/* Remove proc entries */
static void remove_proc_entries(void)
{
	int len, i;

       /* Remove proc files */
        len = sizeof(procfile_entry_data) / sizeof(procfile_entry_t);
        for (i = len - 1; i >= 0; i--) {
                if (procfile_entry_data[i].entry != NULL)
                        remove_proc_entry(procfile_entry_data[i].name,
                                          procdir_entry.entry);

        }
        /* Remove proc directory */
        remove_proc_entry(procdir_entry.name, NULL);

}
/*
 * Initialize the module
 */
static int __init L2Cacheflush_init(void)
{
	int error = 0;

	/* Create our proc directory and files */
        error = create_proc_entries();
        if (error) {
                pr_info("%s: Failed to create proc entries (%d)\n", __func__,
                        error);
                goto failed_proc_entry;
        }

        return 0;

failed_proc_entry:
        remove_proc_entries();

	return error;
}

static void L2Cacheflush_exit(void)
{
	/* Clean up the proc entries */
	remove_proc_entries();

}

module_init(L2Cacheflush_init);
module_exit(L2Cacheflush_exit);
MODULE_LICENSE("Proprietary");
