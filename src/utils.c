/*!
 * \file    utils.c
 * \brief   Some useful functions.
 * \author  ashwin sankar
 * \version 1.0
 * \date    15.01.2014
 */

#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include "utils.h"

/*****************************[ Static functions ]*****************************/

/*!
 * \brief	Recursive mkdir.
 * \param	path     path to create
 * \param	mode     path access mode
 * \param	size     path size
 * \param	ignore   bottom end sub path to ignore in path creation
 * \return	void
 */
static void rmkdir(char* path, mode_t mode, size_t size, unsigned int ignore){
	if(access(path , F_OK) != 0) {
		do {
			size--;
		} while(size >= 0 && path[size] != '/');

		path[size] = '\0';
		rmkdir(path, mode, size, ignore ? ignore -1 : 0);
		path[size] = '/';
		if (!ignore)
			mkdir(path, mode);
	}
}

/*****************************[ Public functions ]*****************************/

void mkdirDeep(char* path, mode_t mode) {
	int size = strlen(path);
	char* cpath = malloc(size+1);
	strcpy(cpath, path);
	rmkdir(cpath, mode, size, 0);
	free(cpath);
}

void mkdirFile(char* path, mode_t mode) {
	int size = strlen(path);
	char* cpath = malloc(size+1);
	strcpy(cpath, path);
	rmkdir(cpath, mode, size, 1);
	free(cpath);
}

char* readLine(FILE *file) {
	int c, length = 0, buffSize = 10;
	char* buffer = malloc(buffSize);

	while((c = fgetc(file)) != EOF && c != '\n') {
		if(++length > buffSize || buffer == NULL) {
			buffSize *= 2;
			buffer = realloc(buffer, buffSize);
		}
		buffer[length-1] = c;
	}

	buffer = realloc(buffer, length+1);
	buffer[length] = '\0';

	return length != 0 ? buffer : NULL;
}
