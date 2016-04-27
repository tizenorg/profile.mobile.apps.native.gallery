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
#include <string.h>
#include <glib.h>
#include "gl-fs.h"
#include "gl-ui-util.h"
#include "gl-util.h"
#include "gl-ext-ug-load.h"
#include "gl-ext-exec.h"
#include "gl-button.h"
#include "gl-thumbs.h"
#include "gl-controlbar.h"
#include "gl-albums.h"
#include "gl-debug.h"
#include "gl-data.h"
#include "gl-thread-util.h"
#include "gl-pinchzoom.h"
#include "gl-popup.h"
#include "gl-ctxpopup.h"
#include "gl-progressbar.h"
#include "gl-albums-sel.h"
#include "gl-thumbs-edit.h"
#include "gl-thumbs-sel.h"
#include "gl-strings.h"
#include "gl-icons.h"
#include "gl-notify.h"
#include "gl-file-util.h"
#include "gl-timeline.h"

#define GL_NAVIBAR_STYLE_DEFAULT GL_CHECKBOX_STYLE_DEFAULT
#define GL_SELALL_TIMER_INT 0.3

int _gl_ui_hide_back_button(Elm_Object_Item *nf_it)
{
	GL_CHECK_VAL(nf_it, -1);
	elm_object_item_part_content_set(nf_it, GL_NAVIFRAME_PREV_BTN, NULL);
	return 0;
}

static void __gl_ui_show_title_toolbar(void *data, Evas_Object *obj, void *ei)
{
	GL_CHECK(data);
	gl_appdata *ad = (gl_appdata *)data;
	ad->entryinfo.b_hide_sip = true;
	Evas_Object *nf = ad->maininfo.naviframe;
	Elm_Object_Item *navi_it = elm_naviframe_top_item_get(nf);
	GL_CHECK(navi_it);
	switch (ad->maininfo.rotate_mode) {
	case APP_DEVICE_ORIENTATION_90:
	case APP_DEVICE_ORIENTATION_270:
		elm_naviframe_item_title_enabled_set(navi_it, EINA_FALSE, EINA_FALSE);
		break;
	default:
		break;
	}
}

static void __gl_ui_hide_title_toolbar(void *data, Evas_Object *obj, void *ei)
{
	GL_CHECK(data);
	gl_appdata *ad = (gl_appdata *)data;
	ad->entryinfo.b_hide_sip = false;
	Evas_Object *nf = ad->maininfo.naviframe;
	Elm_Object_Item *navi_it = elm_naviframe_top_item_get(nf);
	GL_CHECK(navi_it);
	elm_naviframe_item_title_enabled_set(navi_it, EINA_TRUE, EINA_FALSE);
}

int _gl_ui_clear_btns(void *data)
{
	GL_CHECK_VAL(data, -1);
	Evas_Object *btn = NULL;
	gl_dbg("");

	/* Remove more */
	btn = _gl_ui_get_btn(data, NULL, GL_NAVIFRAME_MORE);
	GL_IF_DEL_OBJ(btn);
	/* Remove TOOLBAR */
	btn = _gl_ui_get_btn(data, NULL, GL_NAVIFRAME_TOOLBAR);
	GL_IF_DEL_OBJ(btn);
	btn = _gl_ui_get_btn(data, NULL, GL_NAVIFRAME_TITLE_RIGHT_BTN);
	GL_IF_DEL_OBJ(btn);
	btn = _gl_ui_get_btn(data, NULL, GL_NAVIFRAME_TITLE_LEFT_BTN);
	GL_IF_DEL_OBJ(btn);
	return 0;
}

