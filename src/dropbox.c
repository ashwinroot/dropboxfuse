/*!
 * \file    dropbox.h
 * \brief   Unofficial C dropbox API.
 * \author  Ashwin Sankar
 * \version 1.0
 * \date    29.10.2013
 */

#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>
#include <stdarg.h>
#include "dropbox.h"
#include "dropboxOAuth.h"
#include "dropboxJson.h"
#include "dropboxUtils.h"

#define DRBOPT_VOID 0

// Special arguments
static const long DRBSA_ACC_INFO      = DRBOPT_VOID;
static const long DRBSA_GET_FILES     = DRBOPT_ROOT | DRBOPT_PATH | DRBOPT_IO_DATA | DRBOPT_IO_FUNC;
static const long DRBSA_PUT_FILES     = DRBOPT_ROOT | DRBOPT_PATH | DRBOPT_IO_DATA | DRBOPT_IO_FUNC;
static const long DRBSA_METADATA      = DRBOPT_ROOT | DRBOPT_PATH;
static const long DRBSA_DELTA         = DRBOPT_VOID;
static const long DRBSA_REVISIONS     = DRBOPT_ROOT | DRBOPT_PATH;
static const long DRBSA_RESTORE       = DRBOPT_ROOT | DRBOPT_PATH;
static const long DRBSA_SEARCH        = DRBOPT_ROOT | DRBOPT_PATH;
static const long DRBSA_THUMBNAILS    = DRBOPT_ROOT | DRBOPT_PATH | DRBOPT_IO_DATA | DRBOPT_IO_FUNC;
static const long DRBSA_SHARES        = DRBOPT_ROOT | DRBOPT_PATH;
static const long DRBSA_MEDIA         = DRBOPT_ROOT | DRBOPT_PATH;
static const long DRBSA_COPY          = DRBOPT_VOID;
static const long DRBSA_COPY_REF      = DRBOPT_ROOT | DRBOPT_PATH;
static const long DRBSA_CREATE_FOLDER = DRBOPT_VOID;
static const long DRBSA_DELETE        = DRBOPT_VOID;
static const long DRBSA_MOVE          = DRBOPT_VOID;

// Dropbox API URIs
static const char* DRBURI_REQUEST        = "https://api.dropbox.com/1/oauth/request_token";
static const char* DRBURI_AUTHORIZATION  = "https://www.dropbox.com/1/oauth/authorize";
static const char* DRBURI_ACCESS         = "https://api.dropbox.com/1/oauth/access_token";
static const char* DRBURI_ACCOUNT_INFO   = "https://api.dropbox.com/1/account/info";
static const char* DRBURI_METADATA       = "https://api.dropbox.com/1/metadata";
static const char* DRBURI_GET_FILES      = "https://api-content.dropbox.com/1/files";
static const char* DRBURI_PUT_FILES      = "https://api-content.dropbox.com/1/files_put";
static const char* DRBURI_REVISIONS      = "https://api.dropbox.com/1/revisions";
static const char* DRBURI_SEARCH         = "https://api.dropbox.com/1/search";
static const char* DRBURI_THUMBNAILS     = "https://api-content.dropbox.com/1/thumbnails";
static const char* DRBURI_COPY           = "https://api.dropbox.com/1/fileops/copy";
static const char* DRBURI_CREATE_FOLDER  = "https://api.dropbox.com/1/fileops/create_folder";
static const char* DRBURI_DELETE         = "https://api.dropbox.com/1/fileops/delete";
static const char* DRBURI_MOVE           = "https://api.dropbox.com/1/fileops/move";
static const char* DRBURI_DELTA          = "https://api.dropbox.com/1/delta";
static const char* DRBURI_RESTORE        = "https://api.dropbox.com/1/restore";
static const char* DRBURI_SHARES         = "https://api.dropbox.com/1/shares";
static const char* DRBURI_MEDIA          = "https://api.dropbox.com/1/media";
static const char* DRBURI_COPY_REF       = "https://api.dropbox.com/1/copy_ref";

