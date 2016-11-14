/*!
 * \file    htbl.h
 * \brief   Hash table library header.
 * \author  ashwin sankar
 * \version 2.0
 */

#ifndef __HTBL_H
#define __HTBL_H

#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

typedef struct htbl htbl;

/*!
 * \brief	Create an empty hash table.
 * \param	size   Table size.
 * \return	Created hash table.
 */
htbl* htblCreate(unsigned int size);

/*!
 * \brief	Destroy an hash table.
 * \param	tbl        Table to destroy.
 * \param	freeData   Function called to free hash table data.
 * \param	arg        Argument for freeData function.
 * \return	Nothing.
 */
void htblDestroy(htbl* tbl, void(*freeData)(void* data, void* arg), void* arg);

/*!
 * \brief	Found data associated to the key.
 * \param	tbl   Table to query.
 * \param	key   Key of data.
 * \return	Founded data.
 */
void* htblGet(htbl* tbl, char* key);

/*!
 * \brief	Set a data to a key in the table.
 * \param	tbl    Table to update.
 * \param	key    Key to set.
 * \param	data   Data to set for the key.
 * \return	Previous data.
 */
void* htblSet(htbl* tbl, char* key, void* data);

/*!
 * \brief	Remove a command from the table.
 * \param	key    Key to remove.
 * \param	tbl    Table to update.
 * \return	Removed key data.
 */
void* htblRemove(htbl* tbl, char* key);

/*!
 * \brief	Check whether the key exists in the table.
 * \param	key    Key to check.
 * \param	tbl    Table to query.
 * \return	Boolean that indicates whether the key was found or not.
 */
bool htblExists(htbl* tbl, char* key);

/*!
 * \brief	Count hash table entries.
 * \param	tbl    Table to query.
 * \return	Hash table entries count.
 */
unsigned int htblCount(htbl* tbl);

/*!
 * \brief	Do something for each hash table entry.
 * \param	tbl      Table to process.
 * \param	action   Function called for each table entry.
 * \param	arg      Action function argument.
 * \return	Hash table size.
 */
void htblForEach(htbl* tbl, bool(*action)(char* key, void* data, void* arg), void* arg);


#endif /* __HTBL_H */
