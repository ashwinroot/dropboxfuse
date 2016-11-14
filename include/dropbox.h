/*!
 * \file    dropbox.h
 * \brief   Unofficial C dropbox API.
 * \author  Ashwin Sankar
 * \version 1.0
 */

#ifndef DROPBOX_H
#define DROPBOX_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include <stdbool.h>

#define DRBVAL_SIZE_XSMALL "xs"
#define DRBVAL_SIZE_SMALL  "s"
#define DRBVAL_SIZE_MEDIUM "m"
#define DRBVAL_SIZE_LARGE  "l"
#define DRBVAL_SIZE_XLARGE "xl"
#define DRBVAL_ROOT_DROPBOX "dropbox"
#define DRBVAL_ROOT_SANDBOX "sandbox"
#define DRBVAL_IGNORE_BOOL -1
#define DRBVAL_IGNORE_STR  NULL
#define DRBVAL_IGNORE_INT  -1
#define DRBVAL_IGNORE_SIZE DRBVAL_IGNORE_STR

/*!
 * \struct	drbClient
 * \breif	Dropbox client.
 *
 * Must be freed with drbDestroyClient.
 */
typedef struct drbClient drbClient;

/*!
 * \struct	drbOAuthToken
 * \breif	OAuth 1.0 Token (credentials).
 */
typedef struct {
	char* key;
	char* secret;
} drbOAuthToken;

/*!
 * \struct	drbAccountInfo
 * \breif	Dropbox account informations.
 *
 * Missing fields in the server answer are left blank with NULL value.
 * Check https://www.dropbox.com/developers/core/docs#account-info for more
 * details about these fields.
 *
 * Must be freed with drbDestroyAccountInfo.
 */
typedef struct {
	char* referralLink;
	char* displayName;
	unsigned int* uid;
	char* country;
	char* email;
	struct {
		unsigned int* datastores;
		unsigned int* shared;
		unsigned int* quota;
		unsigned int* normal;
	} quotaInfo ;
} drbAccountInfo;

/*!
 * \struct	drbMetadataList
 * \breif	Dropbox file or folder metadata.
 *
 * Missing fields in the server answer are left blank with NULL value.
 * Check https://www.dropbox.com/developers/core/docs#metadata for more details
 * about these fields.
 *
 * Must be freed with drbDestroyMetadata.
 */
typedef struct drbMetadataList drbMetadataList;

typedef struct {
	unsigned int*  bytes;
	char* clientMtime;
	char* icon;
	bool* isDir;
	char* mimeType;
	char* modified;
	char* path;
	char* rev;
	unsigned int* revision;
	char* root;
	char* size;
	bool* thumbExists;
	bool* isDeleted;
	char* hash; /*!< Only defined for folders (isDir = true). */
	drbMetadataList* contents;
} drbMetadata;

struct drbMetadataList{
	drbMetadata **array; /*!< List of all metadata. */
	unsigned int size;  /*!< List size. */
};

/*!
 * \struct	drbLink
 * \breif	Dropbox temporary link to a file.
 *
 * Missing fields in the server answer are left blank with NULL value.
 * Check https://www.dropbox.com/developers/core/docs#shares for more details
 * about these fields.
 *
 * Must be freed with drbDestroyLink.
 */
typedef struct {
	char* url;
	char* expires;
} drbLink;

/*!
 * \struct	drbLink
 * \breif	Dropbox file copy reference.
 *
 * Missing fields in the server answer are left blank with NULL value.
 * Check https://www.dropbox.com/developers/core/docs#copy_ref for more details
 * about these fields.
 *
 * Must be freed with drbDestroyCopyRef.
 */
typedef struct {
	char* copyRef;
	char* expires;
} drbCopyRef;

/*!
 * \struct	drbDeltaEntry
 * \breif	Dropbox delta entry.
 *
 * Missing fields in the server answer are left blank with NULL value.
 * Check https://www.dropbox.com/developers/core/docs#delta for more details
 * about these fields.
 */