static const char* DRB_HEADER_FIELD_METADATA = "x-dropbox-metadata";

/*!
 * \struct	drbOAuthToken
 * \breif	Option expected argument types''.
 */
typedef enum {
	DRBTYPE_BOOL,
	DRBTYPE_STR,
	DRBTYPE_INT,
	DRBTYPE_PATH
} drbOptType;

/*!
 * \brief   Convert the current option argument to a string.
 * \param		ap        va_list to parse
 * \param		type      expected option argument type
 * \param[out]	value     converted argument value
 * \param[out]	ignored   indicates whether the option is ignored or not
 * \return	Error code (DRBERR_XXX).
 */
static int drbGetOptArg(va_list* ap, drbOptType type, char** value, bool* ignored) {
	int err = DRBERR_OK;
	*ignored = false;

	switch (type) {
		case DRBTYPE_BOOL: {
			bool boolValue = va_arg(*ap, int);
			if (boolValue != DRBVAL_IGNORE_BOOL) {
				if((*value = strdup(boolValue ? "true" : "false")) == NULL )
					err = DRBERR_MALLOC; // strdup went wrong
			} else *ignored = true;
			break;
		}
		case DRBTYPE_INT: {
			int intValue = va_arg(*ap, int);
			if (intValue != DRBVAL_IGNORE_INT) {
				if (intValue >= 0) {
					if(asprintf(value, "%d", intValue) == -1)
						err = DRBERR_MALLOC; // asprintf went wrong
				} else err = DRBERR_INVALID_VAL;
			} else *ignored = true;
			break;
		}
		case DRBTYPE_STR: {
			char *strValue = va_arg(*ap, char*);
			if (strValue != DRBVAL_IGNORE_STR) {
				if((*value = strdup(strValue)) == NULL)
					err = DRBERR_MALLOC;	// strdup went wrong
			} else *ignored = true;
			break;
		}
		case DRBTYPE_PATH: {
			char *strValue = va_arg(*ap, char*);
			if (strValue != DRBVAL_IGNORE_STR) {
				if ((*value = drbEncodePath(strValue)) == NULL) {
					err = DRBERR_MALLOC;
				}
			} else *ignored = true;
			break;
		}
		default: {
			err = DRBERR_UNKOWN;
			break;
		}
	}

	return err;
}

/*!
 * \brief   Parse all options and their arguments.
 *
 * Generic options parsing for API functions. There's two type of options:
 *   1. Function specific options:
 *      Argument value from these options are handled by the special handler
 *      function (sh) and all of them should appear in the va_list.
 *
 *   2. Normal options:
 *      These options (with their values) are concatenated in the args string.
 *
 * \param		ap        va_list to parse
 * \param		special   list of expected special options
 * \param[out]	args      arguments list, must be freed by the called.
 * \param	    sh        special handler function
 * \param	    shArg     special handler function argument
 * \return	Error code (DRBERR_XXX).
 */
