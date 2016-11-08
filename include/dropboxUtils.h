/*!
 * \file    dropboxUtils.h
 * \brief   Common function library for dropbox library.
 * \author  Ashwin Sankar
 * \version 1.0
 * \date    29.10.2013
 */

#ifndef DROPBOX_UTILS_H
#define DROPBOX_UTILS_H

/*!
 * \struct	drbMemory
 * \breif	Memory space to read/write with drbMemoryRead/drbMemoryWrite.
 */
typedef struct {
	char* data;    /*< where the data memory start */
	size_t size;   /*< data memory size */
	size_t cursor; /*< where drbMemoryRead is currently reading data. */
} drbMemory;

void* drbRealloc(void* ptr, size_t size);
char* drbGetHeaderFieldContent(const char* field, char* header);
size_t drbMemoryWrite(const void *ptr, size_t size, size_t count, drbMemory *mem);
size_t drbMemoryRead(void *ptr, size_t size, size_t count, drbMemory *mem);

#endif /* DROPBOX_UTILS_H */