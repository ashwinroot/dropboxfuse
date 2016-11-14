/*!
 * \file    dropboxUtils.h
 * \brief   Common function library for dropbox library.
 * \author  Ashwin Sankar
 * \version 1.0
 */

#define _GNU_SOURCE

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <oauth.h>
#include <stdbool.h>
#include "dropboxUtils.h"

/*!
 * \brief	As realloc also, if it fail, free thee unreallocated memory.
 * \param	ptr    pointer on the memory to realloc
 * \param	size   need memory size
 * \return	pointer to the reallocated memory
 */
void* drbRealloc(void* ptr, size_t size) {
	char* newPtr = realloc(ptr, size);
	if (newPtr == NULL)
		free(ptr), ptr = NULL;
	return newPtr;
}

/*!
 * \brief	Found and return the content of an http header field.
 * \param	field    field name
 * \param	header   header to parse
 * \return	copy of the field content (must be freed by caller)
 */
char* drbGetHeaderFieldContent(const char* field, char* header) {
	char* content = NULL;
	char* fieldLine = strstr(header, field); // find field line

	if (fieldLine && (fieldLine == header || *(fieldLine-1) == '\n')) {
		fieldLine += strlen(field) + 1; // Get field content starting point

		// Measure the field size
		int length = 0;
		while (fieldLine[length] && fieldLine[length] != '\n' && fieldLine[length] != '\r')
			length++;

		// Create a copy of the field content
		if((content = malloc(length + 1)) != NULL)
			strncpy(content, fieldLine, length);
		content[length] = '\0';
	}

	return content;
}

size_t drbMemoryWrite(const void *ptr, size_t size, size_t count, drbMemory *mem) {
	size_t realSize = size * count;

	if((mem->data = drbRealloc(mem->data, mem->size + realSize + 1)) != NULL) {
		memcpy(mem->data + mem->size, ptr, realSize);
		mem->size += realSize;
		mem->data[mem->size] = '\0';
		return realSize;
	} else
		return 0;
}

size_t drbMemoryRead(void *ptr, size_t size, size_t count, drbMemory *mem) {
	size_t realSize = size * count;
	if(mem->cursor + realSize > mem->size)
		realSize = mem->size - mem->cursor;
	if (realSize) {
		memcpy((void*)ptr, mem->data + mem->cursor, realSize);
		mem->cursor+=realSize;
	}
	return realSize;
}