static int drbGetOpt(va_list* ap, int special, char** args,
					 int sh(int, va_list* ap, void*, bool*), void* shArg) {
	int opt;
	int err = DRBERR_OK;
	int parsedOpts = 0;
	bool ignored;
	char *value, *name, *tmp;

	if ((*args = calloc(1, 1)) == NULL)
		err = DRBERR_MALLOC;

	while (((opt = va_arg(*ap, int)) != DRBOPT_END) && !err)
		if (!(opt & parsedOpts)) { // if argument was not already parsed...
			parsedOpts ^= opt;     // add it to the parsed list

			if (opt & special) {   // if argument need a special care...
				// and argument was handled without problem and was not ignored...
				if ((err = sh(opt, ap, shArg, &ignored)) == DRBERR_OK && !ignored)
					special ^= opt; // remove it from the list
			} else {
				// set the opt string ID and his expected value type
				drbOptType type;
				switch (opt) {
					case DRBOPT_CURSOR:        name = "cursor",          type = DRBTYPE_STR;  break;
					case DRBOPT_FILE_LIMIT:    name = "file_limit",      type = DRBTYPE_INT;  break;
					case DRBOPT_FORMAT:        name = "format",          type = DRBTYPE_STR;  break;
					case DRBOPT_FROM_COPY_REF: name = "from_copy_ref",   type = DRBTYPE_STR;  break;
					case DRBOPT_FROM_PATH:     name = "from_path",       type = DRBTYPE_PATH; break;
					case DRBOPT_HASH:          name = "hash",            type = DRBTYPE_STR;  break;
					case DRBOPT_INCL_DELETED:  name = "include_deleted", type = DRBTYPE_BOOL; break;
					case DRBOPT_LIST:          name = "list",            type = DRBTYPE_BOOL; break;
					case DRBOPT_LOCALE:        name = "locale",          type = DRBTYPE_STR;  break;
					case DRBOPT_OVERWRITE:     name = "overwrite",       type = DRBTYPE_BOOL; break;
					case DRBOPT_PATH:          name = "path",            type = DRBTYPE_PATH; break;
					case DRBOPT_PARENT_REV:    name = "parent_rev",      type = DRBTYPE_STR;  break;
					case DRBOPT_QUERY:         name = "query",           type = DRBTYPE_STR;  break;
					case DRBOPT_REV:           name = "rev",             type = DRBTYPE_STR;  break;
					case DRBOPT_REV_LIMIT:     name = "rev_limit",       type = DRBTYPE_INT;  break;
					case DRBOPT_ROOT:          name = "root",            type = DRBTYPE_STR;  break;
					case DRBOPT_SHORT_URL:     name = "short_url",       type = DRBTYPE_BOOL; break;
					case DRBOPT_SIZE:          name = "size",            type = DRBTYPE_STR;  break;
					case DRBOPT_TO_PATH:       name = "to_path",         type = DRBTYPE_PATH; break;
					default:
						err = DRBERR_UNKNOWN_OPT;
						break;
				}

				// Add the new argument to the arguments list string IF:
				//   - there is no error so far
				//   - drbGetOptArg parsed the argument with success
				//   - the argument is not ignored
				if(!err) {
					err = drbGetOptArg(ap, type, &value, &ignored);
					if (!err && !ignored) {
						if(asprintf(&tmp, "%s&%s=%s", *args, name, value) != -1) {
							free(*args);
							*args = tmp;
						} else
							err = DRBERR_MALLOC;
						free(value);
					}
				}
			}
		} else
			err = DRBERR_DUPLICATED_OPT;

	// if there's no error but all special arguments are not set...
	if (!err && special != 0)
		err = DRBERR_MISSING_OPT;

	return err;
}

/*!
 * \brief   Handle special options.
 * \param		opt       option to handle
 * \param		ap        va_list to parse
 * \param[out]	shArg     handler argument
 * \param[out]	ignored   indicates whether the option is ignored or not
 * \return	Error code (DRBERR_XXX).
 */
static int defaultHandler(int opt, va_list* ap, void* shArg, bool* ignored) {
	switch (opt) {
		case DRBOPT_ROOT:
			return drbGetOptArg(ap, DRBTYPE_STR, &(((char**)shArg)[0]), ignored);
		case DRBOPT_PATH:
			return drbGetOptArg(ap, DRBTYPE_PATH, &(((char**)shArg)[1]), ignored);
		case DRBOPT_IO_DATA:
			((void**)shArg)[2] = va_arg(*ap, void*);
			*ignored = false; // a NULL data is allowed
			return DRBERR_OK;
		case DRBOPT_IO_FUNC:
			*ignored = (((void**)shArg)[3] = va_arg(*ap, void*)) == NULL;
			return DRBERR_OK;
		default:
		    *ignored = true;
			return DRBERR_UNKOWN;
	}
}

