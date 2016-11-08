/*!
 * \file    dropboxOAuth.h
 * \brief   OAuth library for dropbox library.
 * \author  Ashwin Sankar
 * \version 1.0
 * \date    29.10.2013
 */

#define _GNU_SOURCE

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <oauth.h>
#include <curl/curl.h>
#include "dropboxOAuth.h"
#include "dropboxUtils.h"

#define BUFFER_SIZE 1024


/*!
 * \brief	Found a copy the OAuth key and secret from the server answer.
 * \param       answer   server raw answer
 * \param[out]  key      char* where the OAuth key should be stored
 * \param[out]  secret   char* where the OAuth secret should be stored
 * \return	indicates whether the key and the secret were founded or not.
 */
bool drbParseOauthTokenReply(const char *answer, char **key, char **secret) {
	bool ok = true;
	char **rv = NULL;
	char* params[] = {
		"oauth_token=",
		"oauth_token_secret="
	};
	
	*key = *secret = NULL;
	
	int nbParam = sizeof(params) / sizeof(params[0]);
	
	int rc = oauth_split_url_parameters(answer, &rv);	
	qsort(rv, rc, sizeof(char *), oauth_cmpstringp);
	
	if (rc >= nbParam) {
		int *paramIndex, j = 0;
		size_t paramLen = strlen(params[j]);
		if ((paramIndex = malloc(nbParam * sizeof(int))) != NULL) {
			for (int i = 0; i < rc && j < nbParam; i++)
				if (!strncmp(params[j], rv[i], paramLen)) {
					paramIndex[j++] = i;
					if (j < nbParam) 
						paramLen = strlen(params[j]); 
				}
			if (j == nbParam) {
				if (key)    *key    = strdup(&(rv[paramIndex[0]][12]));
				if (secret) *secret = strdup(&(rv[paramIndex[1]][19]));
			}
			free(paramIndex);
		}else	ok = false;
	} else	ok = false;
	
	if (!ok) {
		free(*key), *key = NULL;
		free(*secret), *secret = NULL;
	}
	
	free(rv);
	return ok;
}

/*!
 * \brief	Encode a path string to be OAuth complient.
 * 
 * This is just a slight modification of oauth_url_escape from liboauth library:
 *   -# '/' is not escaped anymore
 *   -# memory allocation is checked
 *   -# code aspect was modified to match the library coding convention
 *
 * \param   path   path to encode
 * \return	encoded path (must be freed by caller)
 */
char *drbEncodePath(const char *path) {
	size_t alloc, newLength, length, i = 0;
	
	newLength = alloc = ( length = (path ? strlen(path) : 0) ) + 1;
	
	char *encPath = (char*) malloc(alloc);
	
	// loop while string end isnt reached and while ns is a valid pointer
	while(length-- && encPath) { 
		unsigned char in = *path++;
		
		switch(in){
			case '0': case '1': case '2': case '3': case '4':
			case '5': case '6': case '7': case '8': case '9':
			case 'a': case 'b': case 'c': case 'd': case 'e':
			case 'f': case 'g': case 'h': case 'i': case 'j':
			case 'k': case 'l': case 'm': case 'n': case 'o':
			case 'p': case 'q': case 'r': case 's': case 't':
			case 'u': case 'v': case 'w': case 'x': case 'y': case 'z':
			case 'A': case 'B': case 'C': case 'D': case 'E':
			case 'F': case 'G': case 'H': case 'I': case 'J':
			case 'K': case 'L': case 'M': case 'N': case 'O':
			case 'P': case 'Q': case 'R': case 'S': case 'T':
			case 'U': case 'V': case 'W': case 'X': case 'Y': case 'Z':
			case '_': case '~': case '.': case '-': case '/':
				encPath[i++] = in;
				break;
			default:
				newLength += 2; /* this'll become a %XX */
				if(newLength > alloc) {
					alloc *= 2;
					if((encPath = drbRealloc(encPath, alloc)) == NULL)
						continue; // 
				}
				snprintf(&encPath[i], 4, "%%%02X", in);
				i += 3;
				break;
		}
	}
	
	// Shrink and finilaze the new string if ns is a valid pointer
	if (encPath && (encPath = drbRealloc(encPath, newLength)) != NULL) {
		encPath[i] = '\0';
	}
	
	return encPath;
}

/*!
 * \brief   Perform an OAuth GET or POST request for a Dropbox client.
 * \param        cli        authenticated dropbox client
 * \param        url        request base url
 * \param        method     DRB_HTTP_GET or DRB_HTTP_POST
 * \param        data       where the request answer is written (e.g. FILE*)
 * \param        writeFct   called function to write in data (e.g. fwrite)
 * \param[out]   header     server answer header (must be freed by caller)
 * \return	error code (DRBERR_XXX or http error returned by the Dropbox server)
 */
