/* 
   Unix SMB/CIFS implementation.
   SMB client library implementation (Old interface compatibility)
   Copyright (C) Andrew Tridgell 1998
   Copyright (C) Richard Sharpe 2000
   Copyright (C) John Terpstra 2000
   Copyright (C) Tom Jansen (Ninja ISD) 2002 
   
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.
   
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
   
   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/


#include "includes.h"

#include "../include/libsmb_internal.h"

struct smbc_compat_fdlist {
	SMBCFILE * file;
	int fd;
	struct smbc_compat_fdlist *next, *prev;
};

static SMBCCTX * statcont = NULL;
static int smbc_compat_initialized = 0;
static int smbc_currentfd = 10000;
static struct smbc_compat_fdlist * smbc_compat_fdlist = NULL;


/* Find an fd and return the SMBCFILE * or NULL on failure */
static SMBCFILE * find_fd(int fd)
{
	struct smbc_compat_fdlist * f = smbc_compat_fdlist;
	while (f) {
		if (f->fd == fd) 
			return f->file;
		f = f->next;
	}
	return NULL;
}

/* Add an fd, returns 0 on success, -1 on error with errno set */
static int add_fd(SMBCFILE * file)
{
	struct smbc_compat_fdlist * f = malloc(sizeof(struct smbc_compat_fdlist));
	if (!f) {
		errno = ENOMEM;
		return -1;
	}
	
	f->fd = smbc_currentfd++;
	f->file = file;
	
	DLIST_ADD(smbc_compat_fdlist, f);

	return f->fd;
}



/* Delete an fd, returns 0 on success */
static int del_fd(int fd)
{
	struct smbc_compat_fdlist * f = smbc_compat_fdlist;
	while (f) {
		if (f->fd == fd) 
			break;
		f = f->next;
	}
	if (f) {
		/* found */
		DLIST_REMOVE(smbc_compat_fdlist, f);
		SAFE_FREE(f);
		return 0;
	}
	return 1;
}
 


int smbc_init(smbc_get_auth_data_fn fn, int debug)
{
	if (!smbc_compat_initialized) {
		statcont = smbc_new_context();
		if (!statcont) 
			return -1;

		statcont->debug = debug;
		statcont->callbacks.auth_fn = fn;
		
		if (!smbc_init_context(statcont)) {
			smbc_free_context(statcont, False);
			return -1;
		}

		smbc_compat_initialized = 1;

		return 0;
	}
	return 0;
}


int smbc_open(const char *furl, int flags, mode_t mode)
{
	SMBCFILE * file;
	int fd;

	file = statcont->open(statcont, furl, flags, mode);
	if (!file)
		return -1;

	fd = add_fd(file);
	if (fd == -1) 
		statcont->close(statcont, file);
	return fd;
}


int smbc_creat(const char *furl, mode_t mode)
{
	SMBCFILE * file;
	int fd;

	file = statcont->creat(statcont, furl, mode);
	if (!file)
		return -1;

	fd = add_fd(file);
	if (fd == -1) {
		/* Hmm... should we delete the file too ? I guess we could try */
		statcont->close(statcont, file);
		statcont->unlink(statcont, furl);
	}
	return fd;
}


ssize_t smbc_read(int fd, void *buf, size_t bufsize)
{
	SMBCFILE * file = find_fd(fd);
	return statcont->read(statcont, file, buf, bufsize);
}

ssize_t smbc_write(int fd, void *buf, size_t bufsize)
{
	SMBCFILE * file = find_fd(fd);
	return statcont->write(statcont, file, buf, bufsize);
}

off_t smbc_lseek(int fd, off_t offset, int whence)
{
	SMBCFILE * file = find_fd(fd);
	return statcont->lseek(statcont, file, offset, whence);
}

int smbc_close(int fd)
{
	SMBCFILE * file = find_fd(fd);
	del_fd(fd);
	return statcont->close(statcont, file);
}

int smbc_unlink(const char *fname)
{
        return statcont->unlink(statcont, fname);
}

int smbc_rename(const char *ourl, const char *nurl)
{
	return statcont->rename(statcont, ourl, statcont, nurl);
}

int smbc_opendir(const char *durl)
{
	SMBCFILE * file;
	int fd;

	file = statcont->opendir(statcont, durl);
	if (!file)
		return -1;

	fd = add_fd(file);
	if (fd == -1) 
		statcont->closedir(statcont, file);

	return fd;
}

int smbc_closedir(int dh) 
{
	SMBCFILE * file = find_fd(dh);
	del_fd(dh);
	return statcont->closedir(statcont, file);
}

int smbc_getdents(unsigned int dh, struct smbc_dirent *dirp, int count)
{
	SMBCFILE * file = find_fd(dh);
	return statcont->getdents(statcont, file,dirp, count);
}

struct smbc_dirent* smbc_readdir(unsigned int dh)
{
	SMBCFILE * file = find_fd(dh);
	return statcont->readdir(statcont, file);
}

off_t smbc_telldir(int dh)
{
	SMBCFILE * file = find_fd(dh);
	return statcont->telldir(statcont, file);
}

int smbc_lseekdir(int fd, off_t offset)
{
	SMBCFILE * file = find_fd(fd);
	return statcont->lseekdir(statcont, file, offset);
}

int smbc_mkdir(const char *durl, mode_t mode)
{
	return statcont->mkdir(statcont, durl, mode);
}

int smbc_rmdir(const char *durl)
{
	return statcont->rmdir(statcont, durl);
}

int smbc_stat(const char *url, struct stat *st)
{
	return statcont->stat(statcont, url, st);
}

int smbc_fstat(int fd, struct stat *st)
{
	SMBCFILE * file = find_fd(fd);
	return statcont->fstat(statcont, file, st);
}

int smbc_chmod(const char *url, mode_t mode)
{
	/* NOT IMPLEMENTED IN LIBSMBCLIENT YET */
	return -1;
}

int smbc_print_file(const char *fname, const char *printq)
{
	return statcont->print_file(statcont, fname, statcont, printq);
}

int smbc_open_print_job(const char *fname)
{
	SMBCFILE * file = statcont->open_print_job(statcont, fname);
	if (!file) return -1;
	return (int) file;
}

int smbc_list_print_jobs(const char *purl, smbc_list_print_job_fn fn)
{
	return statcont->list_print_jobs(statcont, purl, fn);
}

int smbc_unlink_print_job(const char *purl, int id)
{
	return statcont->unlink_print_job(statcont, purl, id);
}


