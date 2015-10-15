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
#ifndef _GL_DATA_UTIL_H_
#define _GL_DATA_UTIL_H_

#include <eina_list.h>
#include <glib.h>
#include <Elementary.h>
#include "gl-data-type.h"

#ifdef __cplusplus
extern "C"
{
#endif

#define GL_MAX_BYTES_FOR_CHAR 3
#define GL_FILE_PATH_LEN_MAX (4095 * GL_MAX_BYTES_FOR_CHAR + 1)
#define GL_FILE_NAME_LEN_MAX (255 * GL_MAX_BYTES_FOR_CHAR + 1)
#define GL_DIR_PATH_LEN_MAX GL_FILE_PATH_LEN_MAX
#define GL_ARRAY_LEN_MAX 256
#define GL_FILE_EXT_LEN_MAX 256
#define GL_FIRST_VIEW_START_POS 0
#define GL_FIRST_VIEW_END_POS 47	//27            /*maybe 2 pages(=48 medias) is better, especially select album then drag up immediately */
#define GL_GET_UNTIL_LAST_RECORD 65536	//-1      /* Fixme: Do not use 65536. */
#define GL_GET_ALL_RECORDS (-1)
#define GL_GET_ONE_RECORD 1
#define GL_GET_ALL_RECORDS_ID NULL

#define GL_ALBUM_COVER_THUMB_NUM 1

/**
* Get a new mtype item from libmedia-info,
* if ID is NULL, libmedia-info just allocate memory for mtype item,
* then return it, instead of loading record from DB to initialize each field.
*/
#define GL_NEW_RECORD_ID NULL
#define GL_PINCH_WIDTH 4
#define GL_PINCH_LANDSCAPE_WIDTH 9

#define GL_JPEG_FILE_EXT "jpg"

typedef enum {
	GL_SORT_BY_NONE,	/**< No Sort */
	GL_SORT_BY_NAME_DESC, 	/**< Sort by display name descending */
	GL_SORT_BY_NAME_ASC, 	/**< Sort by display name ascending */
	GL_SORT_BY_DATE_DESC, 	/**< Sort by modified_date descending */
	GL_SORT_BY_DATE_ASC, 	/**< Sort by modified_date ascending */
} gl_sort_type_e;

typedef enum {
	GL_ICON_NORMAL,
	GL_ICON_CORRUPTED_FILE
}gl_icon_type_e;

typedef struct {
	Eina_List *clist;
	/* Editable albums count */
	int edit_cnt;
	int phone_cnt; /* Phone/MMC albums count */
} gl_cluster_list;

typedef struct _gl_thumb_data_t gl_thumb_data_s;
typedef struct _gl_album_data_t gl_album_data_s;
typedef struct _gl_cluster gl_cluster;

typedef struct _gl_item {
	gl_media_s *item;
	int icon_idx;
	int list_item_cnt;

	int list_idx[GL_PINCH_LANDSCAPE_WIDTH];

	Elm_Object_Item *elm_item;
	bool preappend;
	bool checked;
	int sequence;
	bool b_deleted; /* Gengrid item is deleted, to free item data */
	/* To indicate media storage type, especially used for PTP/AllShare */
	int store_type;
	struct _gl_item *pre_gitem;
	void *ad; /* Save ad to prevent use global variable */
	gl_thumb_data_s *thumb_data;
	gl_cluster *album;
} gl_item;

struct _gl_cluster {
	gl_album_s *cluster;
	int index;
	Eina_List *elist; /* List of all selected medias of a album */
	Eina_List *elist1; /* List of all selected medias of a album */
	Elm_Object_Item *item;
	bool checked;

	void *ad; /* Save ad to prevent use global variable */
	gl_item *cover;
	int cover_thumbs_cnt;
	gl_album_data_s *album_data;
};

struct _gl_album_data_t {
	void *ad;
	gl_cluster *album;
};

struct _gl_thumb_data_t {
	void *ad;
	gl_item *item;
};

bool _gl_data_util_get_file_dir_name(const char *file_path, char *filename,
				     char *dir_name, char *dir_path);
gl_item *_gl_data_util_calloc_gitem(void);
int _gl_data_util_free_gitem(gl_item *gitem);
gl_cluster *_gl_data_util_calloc_gcluster(void);
int _gl_data_util_free_gcluster(gl_cluster *gcluster);
gl_cluster *_gl_data_util_new_gcluster_all(void *data, int count);
gl_cluster *_gl_data_util_new_gcluster_fav(void *data, int count);
gl_cluster_list *_gl_data_util_calloc_cluster_list(void);
int _gl_data_util_clear_gtype_item_list(Eina_List **elist);
int _gl_data_util_clear_item_list(Eina_List **elist);
int _gl_data_util_clear_cluster_list(Eina_List **elist);
int _gl_data_util_get_selected_cluster_id_list(void *data,
					       Eina_List **sel_id_list);
int _gl_data_util_get_selected_str_ids(Eina_List **sel_list, Eina_List **elist);
bool _gl_data_util_check_selected_str_id(Eina_List **sel_id_list, const char *id);
int _gl_data_util_free_selected_str_list(Eina_List **sel_id_list);
bool _gl_data_util_check_selected_int_id(Eina_List **sel_id_list, int id);
int _gl_data_util_check_album_selected_files(gl_cluster *album,
					     int *img_cnt, int *sel_cnt,
					     int *web_cnt);
int _gl_data_util_get_cond(char *cond);
int _gl_data_util_get_fav_cond(char *cond);

#ifdef __cplusplus
}
#endif
#endif /* _GL_DATA_UTIL_H_ */