typedef struct {
	char* path;
	drbMetadata* metadata;
} drbDeltaEntry;

/*!
 * \struct	drbDelta
 * \breif	Dropbox delta informations.
 *
 * Missing fields in the server answer are left blank with NULL value.
 * Check https://www.dropbox.com/developers/core/docs#delta for more details
 * about these fields.
 *
 * Must be freed with drbDestroyDelta.
 */
typedef struct {
	bool* reset;
	char* cursor;
	bool* hasMore;
	struct {
		drbDeltaEntry* array; /*!< List of all delta entries. */
		int size;             /*!< List size. */
	} entries;
} drbDelta;

/*!
 * \breif Function options and expected arguement type.
 *
 * Depending on the function, an option is either:
 *   1. illegal
 *   2. optional
 *   3. required by the dropbox server
 *   4. required by the function itself
 *
 * If this last kind of options are forgotten, the DRBERR_MISSING_OPT error code
 * is returned by the function. Otherwise, for illegal options or the
 * forgetting of an option of the third kind, an http error is returned.
 *
 */
enum {
	DRBOPT_END             = 0,     /*!< no arg! ALWAYS REQUIRED as last opt */
	DRBOPT_CURSOR          = 1<<0,  /*!< string  */
	DRBOPT_FILE_LIMIT      = 1<<1,  /*!< integer */
	DRBOPT_FORMAT          = 1<<2,  /*!< string  */
	DRBOPT_FROM_COPY_REF   = 1<<3,  /*!< string  */
	DRBOPT_FROM_PATH       = 1<<4,  /*!< string  */
	DRBOPT_HASH            = 1<<5,  /*!< string  */
	DRBOPT_INCL_DELETED    = 1<<6,  /*!< boolean */
	DRBOPT_LIST            = 1<<7,  /*!< boolean */
	DRBOPT_LOCALE          = 1<<8,  /*!< string  */
	DRBOPT_OVERWRITE       = 1<<9,  /*!< boolean */
	DRBOPT_PATH            = 1<<10, /*!< string  */
	DRBOPT_PARENT_REV      = 1<<11, /*!< string  */
	DRBOPT_QUERY           = 1<<12, /*!< string  */
	DRBOPT_REV             = 1<<13, /*!< string  */
	DRBOPT_REV_LIMIT       = 1<<14, /*!< integer */
	DRBOPT_ROOT            = 1<<15, /*!< string  */
	DRBOPT_SHORT_URL       = 1<<16, /*!< boolean */
	DRBOPT_SIZE            = 1<<17, /*!< string  */
	DRBOPT_TO_PATH         = 1<<18, /*!< string  */
	DRBOPT_IO_DATA         = 1<<19, /*!< void* (e.i FILE*) */
	DRBOPT_IO_FUNC         = 1<<20, /*!< size_t(*)(const void*,size_t,size_t,void*) */

};

/*!
 * Error code returned by functions. However, dropbox http errors codes (>100)
 * are not defined here. Check https://www.dropbox.com/developers/core/docs
 * under "Standard API errors" for more details about these kind of errors.
 */
enum {
	DRBERR_OK = 0,         /*!< no error (this const value will forever be 0) */
	DRBERR_MISSING_OPT,    /*!< an option required by the function is missing */
	DRBERR_UNKNOWN_OPT,    /*!< unknown option code encountered */
	DRBERR_DUPLICATED_OPT, /*!< an option code was set twice or more */
	DRBERR_INVALID_VAL,    /*!< invalid argument value encountered */
	DRBERR_MALLOC,         /*!< memory allocation failed */
	DRBERR_UNKOWN,         /*!< something that shouldn't happen, has happened */
};

typedef enum {
	Arg1, Arg2, Arg3

}TotoArgs;



/*!
 * \brief   Sets up the programm environment that dropbox library needs.
 * \return	void
 */
void drbInit();