void drbDestroyMetadata(drbMetadata* meta, bool withList) {
	if (meta) {
		free(meta->hash);
		free(meta->rev);
		free(meta->thumbExists);
		free(meta->bytes);
		free(meta->modified);
		free(meta->path);
		free(meta->isDir);
		free(meta->icon);
		free(meta->root);
		free(meta->size);
		free(meta->clientMtime);
		free(meta->isDeleted);
		free(meta->mimeType);
		free(meta->revision);

		if (withList)
			drbDestroyMetadataList(meta->contents, true);
		free(meta);
	}
}

void drbDestroyMetadataList(drbMetadataList* list, bool withMetadata) {
	if (list) {
		if (withMetadata)
			for (int i = 0; i < list->size; i++)
				drbDestroyMetadata(list->array[i], true);
		free(list->array);
		free(list);
	}
}

void drbDestroyCopyRef(drbCopyRef* ref) {
	if (ref) {
		free(ref->copyRef);
		free(ref->expires);
		free(ref);
	}
}

void drbDestroyLink(drbLink* link) {
	if (link) {
		free(link->url);
		free(link->expires);
		free(link);
	}
}

void drbDestroyAccountInfo(drbAccountInfo* info) {
	if (info) {
		free(info->referralLink);
		free(info->displayName);
		free(info->uid);
		free(info->country);
		free(info->email);
		free(info->quotaInfo.datastores);
		free(info->quotaInfo.shared);
		free(info->quotaInfo.quota);
		free(info->quotaInfo.normal);
		free(info);
	}
}

void drbDestroyDelta(drbDelta* delta, bool withMetadata) {
	if (delta) {
		free(delta->reset);
		free(delta->cursor);
		free(delta->hasMore);
		for (int i = 0; i < delta->entries.size; i++) {
			free(delta->entries.array[i].path);
			if (withMetadata)
				drbDestroyMetadata(delta->entries.array[i].metadata, true);
		}
		free(delta->entries.array);
		free(delta);
	}
}

drbClient* drbCreateClient(char* cKey, char* cSecret, char* tKey, char* tSecret) {
	drbClient* cli = NULL;
	if (cKey && cSecret) {
		if((cli = malloc(sizeof(drbClient))) != NULL) {
			cli->c.key = strdup(cKey);
			cli->c.secret = strdup(cSecret);

			cli->t.key = tKey ? strdup(tKey) : NULL;
			cli->t.secret = tSecret ? strdup(tSecret) : NULL;
		}
	}
	return cli;
}

void drbDestroyClient(drbClient* cli) {
	if (cli) {
		free(cli->c.key);
		free(cli->c.secret);
		free(cli->t.key);
		free(cli->t.secret);
		free(cli);
	}
}

void drbInit() {
	curl_global_init(CURL_GLOBAL_DEFAULT);
}

void drbCleanup() {
	curl_global_cleanup();
}

drbOAuthToken* drbObtainRequestToken(drbClient* cli) {
	drbOAuthToken* token = NULL;
	char *tKey, *tSecret;

	drbMemory answer; memset(&answer, 0, sizeof(drbMemory));
	drbOAuthRequest(cli, DRBURI_REQUEST, DRB_HTTP_POST, &answer, drbMemoryWrite, NULL);

	if (answer.data) {
		if (drbParseOauthTokenReply((char*)answer.data, &tKey, &tSecret)) {
			free(cli->t.key),    cli->t.key    = tKey;
			free(cli->t.secret), cli->t.secret = tSecret;
			token = &cli->t;
		}
		free(answer.data);
	}

	return token;
}

char* drbBuildAuthorizeUrl(drbOAuthToken* reqTok) {
	char* url = NULL;
	asprintf(&url, "%s?oauth_token=%s", DRBURI_AUTHORIZATION, reqTok->key);
	return url;
}

drbOAuthToken* drbObtainAccessToken(drbClient* cli) {
	drbOAuthToken* token = NULL;
	char *tKey, *tSecret;
	drbMemory answer; memset(&answer, 0, sizeof(drbMemory));
	drbOAuthRequest(cli, DRBURI_ACCESS, DRB_HTTP_POST, &answer, drbMemoryWrite, NULL);

	if (answer.data) {
		if (drbParseOauthTokenReply((char*)answer.data, &tKey, &tSecret)) {
			free(cli->t.key),    cli->t.key    = tKey;
			free(cli->t.secret), cli->t.secret = tSecret;
			token = &cli->t;
		}
		free(answer.data);
	}

	return token;
}

