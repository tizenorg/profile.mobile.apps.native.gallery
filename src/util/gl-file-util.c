/*
* Copyright (c) 2000-2015 Samsung Electronics Co., Ltd All Rights Reserved
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
* http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*
*/

#include "gl-file-util.h"
#include "gl-debug.h"
#include <libgen.h>
#include <unistd.h>

static mode_t default_mode = S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH;

char *gl_file_dir_get(const char path[])
{
	char *p = NULL;
	char buf[256] = {0,};
	struct stat info = {0,};

	if (stat(path, &info) == 0) {
		strncpy(buf, path, 256);
		buf[256 - 1] = 0;
		p = dirname(buf);
		return strdup(p);
	} else {
		return "";
	}
}

int gl_file_exists(const char *path)
{
	struct stat info = {0,};

	if (stat(path, &info) == 0) {
		return 1;
	} else {
		return 0;
	}
}

int gl_file_is_dir(const char *path)
{
	struct stat info = {0,};

	if (stat(path, &info) == 0) {
		if (S_ISDIR(info.st_mode)) {
			return 1;
		}
	}

	return 0;
}

int gl_file_dir_is_empty(const char *path)
{
	struct dirent *dp = NULL;
	DIR *dirp = NULL;
	struct dirent ent_struct;

	dirp = opendir(path);
	if (!dirp) {
		return -1;
	}

	while ((readdir_r(dirp, &ent_struct, &dp) == 0) && dp) {
		if ((strcmp(dp->d_name, ".")) && (strcmp(dp->d_name, ".."))) {
			closedir(dirp);
			return 0;
		}
	}
	closedir(dirp);
	return 1;
}

int gl_mkdir(const char *dir)
{
	if (mkdir(dir, default_mode) < 0) {
		return 0;
	} else {
		return 1;
	}
}

static int
gl_mkpath_if_not_exists(const char *path)
{
	struct stat st = {0,};
	if (stat(path, &st) < 0) {
		return gl_mkdir(path);
	} else if (!S_ISDIR(st.st_mode)) {
		return 0;
	} else {
		return 1;
	}
}

int gl_file_mkpath(const char *path)
{
	char ss[500] = {0,};
	unsigned int i = 0;

	if (gl_file_is_dir(path)) {
		return 1;
	}

	for (i = 0; path[i] != '\0'; ss[i] = path[i], i++) {
		if (i == sizeof(ss) - 1) {
			return 0;
		}

		if ((path[i] == '/') && (i > 0)) {
			ss[i] = '\0';
			if (!gl_mkpath_if_not_exists(ss)) {
				return 0;
			}
		}
	}
	ss[i] = '\0';

	return gl_mkpath_if_not_exists(ss);
}

int gl_file_unlink(const char *filename)
{
	int status = unlink(filename);
	if (status < 0) {
		return 0;
	} else {
		return 1;
	}
}

int gl_file_size(const char *filename)
{
	struct stat info = {0,};
	if (stat(filename, &info) == 0) {
		if (!S_ISDIR(info.st_mode)) {
			return info.st_size;
		}
	}

	return 0;
}

int gl_file_rmdir(const char *filename)
{
	int status = rmdir(filename);
	if (status < 0) {
		return 0;
	} else {
		return 1;
	}
}

Eina_List *gl_file_ls(const char *dir)
{
	char *f;
	DIR *dirp = NULL;
	struct dirent *dp = NULL;
	Eina_List *list = NULL;
	struct dirent ent_struct;

	dirp = opendir(dir);
	if (!dirp) {
		return NULL;
	}

	while ((readdir_r(dirp, &ent_struct, &dp) == 0) && dp) {
		if ((strcmp(dp->d_name , ".")) && (strcmp(dp->d_name , ".."))) {
			f = strdup(dp->d_name);
			list = eina_list_append(list , f);
		}
	}
	closedir(dirp);

	list = eina_list_sort(list, eina_list_count(list), EINA_COMPARE_CB(strcoll));

	return list;
}

int gl_file_recursive_rm(const char *dir)
{
	char buf[PATH_MAX] = {0,};
	struct dirent *dp = NULL;
	DIR *dirp = NULL;
	struct dirent ent_struct;

	if (readlink(dir, buf, sizeof(buf)) > 0) {
		return gl_file_unlink(dir);
	}

	int ret = gl_file_is_dir(dir);
	if (ret) {
		ret = 1;
		dirp = opendir(dir);
		if (dirp) {
			while ((readdir_r(dirp, &ent_struct, &dp) == 0)
			        && dp) {
				if ((strcmp(dp->d_name , ".")) && (strcmp(dp->d_name, ".."))) {
					if (!gl_file_recursive_rm(dp->d_name)) {
						ret = 0;
					}
				}
			}
			closedir(dirp);
		}

		if (!gl_file_rmdir(dir)) {
			ret = 0;
		}

		return ret;
	} else {
		return gl_file_unlink(dir);
	}
}

int gl_file_cp(const char *src, const char *dst)
{
	FILE *f1 = NULL;
	FILE *f2 = NULL;
	char buf[16384] = {0,}; //TODO: How about moving buf to heap instead of stack
	char realpath1[256] = {0,};
	char realpath2[256] = {0,};
	size_t num;
	int ret = 1;

	if (!realpath(src, realpath1)) {
		return 0;
	}

	if (realpath(dst, realpath2) && !strcmp(realpath1, realpath2)) {
		return 0;
	}

	f1 = fopen(src, "rb");
	if (!f1) {
		return 0;
	}

	f2 = fopen(dst, "wb");
	if (!f2) {
		fclose(f1);
		return 0;
	}

	while ((num = fread(buf, 1, sizeof(buf), f1)) > 0) {
		if (fwrite(buf, 1, num, f2) != num) {
			ret = 0;
		}
	}

	fclose(f1);
	fclose(f2);

	return ret;
}

int gl_file_mv(const char *src, const char *dst)
{
	struct stat info = {0,};
	if (stat(dst, &info) == 0) {
		return 0;
	}

	if (rename(src, dst)) {
		memset(&info, 0x00, sizeof(struct stat));
		if (stat(src, &info) == 0) {
			if (S_ISREG(info.st_mode)) {
				gl_file_cp(src, dst);
				chmod(dst, info.st_mode);
				unlink(src);
				return 1;
			}
		}
		return 0;
	}
	return 1;
}
