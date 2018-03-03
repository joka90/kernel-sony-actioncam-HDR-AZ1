/* 2013-03-14: File changed by Sony Corporation */
#ifndef _LINUX_TTY_FLIP_H
#define _LINUX_TTY_FLIP_H

#ifdef CONFIG_SNSC_VUART
#include <linux/snsc_vuart.h>
#endif
extern int tty_buffer_request_room(struct tty_struct *tty, size_t size);
extern int tty_insert_flip_string_flags(struct tty_struct *tty, const unsigned char *chars, const char *flags, size_t size);
extern int tty_insert_flip_string_fixed_flag(struct tty_struct *tty, const unsigned char *chars, char flag, size_t size);
extern int tty_prepare_flip_string(struct tty_struct *tty, unsigned char **chars, size_t size);
extern int tty_prepare_flip_string_flags(struct tty_struct *tty, unsigned char **chars, char **flags, size_t size);
void tty_schedule_flip(struct tty_struct *tty);

static inline int tty_insert_flip_char(struct tty_struct *tty,
					unsigned char ch, char flag)
{
	struct tty_buffer *tb = tty->buf.tail;
#ifdef CONFIG_SNSC_VUART
	static int header_avail, vuart_data_size;
	static char vuart_header[SNSC_VUART_HEADER_SIZE];

	if (!tty->buddy || !tty->buddy->count)
		goto normal_data;

	if (vuart_data_size) {
		/* Process VUART data */
		tty = tty->buddy;
		tb = tty->buf.tail;
		vuart_data_size--;
		goto normal_data;
	} else {
		/* Parse VUART header */
		if (ch == SNSC_VUART_HEADER_FLAG || header_avail > 1)
			vuart_header[header_avail++] = ch;
		else
			goto roll_back;

		/* vuart_header[4]: [0]=[1]=0xff, size=[2]<<8|[3] */
		if (header_avail == SNSC_VUART_HEADER_SIZE) {
			vuart_data_size = (vuart_header[2] << 8);
			vuart_data_size |= (vuart_header[3] & 0xff);
			header_avail = 0;

			if (!vuart_data_size)
				goto roll_back;
		}
		return 1;
	}

roll_back:
	if (header_avail) {
		unsigned int idx;
		unsigned char ch;
		for (idx = 0; idx < header_avail; idx++) {
			ch = vuart_header[idx];

			if (tb && tb->used < tb->size) {
				tb->flag_buf_ptr[tb->used] = flag;
				tb->char_buf_ptr[tb->used++] = ch;
			} else {
				tty_insert_flip_string_flags(tty,
					&ch, &flag, 1);
			}
		}
		header_avail = 0;
	}

normal_data:
#endif /* CONFIG_SNSC_VUART */
	if (tb && tb->used < tb->size) {
		tb->flag_buf_ptr[tb->used] = flag;
		tb->char_buf_ptr[tb->used++] = ch;
		return 1;
	}
	return tty_insert_flip_string_flags(tty, &ch, &flag, 1);
}

static inline int tty_insert_flip_string(struct tty_struct *tty, const unsigned char *chars, size_t size)
{
	return tty_insert_flip_string_fixed_flag(tty, chars, TTY_NORMAL, size);
}

#endif /* _LINUX_TTY_FLIP_H */
