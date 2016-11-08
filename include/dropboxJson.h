/*!
 * \file    dropboxJson.h
 * \brief   Json parsing library for dropbox library structures.
 * \author  Ashwin Sankar
 * \version 1.0
 * \date    29.10.2013
 */

#ifndef DROPBOX_JSON_H
#define DROPBOX_JSON_H

#include "dropbox.h"

drbCopyRef* drbParseCopyRef(char* str);
drbLink* drbParseLink(char* str);
drbMetadataList* drbParseMetadataList(char* str);
drbMetadata* drbParseMetadata(char* str);
drbAccountInfo* drbParseAccountInfo(char* str);
drbDelta* drbParseDelta(char* str);
drbMetadataList* drbStrParseMetadataList(char *str);

#endif /* DROPBOX_JSON_H */