/*!
 * \brief   Release ressources acquired by drbInit.
 * \return	void
 */
void drbCleanup();

/*!
 * \brief	Create and initilize a drbClient.
 * \param	cKey      consumer key (Client credentials)
 * \param	cSecret   consumer secret (Client credentials)
 * \param	tKey      request or access key (Temporary or Token credentials)
 * \param	tSecret   request or access secret (Temporary or Token credentials)
 * \return	drbClient pointer must be freed with drbDestroyClient by caller
 */
drbClient* drbCreateClient(char* cKey, char* cSecret, char* tKey, char* tSecret);

/*!
 * \brief	Obtain the request token (temporary credentials).
 *
 * Achieves the 1st step of OAuth 1.0 protocol by obtaining the temporary creds.
 * The token and its fields lifetime is only guaranteed until the next API
 * function call. If the user code need them afterward, they must be duplicated.
 *
 * \param	cli   client to authorize dropbox access.
 * \return	obtained request token (must NOT be freed or modified by caller).
 */
drbOAuthToken* drbObtainRequestToken(drbClient* cli);

/*!
 * \brief	Build the url for client accces authorization.
 *
 * Build url for 2nd step of OAuth 1.0 protocol (Resource Owner Authorization).
 *
 * \param	cli   client to authorize dropbox access.
 * \return	url to authorize client to acces the user dropbox
 */
char* drbBuildAuthorizeUrl(drbOAuthToken* reqTok);

/*!
 * \brief	Obtain the access token (Token credentials).
 *
 * Achieves the last step of OAuth 1.0 protocol by obtaining the token creds.
 * The token and its fields lifetime is only guaranteed until the next API
 * function call. If the user code need them afterward, they must be duplicated.
 *
 * \param	cli   client to authorize dropbox access.
 * \return	obtained access token (must NOT be freed or modified by caller)
 */
drbOAuthToken* drbObtainAccessToken(drbClient* cli);


/*!
 * \brief   Get account general informations.
 * \param		cli    authenticated dropbox client
 * \param[out]	info   account informations
 * \param		...    option/value pairs (function arguments) :
 *                       -# DRBOPT_LOCALE
 * \return	error code (DRBERR_XXX or http error returned by the Dropbox server)
 */
int drbGetAccountInfo(drbClient* cli, drbAccountInfo** info, ...);

/*!
 * \brief   Get a file or folder metadata.
 * \param		cli    authenticated dropbox client
 * \param[out]	meta   file or fold metadata
 * \param		...    legal option/value pairs:
 *                       -# DRBOPT_ROOT (required)
 *                       -# DRBOPT_PATH (required)
 *                       -# DRBOPT_FILE_LIMIT
 *                       -# DRBOPT_HASH
 *                       -# DRBOPT_LIST
 *                       -# DRBOPT_INCL_DELETED
 *                       -# DRBOPT_REV
 *                       -# DRBOPT_LOCALE
 * \return	error code (DRBERR_XXX or http error returned by the Dropbox server)
 */
int drbGetMetadata(drbClient* cli, drbMetadata** meta, ...);

/*!
 * \brief   Download a file.
 * \param		cli    authenticated dropbox client
 * \param[out]	meta   file metadata
 * \param		...    option/value pairs (function arguments) :
 *                       -# DRBOPT_ROOT (required)
 *                       -# DRBOPT_PATH (required)
 *                       -# DRBOPT_IO_DATA, e.i. FILE*  (required)
 *                       -# DRBOPT_IO_FUNC, e.i. fwrite (required)
 *                       -# DRBOPT_REV
 * \return	error code (DRBERR_XXX or http error returned by the Dropbox server)
 */
int drbGetFile(drbClient* cli, drbMetadata** meta, ...);

