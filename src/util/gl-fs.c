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

#include <sys/stat.h>
#include <sys/statvfs.h>
#include <sys/time.h>
#include <errno.h>
#include <glib/gprintf.h>
#include <Elementary.h>
#include "gl-fs.h"
#include "gallery.h"
#include "gl-debug.h"
#include "gl-thread-util.h"
#include "gl-file-util.h"
#include <storage.h>

static int __gl_fs_free_node(gl_node_info_s *pnode)
{
	GL_CHECK_VAL(pnode, -1);
	if (pnode->path) {
		g_free(pnode->path);
		pnode->path = NULL;
	}
	if (pnode->name) {
		g_free(pnode->name);
		pnode->name = NULL;
	}
	g_free(pnode);
	return 0;
}

static int __gl_fs_clear_list(Eina_List **elist)
{
	GL_CHECK_VAL(elist, -1);
	gl_node_info_s *pnode = NULL;
	if (*elist) {
		EINA_LIST_FREE(*elist, pnode) {
			__gl_fs_free_node(pnode);
			pnode = NULL;
		}
		*elist = NULL;
	}
	return 0;
}

/* Append bigger one, prepend smaller one, first node is oldest one */
static int __gl_fs_compare(const void *data1, const void *data2)
{
	gl_node_info_s *pnode1 = (gl_node_info_s *)data1;
	gl_node_info_s *pnode2 = (gl_node_info_s *)data2;

	int result = -(pnode1->mtime - pnode2->mtime);
	gl_dbg("mtime1[%d], mtime2[%d], result[%d]", pnode1->mtime,
	       pnode2->mtime, result);
	return result;
}

static int __gl_fs_read_dir(char *path, Eina_List **dir_list,
			    Eina_List **file_list)
{
	GL_CHECK_VAL(file_list, -1);
	GL_CHECK_VAL(dir_list, -1);
	GL_CHECK_VAL(path, -1);

	DIR *pdir = NULL;
	struct dirent *ent = NULL;
	struct dirent ent_struct;
	int count = 0;
	char *childpath = NULL;
	int cp_len = 0;
	gl_node_info_s *pnode = NULL;
	int copiednum = 0;

	pdir = opendir(path);
	GL_CHECK_VAL(pdir, -1);

	while ((readdir_r(pdir, &ent_struct, &ent) == 0) && ent) {
		if (g_strcmp0(ent->d_name, ".") == 0 ||
		    g_strcmp0(ent->d_name, "..") == 0) {
			continue;
		}
		/*only deal with dirs and regular files*/
		if ((ent->d_type & DT_DIR) == 0 && (ent->d_type & DT_REG) == 0)
			continue;

		/*get date & size*/
		cp_len = strlen(path) + 1 + strlen(ent->d_name) + 1;
		childpath = g_new0(char, cp_len);
		if (childpath == NULL)
			continue;

		copiednum = g_snprintf(childpath, cp_len, "%s/%s", path,
				       ent->d_name);
		if (copiednum < 0) {
			g_free(childpath);
			childpath = NULL;
			continue;
		}

		pnode = g_new0(gl_node_info_s, 1);
		if (pnode == NULL) {
			g_free(childpath);
			childpath = NULL;
			continue;
		}
		/*get path*/
		pnode->path = g_strdup(path);
		/*get name*/
		pnode->name = g_strdup(ent->d_name);

		if (_gl_fs_get_file_stat(childpath, &pnode) < 0) {
			g_free(pnode);
			pnode = NULL;
			g_free(childpath);
			childpath = NULL;
			continue;
		}
		g_free(childpath);
		childpath = NULL;

		if (ent->d_type & DT_DIR)
			*dir_list = eina_list_append(*dir_list, pnode);
		else
			*file_list = eina_list_append(*file_list, pnode);
		count++;
	}
	closedir(pdir);

	return 0;
}

