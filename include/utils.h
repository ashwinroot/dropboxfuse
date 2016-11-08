/*!
 * \file    utils.h
 * \brief   Some useful functions.
 * \author  ashwin sankar
 * \version 1.0
 * \date    15.01.2014
 */

#ifndef __UTILS_H
#define __UTILS_H

#include <sys/stat.h>
#include <stdio.h>

/*!
 * \brief	Recursively create folders to create a path.
 * \param	path   Path to 'dig'
 * \param	mode   Access mode
 * \return	void
 */
void mkdirDeep(char* path, mode_t mode);

/*!
 * \brief	Recursively create folders to create a file path.
 * \param	path   File path to 'dig' (file wont be created)
 * \param	mode   Access mode
 * \return	void
 */
void mkdirFile(char* path, mode_t mode);

/*!
 * \brief	Read the next line from an opened text file.
 * \param	file   file to read.
 * \return	Allocated string with readed line.
 */
char* readLine(FILE *file);

#endif /* __UTILS_H */
