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

#ifndef _GL_TIMELINE_H_
#define _GL_TIMELINE_H_

#include <Elementary.h>
#include "gallery.h"

int _gl_timeline_create_view(void *data, Evas_Object *parent);
int _gl_timeline_update_view(void *data);
int _gl_timeline_update_lang(void *data);
int _gl_timeline_hide_view(void *data);
int _gl_timeline_view_rotate(void *data);
int _gl_timeline_rotate_view(void *data);
void __gl_timeline_asc_mode_set(void *data);
void __gl_timeline_desc_mode_set(void *data);
bool __gl_update_timeline(void *data);
int _gl_timeline_update_grid_size(void *data);
void _gl_update_timeview_iv_select_mode_reply(void *data, char **select_result, int count);
int _gl_ext_load_time_iv_selected_list(app_control_h service, void *data);
bool _gl_is_timeline_edit_mode(void *data);

#endif

