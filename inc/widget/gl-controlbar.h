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

#ifndef _GL_CONTROLBAR_H_
#define _GL_CONTROLBAR_H_

#include <Elementary.h>
#include "gallery.h"

#define GL_CBAR_OCCUPIED_PBAR 4
#define GL_CBAR_OCCUPIED_SEGMENT 4

/* INITIAL control mode for app */
#define GL_CTRL_TAB_INIT (-1)

typedef enum _gl_ctrl_tab_mode
{
	GL_CTRL_TAB_ALBUMS,
	GL_CTRL_TAB_TIMELINE,
	GL_CTRL_TAB_CNT,
} gl_ctrl_tab_mode;

int _gl_ctrl_show_title(void *data, int tab_mode);
int _gl_ctrl_show_view(void *data, const char *label);
Evas_Object *_gl_ctrl_add_layout(Evas_Object *parent);
Evas_Object *_gl_ctrl_add_main_layout(Evas_Object *parent);
int _gl_ctrl_sel_tab(void *data, int tab_mode);
int _gl_ctrl_set_tab_mode(void *data, int mode);
int _gl_ctrl_get_tab_mode(void *data);
int _gl_ctrl_change_easymode(void *data);
int _gl_ctrl_enable_btns(void *data, Elm_Object_Item *nf_it);
Evas_Object *_gl_ctrl_add_toolbar(Evas_Object *parent);
Elm_Object_Item *_gl_ctrl_append_item(Evas_Object *obj, const char *icon,
				      const char *label, Evas_Smart_Cb func,
				      const void *data);
int _gl_ctrl_disable_items(Elm_Object_Item *nf_it, bool b_disabled);
int _gl_ctrl_disable_items_with_check(Elm_Object_Item *nf_it, bool b_disabled,
				      char *check_str, bool b_check_disabled);
int _gl_ctrl_disable_item_with_check(Elm_Object_Item *nf_it, char *check_str,
				     bool b_disabled);
int _gl_ctrl_disable_item(Elm_Object_Item *it, Eina_Bool b_disabled);

#endif /* _GL_CONTROLBAR_H_ */