int drbGetAccountInfo(drbClient* cli, drbAccountInfo** info, ...) {
	char *args = NULL;
	char *url = NULL;

	va_list ap;
	va_start(ap, info);
	int err = drbGetOpt(&ap, DRBSA_ACC_INFO, &args, NULL, NULL);
	va_end(ap);

	if (!err) {
		int res = asprintf(&url, "%s%s", DRBURI_ACCOUNT_INFO, args);
		if(res != -1) {
			drbMemory answer; memset(&answer, 0, sizeof(drbMemory));
			err = drbOAuthRequest(cli, url, DRB_HTTP_POST, &answer, drbMemoryWrite, NULL);
			if(!err && info)
				*info = drbParseAccountInfo(answer.data);
			free(answer.data);
			free(url);
		} else
			err = DRBERR_MALLOC;
	}

	free(args);

	return err;
}

int drbGetMetadata(drbClient* cli, drbMetadata** meta, ...) {
	char *args = NULL;
	char *url = NULL;
	void *sArgs[2]; memset(sArgs, 0, 2 * sizeof(void*));

	va_list ap;
	va_start(ap, meta);
	int err = drbGetOpt(&ap, DRBSA_METADATA, &args, defaultHandler, sArgs);
	va_end(ap);

	if (!err) {
		int res = asprintf(&url, "%s/%s%s?%s", DRBURI_METADATA, (char*)sArgs[0], (char*)sArgs[1], args);
		if (res != -1) {
			drbMemory answer; memset(&answer, 0, sizeof(drbMemory));
			err = drbOAuthRequest(cli, url, DRB_HTTP_GET, &answer, drbMemoryWrite, NULL);
			if (!err && meta)
				*meta = drbParseMetadata(answer.data);
			free(answer.data);
			free(url);
		} else
			err = DRBERR_MALLOC;
	}

	free(args),	free(sArgs[0]),	free(sArgs[1]);

	return err;
}

int drbGetFile(drbClient* cli, drbMetadata** meta, ...) {
	char *args = NULL;
	char *url = NULL;
	void *sArgs[4]; memset(sArgs, 0, 4 * sizeof(void*));

	va_list ap;
	va_start(ap, meta);
	int err = drbGetOpt(&ap, DRBSA_GET_FILES, &args, defaultHandler, sArgs);
	va_end(ap);

	if (!err) {
		int res = asprintf(&url,"%s/%s%s%s", DRBURI_GET_FILES, (char*)sArgs[0], (char*)sArgs[1], args);
		if (res != -1) {
			char* header = NULL;
			char** headerPtr = meta ? &header : NULL;
			err = drbOAuthRequest(cli, url, DRB_HTTP_GET, sArgs[2], sArgs[3], headerPtr);
			if (!err && meta && header) {
				char* answer = drbGetHeaderFieldContent(DRB_HEADER_FIELD_METADATA, header);
				*meta = drbParseMetadata(answer);
				free(answer);
			}
			free(header);
			free(url);
		} else
			err = DRBERR_MALLOC;
	}

	free(args),	free(sArgs[0]),	free(sArgs[1]);

	return err;
}

int drbGetRevisions(drbClient* cli, drbMetadataList** list, ...) {
	char *args = NULL;
	char *url = NULL;
	void *sArgs[2]; memset(sArgs, 0, 2 * sizeof(void*));

	va_list ap;
	va_start(ap, list);
	int err = drbGetOpt(&ap, DRBSA_REVISIONS, &args, defaultHandler, sArgs);
	va_end(ap);

	if (!err) {
		int res = asprintf(&url,"%s/%s%s%s", DRBURI_REVISIONS, (char*)sArgs[0], (char*)sArgs[1], args);
		if (res != -1) {
			drbMemory answer; memset(&answer, 0, sizeof(drbMemory));
			err = drbOAuthRequest(cli, url, DRB_HTTP_GET, &answer, drbMemoryWrite, NULL);
			if (!err && list)
				*list = drbStrParseMetadataList(answer.data);
			free(answer.data);
			free(url);
		} else
			err = DRBERR_MALLOC;
	}

	free(args),	free(sArgs[0]),	free(sArgs[1]);
	return err;
}

