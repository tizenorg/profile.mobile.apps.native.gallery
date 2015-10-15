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

#ifndef _GL_THUMBS_H_
#define _GL_THUMBS_H_

#include <Elementary.h>
#include "gallery.h"

typedef enum _gl_thumb_mode {
	GL_THUMB_ALL,
	GL_THUMB_IMAGES,
	GL_THUMB_VIDEOS,
} gl_thumb_mode;

/* Show edit view(add checkbox) when creating thumbnails */
#define GL_THUMBS_EDIT_FIRST (0x1 << 8)

typedef enum _gl_thumbs_edit_mode_t {
	GL_THUMBS_EDIT_NONE = (0x00),
	GL_THUMBS_EDIT_NORMAL = (0x1 << 0),
	GL_THUMBS_EDIT_SHARE = (0x1 << 1),
	GL_THUMBS_EDIT_COPY = (0x1 << 2),
	GL_THUMBS_EDIT_MOVE = (0x1 << 3),
#ifdef _USE_APP_SLIDESHOW
	GL_THUMBS_EDIT_SLIDESHOW = (0x1 << 4) | GL_THUMBS_EDIT_FIRST,
#endif
} gl_thumbs_edit_e;

Eina_Bool _gl_thumbs_append_items(void *data);
int _gl_thumbs_update_realized_items(void *data);
Evas_Object *_gl_thumbs_get_content(void *data, Evas_Object *parent,
				    gl_item *gitem, int w, int h);
int _gl_thumbs_add_btns(void *data);
void _gl_thumbs_mouse_down(void *data, Evas *e, Evas_Object *obj, void *ei);
void _gl_thumbs_mouse_up(void *data, Evas *e, Evas_Object *obj, void *ei);
Evas_Object *_gl_thumbs_add_grid(void *data, Evas_Object *parent,
				 gl_thumb_mode mode);
int _gl_thumbs_show_edit_view(void *data);
int _gl_thumbs_show_view(void *data);
Eina_Bool _gl_thumbs_show_items(void *data);
int _gl_thumbs_set_list(void *data, Eina_List *elist);
int _gl_thumbs_clear_and_set_list(void *data, Eina_List *elist);
int _gl_thumbs_get_list(void *data, Eina_List **p_elist);
int _gl_thumbs_create_view(void *data, int nf_mode, char *title, bool b_idler,
			   Elm_Naviframe_Item_Pop_Cb func);
int _gl_thumbs_update_split_view(void *data, int nf_mode, char *title, bool b_idler,
			   Elm_Naviframe_Item_Pop_Cb func);
int _gl_thumbs_update_edit_split_view(void *data, int nf_mode, char *title, bool b_idler,
			   Elm_Naviframe_Item_Pop_Cb func);
int _gl_thumbs_update_items(void *data);
int _gl_thumbs_update_view(void *data);
int _gl_thumbs_destroy_view(void *data, bool b_pop);
int _gl_thumbs_destroy_view_with_cb(void *data, void *cb);
bool _gl_thumbs_get_view(void *data, Evas_Object **view);
bool _gl_thumbs_show_nocontents(void *data);
int _gl_thumbs_set_nocontents(void *data, Evas_Object *noc);
int _gl_thumbs_set_size(void *data, Evas_Object *gridview);
int _gl_thumbs_clear_cbs(Evas_Object *grid);
int _gl_thumbs_delete_view(void *data);
int _gl_thumbs_update_size(void *data, Evas_Object *view);
bool _gl_thumbs_is_append(gl_item *gitem, char **burstshot_id);
bool _gl_thumbs_check_zero(void *data);
int _gl_thumbs_update_label(Elm_Object_Item *nf_it, char *title);
int _gl_thumbs_update_label_text(Elm_Object_Item *nf_it, int sel_cnt,
				 bool b_lang);
int _gl_thumbs_update_lang(void *data);
int _gl_thumbs_disable_slideshow(void *data, bool b_disabled);
int _gl_thumbs_disable_share(void *data, bool b_disabled);
int _gl_thumbs_check_btns(void *data);
int _gl_thumbs_update_sequence(void *data);
int _gl_thumbs_set_edit_mode(void *data, int mode);
int _gl_thumbs_get_edit_mode(void *data);
int _gl_thumbs_rotate_view(void *data);
void _gl_thumbs_sel_cb(void *data, Evas_Object *obj, void *ei);
void _gl_thumbs_open_file(void *data);
void _gl_thumbs_open_file_select_mode(void *data);
int _gl_thumbs_create_thumb(gl_item *gitem);
void _gl_thumbs_change_view(void *data, int prev_x, int current_x);
void _gl_thumb_update_split_view(void *data);

#endif /* _GL_THUMBS_H_ */

