/*!
 * \file    dropboxFuse.c
 * \brief   Dropbox Fuse file system.
 * \author  Ashwin Sankar
 * \version 1.0
 * \note    Mount file system with the compiled program.
 *          Unmount it with fusermount -u command.
 */

#define FUSE_USE_VERSION 26

#ifdef linux
/* For pread()/pwrite()/utimensat() */
#define _XOPEN_SOURCE 700
#endif

#define _GNU_SOURCE

#include <fuse.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <dirent.h>
#include <errno.h>
#include <sys/time.h>
#include <libgen.h>
#include "dropbox.h"
#include <stdarg.h>
#include "htbl.h"
#include "utils.h"

#define TABLE_SIZE 100

static char* cacheRoot = NULL;
static htbl* filesTable;
static drbClient* client;
static char* cKey    = "c71kbzgwm5na2xo";
static char* cSecret = "wa78tvw06awn0q8";

FILE* logFile = NULL;


static void writeLog(char* format, ...) {
	if(logFile) {
		va_list args;
		va_start (args, format);
		vfprintf(logFile, format, args);
		fflush(logFile);
		va_end(args);
	}
}

static void usage(char* program) {
	fprintf(stderr, "usage: %s MOUNT_POINT -c CACHE_ROOT -t TOKEN_FILE [-l LOG_FILE]\n", basename(program));
	exit(EXIT_FAILURE);
}

static int dropbox_getattr(const char *path, struct stat *stbuf) {
	int res = DRBERR_OK;
	drbMetadata* meta = htblGet(filesTable, (char*)path);
	if (!meta) {

		res = drbGetMetadata(client, &meta,
	                         DRBOPT_ROOT, DRBVAL_ROOT_DROPBOX,
	                         DRBOPT_PATH, path,
	                         DRBOPT_END);

		// if file exists and is not deleted
		if(res == DRBERR_OK && (!meta->isDeleted || !*meta->isDeleted))
			htblSet(filesTable, (char*)path, meta);
		else
			res = -ENOENT;
	}

	if(res == DRBERR_OK) {
		char* cachePath;
		asprintf(&cachePath, "%s%s", cacheRoot, path);
		if (access(cachePath, F_OK) == 0 && lstat(cachePath, stbuf) == 0) {
			stbuf->st_mode = 755 | (meta->isDir && *meta->isDir? S_IFDIR : S_IFREG);
			res = 0;
		} else {
			memset(stbuf, 0, sizeof(struct stat));
			if(meta->bytes) stbuf->st_size = *meta->bytes;
			stbuf->st_mode = 755 | (meta->isDir && *meta->isDir? S_IFDIR : S_IFREG);
			stbuf->st_nlink = 1;
		}
		free(cachePath);
	}

	writeLog("getattr(%s) : %d\n", path, res);

	return res;
}

static int dropbox_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi) {
	drbMetadata* meta;
	int res = drbGetMetadata(client, &meta,
			                 DRBOPT_ROOT, DRBVAL_ROOT_DROPBOX,
			                 DRBOPT_PATH, path,
			                 DRBOPT_END);
	if(res == DRBERR_OK) {
		filler(buf, ".", NULL, 0);
		filler(buf, "..", NULL, 0);
		if(meta->contents)
			for(int i = 0; i < meta->contents->size; i++) {
				drbMetadata* curContent = meta->contents->array[i];
				drbMetadata* oldContent = htblSet(filesTable, curContent->path, curContent);

				// remove older file version from cache
				if(!*meta->contents->array[i]->isDir && oldContent && *oldContent->revision != *curContent->revision) {
					char* cachePath;
					asprintf(&cachePath, "%s%s", cacheRoot, curContent->path);
					if (access(cachePath, F_OK) == 0)
						unlink(cachePath);
					free(cachePath);
				}

				drbDestroyMetadata(oldContent, true);
				filler(buf, basename(meta->contents->array[i]->path), NULL, 0);
			}

		drbDestroyMetadataList(meta->contents, false);
		drbDestroyMetadata(meta, false);
	} else
		res = -ENOENT;

	writeLog("readdir(%s) : %d\n", path, res);
	return res;
}

static int dropbox_mkdir(const char *path, mode_t mode) {
	drbMetadata* meta;
	int res = drbCreateFolder(client, &meta,
			                  DRBOPT_ROOT, DRBVAL_ROOT_DROPBOX,
			                  DRBOPT_PATH, (char*)path,
	                          DRBOPT_END);
	if (res == DRBERR_OK) {
		drbMetadata* oldContent = htblSet(filesTable, (char*)path, meta);
		drbDestroyMetadata(oldContent, true);
	} else
		res = -EEXIST;

	writeLog("mkdir(%s) : %d\n", path, res);
	return res;
}

