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

#ifndef _GL_UI_UTIL_H_
#define _GL_UI_UTIL_H_

#include "gallery.h"
#include "gl-icons.h"

#ifdef __cplusplus
extern "C"
{
#endif

#define _EDJ(o)         elm_layout_edje_get(o)
#define _GET_ICON(_path) \
		((1 == gl_file_exists(_path) && (gl_file_size(_path) > 0)) ? _path : GL_ICON_NO_THUMBNAIL)
#define _GET_APP_ICON(_path) \
		((1 == gl_file_exists(_path) && (gl_file_size(_path) > 0)) ? _path : GL_ICON_DEFAULT)

/* Signals used in Gallery's views */
#define GL_SIGNAL_GESTURE_DEFAULT "elm,gesture,state,default"
#define GL_SIGNAL_GESTURE_DISABLED "elm,gesture,state,disabled"
#define GL_TRANS_FINISHED "transition,finished"

typedef enum
{
	GL_NAVI_ALBUMS,
	GL_NAVI_ALBUMS_EDIT,
	GL_NAVI_ALBUMS_RENAME,
	GL_NAVI_ALBUMS_NEW,
	GL_NAVI_ALBUMS_SELECT,
	GL_NAVI_PLACES,
	GL_NAVI_TAGS,
	GL_NAVI_TAGS_EDIT,
	GL_NAVI_TAGS_NEW,
	GL_NAVI_TAGS_RENAME,
	GL_NAVI_THUMBS,
	GL_NAVI_THUMBS_EDIT,
	GL_NAVI_THUMBS_SELECT,
	GL_NAVI_WEB,
	GL_NAVI_WEB_EDIT,
} gl_navi_mode;

typedef enum
{
	GL_POPUP_ADDHOME_SIZE_1,
	GL_POPUP_ADDHOME_SIZE_2,
	GL_POPUP_ADDHOME_SIZE_NUM,
} gl_popup_addhome_size;

typedef enum _gl_ui_xpt_btn_t gl_ui_xpt_btn_e;

enum _gl_ui_xpt_btn_t {
	GL_UI_XPT_CAMERA = (0x1 << 0),
	GL_UI_XPT_SEL_ALL = (0x1 << 1),
	GL_UI_XPT_SHARE = (0x1 << 2),
	GL_UI_XPT_DONE = (0x1 << 3),
};

Evas_Object *_gl_ui_create_gridview_ly(void *data, Evas_Object *parent);
Evas_Object *gl_ui_load_edj(Evas_Object * parent, const char *file, const char *group);
int _gl_ui_rm_selall_btn(Elm_Object_Item *nf_it);
int _gl_ui_add_selall_btn(Evas_Object *naviframe, Elm_Object_Item *nf_it,
			  Evas_Smart_Cb cb_func, void *data);
#if 0
int _gl_ui_add_xpt_btns(Elm_Object_Item *nf_it, gl_ui_xpt_btn_e xpt_btn,
			Evas_Smart_Cb r_cb, Evas_Smart_Cb l_cb, void *data);
Evas_Object *_gl_ui_get_xpt_btn(Elm_Object_Item *nf_it, gl_ui_xpt_btn_e ui_btn);
int _gl_ui_disable_xpt_btn(Elm_Object_Item *nf_it, gl_ui_xpt_btn_e ui_btn, bool b_disabled);
int _gl_ui_clear_xpt_btns(Elm_Object_Item *nf_it);
#endif
int _gl_ui_clear_btns(void *data);
int _gl_ui_hide_back_button(Elm_Object_Item *nf_it);
gl_icon_type_e _gl_ui_elm_bg_file_set(Evas_Object *bg, gl_item *git);
int _gl_ui_set_content(Evas_Object *layout, const char *part,
		       Evas_Object *new_content);
int _gl_ui_add_conform_cbs(void *data);
int _gl_ui_del_conform_cbs(Evas_Object *naviframe);
int _gl_ui_set_conform_overlap(Evas_Object *naviframe);
int _gl_ui_disable_btn(Evas_Object *btn);
int _gl_ui_enable_btn(Evas_Object *btn);
int _gl_ui_disable_menu(Elm_Object_Item *nf_it, bool b_disabled);
Evas_Object *_gl_ui_get_btn(void *data, Elm_Object_Item *nf_it,
			    const char *part);
int _gl_ui_update_realized_list_ck(Evas_Object *view, const char *part,
				   Eina_Bool state);
int _gl_ui_update_realized_grid_ck(Evas_Object *view, const char *part1,
				   const char *part2, Eina_Bool state);
int _gl_ui_set_checkbox_state(Elm_Object_Item *it, const char *part,
			      Eina_Bool state);
int _gl_ui_update_realized_list_field(Evas_Object *view, const char *part);
int _gl_ui_update_list_field(Elm_Object_Item *it, const char *part);
int _gl_ui_reset_scroller_pos(Evas_Object *obj);
int _gl_ui_del_scroller_pos(Evas_Object *obj);
int _gl_ui_save_scroller_pos(Evas_Object *obj);
int _gl_ui_restore_scroller_pos(Evas_Object *obj);
int _gl_ui_rotate_view(void *data, int view_mode);
int _gl_ui_set_toolbar_state(void *data, bool b_hide);
bool _gl_ui_hide_keypad(void *data);
int _gl_ui_del_nf_items(Evas_Object *nf, Elm_Object_Item *to_it);
int _gl_ui_set_translate_str(Evas_Object *obj, const char *str);
int _gl_ui_set_translatable_item(Elm_Object_Item *nf_it,  const char *str);
int _gl_ui_set_translatable_item_part(Elm_Object_Item *nf_it, const char *part,
				      const char *str);
int _gl_ui_set_translate_part_str(Evas_Object *obj, const char *part,
				  const char *str);
int _gl_ui_change_title(Elm_Object_Item *nf_it, const char *text);
int _gl_ui_update_label_text(Elm_Object_Item *nf_it, int sel_cnt, bool b_lang);
int _gl_ui_change_navi_title(Elm_Object_Item *nf_it, char *text, bool b_dropdown);
int _gl_ui_update_navi_title_text(Elm_Object_Item *nf_it, int sel_cnt, bool b_lang);
int _gl_ui_add_selall_ck(Evas_Object *parent, char *part, char *part_btn,
			  Evas_Smart_Cb cb_func, void *data);
#ifdef _USE_GRID_CHECK
int _gl_show_grid_checks(Evas_Object *view, const char *ck_part);
int _gl_show_grid_checks_dual(Evas_Object *view, const char *ck_part, const char *ck_part2);
#endif

#ifdef __cplusplus
}
#endif

#endif // end of _GL_UI_UTIL_H_