static int __gl_fs_get_file_list(GString *folder_name, Eina_List **dir_list,
				 Eina_List **file_list)
{
	GL_CHECK_VAL(folder_name, -1);
	GL_CHECK_VAL(folder_name->str, -1);
	GL_CHECK_VAL(folder_name->len, -1);
	return __gl_fs_read_dir(folder_name->str, dir_list, file_list);
}

/* Get sorted folders list by folder's modified time */
static unsigned long long __gl_fs_sort_folders_by_mtime(char *path,
							Eina_List **sorted_list)
{
	if (!gl_file_is_dir(path)) {
		gl_dbgE("Not a directory!");
		return 0;
	}

	unsigned long long size = 0;
	unsigned long long sub_size = 0;
	GString *fullpath = g_string_new(path);
	Eina_List *dir_list = NULL;
	Eina_List *file_list = NULL;
	int error_code = 0;
	error_code = __gl_fs_get_file_list(fullpath, &dir_list, &file_list);

	if (error_code == 0) {
		int i = 0;
		int dir_cnt = eina_list_count(dir_list);
		int file_cnt = eina_list_count(file_list);
		gl_node_info_s *pnode = NULL;
		for (i = 0; i < file_cnt; i++) {
			pnode = (gl_node_info_s *)eina_list_nth(file_list, i);
			if (pnode == NULL) {
				gl_dbgE("Invalid node!");
				continue;
			}
			size += pnode->size;
			pnode = NULL;
		}
		gl_dbg("file_cnt[%d], size[%llu]", file_cnt, size);

		i = 0;
		pnode = NULL;
		char *sub_folder = NULL;
		gl_dbg("dir_cnt[%d]", dir_cnt);
		for (i = 0; i < dir_cnt; i++) {
			pnode = (gl_node_info_s *)eina_list_nth(dir_list, i);
			if (pnode == NULL) {
				gl_dbgE("Invalid node!");
				continue;
			}
			sub_folder = g_strconcat(pnode->path, "/", pnode->name,
						 NULL);
			if (sub_folder) {
				gl_sdbg("sub_folder[%s]", pnode->name);
				sub_size = __gl_fs_sort_folders_by_mtime(sub_folder,
									 sorted_list);
				pnode->size = sub_size;
				size += sub_size;
				gl_dbg("mtime[%d]", pnode->mtime);
				*sorted_list = eina_list_sorted_insert(*sorted_list,
								     __gl_fs_compare,
								     pnode);
				g_free(sub_folder);
				sub_folder = NULL;
			} else {
				gl_dbgE("Invalid sub folder!");
			}
			pnode = NULL;
		}
	}

	if (fullpath) {
		g_string_free(fullpath, true);
		fullpath = NULL;
	}

	if (file_list)
		__gl_fs_clear_list(&file_list);

	if (dir_list) {
		eina_list_free(dir_list);
		dir_list = NULL;
	}

	return size;
}

unsigned long long gl_fs_get_free_space(int state)
{
	struct statvfs info;
	char *path = NULL;

	if (state == GL_STORE_T_PHONE)
		path = GL_ROOT_PATH_PHONE;
	else if (state == GL_STORE_T_MMC)
		path = GL_ROOT_PATH_MMC;
	else
		return 0;

	if (storage_get_internal_memory_size(&info) < 0) {
		return 0;
	}

	return ((unsigned long long)(info.f_bsize)) * ((unsigned long long)(info.f_bavail));
}

