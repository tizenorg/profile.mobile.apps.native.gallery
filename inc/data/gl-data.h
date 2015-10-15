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

#ifndef _GL_DATA_H_
#define _GL_DATA_H_

#include <eina_list.h>
#include <glib.h>
#include <Elementary.h>
#include "gl-local-data.h"
#include "gl-data-util.h"

#ifdef __cplusplus
extern "C"
{
#endif

int _gl_data_cp_item_list(void *data, Eina_List *elist, Eina_List **p_elist);
int _gl_data_clear_cluster_list(void *data, bool b_force);
bool _gl_data_get_cluster_list(void *data);
int _gl_data_update_cluster_list(void *data);
int _gl_data_cluster_list_remove(void *data, gl_cluster *item);
bool _gl_data_exists_item(void *data, const char *id);
int _gl_data_get_cluster_by_id(void *data, const char *cluster_id,
			       gl_cluster **cluster);
int _gl_data_get_cluster_by_path(void *data, const char *path,
				 gl_cluster **cluster);
int _gl_data_get_item_cnt(const char *cluster_id, int album_type, int *item_cnt);
int _gl_data_update_item_cnt(gl_cluster *album);
int _gl_data_get_items(int start_pos, int end_pos, Eina_List **list);
int _gl_data_get_items_album(void *data, gl_cluster *album, int start_pos,
			     int end_pos, Eina_List **p_elist);
int _gl_data_update_item_list(void *data, Eina_List **sel_id_list,
			      Eina_List **p_elist);
bool _gl_data_is_item_cnt_zero(void *data, media_content_type_e type,
			       Eina_List *elist);
int _gl_data_get_first_item(media_content_type_e type, Eina_List *elist,
			    gl_item **p_gitem);
int _gl_data_get_album_cover(gl_cluster *album, gl_item **pgitem,
			     media_content_order_e sort_type);
gl_item *_gl_data_new_item_mitem(gl_media_s *mitem);
int _gl_data_get_orig_item_by_index(void *data, gl_item **gitem, int idx);
int _gl_data_delete_file(char *path, char *uuid, bool is_hide);
int _gl_data_selected_list_append(void *data, gl_item *gitem);
int _gl_data_selected_list_remove(void *data, gl_item *gitem);
int _gl_data_selected_fav_list_remove(void *data, gl_item *gitem);
int _gl_data_selected_list_remove_one(void *data, gl_item *gitem);
gl_item *_gl_data_selected_list_get_nth(void *data, int idx);
int _gl_data_selected_list_finalize(void *data);
int _gl_data_clear_selected_info(void *data);
int _gl_data_selected_list_count(void *data);
bool _gl_data_is_albums_selected_empty(void *data);
int _gl_data_get_albums_selected_files(void *data, void *get_cb, void *cb_data);
int _gl_data_get_albums_selected_cnt(void *data);
int _gl_data_selected_add_burstshot(void *data, gl_item *gitem, Eina_List *elist);
bool _gl_data_get_burstshot_status(void *data, gl_item *gitem);
int _gl_data_albums_selected_list_append(void *data, gl_cluster *item);
int _gl_data_albums_selected_list_remove(void *data, gl_cluster *item);
int _gl_data_finalize_albums_selected_list(void *data);
int _gl_data_init(void *data);
int _gl_data_finalize(void *data);
gl_item *_gl_data_new_gitem(void *data, const char *item_id);
bool _gl_data_is_root_path(const char *path);
bool _gl_data_check_root_type(const char *path, const char *root);
bool _gl_data_is_default_album(const char *match_folder, gl_album_s *album);
bool _gl_data_is_screenshot_album(const char *match_folder, gl_album_s *album);
bool _gl_data_is_camera_album(gl_album_s *album);
int _gl_data_delete_media(void *data, gl_media_s *media_item);
int _gl_data_create_thumb(gl_media_s *item,
			  media_thumbnail_completed_cb callback,
			  void *user_data);
int _gl_data_cancel_thumb(gl_media_s *item);
bool _gl_data_is_album_exists(void *data, gl_cluster *album);
int _gl_data_save_selected_str_ids(void *data, Eina_List **elist);
int _gl_data_restore_selected(Eina_List *sel_ids, gl_item *gitem);
bool _gl_data_check_selected_id(Eina_List *sel_id_list, const char *id);

#ifdef __cplusplus
}
#endif
#endif /* _GL_DATA_H_ */
