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

#ifndef _GL_FILE_UTIL_H_
#define _GL_FILE_UTIL_H_

#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <dirent.h>
#include <string.h>
#include <Eina.h>

#ifdef __cplusplus
extern "C"
{
#endif

int gl_file_exists(const char *path);

int gl_file_is_dir(const char *path);

int gl_file_unlink(const char *filename);

int gl_file_size(const char *filename);

int gl_file_rmdir(const char *filename);

int gl_file_dir_is_empty(const char *path);

Eina_List *gl_file_ls(const char *dir);

int gl_file_recursive_rm(const char *dir);

char* gl_file_dir_get(const char path[]);

int gl_file_mkpath(const char *path);

int gl_file_mv(const char *src, const char *dst);

#ifdef __cplusplus
}
#endif

#endif // end of _GL_FILE_UTIL_H_