/* Add checking for cancellation to terminate RW loop */
Eina_Bool __gl_fs_cp_file(void *data, const char *src, const char *dst)
{
#define PATH_MAX 4096
#define BUF_MAX 16384
	FILE *f1 = NULL;
	FILE *f2 = NULL;
	char buf[BUF_MAX] = { 0, };
	char realpath1[PATH_MAX] = { 0, };
	char realpath2[PATH_MAX] = { 0, };
	size_t num = 0;
	Eina_Bool ret = EINA_TRUE;

	if (!realpath(src, realpath1))
		return EINA_FALSE;
	if (realpath(dst, realpath2) && !g_strcmp0(realpath1, realpath2))
		return EINA_FALSE;

	f1 = fopen(src, "rb");
	if (!f1)
		return EINA_FALSE;
	f2 = fopen(dst, "wb");
	if (!f2) {
		fclose(f1);
		return EINA_FALSE;
	}
	while ((num = fread(buf, 1, sizeof(buf), f1)) > 0) {
		int pbar_cancel = false;
		gl_thread_get_cancel_state(data, &pbar_cancel);
		if (pbar_cancel > GL_PB_CANCEL_NORMAL) {
			gl_dbgW("Cancelled[%d]!", pbar_cancel);
			fclose(f1);
			fclose(f2);
			gl_file_unlink(dst);
			return EINA_FALSE;
		}
		if (fwrite(buf, 1, num, f2) != num)
			ret = EINA_FALSE;
	}
	fclose(f1);
	fclose(f2);
	return ret;
}

#if 0 //Unused function
int _gl_fs_update_file_mtime(const char *path)
{
	GL_CHECK_VAL(path, -1);
	struct timeval tv;
	gettimeofday(&tv, NULL);
	struct timeval times[2] = { {0, 0}, {0, 0} };
	times[0].tv_sec = tv.tv_sec;
	times[0].tv_usec = tv.tv_usec;
	times[1].tv_sec = tv.tv_sec;
	times[1].tv_usec = tv.tv_usec;
	if (utimes(path, times) < 0) {
		gl_dbgE("utimes failed!");
		return -1;
	}
	return 0;
}
#endif

int _gl_fs_get_file_stat(const char *filename, gl_node_info_s **node)
{
	struct stat statbuf;
	GL_CHECK_VAL(node, -1);
	GL_CHECK_VAL(*node, -1);
	GL_CHECK_VAL(filename, -1);

	if (stat(filename, &statbuf) == -1)
		return -1;

	/* total size, in bytes */
	(*node)->size = statbuf.st_size;
	(*node)->mtime = statbuf.st_mtime;
	return 0;
}

unsigned long long _gl_fs_get_folder_size(char *path)
{
	if (!gl_file_is_dir(path)) {
		gl_dbgE("Not a directory!");
		return 0;
	}

	unsigned long long size = 0;
	unsigned long long sub_size = 0;
	GString *fullpath = g_string_new(path);
	Eina_List *file_list = NULL;
	Eina_List *dir_list = NULL;
	int error_code = 0;
	error_code = __gl_fs_get_file_list(fullpath, &dir_list, &file_list);

	if (error_code == 0) {
		int i = 0;
		int dir_cnt = eina_list_count(dir_list);
		int file_cnt = eina_list_count(file_list);
		gl_node_info_s *pnode = NULL;
		for (i = 0; i < file_cnt; i++) {
			pnode = (gl_node_info_s *)eina_list_nth(file_list, i);
			if (pnode == NULL) {
				gl_dbgE("Invalid node!");
				continue;
			}
			size += pnode->size;
			pnode = NULL;
		}
		gl_dbg("file_cnt[%d], size[%llu]", file_cnt, size);

		gl_dbg("dir_cnt[%d]", dir_cnt);
		i = 0;
		pnode = NULL;
		char *sub_folder = NULL;
		for (i = 0; i < dir_cnt; i++) {
			pnode = (gl_node_info_s *)eina_list_nth(dir_list, i);
			if (pnode == NULL) {
				gl_dbgE("Invalid node!");
				continue;
			}
			sub_folder = g_strconcat(pnode->path, "/", pnode->name,
						 NULL);
			if (sub_folder) {
				gl_sdbg("sub_folder[%s]", pnode->name);
				sub_size = _gl_fs_get_folder_size(sub_folder);
				pnode->size = sub_size;
				size += sub_size;
				gl_dbg("mtime[%d]", pnode->mtime);
				g_free(sub_folder);
				sub_folder = NULL;
			} else {
				gl_dbgE("Invalid sub folder!");
			}
			pnode = NULL;
		}
	}

	if (fullpath) {
		g_string_free(fullpath, true);
		fullpath = NULL;
	}

	if (file_list)
		__gl_fs_clear_list(&file_list);

	if (dir_list)
		__gl_fs_clear_list(&dir_list);
	return size;
}