int drbSearch(drbClient* cli, drbMetadataList** list, ...) {
	char *args = NULL;
	char *url = NULL;
	void *sArgs[2]; memset(sArgs, 0, 2 * sizeof(void*));

	va_list ap;
	va_start(ap, list);
	int err = drbGetOpt(&ap, DRBSA_SEARCH, &args, defaultHandler, sArgs);
	va_end(ap);

	if (!err) {
		int res = asprintf(&url,"%s/%s%s%s", DRBURI_SEARCH, (char*)sArgs[0], (char*)sArgs[1], args);
		if (res != -1) {
			drbMemory answer; memset(&answer, 0, sizeof(drbMemory));
			err = drbOAuthRequest(cli, url, DRB_HTTP_GET, &answer, drbMemoryWrite, NULL);
			if (!err && list)
				*list = drbStrParseMetadataList(answer.data);
			free(answer.data);
			free(url);
		} else
			err = DRBERR_MALLOC;
	}

	free(args),	free(sArgs[0]),	free(sArgs[1]);
	return err;
}

int drbGetThumbnail(drbClient* cli, drbMetadata** meta, ...) {
	char *args = NULL;
	char *url = NULL;
	void *sArgs[4]; memset(sArgs, 0, 4 * sizeof(void*));

	va_list ap;
	va_start(ap, meta);
	int err = drbGetOpt(&ap, DRBSA_THUMBNAILS, &args, defaultHandler, sArgs);
	va_end(ap);

	if (!err) {
		int res = asprintf(&url,"%s/%s%s%s", DRBURI_THUMBNAILS, (char*)sArgs[0], (char*)sArgs[1], args);
		if (res != -1) {
			char* header = NULL;
			char** headerPtr = meta ? &header : NULL;
			err = drbOAuthRequest(cli, url, DRB_HTTP_GET, sArgs[2], sArgs[3], headerPtr);
			if (!err && meta && header) {
				char* answer = drbGetHeaderFieldContent(DRB_HEADER_FIELD_METADATA, header);
				*meta = drbParseMetadata(answer);
				free(answer);
			}
			free(header);
			free(url);
		} else
			err = DRBERR_MALLOC;
	}

	free(args),	free(sArgs[0]),	free(sArgs[1]);

	return err;
}

int drbCopy(drbClient* cli, drbMetadata** meta, ...) {
	char *args = NULL;
	char *url = NULL;

	va_list ap;
	va_start(ap, meta);
	int err = drbGetOpt(&ap, DRBSA_COPY, &args, NULL, NULL);
	va_end(ap);

	if (!err) {
		int res = asprintf(&url,"%s%s", DRBURI_COPY, args);
		if (res != -1) {
			drbMemory answer; memset(&answer, 0, sizeof(drbMemory));
			err = drbOAuthRequest(cli, url, DRB_HTTP_POST, &answer, drbMemoryWrite, NULL);
			if (!err && meta)
				*meta = drbParseMetadata(answer.data);
			free(answer.data);
			free(url);
		} else
			err = DRBERR_MALLOC;
	}

	free(args);
	return err;
}

int drbCreateFolder(drbClient* cli, drbMetadata** meta, ...) {
	char *args = NULL;
	char *url = NULL;

	va_list ap;
	va_start(ap, meta);
	int err = drbGetOpt(&ap, DRBSA_CREATE_FOLDER, &args, NULL, NULL);
	va_end(ap);

	if (!err) {

		int res = asprintf(&url,"%s%s", DRBURI_CREATE_FOLDER, args);
		if (res != -1) {
			drbMemory answer; memset(&answer, 0, sizeof(drbMemory));
			err = drbOAuthRequest(cli, url, DRB_HTTP_POST, &answer, drbMemoryWrite, NULL);
			if (!err && meta)
				*meta = drbParseMetadata(answer.data);
			free(answer.data);
			free(url);
		} else
			err = DRBERR_MALLOC;
	}

	free(args);
	return err;
}

