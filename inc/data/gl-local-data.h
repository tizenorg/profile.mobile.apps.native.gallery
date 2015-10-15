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

#ifndef _GL_LOCAL_DATA_H_
#define _GL_LOCAL_DATA_H_

#include "gl-data-type.h"

#ifdef __cplusplus
extern "C"
{
#endif


/*MEDIA_TYPE 0-image, 1-video, 2-sound, 3-music*/
#define GL_CONDITION_IMAGE_VIDEO "(MEDIA_TYPE=0 OR MEDIA_TYPE=1)"
#define GL_CONDITION_IMAGE       "(MEDIA_TYPE=0)"
#define GL_CONDITION_VIDEO       "(MEDIA_TYPE=1)"
#define GL_CONDITION_ORDER "MEDIA_TIMELINE, MEDIA_DISPLAY_NAME"

#define CONDITION_LENGTH 512
#define KEYWORD_LENGTH 128

typedef struct _gl_filter_t gl_filter_s;

struct _gl_filter_t {
	char cond[CONDITION_LENGTH];             /*set media type or favorite type, or other query statement*/
	media_content_collation_e collate_type;  /*collate type*/
	media_content_order_e sort_type;         /*sort type*/
	char sort_keyword[KEYWORD_LENGTH];       /*sort keyword*/
	int offset;                              /*offset*/
	int count;                               /*count*/
	bool with_meta;                          /*whether get image or video info*/
};

int _gl_local_data_connect(void);
int _gl_local_data_disconnect(void);
int _gl_local_data_get_album_by_path(char *path, gl_album_s **album);
int _gl_local_data_get_album_list(gl_filter_s *condition, Eina_List **elist);
int _gl_local_data_get_media_by_id(char *media_id, gl_media_s **mitem);
int _gl_local_data_get_media_by_path(const char *path, gl_media_s **mitem);
int _gl_local_data_get_media_count(const char *cluster_id, gl_filter_s *condition,
				   int *item_cnt);
int _gl_local_data_get_all_media_count(gl_filter_s *filter, int *item_cnt);
int _gl_local_data_get_album_media_list(gl_filter_s *condition,
					const char *album_id, Eina_List **elist);
int _gl_local_data_get_album_cover(gl_filter_s *condition, const char *album_id,
				   Eina_List **elist);
int _gl_local_data_get_all_albums_media_list(gl_filter_s *condition,
					     Eina_List **elist);
int _gl_local_data_get_fav_albums_media_list(gl_filter_s *condition,
					     Eina_List **elist);
int _gl_local_data_get_all_albums_cover(gl_filter_s *condition,
					Eina_List **elist);
int _gl_local_data_get_fav_albums_cover(gl_filter_s *condition,
					Eina_List **elist);
int _gl_local_data_delete_album(gl_album_s *cluster, void *cb, void *data, bool is_hide);
int _gl_local_data_add_media(const char *file_url, media_info_h *info);
int _gl_local_data_get_thumb(gl_media_s *mitem, char **thumb);
int _gl_local_data_move_media(gl_media_s *mitem, const char *dst);
bool _gl_local_data_is_album_exists(char *path);
int _gl_local_data_get_path_by_id(const char *uuid, char **path);

#ifdef __cplusplus
}
#endif


#endif

