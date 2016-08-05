/*
  FUSE: Filesystem in Userspace
  Copyright (C) 2001-2007  Miklos Szeredi <miklos@szeredi.hu>

  This program can be distributed under the terms of the GNU GPL.
  See the file COPYING.
*/

/** @file
 *
 * hello.c - minimal FUSE example featuring fuse_main usage
 *
 * \section section_compile compiling this example
 *
 * gcc -Wall hello.c `pkg-config fuse3 --cflags --libs` -o hello
 *
 * \section section_usage usage
 \verbatim
 % mkdir mnt
 % ./hello mnt        # program will vanish into the background
 % ls -la mnt
   total 4
   drwxr-xr-x 2 root root      0 Jan  1  1970 ./
   drwxrwx--- 1 root vboxsf 4096 Jun 16 23:12 ../
   -r--r--r-- 1 root root     13 Jan  1  1970 hello
 % cat mnt/hello
   Hello World!
 % fusermount -u mnt
 \endverbatim
 *
 * \section section_source the complete source
 * \include hello.c
 */


#define _GNU_SOURCE
#define FUSE_USE_VERSION 30

#include <config.h>

#include <fuse.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>

//static const char *hello_str = "Hello World!\n";
static const char *hello_path = "/hello";
static const char *hello_backing_file = "/home/yasker/image";
static const size_t hello_backing_size = 10 * 1024 * 1024 * 1024;

static int f;

static int hello_getattr(const char *path, struct stat *stbuf)
{
	int res = 0;

	memset(stbuf, 0, sizeof(struct stat));
	if (strcmp(path, "/") == 0) {
		stbuf->st_mode = S_IFDIR | 0755;
		stbuf->st_nlink = 2;
	} else if (strcmp(path, hello_path) == 0) {
		stbuf->st_mode = S_IFREG | 0666;
		stbuf->st_nlink = 1;
		//stbuf->st_size = strlen(hello_str);
		stbuf->st_size = hello_backing_size;
	} else
		res = -ENOENT;

	return res;
}

static int hello_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
			 off_t offset, struct fuse_file_info *fi,
			 enum fuse_readdir_flags flags)
{
	(void) offset;
	(void) fi;
	(void) flags;

	if (strcmp(path, "/") != 0)
		return -ENOENT;

	filler(buf, ".", NULL, 0, 0);
	filler(buf, "..", NULL, 0, 0);
	filler(buf, hello_path + 1, NULL, 0, 0);

	return 0;
}

static int hello_open(const char *path, struct fuse_file_info *fi)
{
	if (strcmp(path, hello_path) != 0)
		return -ENOENT;

	/*
	if ((fi->flags & 3) != O_RDONLY)
		return -EACCES;
	*/
	f = open(hello_backing_file, O_RDWR);
	if (f == -1) {
		perror("Fail to open backing file:");
		return f;
	}
        fi->direct_io = 1;

	return 0;
}

static int hello_release(const char *path, struct fuse_file_info *fi) {
        close(f);
}

static int hello_read(const char *path, char *buf, size_t size, off_t offset,
		      struct fuse_file_info *fi)
{
	size_t len;
	(void) fi;
	if(strcmp(path, hello_path) != 0)
		return -ENOENT;

	len = hello_backing_size;
	if (offset < len) {
		if (offset + size > len)
			size = len - offset;
		//memcpy(buf, hello_str + offset, size);
		len = pread(f, buf, size, offset);
		if (len == -1) {
			perror("Fail to read");
		}
	} else
		len = 0;

	return len;
}

static int hello_write(const char * path, const char * buf, size_t size, off_t offset,
		      struct fuse_file_info *fi) {
	size_t len;
	(void) fi;
	if(strcmp(path, hello_path) != 0)
		return -ENOENT;

	len = hello_backing_size;
	if (offset < len) {
		if (offset + size > len) {
			fprintf(stderr, "Overflow writing %lu at %ld",
					size, offset);
			return -1;
		}
		len = pwrite(f, buf, size, offset);
		if (len == -1) {
			perror("Fail to write");
		}
	} else
		len = 0;

	return len;
}

static int hello_setxattr(const char *path, const char *name, const char *value, size_t size, int flag) {
	return 0;
}

static int hello_truncate(const char *path, off_t off) {
	return 0;
}

static int hello_unlink(const char *path) {
	return 0;
}

static struct fuse_operations hello_oper = {
	.getattr	= hello_getattr,
	.readdir	= hello_readdir,
	.open		= hello_open,
        .release        = hello_release,
	.read		= hello_read,
	.write		= hello_write,
        .setxattr       = hello_setxattr,
        .truncate       = hello_truncate,
        .unlink         = hello_unlink,
};

int main(int argc, char *argv[])
{
	return fuse_main(argc, argv, &hello_oper, NULL);
}
