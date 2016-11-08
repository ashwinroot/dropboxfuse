/*!
 * \file    dropboxOAuth.h
 * \brief   OAuth library for dropbox library.
 * \author  Ashwin Sankar
 * \version 1.0
 * \date    29.10.2013
 */

#ifndef DROPBOX_OAUTH_H
#define DROPBOX_OAUTH_H

#include <stdbool.h>
#include "dropbox.h"

/*!
 * \struct	drbClient
 * \breif	Client data used the whole 
 */
struct drbClient{
	drbOAuthToken c; 
	drbOAuthToken t;
};

typedef enum {
	DRB_HTTP_GET,
	DRB_HTTP_POST
} drbHttpMethod;


int drbOAuthRequest(drbClient* cli, const char* url, drbHttpMethod method, 
					void* data, void* writeFct, char** header);
int drbOAuthPostFile(drbClient* cli, const char *url, void* data, 
					 ssize_t (*readFct)(void *, size_t , size_t , FILE *), 
					 char** answer);
char *drbEncodePath(const char *string);
bool drbParseOauthTokenReply(const char *answer, char **key, char **secret);

#endif /* DROPBOX_OAUTH_H */