int _gl_fs_rm_folder(char *path, long long cut_size, long long max_size,
		     unsigned int expired_time)
{
	GL_CHECK_VAL(path, -1);
	gl_node_info_s *pnode = NULL;
	Eina_List *dir_list = NULL;
	Eina_List *l = NULL;
	unsigned long long size = 0;
	gl_dbg("Size cut/max[%lld/%lld]", cut_size, max_size);

	size = __gl_fs_sort_folders_by_mtime(path, &dir_list);
	GL_CHECK_VAL(dir_list, -1);

	char *folder = NULL;
	unsigned long long _cut_size = 0;
	if (size > max_size)
		_cut_size = size - max_size + cut_size;
	else if (size + cut_size > max_size)
		_cut_size = cut_size;
	gl_dbg("Size cut/total[%llu/%llu]", _cut_size, size);

	unsigned int current_t = 0;
	struct timeval tv;
	gettimeofday(&tv, NULL);
	current_t = tv.tv_sec + tv.tv_usec / GL_TIME_USEC_PER_SEC;
	gl_dbg("current time[%d], expired time[%d]", current_t, expired_time);
	unsigned long long rm_size = 0;

	EINA_LIST_FOREACH(dir_list, l, pnode) {
		if (pnode == NULL || pnode->path == NULL || pnode->name == NULL)
			continue;

		/* Get full path of folder */
		folder = g_strconcat(pnode->path, "/", pnode->name, NULL);
		if (folder == NULL)
			continue;

		gl_sdbg("mtime[%d], path[%s]", pnode->mtime, folder);
		if (pnode->mtime + expired_time < current_t) {
			/* Remove folder */
			gl_dbgW("Remove!");
			gl_file_recursive_rm(folder);
			g_free(folder);
			folder = NULL;
			rm_size += pnode->size;
			continue;
		} else if (_cut_size == 0 || rm_size >= _cut_size) {
			gl_dbgW("Done!");
			break;
		}

		/* Remove folder */
		gl_file_recursive_rm(folder);
		g_free(folder);
		folder = NULL;

		gl_dbg("size[cut/dir]=%llu/%llu", _cut_size, pnode->size);
		if (pnode->size >= _cut_size) {
			gl_dbgW("Done!");
			break;
		}

		pnode = NULL;
	}
	__gl_fs_clear_list(&dir_list);
	return 0;
}

#ifdef _RENAME_ALBUM_SENSITIVE
bool _gl_fs_check_name_case(char *dir, char *exist_name)
{
	GL_CHECK_FALSE(dir);
	GL_CHECK_FALSE(exist_name);
	gl_dbg("");

	char dest_dir[GL_DIR_PATH_LEN_MAX] = {0};
	char dest_filename[GL_FILE_NAME_LEN_MAX] = {0};
	char *tmp = NULL;
	tmp = strrchr(dir, '/');
	if (tmp) {
		g_strlcpy(dest_filename, tmp + 1, GL_FILE_NAME_LEN_MAX);
		tmp[0] = '\0';
		g_strlcpy(dest_dir, dir, GL_DIR_PATH_LEN_MAX);
		tmp[0] = '/';
	} else {
		return false;
	}

	bool ret = false;
	Eina_List *name_list = NULL;
	if ((name_list = gl_file_ls(dest_dir)) == NULL) {
		gl_dbgE("open dir failed!");
		return false;
	} else {
		char *dir_name = NULL;
		EINA_LIST_FREE(name_list, dir_name) {
			if (dir_name &&
			    strcasecmp(dir_name, dest_filename) == 0) {
				gl_dbg("Have same name directory");
				ret = true;
				memset(exist_name, 0x00, GL_ALBUM_NAME_LEN_MAX);
				g_strlcpy(exist_name, dir_name,
					  GL_ALBUM_NAME_LEN_MAX);
				break;
			}
			GL_FREEIF(dir_name);
		}
	}
	return ret;
}
#endif