/*!
 * \brief   Get a file revisions.
 * \param		cli    authenticated dropbox client
 * \param[out]	list   list of file revisions
 * \param		...    option/value pairs (function arguments) :
 *                       -# DRBOPT_ROOT (required)
 *                       -# DRBOPT_PATH (required)
 *                       -# DRBOPT_REV_LIMIT
 *                       -# DRBOPT_LOCALE
 * \return	error code (DRBERR_XXX or http error returned by the Dropbox server)
 */
int drbGetRevisions(drbClient* cli, drbMetadataList** list, ...);

/*!
 * \brief   Search a file or a folder.
 * \param		cli    authenticated dropbox client
 * \param[out]	list   list of founded file or folder metadata
 * \param		...    option/value pairs (function arguments) :
 *                       -# DRBOPT_ROOT (required)
 *                       -# DRBOPT_PATH (required)
 *                       -# DRBOPT_QUERY
 *                       -# DRBOPT_FILE_LIMIT
 *                       -# DRBOPT_INCL_DELETED
 *                       -# DRBOPT_LOCALE
 * \return	error code (DRBERR_XXX or http error returned by the Dropbox server)
 */
int drbSearch(drbClient* cli, drbMetadataList** list, ...);

/*!
 * \brief   Download a thumbnail for an image file.
 * \param		cli    authenticated dropbox client
 * \param[out]	meta   image file metadata
 * \param		...    option/value pairs (function arguments) :
 *                       -# DRBOPT_ROOT (required)
 *                       -# DRBOPT_PATH (required)
 *                       -# DRBOPT_IO_DATA, e.i. FILE*  (required)
 *                       -# DRBOPT_IO_FUNC, e.i. fwrite (required)
 *                       -# DRBOPT_FORMAT
 *                       -# DRBOPT_SIZE
 * \return	error code (DRBERR_XXX or http error returned by the Dropbox server)
 */
int drbGetThumbnail(drbClient* cli, drbMetadata** meta, ...);

/*!
 * \brief   Copy a file or folder.
 * \param		cli    authenticated dropbox client
 * \param[out]	meta   new file or folder metadata
 * \param		...    option/value pairs (function arguments) :
 *                       -# DRBOPT_ROOT (required)
 *                       -# DRBOPT_FROM_PATH (required)
 *                       -# DRBOPT_TO_PATH or DRBOPT_FROM_COPY_REF (required)
 *                       -# DRBOPT_LOCALE
 * \return	error code (DRBERR_XXX or http error returned by the Dropbox server)
 */
int drbCopy(drbClient* cli, drbMetadata** meta, ...);

/*!
 * \brief   Create a folder.
 * \param		cli    authenticated dropbox client
 * \param[out]	meta   created folder metadata
 * \param		...    option/value pairs (function arguments) :
 *                       -# DRBOPT_ROOT (required)
 *                       -# DRBOPT_PATH (required)
 *                       -# DRBOPT_LOCALE
 * \return	error code (DRBERR_XXX or http error returned by the Dropbox server)
 */
int drbCreateFolder(drbClient* cli, drbMetadata** meta, ...);

/*!
 * \brief   Delete a file or folder.
 * \param		cli    authenticated dropbox client
 * \param[out]	meta   deleted file or folder metadata
 * \param		...    option/value pairs (function arguments) :
 *                       -# DRBOPT_ROOT (required)
 *                       -# DRBOPT_PATH (required)
 *                       -# DRBOPT_LOCALE
 * \return	error code (DRBERR_XXX or http error returned by the Dropbox server)
 */
int drbDelete(drbClient* cli, drbMetadata** meta, ...);

/*!
 * \brief   Move a file or folder.
 * \param		cli    authenticated dropbox client
 * \param[out]	meta   moved file or folder metadata
 * \param		...    option/value pairs (function arguments) :
 *                       -# DRBOPT_ROOT (required)
 *                       -# DRBOPT_FROM_PATH (required)
 *                       -# DRBOPT_TO_PATH or DRBOPT_FROM_COPY_REF (required)
 *                       -# DRBOPT_LOCALE
 * \return	error code (DRBERR_XXX or http error returned by the Dropbox server)
 */
