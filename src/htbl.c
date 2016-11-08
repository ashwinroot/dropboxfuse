/*!
 * \file    htbl.c
 * \brief   Hash table library source code.
 * \author  ashwin sankar
 * \version 2.0
 * \date    06.01.2014
 */

#include "htbl.h"

#define HASH_NUMBER 5381

/*!
 * \struct	entryNode
 * \breif	Table entry node structure.
 *
 * Table entry associated to an hash code. Contains the key/data pair
 * and link to the next and previous entry with a the same hash code.
 *
 */
struct entryNode{
	struct {
		char* key;  /*!< String key of this entry. */
		void* data; /*!< Data associated to the key. */
	} pair;         /*!< Key/data pair of this entry */
	struct entryNode* next;     /*!< Next entry with the same hash code. */
	struct entryNode* previous; /*!< Previous entry with the same hash code. */
};

/*!
 * \struct	htbl
 * \breif	Hash table structure.
 *
 * Hash table structure used to store entries associated to a string key.
 *
 */
struct htbl{
	struct entryNode** matrix; /*!< Store table entries. */
	unsigned int size;         /*!< Matrix size. */
};

/*****************************[ Static functions ]*****************************/

/*!
 * \brief	Compute an hash code for a key.
 * \param	key   Key to hash.
 * \return	key hash code.
 */
static unsigned hashKey(char* key){
	unsigned int i, hash = HASH_NUMBER;
	for (i = 0; key[i]; i++)
		hash = ((hash << 5) + hash) + key[i];
	return hash;
}

/*!
 * \brief	Found a node in the table from the key.
 * \param	key     Key of node.
 * \param	tbl     Table to query.
 * \param	index   Node index in the table.
 * \return	Founded node.
 */
static struct entryNode* getNodeForKey(htbl* tbl, char* key, unsigned *index){
	*index = hashKey(key) % tbl->size;
	struct entryNode* node = tbl->matrix[*index];
	while(node && strcmp(key, node->pair.key) != 0)
		node = node->next;
	return node;
}

/*!
 * \brief	Insert a new node at first position of the chained list.
 * \param	tbl     Table to query.
 * \param	index   Node index in the table.
 * \return	Created node.
 */
static struct entryNode* newNode(htbl* tbl, unsigned int index) {
	struct entryNode* newNode = malloc(sizeof(struct entryNode));
	newNode->next = tbl->matrix[index];
	newNode->previous = NULL;
	tbl->matrix[index] = newNode;
	if (newNode->next != NULL)
		newNode->next->previous = newNode;
	return newNode;
}

/*****************************[ Public functions ]*****************************/

htbl* htblCreate(unsigned int size) {
	htbl* tbl = malloc(sizeof(htbl));
	tbl->matrix = calloc(size, sizeof(struct entryNode*));
	tbl->size = size;
	return tbl;
}

void htblDestroy(htbl* tbl, void(*freeData)(void* data, void* arg), void* arg) {
	for (int i = 0; i < tbl->size; i++) 
		for (struct entryNode *next, *n = tbl->matrix[i]; n != NULL; n = next) {
			next = n->next;
			free(n->pair.key);
			freeData(n->pair.data, arg);
		}
	free(tbl->matrix);
	free(tbl);
}

void* htblGet(htbl* tbl, char* key) {
	unsigned int index;
	struct entryNode* node = getNodeForKey(tbl, key, &index);
	return node ? node->pair.data : NULL;
}

void* htblSet(htbl* tbl, char* key, void* data) {
	unsigned int tblIndex;
	void* previousData = NULL;
	struct entryNode* node = getNodeForKey(tbl, key, &tblIndex);
	if(node != NULL) {
		previousData = node->pair.data;
		node->pair.data = data;
	} else {
		node = newNode(tbl, tblIndex);
		node->pair.key = malloc(sizeof(char)*(strlen(key)+1));
		strcpy(node->pair.key, key);
		node->pair.data = data;
	}
	return previousData;
}

void* htblRemove(htbl* tbl, char* key) {
	unsigned int index;
	struct entryNode* node = getNodeForKey(tbl, key, &index);
	void* data = NULL;
	if (node != NULL) {
		data = node->pair.data;
		if(node->previous == NULL && node->next == NULL)
			tbl->matrix[index] = NULL;
		else {
			if (node->previous != NULL)
				node->previous->next = node->next;
			if (node->next != NULL)
				node->next->previous = node->previous;
		}
		free(node);
	}
	return data;
}

bool htblExists(htbl* tbl, char* key) {
	unsigned int tblIndex;
	return getNodeForKey(tbl, key, &tblIndex) != NULL;
}

unsigned int htblCount(htbl* tbl) {
	unsigned int count = 0;
	for (int i = 0; i < tbl->size; i++)
		for (struct entryNode* n = tbl->matrix[i] ; n != NULL ; n = n->next)
			count++;
	return count;
}

void htblForEach(htbl* tbl, bool(*action)(char* key, void* data, void* arg), void* arg) {
	for (int i = 0; i < tbl->size; i++) {
		struct entryNode* n = tbl->matrix[i];
		while (n != NULL && action(n->pair.key, n->pair.data, arg))
			n = n->next;
	}
}
