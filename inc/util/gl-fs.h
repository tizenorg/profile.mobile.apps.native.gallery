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
#ifndef _GL_FS_H_
#define _GL_FS_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <linux/fs.h>
#include <Eina.h>
#include <glib.h>

typedef struct _gl_node_info_t gl_node_info_s;
struct _gl_node_info_t {
	/* Parent path */
	char *path;
	/* File/folder name */
	char *name;
	/* Modified time */
	time_t mtime;
	/* File size / folder's content size */
	unsigned long long size;
};

int _gl_fs_update_file_mtime(const char *path);
int _gl_fs_get_file_stat(const char *filename, gl_node_info_s **node);
unsigned long long _gl_fs_get_folder_size(char *path);
int _gl_fs_rm_folder(char *path, long long cut_size, long long max_size,
		     unsigned int expired_time);
#ifdef _RENAME_ALBUM_SENSITIVE
bool _gl_fs_check_name_case(char *dir, char *exist_name);
#endif
bool _gl_fs_is_low_memory(unsigned long long total_size);
unsigned long long gl_fs_get_free_space(int state);
int _gl_fs_get_file_ext(const char *file_path, char *file_ext, int max_len);
bool _gl_fs_get_path_without_ext(const char *file_path, char *file_ext,
				 char *new_path);
Eina_Bool _gl_fs_move(void *data, const char *src, const char *dst);
Eina_Bool _gl_fs_copy(void *data, const char *src, const char *dst);
char *_gl_fs_get_unique_full_path(char *file_path, char *ext);
char *_gl_fs_get_unique_new_album_name(char *parent_path, char *default_name,
				       char **new_name);
bool _gl_fs_validate_name(const char *new_name);
bool _gl_fs_validate_character(const char name_char);
int _gl_fs_get_default_folder(char *path);
bool _gl_fs_mkdir(const char *path);

#ifdef __cplusplus
}
#endif

#endif