int drbDelete(drbClient* cli, drbMetadata** meta, ...) {
	char *args = NULL;
	char *url = NULL;

	va_list ap;
	va_start(ap, meta);
	int err = drbGetOpt(&ap, DRBSA_DELETE, &args, NULL, NULL);
	va_end(ap);

	if (!err) {
		int res = asprintf(&url,"%s%s", DRBURI_DELETE, args);
		if (res != -1) {
			drbMemory answer; memset(&answer, 0, sizeof(drbMemory));
			err = drbOAuthRequest(cli, url, DRB_HTTP_POST, &answer, drbMemoryWrite, NULL);
			if (!err && meta)
				*meta = drbParseMetadata(answer.data);
			free(answer.data);
			free(url);
		} else
			err = DRBERR_MALLOC;

	}

	free(args);
	return err;
}

int drbMove(drbClient* cli, drbMetadata** meta, ...) {
	char *args = NULL;
	char *url = NULL;

	va_list ap;
	va_start(ap, meta);
	int err = drbGetOpt(&ap, DRBSA_MOVE, &args, NULL, NULL);
	va_end(ap);

	if (!err) {
		int res = asprintf(&url,"%s%s", DRBURI_MOVE, args);
		if (res != -1) {
			drbMemory answer; memset(&answer, 0, sizeof(drbMemory));
			err = drbOAuthRequest(cli, url, DRB_HTTP_POST, &answer, drbMemoryWrite, NULL);
			if (!err && meta)
				*meta = drbParseMetadata(answer.data);
			free(answer.data);
			free(url);
		} else
			err = DRBERR_MALLOC;
	}

	free(args);
	return err;
}

int drbGetDelta(drbClient* cli, drbDelta** delta, ...) {
	char *args = NULL;
	char *url = NULL;

	va_list ap;
	va_start(ap, delta);
	int err = drbGetOpt(&ap, DRBSA_DELTA, &args, NULL, NULL);
	va_end(ap);

	if (!err) {
		int res = asprintf(&url,"%s?%s", DRBURI_DELTA, args);
		if (res != -1) {
			drbMemory answer; memset(&answer, 0, sizeof(drbMemory));
			err = drbOAuthRequest(cli, url, DRB_HTTP_POST, &answer, drbMemoryWrite, NULL);
			if (!err && delta)
				*delta = drbParseDelta(answer.data);
			free(answer.data);
			free(url);
		} else
			err = DRBERR_MALLOC;
	}
	free(args);

	return err;
}

int drbRestore(drbClient* cli, drbMetadata** meta, ...) {
	char *args = NULL;
	char *url = NULL;
	void *sArgs[2]; memset(sArgs, 0, 2 * sizeof(void*));

	va_list ap;
	va_start(ap, meta);
	int err = drbGetOpt(&ap, DRBSA_RESTORE, &args, defaultHandler, sArgs);
	va_end(ap);

	if (!err) {
		int res = asprintf(&url,"%s/%s%s%s", DRBURI_RESTORE, (char*)sArgs[0], (char*)sArgs[1], args);
		if (res != -1) {
			drbMemory answer; memset(&answer, 0, sizeof(drbMemory));
			err = drbOAuthRequest(cli, url, DRB_HTTP_POST, &answer, drbMemoryWrite, NULL);
			if (!err && meta)
				*meta = drbParseMetadata(answer.data);
			free(answer.data);
			free(url);
		} else
			err = DRBERR_MALLOC;
	}

	free(args),	free(sArgs[0]),	free(sArgs[1]);
	return err;
}