static int dropbox_unlink(const char *path) {
	int res = drbDelete(client, NULL,
			            DRBOPT_ROOT, DRBVAL_ROOT_DROPBOX,
			            DRBOPT_PATH, path,
	                    DRBOPT_END);
	if (res == DRBERR_OK) {
		drbMetadata* oldContent = htblRemove(filesTable, (char*)path);
		drbDestroyMetadata(oldContent, true);

		//remove file from cache
		char* cachePath;
		asprintf(&cachePath, "%s%s", cacheRoot, path);
		if (access(cachePath, F_OK) == 0)
			unlink(cachePath);

		free(cachePath);
	} else
		res = -ENOENT;

	writeLog("unlink(%s) : %d\n", path, res);
	return res;
}

static int dropbox_rmdir(const char *path) {
	int res = drbDelete(client, NULL,
			            DRBOPT_ROOT, DRBVAL_ROOT_DROPBOX,
			            DRBOPT_PATH, path,
	                    DRBOPT_END);
	if (res == DRBERR_OK) {
		drbMetadata* oldContent = htblRemove(filesTable, (char*)path);
		drbDestroyMetadata(oldContent, true);
	} else
		res = -ENOENT;

	writeLog("rmdir(%s): %d\n", path, res);
	return -res;
}

static int dropbox_rename(const char *from, const char *to) {
	drbMetadata* meta;
	int res = drbMove(client, &meta,
			          DRBOPT_ROOT, DRBVAL_ROOT_DROPBOX,
			          DRBOPT_FROM_PATH, from,
			          DRBOPT_TO_PATH, to,
	                  DRBOPT_END);
	if (res == DRBERR_OK) {
		drbMetadata* oldContent = htblRemove(filesTable, (char*)from);
		drbDestroyMetadata(oldContent, true);
		oldContent = htblSet(filesTable, (char*)to, meta);
		drbDestroyMetadata(oldContent, true);
	} else
		res = -ENOENT;

	writeLog("rename(%s, %s): %d\n", from, to, res);
	return res;
}

static int dropbox_open(const char *path, struct fuse_file_info *fi) {
	char* cachePath;
	asprintf(&cachePath, "%s%s", cacheRoot, path);
	int res = htblExists(filesTable, (char*)path) ? 0 : -ENOENT;
	free(cachePath);

	writeLog("open(%s): %d\n", path, res);
	return res;
}

static int dropbox_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi) {
	char* cachePath;
	asprintf(&cachePath, "%s%s", cacheRoot, path);
	(void) fi;
	int res = DRBERR_OK;
	int fd = open(cachePath, O_RDONLY);

	if (fd == -1) {
		mkdirFile(cachePath, 0777);
		FILE* file = fopen(cachePath, "w+");
		res = drbGetFile(client, NULL,
				         DRBOPT_ROOT, DRBVAL_ROOT_DROPBOX,
				         DRBOPT_PATH, path,
				         DRBOPT_IO_DATA, file,
				         DRBOPT_IO_FUNC, fwrite,
				         DRBOPT_END);
		fclose(file);
		if (res != DRBERR_OK) {
			res = errno;
			unlink(cachePath);
		}
	}
	free(cachePath);


	if (res == DRBERR_OK) {
		res = pread(fd, buf, size, offset);
		if (res == -1)
			res = -errno;
		close(fd);
	}

	writeLog("read(%s): %d\n", path, res);
	return res;
}

static int dropbox_mknod(const char *path, mode_t mode, dev_t rdev) {
	int res;
	char* cachePath = NULL;
	asprintf(&cachePath, "%s%s", cacheRoot, path);
	mkdirFile(cachePath, mode);

	if (S_ISREG(mode)) {
		res = open(cachePath, O_CREAT | O_EXCL | O_WRONLY, mode);
		if (res >= 0)
			res = close(res);
	} else if (S_ISFIFO(mode))
		res = mkfifo(cachePath, mode);
	else
		res = mknod(cachePath, mode, rdev);

	if (res == -1) {
		res = -errno;
	} else {
		FILE* file = fopen(cachePath, "r+");
		drbMetadata* meta = NULL;
		res = drbPutFile(client, &meta,
		                 DRBOPT_ROOT, DRBVAL_ROOT_DROPBOX,
		                 DRBOPT_PATH, path,
		                 DRBOPT_IO_DATA, file,
		                 DRBOPT_IO_FUNC, fread,
		                 DRBOPT_END);
		fclose(file);
		if (res == DRBERR_OK) {
			drbMetadata* oldContent = htblSet(filesTable, (char*)path, meta);
			drbDestroyMetadata(oldContent, true);
		} else {
			res = ENOENT;
			unlink(cachePath);
		}
	}
	free(cachePath);

	writeLog("mknod(%s): %d\n", path, res);
	return res;
}