bool _gl_fs_is_low_memory(unsigned long long total_size)
{
	unsigned long long free_size = gl_fs_get_free_space(GL_STORE_T_PHONE);
	if (total_size && free_size < total_size) {
		gl_dbgE("Low memory: Free(%llu Byte) < required(%llu Byte)!",
			free_size, total_size);
		return true;
	}

	return false;
}

int _gl_fs_get_file_ext(const char *file_path, char *file_ext, int max_len)
{
	int i = 0;

	for (i = strlen(file_path); i >= 0; i--) {
		if ((file_path[i] == '.') && (i < strlen(file_path))) {
			strncpy(file_ext, &file_path[i + 1], max_len);
			return 0;
		}

		/* meet the dir. no ext */
		if (file_path[i] == '/') {
			return -1;
		}
	}

	return -1;
}

/*
*   return file extension, f.e. jpg; then return new path without ext.
*/
bool _gl_fs_get_path_without_ext(const char *file_path, char *file_ext,
				 char *new_path)
{
	int i = 0;
	for (i = strlen(file_path); i >= 0; i--) {
		if ((file_path[i] == '.') && (i < GL_FILE_PATH_LEN_MAX)) {
			g_strlcpy(file_ext, &file_path[i + 1], GL_FILE_EXT_LEN_MAX);
			g_strlcpy(new_path, file_path, i + 1);
			new_path[i] = '\0';
			gl_sdbg("path without extension :%s", new_path);
			return true;
		}

		/* meet the dir. no ext */
		if (file_path[i] == '/')
			return true;
	}
	return true;
}

Eina_Bool _gl_fs_copy(void *data, const char *src, const char *dst)
{
	GL_CHECK_FALSE(src);
	GL_CHECK_FALSE(dst);
	GL_CHECK_FALSE(data);

	gl_sdbg("\tSrc: %s", src);
	gl_sdbg("\tDest: %s", dst);
	struct stat st;

	/*
	 * Make sure this is a regular file before
	 * we do anything fancy.
	 */
	if (stat(src, &st) < 0) {
		gl_dbgE("stat fail[%d]!", errno);
		return EINA_FALSE;
	}
	if (S_ISREG(st.st_mode)) {
		/*
		 * Copy to dst file
		 */
		if (!__gl_fs_cp_file(data, src, dst)) {
			gl_dbgE("Copy file failed[%d]!", errno);
			return EINA_FALSE;
		}
		sync();
		return EINA_TRUE;
	}
	gl_dbgE("S_ISREG fail[%d]!", errno);
	return EINA_FALSE;
}

/*
* Use gl_file_mv() to move medias to other album.
* Media-server, which is different with libmedia-info, watches src and dest folder,
* it updates libmedia-info DB asynchronously.
* While move/copy mass data in My Files appliation,
* After move/copy done in My files, check the dest folder in Gallery.
* You'll find its content is changing.
* gl_file_mv() operate libmedia-info DB synchronously, so it's faster than media-server.
*/