int drbShare(drbClient* cli, drbLink** link, ...) {
	char *args = NULL;
	char *url = NULL;
	void *sArgs[2]; memset(sArgs, 0, 2 * sizeof(void*));

	va_list ap;
	va_start(ap, link);
	int err = drbGetOpt(&ap, DRBSA_SHARES, &args, defaultHandler, sArgs);
	va_end(ap);

	if (!err) {
		int res = asprintf(&url,"%s/%s%s%s", DRBURI_SHARES, (char*)sArgs[0], (char*)sArgs[1], args);
		if (res != -1) {
			drbMemory answer; memset(&answer, 0, sizeof(drbMemory));
			err = drbOAuthRequest(cli, url, DRB_HTTP_POST, &answer, drbMemoryWrite, NULL);
			if (!err && link)
				*link = drbParseLink(answer.data);
			free(answer.data);
			free(url);
		} else
			err = DRBERR_MALLOC;
	}

	free(args),	free(sArgs[0]),	free(sArgs[1]);
	return err;
}

int drbGetMedia(drbClient* cli, drbLink** link, ...) {
	char *args = NULL;
	char *url = NULL;
	void *sArgs[2]; memset(sArgs, 0, 2 * sizeof(void*));

	va_list ap;
	va_start(ap, link);
	int err = drbGetOpt(&ap, DRBSA_MEDIA, &args, defaultHandler, sArgs);
	va_end(ap);

	if (!err) {
		int res = asprintf(&url,"%s/%s%s?%s", DRBURI_MEDIA, (char*)sArgs[0], (char*)sArgs[1], args);
		if (res != -1) {
			drbMemory answer; memset(&answer, 0, sizeof(drbMemory));
			err = drbOAuthRequest(cli, url, DRB_HTTP_POST, &answer, drbMemoryWrite, NULL);
			if (!err && link)
				*link = drbParseLink(answer.data);
			free(answer.data);
			free(url);
		} else
			err = DRBERR_MALLOC;
	}

	free(args),	free(sArgs[0]),	free(sArgs[1]);
	return err;
}

int drbGetCopyRef(drbClient* cli, drbCopyRef** ref, ...) {
	char *args = NULL;
	char *url = NULL;
	void *sArgs[2]; memset(sArgs, 0, 2 * sizeof(void*));

	va_list ap;
	va_start(ap, ref);
	int err = drbGetOpt(&ap, DRBSA_COPY_REF, &args, defaultHandler, sArgs);
	va_end(ap);

	if (!err) {
		int res = asprintf(&url,"%s/%s%s%s", DRBURI_COPY_REF, (char*)sArgs[0], (char*)sArgs[1], args);
		if (res != -1) {
			drbMemory answer; memset(&answer, 0, sizeof(drbMemory));
			err = drbOAuthRequest(cli, url, DRB_HTTP_GET, &answer, drbMemoryWrite, NULL);
			if (!err && ref)
				*ref = drbParseCopyRef(answer.data);
			free(answer.data);
			free(url);
		} else
			err = DRBERR_MALLOC;
	}

	free(args),	free(sArgs[0]),	free(sArgs[1]);
	return err;
}

int drbPutFile(drbClient* cli, drbMetadata** meta, ...) {
	char *args = NULL;
	char *url = NULL;
	void *sArgs[4]; memset(sArgs, 0, 4 * sizeof(void*));

	va_list ap;
	va_start(ap, meta);
	int err = drbGetOpt(&ap, DRBSA_PUT_FILES, &args, defaultHandler, sArgs);
	va_end(ap);

	if (!err) {
		int res =  asprintf(&url, "%s/%s%s%s", DRBURI_PUT_FILES, (char*)sArgs[0], (char*)sArgs[1], args);
		if (res != -1) {
			char *answer = NULL;
			err = drbOAuthPostFile(cli, url, sArgs[2], sArgs[3], meta? &answer:NULL);
			if (!err && meta && answer)
				*meta = drbParseMetadata(answer);
			free(answer);
			free(url);
		} else
			err = DRBERR_MALLOC;
	}

	free(args),	free(sArgs[0]),	free(sArgs[1]);
	return err;
}