int drbMove(drbClient* cli, drbMetadata** meta, ...);

/*!
 * \brief   Get changed files and folders.
 * \param		cli     authenticated dropbox client
 * \param[out]	delta   delta informations (changed files and folder)
 * \param		...     option/value pairs (function arguments) :
 *                        -# DRBOPT_CURSOR
 *                        -# DRBOPT_LOCALE
 * \return	error code (DRBERR_XXX or http error returned by the Dropbox server)
 */
int drbGetDelta(drbClient* cli, drbDelta** delta, ...);

/*!
 * \brief   Restore a file.
 * \param		cli    authenticated dropbox client
 * \param[out]	meta   restored file metadata
 * \param		...    option/value pairs (function arguments) :
 *                       -# DRBOPT_ROOT (required)
 *                       -# DRBOPT_PATH (required)
 *                       -# DRBOPT_REV  (required)
 *                       -# DRBOPT_LOCALE
 * \return	error code (DRBERR_XXX or http error returned by the Dropbox server)
 */
int drbRestore(drbClient* cli, drbMetadata** meta, ...);

/*!
 * \brief   Create a dropbox link to a file.
 * \param		cli    authenticated dropbox client
 * \param[out]	link   link to the file informations
 * \param		...    option/value pairs (function arguments) :
 *                       -# DRBOPT_ROOT (required)
 *                       -# DRBOPT_PATH (required)
 *                       -# DRBOPT_SHORT_URL
 *                       -# DRBOPT_LOCALE
 * \return	error code (DRBERR_XXX or http error returned by the Dropbox server)
 */
int drbShare(drbClient* cli, drbLink** link, ...);

/*!
 * \brief   Create a direct link to a file.
 * \param		cli    authenticated dropbox client
 * \param[out]	link   link to the file informations
 * \param		...    option/value pairs (function arguments) :
 *                       -# DRBOPT_ROOT (required)
 *                       -# DRBOPT_PATH (required)
 *                       -# DRBOPT_LOCALE
 * \return	error code (DRBERR_XXX or http error returned by the Dropbox server)
 */
int drbGetMedia(drbClient* cli, drbLink** link, ...);

/*!
 * \brief   Create a copy reference to a file.
 * \param		cli   authenticated dropbox client
 * \param[out]	ref   copy reference to the file
 * \param		...   option/value pairs (function arguments) :
 *                      -# DRBOPT_ROOT (required)
 *                      -# DRBOPT_PATH (required)
 * \return	error code (DRBERR_XXX or http error returned by the Dropbox server)
 */
int drbGetCopyRef(drbClient* cli, drbCopyRef** ref, ...);

/*!
 * \brief   Upload a file.
 * \param		cli    authenticated dropbox client
 * \param[out]	meta   uploaded file metadata
 * \param		...    option/value pairs (function arguments) :
 *                       -# DRBOPT_ROOT (required)
 *                       -# DRBOPT_PATH (required)
 *                       -# DRBOPT_IO_DATA (required)
 *                       -# DRBOPT_IO_FUNC (required)
 *                       -# DRBOPT_OVERWRITE
 *                       -# DRBOPT_PARENT_REV
 *                       -# DRBOPT_LOCALE
 * \return	error code (DRBERR_XXX or http error returned by the Dropbox server)
 */
int drbPutFile(drbClient* cli, drbMetadata** meta, ...);


void drbDestroyClient(drbClient* cli);
void drbDestroyCopyRef(drbCopyRef* ref);
void drbDestroyMedia(drbLink* link);
void drbDestroyMetadata(drbMetadata* meta, bool destroyList);
void drbDestroyMetadataList(drbMetadataList* list, bool destroyMetadata);
void drbDestroyAccountInfo(drbAccountInfo* info);
void drbDestroyLink(drbLink* link);
void drbDestroyDelta(drbDelta* delta, bool withMetadata);

#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif /* DROPBOX_H */