/*
*  stop using "rename" when moving from mmc to phone for correct db update.
*/
Eina_Bool _gl_fs_move(void *data, const char *src, const char *dst)
{
	GL_CHECK_FALSE(src);
	GL_CHECK_FALSE(dst);
	GL_CHECK_FALSE(data);

	gl_sdbg("\tSrc: %s", src);
	gl_sdbg("\tDest: %s", dst);
	/*
	 * From->To: MMC->MMC or Phone->Phone
	 */
	if (rename(src, dst)) {
		/*
		 * File cannot be moved directly because
		 * it resides on a different mount point.
		 */
		if (errno == EXDEV) {
			/*
			 * From->To: MMC->Phone or Phone->MMC
			 */
			gl_dbgW("errno = EXDEV(%d): Cross-device link", errno);
			struct stat st;

			/*
			 * Make sure this is a regular file before
			 * we do anything fancy.
			 */
			if (stat(src, &st) < 0) {
				gl_dbgE("stat fail[%d]!", errno);
				return EINA_FALSE;
			}
			if (S_ISREG(st.st_mode)) {
				/*
				 * Copy to dst file
				 */
				if (!__gl_fs_cp_file(data, src, dst)) {
					gl_dbgE("Copy file failed[%d]!", errno);
					return EINA_FALSE;
				}


				/*
				 * Delete src file
				 */
				if (!gl_file_unlink(src))
					gl_dbgE("Delete file failed[%d]!", errno);

				/* Write file to filesystem immediately */
				sync();
				return EINA_TRUE;
			}
			gl_dbgE("S_ISREG fail[%d]!", errno);
			return EINA_FALSE;
		}

		gl_dbgE("Fail[%d]!", errno);
		return EINA_FALSE;
	}
	/* Write file to filesystem immediately */
	sync();
	return EINA_TRUE;
}

char *_gl_fs_get_unique_full_path(char *file_path, char *ext)
{
	char *file_name = file_path;
	char *extension = ext;
	char *final_path = NULL;
	int final_path_len = 0;
	int extension_len = 0;
	int suffix_count = 0;
	/* means suffix on file name. up to "_99999" */
	const int max_suffix_count = 99999;
	/* 1 means "_" */
	int suffix_len = (int)log10(max_suffix_count + 1) + 1;

	if (!file_path)
		return NULL;

	gl_sdbg("file_path=[%s], ext=[%s]", file_path, ext);

	if (extension)
		extension_len = strlen(extension);

	/* first 1 for ".", last 1 for NULL */
	final_path_len = strlen(file_name) + 1 + suffix_len + extension_len + 1;

	final_path = (char *)calloc(1, final_path_len);
	if (!final_path) {
		gl_dbgE("calloc failed!");
		return NULL;
	}

	do {
		/* e.g) /tmp/abc.jpg
		 * if there is no extension name, just make a file name without extension */
		if (0 == extension_len) {
			if (suffix_count == 0) {
				snprintf(final_path, final_path_len, "%s",
					 file_name);
			} else {
				snprintf(final_path, final_path_len, "%s_%d",
					 file_name, suffix_count);
			}
		} else {
			if (suffix_count == 0) {
				snprintf(final_path, final_path_len, "%s.%s",
					 file_name, extension);
			} else {
				snprintf(final_path, final_path_len, "%s_%d.%s",
					 file_name, suffix_count, extension);
			}
		}

		if (gl_file_exists(final_path)) {
			suffix_count++;
			if (suffix_count > max_suffix_count) {
				gl_dbgE("Max suffix count!");
				GL_FREE(final_path);
				break;
			} else {
				memset(final_path, 0x00, final_path_len);
				continue;
			}
		}

		break;
	} while (1);

	gl_sdbg("Decided path = [%s]", final_path);
	return final_path;
}

