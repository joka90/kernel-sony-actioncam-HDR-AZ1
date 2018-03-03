/* 2012-05-10: File added by Sony Corporation */
/*
 * CompSwap device
 *
 *  Copyright 2012 Sony Corporation
 *
 * This code is released using a dual license strategy: BSD/GPL
 * You can choose the licence that better fits your requirements.
 *
 *  CompSwap is based on "ramzswap" or "zram"
 *  Copyright (C) 2008, 2009, 2010  Nitin Gupta
 */

#ifndef COMPSWAP_H
#define COMPSWAP_H

struct compswap_compressor_ops {
	int (*init)(void **priv);
	void (*exit)(void *priv);
	int (*compress)(const void *src, size_t src_len,
			void *dst, size_t *dst_len,
			void *priv);
};

struct compswap_decompressor_ops {
	int (*init)(void **priv);
	void (*exit)(void *priv);
	int (*decompress)(const void *src, size_t src_len,
			void *dst, size_t *dst_len,
			void *priv);
};

extern int compswap_compressor_register(struct compswap_compressor_ops *ops);
extern int compswap_compressor_unregister(struct compswap_compressor_ops *ops);
extern int compswap_decompressor_register(struct compswap_decompressor_ops *ops);
extern int compswap_decompressor_unregister(struct compswap_decompressor_ops *ops);

#endif
