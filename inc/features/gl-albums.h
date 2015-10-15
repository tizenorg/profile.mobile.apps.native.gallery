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

#ifndef _GL_ALBUMS_H_
#define _GL_ALBUMS_H_

#include <Elementary.h>
#include "gallery.h"

int _gl_albums_close_album(void *data);
int gl_albums_update_items(void *data);
int _gl_albums_create_view(void *data, Evas_Object *parent);
int gl_albums_update_view(void *data);
int gl_albums_change_to_view(void *data);
int gl_albums_remove_nocontents(void *data);
int gl_albums_free_data(void *data);
int _gl_albums_show_view_tab(void *data, Evas_Object *parent);
int _gl_albums_hide_view_tab(void *data);
int _gl_albums_change_mode(void *data, bool b_edit);
int _gl_albums_check_btns(void *data);
int _gl_albums_rotate_view(void *data);
int _gl_albums_set_current(void *data, gl_cluster *current);
gl_cluster *_gl_albums_get_current(void *data);
int _gl_albums_clear_cbs(Evas_Object *view);
int gl_albums_open_album(gl_cluster * album_item);
Evas_Object *_gl_albums_add_gengrid(void *data, Evas_Object *parent);
Evas_Object *_gl_albums_add_view(void *data, Evas_Object *parent);
Evas_Object *_gl_albums_sel_add_view(void *data, Evas_Object *parent);
void __gl_albums_new_album_sel(void *data, Evas_Object *obj, void *ei);
int _gl_albums_create_items(void *data, Evas_Object *parent);
int _gl_split_albums_create_items(void *data, Evas_Object *parent);

#endif /* _GL_ALBUMS_H_ */