char *_gl_fs_get_unique_new_album_name(char *parent_path, char *default_name,
				       char **new_name)
{
	char *final_path = NULL;
	char *final_name = NULL;
	int final_path_len = 0;
	int final_name_len = 0;
	int suffix_count = 0;
	/* means suffix on file name. up to "_99999" */
	const int max_suffix_count = 99999;
	/* 1 means "_" */
	int suffix_len = (int)log10(max_suffix_count + 1) + 1;

	if (!parent_path || !default_name || !new_name)
		return NULL;

	gl_sdbg("parent_path=[%s], default_name=[%s]", parent_path, default_name);

	/* 1 for NULL */
	final_name_len = strlen(default_name) + suffix_len + 1;
	/* 1 for / */
	final_path_len = strlen(parent_path) + 1 + final_name_len;

	final_path = (char *)calloc(1, final_path_len);
	if (!final_path) {
		gl_dbgE("calloc failed!");
		return NULL;
	}
	final_name = (char *)calloc(1, final_name_len);
	if (!final_name) {
		gl_dbgE("calloc failed!");
		GL_FREE(final_path);
		return NULL;
	}

	do {
		if (suffix_count == 0) {
			snprintf(final_name, final_name_len, "%s", default_name);
		} else {
			snprintf(final_name, final_name_len, "%s %d",
				 default_name, suffix_count);
		}
		snprintf(final_path, final_path_len, "%s/%s", parent_path,
			 final_name);

		/**
		* If dir is empty, 1 is returned,
		* if it contains at least 1 file, 0 is returned.
		* On failure, -1 is returned.
	 	*/
		if (gl_file_dir_is_empty(final_path) == 0) {
			suffix_count++;
			if (suffix_count > max_suffix_count) {
				gl_dbgE("Max suffix count!");
				GL_FREE(final_path);
				GL_FREE(final_name);
				break;
			} else {
				memset(final_path, 0x00, final_path_len);
				memset(final_name, 0x00, final_name_len);
				continue;
			}
		}

		break;
	} while (1);

	*new_name = final_name;
	gl_sdbg("Decided path = [%s][%s]", final_path, final_name);
	return final_path;
}

bool _gl_fs_validate_name(const char *new_name)
{
	GL_CHECK_FALSE(new_name);
	char invalid_chars[] = { '/', '\\', ':', '*', '?', '"', '<', '>', '|', '\0' };
	char *ptr = invalid_chars;

	if (strncmp(new_name, ".", strlen(".")) == 0) {
		return false;
	}

	gl_sdbg("new album name is %s\n", new_name);
	while (*ptr != '\0') {
		if (strchr(new_name, (*ptr)) != NULL) {
			gl_dbg("invalid character is %c", *ptr);
			return false;
		}
		++ptr;
	}

	return true;
}

bool _gl_fs_validate_character(const char name_char)
{
	char invalid_chars[] = { '/', '\\', ':', '*', '?', '"', '<', '>', '|', '\0' };
	char *ptr = invalid_chars;

	gl_dbg("character is %c", name_char);
	while (*ptr != '\0') {
		if (name_char == *ptr) {
			gl_dbg("invalid character is %c", *ptr);
			return false;
		}
		++ptr;
	}

	return true;
}

int _gl_fs_get_default_folder(char *path)
{
	int len = 0;
	GL_CHECK_VAL(path, -1);

	len = snprintf(path, GL_DIR_PATH_LEN_MAX, "%s",
		       GL_ROOT_PATH_PHONE);
	if (len < 0) {
		gl_dbgE("snprintf returns failure!");
		return -1;
	} else {
		path[len] = '\0';
		len = -1;
	}

	len = g_strlcat(path, GL_DEFAULT_PATH_IMAGES,
			GL_DIR_PATH_LEN_MAX);
	if (len >= GL_DIR_PATH_LEN_MAX) {
		gl_dbgE("strlcat returns failure(%d)!", len);
		return -1;
	}
	gl_sdbg("Default images path: %s.", path);

	return 0;
}

bool _gl_fs_mkdir(const char *path)
{
	GL_CHECK_FALSE(path);
	struct stat st;
	if (stat(path, &st) < 0) {
		if (!gl_file_mkpath(path)) {
			gl_sdbgE("Failed to mkdir[%s]!", path);
			return false;
		}
	}
	return true;
}