static int dropbox_access(const char *path, int mask) {
	char* cachePath;
	asprintf(&cachePath, "%s%s", cacheRoot, path);
	int res = htblExists(filesTable, (char*)path) ? 0 : -ENOENT;
	free(cachePath);
	writeLog("access(%s): %d\n", path, res);
	return res;
}

static int dropbox_write(const char *path, const char *buf, size_t size, off_t offset, struct fuse_file_info *fi) {
	char* cachePath;
	asprintf(&cachePath, "%s%s", cacheRoot, path);

	(void) fi;
	int fd = open(cachePath, O_WRONLY);
	if (fd == -1)
		return -errno;

	int res = pwrite(fd, buf, size, offset);
	close(fd);

	if (res == -1)
		res = -errno;
	else {
		FILE* file = fopen(cachePath, "r+");
		drbMetadata* meta = NULL;
		int drbRes = drbPutFile(client, &meta,
		                        DRBOPT_ROOT, DRBVAL_ROOT_DROPBOX,
		                        DRBOPT_PATH, path,
		                        DRBOPT_IO_DATA, file,
		                        DRBOPT_IO_FUNC, fread,
		                        DRBOPT_END);
		fclose(file);
		if (drbRes == DRBERR_OK) {
			drbMetadata* oldContent = htblSet(filesTable, (char*)path, meta);
			drbDestroyMetadata(oldContent, true);
		}
	}

	writeLog("write(%s): %d \n", path, res);
	return res;
}

void destroyMetadataFromTable(void* meta, void* arg) {
	drbDestroyMetadata(meta, true);
}

void dropbox_destroy(void* foo) {
	htblDestroy(filesTable, destroyMetadataFromTable, NULL);
	drbDestroyClient(client);
	if (logFile) fclose(logFile);
}

static struct fuse_operations dropbox_operations = {
	.getattr	= dropbox_getattr,
	.unlink 	= dropbox_unlink,
	.readdir	= dropbox_readdir,
	.mkdir		= dropbox_mkdir,
	.rmdir		= dropbox_rmdir,
	.rename		= dropbox_rename,
	.destroy    = dropbox_destroy,
	.open		= dropbox_open,
	.read		= dropbox_read,
	.mknod		= dropbox_mknod,
	.access		= dropbox_access,
	.write		= dropbox_write,
};

int main(int argc, char *argv[])
{
	char* logPath   = NULL;
	char* tokenPath = NULL;
	char* tKey      = NULL;
	char* tSecret   = NULL;

	if(argc < 3)
		usage(argv[0]);

	// Read arguments
	optind = 2;
	while (optind < argc) {
		switch (getopt(argc, argv, "l:c:t:")) {
			case -1 : optind++; break;
			case 'l': logPath   = optarg; break;
			case 't': tokenPath = optarg; break;
			case 'c': cacheRoot = strdup(optarg); break;
			default:
				printf("arg[%d] = %s\n", optind, argv[optind]);
		}
	}

	// check mandatory arguments
	if(!tokenPath || !cacheRoot)
		usage(argv[0]);

	// if path end with / then remove it!
	int len = strlen(cacheRoot);
	if(cacheRoot[len-1] == '/')
		cacheRoot[len-1] = '\0';

	// Create log file
	if (logPath)
		logFile = fopen(logPath, "w");

	// Read token file
	if(access(tokenPath, F_OK) == 0) {
		FILE* token = fopen(tokenPath, "r");
		tKey    = readLine(token);
		tSecret = readLine(token);
		if(!tKey || !tSecret) {
			free(tKey), tKey = NULL;
			free(tSecret), tSecret = NULL;
		}
		fclose(token);
	}

	client = drbCreateClient(cKey, cSecret, tKey, tSecret);

	// Get token and store it in the token file
	if(!tKey || !tSecret) {
		drbOAuthToken* reqTok = drbObtainRequestToken(client);
		printf("Please visit this site and then press ENTER:\n   %s\n", drbBuildAuthorizeUrl(reqTok));
		fgetc(stdin);
		drbOAuthToken* accTok = drbObtainAccessToken(client);

		FILE* token = fopen(tokenPath, "w");
		fprintf(token, "%s\n%s", accTok->key, accTok->secret);
		fclose(token);
	}

	filesTable = htblCreate(TABLE_SIZE);

	printf("Mount DropboxFuse file system...\n");

	umask(0);
	return fuse_main(2, argv, &dropbox_operations, NULL);
}