int drbOAuthRequest(drbClient* cli, const char* url, drbHttpMethod method, 
					void* data, void* writeFct, char** header) {
	
	long httpCode = 0;
	char* postArg = NULL;
	char* reqUrl = oauth_sign_url2(url, method ? &postArg : NULL, OA_HMAC, NULL, 
								   cli->c.key, cli->c.secret, 
								   cli->t.key, cli->t.secret);
	// Perform the request
    CURL *curl = curl_easy_init();
    if (curl) {
		drbMemory headerData; memset(&headerData, 0, sizeof(drbMemory));

		curl_easy_setopt(curl, CURLOPT_URL, reqUrl);
		if (postArg)
			curl_easy_setopt(curl, CURLOPT_POSTFIELDS, postArg);
		
		// Only write the answer if there's at least the write function
		if (writeFct) {
			curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeFct);
			curl_easy_setopt(curl, CURLOPT_WRITEDATA, data);
		} else
			curl_easy_setopt(curl, CURLOPT_NOBODY, 1L);
				
		if (header) { // Only read the header if it's needed
			curl_easy_setopt(curl, CURLOPT_HEADERDATA, &headerData);
			curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, drbMemoryWrite);
		}
		
		// General curl options
		curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L); // thread safe requirement
		curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 1L);	
        curl_easy_setopt(curl, CURLOPT_FAILONERROR, 1L); // do not write errors
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0);
		
		curl_easy_perform(curl);
		curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &httpCode);
        curl_easy_cleanup(curl);
		
		if (header) *header = headerData.data;
		
    }
	
	free(postArg);
	free(reqUrl);
	
	return httpCode == 200 ? DRBERR_OK : httpCode;
}

/*!
 * \brief   Perform an OAuth GET or POST request for a Dropbox client.
 * \param        cli        authenticated dropbox client
 * \param        url        request base url
 * \param        data       where the request answer is readed (e.g. FILE*)
 * \param        readFct    called function to read data content (e.g. fwrite)
 * \param[out]   answer     server answer (must be freed by caller)
 * \return	error code (DRBERR_XXX or http error returned by the Dropbox server)
 */
int drbOAuthPostFile(drbClient* cli, const char *url, void* data,
					 ssize_t (*readFct)(void *, size_t , size_t , FILE *),
				     char** answer) {
	
	char buffer[BUFFER_SIZE], *tmpUrl1, *tmpUrl2, *reqUrl, *postArg = NULL;
	long len, httpCode;
	
	// Load file in memory
	drbMemory fileData; memset(&fileData, 0, sizeof(drbMemory));
	size_t allocSize = 0;
	
	while ((len = readFct(buffer, sizeof(char), BUFFER_SIZE, data)) > 0) {
		fileData.size += len;
		if (fileData.size > allocSize) {
			allocSize = fileData.size * 2;
			if ((fileData.data = drbRealloc(fileData.data, allocSize)) == NULL)
				break;
		}
		memcpy(fileData.data + fileData.size - len, buffer, len);
	} 
	
	// if allocated memory was shrinked with success...
	if ((fileData.data = drbRealloc(fileData.data, fileData.size)) != NULL) {

		// Build request url
		char* key = oauth_catenc(2, cli->c.secret, cli->t.secret);
		char* sign = oauth_sign_hmac_sha1_raw(fileData.data, fileData.size, 
											  key, strlen(key));
		
		asprintf(&tmpUrl1,"%s&xoauth_body_signature=%s&param=val"
		                  "&xoauth_body_signature_method=HMAC_SHA1", url, sign);
		
		tmpUrl2 = oauth_sign_url2(tmpUrl1, &postArg, OA_HMAC, NULL, 
								  cli->c.key, cli->c.secret, 
								  cli->t.key, cli->t.secret);
		
		asprintf(&reqUrl,"%s?%s", tmpUrl2, postArg);
		
		free(key), free(sign), free(tmpUrl1), free(tmpUrl2), free(postArg);
		
		// Perform the request
		CURL *curl = curl_easy_init();
		if(curl) {
			drbMemory answerData; memset(&answerData, 0, sizeof(drbMemory));

			char* header;
			asprintf(&header, "Content-Type: application/octet-stream\r\n " 
			                  "Content-Length: %lu\r\n"
			                  "accept-ranges: bytes", len);
			
			struct curl_slist *slist = curl_slist_append(NULL, header);
			
			curl_easy_setopt(curl, CURLOPT_URL, reqUrl);
			curl_easy_setopt(curl, CURLOPT_POST, 1L);
			curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, fileData.size);
			curl_easy_setopt(curl, CURLOPT_HTTPHEADER, slist); 
			curl_easy_setopt(curl, CURLOPT_READDATA, &fileData);
			curl_easy_setopt(curl, CURLOPT_READFUNCTION, drbMemoryRead);
			
			if (answer) {
				curl_easy_setopt(curl, CURLOPT_WRITEDATA, &answerData);
				curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, drbMemoryWrite);
			}
			
			// General curl options
			curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L); // thread safe requirement
			curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 1L);		
			curl_easy_setopt(curl, CURLOPT_FAILONERROR, 1L);
	        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0);

			curl_easy_perform(curl);
			curl_easy_getinfo (curl, CURLINFO_RESPONSE_CODE, &httpCode);
			
			curl_slist_free_all(slist);
			curl_easy_cleanup(curl);
			
			free(header);
			free(fileData.data);
			
			if (answer) { 
				*answer = answerData.data;
				answerData.data = NULL;
			}
		}
	}
	
	free(reqUrl);
	return httpCode == 200 ? DRBERR_OK : httpCode;
}