Evas_Object *gl_ui_load_edj(Evas_Object *parent, const char *file, const char *group)
{
	Evas_Object *eo = NULL;
	eo = elm_layout_add(parent);
	GL_CHECK_NULL(eo);

	int r = 0;
	r = elm_layout_file_set(eo, file, group);
	if (!r) {
		evas_object_del(eo);
		return NULL;
	}

	evas_object_size_hint_weight_set(eo, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	evas_object_size_hint_align_set(eo, EVAS_HINT_FILL, EVAS_HINT_FILL);
	return eo;
}

int _gl_ui_rm_selall_btn(Elm_Object_Item *nf_it)
{
	GL_CHECK_VAL(nf_it, -1);
	Evas_Object *btn = NULL;
	btn = elm_object_item_part_content_get(nf_it,
	                                       GL_NAVIFRAME_TITLE_RIGHT_BTN);
	GL_IF_DEL_OBJ(btn);
	return 0;
}

int _gl_ui_add_selall_btn(Evas_Object *naviframe, Elm_Object_Item *nf_it,
                          Evas_Smart_Cb cb_func, void *data)
{
	GL_CHECK_VAL(data, -1);
	GL_CHECK_VAL(naviframe, -1);
	gl_appdata *ad = (gl_appdata *)data;
	Elm_Object_Item *_nf_it = nf_it;

	if (_nf_it == NULL) {
		_nf_it = elm_naviframe_top_item_get(naviframe);
		GL_CHECK_VAL(_nf_it, -1);
	}
	Evas_Object *sel_all_btn = NULL;
	sel_all_btn = _gl_but_create_image_but(naviframe,
	                                       GL_ICON_TITLE_SELECT_ALL, NULL,
	                                       GL_BUTTON_STYLE_NAVI_TITLE_ICON,
	                                       cb_func, data, EINA_FALSE);
	GL_CHECK_VAL(sel_all_btn, -1);
	elm_object_item_part_content_set(_nf_it, GL_NAVIFRAME_TITLE_RIGHT_BTN,
	                                 sel_all_btn);
	ad->selinfo.ck_state = false;
	return 0;
}

#if 0
int _gl_ui_add_xpt_btns(Elm_Object_Item *nf_it, gl_ui_xpt_btn_e xpt_btn,
                        Evas_Smart_Cb r_cb, Evas_Smart_Cb l_cb, void *data)
{
	/* Get expanded title */
	Evas_Object *title = NULL;
	title = elm_object_item_part_content_get(nf_it, "title");
	GL_CHECK_VAL(title, -1);
	/* Camera */
	if (xpt_btn & GL_UI_XPT_CAMERA) {
		/* New-album , add left before right for customized TTS */
		if (xpt_btn & GL_UI_XPT_SHARE)
			_gl_expn_title_add_left_btn(title, GL_ICON_SHARE,
			                            _gl_str(GL_STR_ID_NEW_ALBUM),
			                            l_cb, data);
		_gl_expn_title_add_right_btn(title, GL_ICON_CAMERA,
		                             _gl_str(GL_STR_ID_CAMERA), r_cb,
		                             data);
		/* Select-all */
	} else if (xpt_btn & GL_UI_XPT_SEL_ALL) {
		if (xpt_btn & GL_UI_XPT_DONE)
			_gl_expn_title_add_left_btn(title, GL_ICON_DONE,
			                            _gl_str(GL_STR_ID_DONE),
			                            l_cb, data);
		_gl_expn_title_add_right_btn(title, GL_ICON_TITLE_SELECT_ALL,
		                             GL_STR_SELECT_ALL, r_cb, data);
	} else {
		gl_dbgE("Wrong mode!");
	}
	_gl_expn_title_set_tts(title);
	return 0;
}

Evas_Object *_gl_ui_get_xpt_btn(Elm_Object_Item *nf_it, gl_ui_xpt_btn_e ui_btn)
{
	/* Get expanded title */
	Evas_Object *title = NULL;
	title = elm_object_item_part_content_get(nf_it, "title");
	GL_CHECK_NULL(title);
	gl_xtp_btn_e xpt_btn = GL_XPT_BTN_RIGHT;
	if (ui_btn & GL_UI_XPT_SHARE) {
		xpt_btn = GL_XPT_BTN_LEFT;
	}
	return _gl_expn_title_get_btn(title, xpt_btn);
}

int _gl_ui_disable_xpt_btn(Elm_Object_Item *nf_it, gl_ui_xpt_btn_e ui_btn, bool b_disabled)
{
	Evas_Object *btn = NULL;
	btn = _gl_ui_get_xpt_btn(nf_it, ui_btn);
	if (btn) {
		elm_object_disabled_set(btn, b_disabled);
	} else {
		gl_dbgE("Failed to get xpt button!");
	}
	return 0;
}

int _gl_ui_clear_xpt_btns(Elm_Object_Item *nf_it)
{
	/* Get expanded title */
	Evas_Object *title = NULL;
	title = elm_object_item_part_content_get(nf_it, "title");
	GL_CHECK_VAL(title, -1);
	_gl_expn_title_clear_btns(title);
	return 0;
}
#endif

Evas_Object *_gl_ui_create_gridview_ly(void *data, Evas_Object *parent)
{
	GL_CHECK_NULL(parent);
	GL_CHECK_NULL(data);
	gl_appdata *ad = (gl_appdata *)data;
	gl_dbg("");

	Evas_Object *layout = NULL;
	char *path = _gl_get_edje_path();
	GL_CHECK_NULL(path);
	layout = gl_ui_load_edj(ad->maininfo.naviframe, path,
	                        GL_GRP_GRIDVIEW);
	free(path);
	evas_object_show(layout);

	return layout;
}

gl_icon_type_e _gl_ui_elm_bg_file_set(Evas_Object *bg, gl_item *git)
{
	if (git == NULL || git->item == NULL) {
		gl_dbgE("Invalid item :%p", git);
		elm_bg_file_set(bg, GL_ICON_CONTENTS_BROKEN, NULL);
		return GL_ICON_CORRUPTED_FILE;
	}

	elm_bg_file_set(bg, _GET_ICON(git->item->thumb_url), NULL);
	return GL_ICON_NORMAL;
}

int _gl_ui_set_content(Evas_Object *layout, const char *part,
                       Evas_Object *new_content)
{
	GL_CHECK_VAL(layout, -1);
	GL_CHECK_VAL(new_content, -1);
	Evas_Object *old_content = NULL;

	old_content = elm_object_part_content_get(layout, part);
	if (old_content && old_content == new_content) {
		gl_dbg("Already set");
		return 0;
	}

	gl_dbg("Set content");
	elm_object_part_content_set(layout, part, new_content);

	return 0;
}

int _gl_ui_set_toolbar_state(void *data, bool b_hide)
{
	GL_CHECK_VAL(data, -1);
	gl_appdata *ad = (gl_appdata *)data;
	GL_CHECK_VAL(ad->maininfo.naviframe, -1);
	Elm_Object_Item *navi_it = elm_naviframe_top_item_get(ad->maininfo.naviframe);
	GL_CHECK_VAL(navi_it, -1);
	if (b_hide)
		elm_object_item_signal_emit(navi_it, "elm,state,toolbar,instant_close",
		                            "");
	else
		elm_object_item_signal_emit(navi_it, "elm,state,toolbar,instant_open",
		                            "");
	return 0;

}

bool _gl_ui_hide_keypad(void *data)
{
	GL_CHECK_VAL(data, -1);
	gl_appdata *ad = (gl_appdata *)data;
	if (ad->entryinfo.b_hide_sip) {
		if (ad->entryinfo.context) {
			ad->entryinfo.b_hide_sip = false;
			return true;
		}
	}
	return false;

}

int _gl_ui_add_conform_cbs(void *data)
{
	GL_CHECK_VAL(data, -1);

	gl_appdata *ad = (gl_appdata *)data;
	Evas_Object *naviframe = ad->maininfo.naviframe;
	Evas_Object *conform = NULL;

	/* Get conformant */
	conform = (Evas_Object *)evas_object_data_get(naviframe,
	          GL_NAVIFRAME_OBJ_DATA_KEY);
	GL_CHECK_VAL(conform, -1);

	evas_object_smart_callback_add(conform, GL_IMF_ON,
	                               __gl_ui_show_title_toolbar, data);
	evas_object_smart_callback_add(conform, GL_IMF_OFF,
	                               __gl_ui_hide_title_toolbar, data);
	evas_object_smart_callback_add(conform, GL_STA_ON,
	                               __gl_ui_show_title_toolbar, data);
	evas_object_smart_callback_add(conform, GL_STA_OFF,
	                               __gl_ui_hide_title_toolbar, data);
	return 0;
}

int _gl_ui_del_conform_cbs(Evas_Object *naviframe)
{
	GL_CHECK_VAL(naviframe, -1);
	Evas_Object *conform = NULL;

	/* Get conformant */
	conform = (Evas_Object *)evas_object_data_get(naviframe,
	          GL_NAVIFRAME_OBJ_DATA_KEY);
	GL_CHECK_VAL(conform, -1);

	evas_object_smart_callback_del(conform, GL_IMF_ON,
	                               __gl_ui_show_title_toolbar);
	evas_object_smart_callback_del(conform, GL_IMF_OFF,
	                               __gl_ui_hide_title_toolbar);
	evas_object_smart_callback_del(conform, GL_STA_ON,
	                               __gl_ui_show_title_toolbar);
	evas_object_smart_callback_del(conform, GL_STA_OFF,
	                               __gl_ui_hide_title_toolbar);
	return 0;
}


int _gl_ui_set_conform_overlap(Evas_Object *naviframe)
{
	GL_CHECK_VAL(naviframe, -1);
	Evas_Object *conform = NULL;

	/* Get conformant */
	conform = (Evas_Object *)evas_object_data_get(naviframe,
	          GL_NAVIFRAME_OBJ_DATA_KEY);
	GL_CHECK_VAL(conform, -1);
	/* Modify to start content from 0,0 */
	elm_object_signal_emit(conform, "elm,state,indicator,overlap", "");
	evas_object_data_set(conform, "overlap", (void *)EINA_TRUE);
	return 0;
}

int _gl_ui_disable_btn(Evas_Object *btn)
{
	if (btn) {
		elm_object_disabled_set(btn, EINA_TRUE);
	}
	return 0;
}

int _gl_ui_enable_btn(Evas_Object *btn)
{
	if (btn) {
		elm_object_disabled_set(btn, EINA_FALSE);
	}
	return 0;
}

/* Set button more */
int _gl_ui_disable_menu(Elm_Object_Item *nf_it, bool b_disabled)
{
	GL_CHECK_VAL(nf_it, -1);
	Evas_Object *btn = NULL;
	gl_dbg("more: %d", b_disabled);
	btn = elm_object_item_part_content_get(nf_it, GL_NAVIFRAME_MORE);
	if (btn) {
		elm_object_disabled_set(btn, b_disabled);
	}
	return 0;
}

/* Get button*/
Evas_Object *_gl_ui_get_btn(void *data, Elm_Object_Item *nf_it,
                            const char *part)
{
	Elm_Object_Item *_nf_it = nf_it;

	if (_nf_it == NULL) {
		GL_CHECK_NULL(data);
		gl_appdata *ad = (gl_appdata *)data;
		_nf_it = elm_naviframe_top_item_get(ad->maininfo.naviframe);
		GL_CHECK_NULL(_nf_it);
	}

	return elm_object_item_part_content_get(_nf_it, part);
}

/* Update all realized items's checkbox */
int _gl_ui_update_realized_list_ck(Evas_Object *view, const char *part,
                                   Eina_Bool state)
{
	GL_CHECK_VAL(part, -1);
	GL_CHECK_VAL(view, -1);
	Eina_List *list = NULL;
	Eina_List *l = NULL;
	Elm_Object_Item *it = NULL;

	list = elm_genlist_realized_items_get(view);
	GL_CHECK_VAL(list, -1);

	EINA_LIST_FOREACH(list, l, it) {
		if (it == NULL) {
			gl_dbgE("Invalid item!");
			continue;
		}
		_gl_ui_set_checkbox_state(it, part, state);
	}
	return 0;
}

int _gl_ui_update_realized_grid_ck(Evas_Object *view, const char *part1,
                                   const char *part2, Eina_Bool state)
{
	GL_CHECK_VAL(view, -1);
	Eina_List *grid = NULL;
	Eina_List *l = NULL;
	Elm_Object_Item *it = NULL;

	grid = elm_gengrid_realized_items_get(view);
	GL_CHECK_VAL(grid, -1);

	EINA_LIST_FOREACH(grid, l, it) {
		if (it == NULL) {
			gl_dbgE("Invalid item!");
			continue;
		}
		if (part1) {
			_gl_ui_set_checkbox_state(it, part1, state);
		}
		if (part2) {
			_gl_ui_set_checkbox_state(it, part2, state);
		}
	}
	return 0;
}

int _gl_ui_set_checkbox_state(Elm_Object_Item *it, const char *part,
                              Eina_Bool state)
{
	GL_CHECK_VAL(part, -1);
	GL_CHECK_VAL(it, -1);
	Evas_Object *ck = NULL;

	ck = elm_object_item_part_content_get(it, part);
	if (ck == NULL) {
		gl_dbgE("Invalid checkbox!");
		return -1;
	}

	if (elm_check_state_get(ck) != state) {
		elm_check_state_set(ck, state);
	}
	return 0;
}

/* Update all realized items's field */
int _gl_ui_update_realized_list_field(Evas_Object *view, const char *part)
{
	GL_CHECK_VAL(part, -1);
	GL_CHECK_VAL(view, -1);
	Eina_List *list = NULL;
	Eina_List *l = NULL;
	Elm_Object_Item *it = NULL;

	list = elm_genlist_realized_items_get(view);
	GL_CHECK_VAL(list, -1);

	EINA_LIST_FOREACH(list, l, it) {
		if (_gl_ui_update_list_field(it, part) < 0) {
			return -1;
		}
	}
	return 0;
}

int _gl_ui_update_list_field(Elm_Object_Item *it, const char *part)
{
	GL_CHECK_VAL(part, -1);
	GL_CHECK_VAL(it, -1);
	elm_genlist_item_fields_update(it, part,
	                               ELM_GENLIST_ITEM_FIELD_CONTENT);
	return 0;
}

int _gl_ui_reset_scroller_pos(Evas_Object *obj)
{
	GL_CHECK_VAL(obj, -1);
	evas_object_data_set(obj, "prev_scroller_x", (void *)0);
	evas_object_data_set(obj, "prev_scroller_y", (void *)0);
	evas_object_data_set(obj, "prev_scroller_w", (void *)0);
	evas_object_data_set(obj, "prev_scroller_h", (void *)0);
	return 0;
}

int _gl_ui_del_scroller_pos(Evas_Object *obj)
{
	GL_CHECK_VAL(obj, -1);
	evas_object_data_del(obj, "prev_scroller_x");
	evas_object_data_del(obj, "prev_scroller_y");
	evas_object_data_del(obj, "prev_scroller_w");
	evas_object_data_del(obj, "prev_scroller_h");
	return 0;
}

int _gl_ui_save_scroller_pos(Evas_Object *obj)
{
	GL_CHECK_VAL(obj, -1);
	Evas_Coord x = 0;
	Evas_Coord y = 0;
	Evas_Coord w = 0;
	Evas_Coord h = 0;

	elm_scroller_region_get(obj, &x, &y, &w, &h);
	gl_dbg("(%dx%d), (%dx%d)", x, y, w, h);
	evas_object_data_set(obj, "prev_scroller_x", (void *)x);
	evas_object_data_set(obj, "prev_scroller_y", (void *)y);
	evas_object_data_set(obj, "prev_scroller_w", (void *)w);
	evas_object_data_set(obj, "prev_scroller_h", (void *)h);
	return 0;
}

int _gl_ui_restore_scroller_pos(Evas_Object *obj)
{
	GL_CHECK_VAL(obj, -1);
	Evas_Coord x = 0;
	Evas_Coord y = 0;
	Evas_Coord w = 0;
	Evas_Coord h = 0;

	x = (Evas_Coord)evas_object_data_get(obj, "prev_scroller_x");
	y = (Evas_Coord)evas_object_data_get(obj, "prev_scroller_y");
	w = (Evas_Coord)evas_object_data_get(obj, "prev_scroller_w");
	h = (Evas_Coord)evas_object_data_get(obj, "prev_scroller_h");
	gl_dbg("(%dx%d), (%dx%d)", x, y, w, h);
	if (w > 0 && h > 0) {
		elm_scroller_region_show(obj, x, y, w, h);
	}
	return 0;
}

int _gl_ui_rotate_view(void *data, int view_mode)
{
	GL_CHECK_VAL(data, -1);
	gl_dbg("view_mode: %d", view_mode);

	switch (view_mode) {
	case GL_VIEW_ALBUMS:
	case GL_VIEW_ALBUMS_EDIT:
	case GL_VIEW_ALBUM_REORDER:
		_gl_albums_rotate_view(data);
		break;
	case GL_VIEW_ALBUMS_SELECT:
		_gl_albums_sel_rotate_view(data);
		break;
	case GL_VIEW_THUMBS:
	case GL_VIEW_THUMBS_EDIT:
	case GL_VIEW_THUMBS_SELECT:
		_gl_thumbs_rotate_view(data);
		break;
	case GL_VIEW_TIMELINE:
		_gl_timeline_view_rotate(data);
		break;
	default:
		break;
	}
	return 0;
}

/* Delete naviframe items from top one to @to_it */
int _gl_ui_del_nf_items(Evas_Object *nf, Elm_Object_Item *to_it)
{
	GL_CHECK_VAL(nf, -1);
	Eina_List *nf_items = NULL;
	Eina_List *l = NULL;
	Elm_Object_Item *it = NULL;

	nf_items = elm_naviframe_items_get(nf);
	EINA_LIST_REVERSE_FOREACH(nf_items, l, it) {
		if (it != to_it) {
			gl_dbg("Delete one item!");
			elm_object_item_del(it);
			it = NULL;
		} else {
			break;
		}
	}
	eina_list_free(nf_items);
	return 0;
}

int _gl_ui_set_translate_str(Evas_Object *obj, const char *str)
{
	GL_CHECK_VAL(str, -1);
	GL_CHECK_VAL(obj, -1);
	char *domain = GL_STR_DOMAIN_LOCAL;
	elm_object_domain_translatable_text_set(obj, domain, str);
	return 0;
}

int _gl_ui_set_translatable_item(Elm_Object_Item *nf_it, const char *str)
{
	GL_CHECK_VAL(str, -1);
	GL_CHECK_VAL(nf_it, -1);
	char *domain = GL_STR_DOMAIN_LOCAL;
	elm_object_item_domain_text_translatable_set(nf_it, domain, EINA_TRUE);
	return 0;
}

int _gl_ui_set_translatable_item_part(Elm_Object_Item *nf_it, const char *part,
                                      const char *str)
{
	GL_CHECK_VAL(str, -1);
	GL_CHECK_VAL(nf_it, -1);
	char *domain = GL_STR_DOMAIN_LOCAL;
	elm_object_item_domain_part_text_translatable_set(nf_it, part, domain,
	        EINA_TRUE);
	return 0;
}

int _gl_ui_set_translate_part_str(Evas_Object *obj, const char *part,
                                  const char *str)
{
	GL_CHECK_VAL(str, -1);
	GL_CHECK_VAL(part, -1);
	GL_CHECK_VAL(obj, -1);

	char *domain = GL_STR_DOMAIN_LOCAL;
	elm_object_domain_translatable_part_text_set(obj, part, domain, str);
	return 0;
}

/* Change naviframe item title text */
int _gl_ui_change_title(Elm_Object_Item *nf_it, const char *text)
{
	GL_CHECK_VAL(text, -1);
	GL_CHECK_VAL(nf_it, -1);
	elm_object_item_text_set(nf_it, text);
	_gl_ui_set_translatable_item(nf_it, text);
	return 0;
}

/* Update the label text for selected item showed in naviframe title  */
int _gl_ui_update_label_text(Elm_Object_Item *nf_it, int sel_cnt, bool b_lang)
{
	GL_CHECK_VAL(nf_it, -1);
	gl_dbg("Count: %d, lang: %d", sel_cnt, b_lang);
	char *pd_selected = GL_STR_PD_SELECTED;

	/* Update the label text */
	if (sel_cnt > 0) {
		char *text = NULL;
		text = g_strdup_printf(_gl_str(pd_selected), sel_cnt);
		elm_object_item_text_set(nf_it, text);
		GL_GFREEIF(text);
	} else if (!b_lang) {
		/* Don't need to update text if it's called by language_changed_cb*/
		elm_object_item_text_set(nf_it, GL_STR_ID_SELECT_ITEM);
		_gl_ui_set_translatable_item(nf_it, GL_STR_ID_SELECT_ITEM);
	}
	return 0;
}

/* Change naviframe item title text */
int _gl_ui_change_navi_title(Elm_Object_Item *nf_it, char *text, bool b_dropdown)
{
	GL_CHECK_VAL(text, -1);
	GL_CHECK_VAL(nf_it, -1);

	elm_object_item_part_text_set(nf_it, "elm.text.title", _gl_str(text));
	return 0;
}

/* Update the label text for selected item showed in naviframe title  */
int _gl_ui_update_navi_title_text(Elm_Object_Item *nf_it, int sel_cnt, bool b_lang)
{
	GL_CHECK_VAL(nf_it, -1);
	gl_dbg("Count: %d, lang: %d", sel_cnt, b_lang);
	char *pd_selected = GL_STR_PD_SELECTED;

	Evas_Object *btn = elm_object_item_part_content_get(nf_it, GL_NAVIFRAME_TITLE_RIGHT_BTN);

	if (btn) {
		if (sel_cnt > 0) {
			elm_object_disabled_set(btn, EINA_FALSE);
		} else {
			elm_object_disabled_set(btn, EINA_TRUE);
		}
	}

	/* Update the label text */
	if (sel_cnt > 0) {
		char *text = NULL;
		text = g_strdup_printf(_gl_str(pd_selected), sel_cnt);
		elm_object_item_part_text_set(nf_it, "elm.text.title", _gl_str(text));
		gl_dbg("Text is : %s", text);
		GL_GFREEIF(text);
	} else if (!b_lang) {
		/* Don't need to update text if it's called by language_changed_cb*/
		gl_dbg("Text is : %s", GL_STR_ID_SELECT_ITEM);
		elm_object_item_part_text_set(nf_it, "elm.text.title", _gl_str(GL_STR_ID_SELECT_ITEM));
	}
	return 0;
}

int _gl_ui_add_selall_ck(Evas_Object *parent, char *part, char *part_btn,
                         Evas_Smart_Cb cb_func, void *data)
{
	GL_CHECK_VAL(data, -1);
	GL_CHECK_VAL(parent, -1);
	gl_appdata *ad = (gl_appdata *)data;
	Evas_Object *sel_all_btn = NULL;
	Evas_Object *sel_all_ck = NULL;

	_gl_ui_set_translate_part_str(parent, "select.all.area.label", GL_STR_ID_SELECT_ALL);
	if (part) {
		sel_all_ck = elm_check_add(parent);
		GL_CHECK_VAL(sel_all_ck, -1);
		elm_check_state_set(sel_all_ck, EINA_FALSE);
		elm_object_part_content_set(parent, "select.all.area.check", sel_all_ck);
		ad->selinfo.selectall_ck = sel_all_ck;
		if (part_btn) {
			sel_all_btn = elm_button_add(parent);
			GL_CHECK_VAL(sel_all_btn, -1);
			elm_object_part_content_set(parent, "select.all.area.check.fg", sel_all_btn);
			elm_object_style_set(sel_all_btn, "transparent");
			evas_object_smart_callback_add(sel_all_btn, "clicked", cb_func, data);
		}
	}
	return 0;
}

#ifdef _USE_GRID_CHECK
int _gl_show_grid_checks(Evas_Object *view, const char *ck_part)
{
	GL_CHECK_VAL(ck_part, -1);
	GL_CHECK_VAL(view, -1);

	Elm_Object_Item *first_it = NULL;
	Elm_Object_Item *next_it = NULL;
	Elm_Object_Item *last_it = NULL;
	first_it = elm_gengrid_first_item_get(view);
	last_it = elm_gengrid_last_item_get(view);
	if (first_it) {
		elm_gengrid_item_update(first_it);
	}
	while (first_it) {
		next_it = elm_gengrid_item_next_get(first_it);
		if (next_it == NULL) {
			gl_dbgE("Invalid item! Break!");
			break;
		}
		elm_gengrid_item_update(next_it);
		first_it = next_it;
		if (last_it == first_it) {
			gl_dbg("done!");
			break;
		}
	}
	return 0;
}

int _gl_show_grid_checks_dual(Evas_Object *view, const char *ck_part, const char *ck_part2)
{
	GL_CHECK_VAL(ck_part, -1);
	GL_CHECK_VAL(view, -1);

	Elm_Object_Item *first_it = NULL;
	Elm_Object_Item *next_it = NULL;
	Elm_Object_Item *last_it = NULL;
	first_it = elm_gengrid_first_item_get(view);
	last_it = elm_gengrid_last_item_get(view);
	if (first_it) {
		elm_gengrid_item_update(first_it);
	}
	while (first_it) {
		next_it = elm_gengrid_item_next_get(first_it);
		if (next_it == NULL) {
			gl_dbgE("Invalid item! Break!");
			break;
		}
		elm_gengrid_item_update(next_it);
		first_it = next_it;
		if (last_it == first_it) {
			gl_dbg("done!");
			break;
		}
	}
	return 0;
}
#endif

