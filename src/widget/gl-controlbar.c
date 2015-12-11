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

#include "gl-debug.h"
#include "gl-ui-util.h"
#include "gl-util.h"
#include "gl-controlbar.h"
#include "gl-albums.h"
#include "gl-thumbs.h"
#include "gl-data.h"
#include "gl-strings.h"
#include "gl-icons.h"
#include "gl-progressbar.h"
#include "gl-ctxpopup.h"
#include "gl-thumbs-edit.h"
#include "gl-timeline.h"
#include "gl-albums-edit.h"

static int __gl_ctrl_reset_previous_tab(void *data)
{
	GL_CHECK_VAL(data, -1);
	gl_appdata *ad = (gl_appdata *)data;
	int tab_mode = _gl_ctrl_get_tab_mode(ad);

	switch (tab_mode) {
	case GL_CTRL_TAB_ALBUMS:
		_gl_albums_hide_view_tab(data);
		break;
	case GL_CTRL_TAB_TIMELINE:
		_gl_timeline_hide_view(data);
		break;
	default:
		gl_dbgW("Wrong tabbar mode!");
		return -1;
	}
	/* Hide previous view */
	Evas_Object *pre_view = NULL;
	pre_view = elm_object_part_content_unset(ad->ctrlinfo.ctrlbar_view_ly,
	           "elm.swallow.view");
	evas_object_hide(pre_view);
	return 0;
}

int _gl_ctrl_show_view(void *data, const char *label)
{
	GL_PROFILE_IN;
	GL_CHECK_VAL(data, -1);
	gl_appdata *ad = (gl_appdata *)data;
	int tab_mode = _gl_ctrl_get_tab_mode(ad);
	gl_dbg("tab_mode: %d, label: %s", tab_mode, label);

	if (!g_strcmp0(label, GL_STR_ALBUMS)) {
		gl_dbg("Albums");
		if (tab_mode == GL_CTRL_TAB_ALBUMS) {
			gl_dbg("Already in Albums Tab.");
			return -1;
		}
		__gl_ctrl_reset_previous_tab(ad);
		_gl_ctrl_set_tab_mode(ad, GL_CTRL_TAB_ALBUMS);
		_gl_albums_show_view_tab(data, ad->ctrlinfo.ctrlbar_view_ly);
	} else if (!g_strcmp0(label, "Timeline"/*_gl_str(GL_STR_ID_TIME_LINE)*/)) {
//	:TODO Timeline need to be addressed
		gl_dbg(" Timeline");
		if (tab_mode == GL_CTRL_TAB_TIMELINE) {
			gl_dbg("Already in Timeline Tab.");
			return -1;
		}

		__gl_ctrl_reset_previous_tab(ad);
		_gl_ctrl_set_tab_mode(ad, GL_CTRL_TAB_TIMELINE);
		_gl_timeline_create_view(data, ad->ctrlinfo.ctrlbar_view_ly);
	}
	GL_PROFILE_OUT;
	return 0;
}

/* Enable all buttons except for some special cases */
static int __gl_ctrl_enable_btns(gl_appdata *ad, Elm_Object_Item *nf_it)
{
	gl_dbg("");
	GL_CHECK_VAL(ad, -1);
	GL_CHECK_VAL(nf_it, -1);
	int view_mode = gl_get_view_mode(ad);

	if (view_mode == GL_VIEW_THUMBS_EDIT) {
		_gl_thumbs_edit_disable_btns(ad, false);
	} else if (view_mode == GL_VIEW_ALBUMS_EDIT) {
		_gl_albums_edit_disable_btns(ad, false);
	} else {
		_gl_ui_disable_menu(nf_it, false);
		_gl_ctrl_disable_items(nf_it, false);
	}
	return 0;
}

Evas_Object *_gl_ctrl_add_layout(Evas_Object *parent)
{
	GL_CHECK_NULL(parent);
	Evas_Object *layout = NULL;
	char *path = _gl_get_edje_path();
	layout = gl_ui_load_edj(parent, path, GL_GRP_CONTROLBAR);
	free(path);
	GL_CHECK_NULL(layout);
	evas_object_show(layout);
	return layout;
}

Evas_Object *_gl_ctrl_add_main_layout(Evas_Object *parent)
{
	GL_PROFILE_IN;
	GL_CHECK_NULL(parent);

	Evas_Object *layout = elm_layout_add(parent);
	GL_CHECK_NULL(layout);
	evas_object_size_hint_weight_set(layout, EVAS_HINT_EXPAND,
	                                 EVAS_HINT_EXPAND);
	evas_object_size_hint_align_set(layout, EVAS_HINT_FILL, EVAS_HINT_FILL);
	elm_layout_theme_set(layout, "layout", "application", "default");
	evas_object_show(layout);

	GL_PROFILE_OUT;
	return layout;
}

int _gl_ctrl_show_title(void *data, int tab_mode)
{
	GL_PROFILE_IN;
	GL_CHECK_VAL(data, -1);
	gl_appdata *ad = (gl_appdata *)data;
	GL_CHECK_VAL(ad->ctrlinfo.ctrlbar_ly, -1);
	char *text = NULL;
	gl_dbg("tab_mode: %d", tab_mode);

	switch (tab_mode) {
	case GL_CTRL_TAB_ALBUMS:
		text = GL_STR_ID_ALBUMS;
		break;
	case GL_CTRL_TAB_TIMELINE:
		// :TODO Timeline token need to be addressed
		text = GL_STR_TIME_VIEW;
		break;
	default:
		gl_dbgE("Wrong mode!");
		return -1;
	}

	if (text) {
		gl_sdbg("text: %s", text);
		_gl_ui_change_navi_title(ad->ctrlinfo.nf_it, text, true);
	} else {
		gl_dbgE("Invalid text!");
		return -1;
	}
	GL_PROFILE_OUT;
	return 0;
}

int _gl_ctrl_sel_tab(void *data, int tab_mode)
{
	gl_dbg("_gl_ctrl_sel_tab");
	GL_CHECK_VAL(data, -1);
	gl_appdata *ad = (gl_appdata *)data;
	GL_CHECK_VAL(ad->ctrlinfo.ctrlbar_ly, -1);
	const char *text = NULL;

	switch (tab_mode) {
	case GL_CTRL_TAB_ALBUMS:
		text = GL_STR_ALBUMS;
		break;
	case GL_CTRL_TAB_TIMELINE:
		gl_dbg("GL_CTRL_TAB_TIMELINE");
		// :TODO Timeline token need to be addressed
		text = "Timeline";//GL_STR_TIME_LINE;//_gl_str(GL_STR_ID_TIME_LINE);
		break;
	default:
		gl_dbgE("Wrong mode!");
		return -1;
	}

	gl_dbg("text: %s[%d]", text, tab_mode);
	_gl_ctrl_show_view(data, text);
	return 0;
}

int _gl_ctrl_set_tab_mode(void *data, int mode)
{
	GL_CHECK_VAL(data, -1);
	gl_appdata *ad = (gl_appdata *)data;
	gl_dbg("mode: %d", mode);
	ad->ctrlinfo.tab_mode = mode;
	return 0;
}

int _gl_ctrl_get_tab_mode(void *data)
{
	GL_CHECK_VAL(data, -1);
	gl_appdata *ad = (gl_appdata *)data;
	return ad->ctrlinfo.tab_mode;
}

int _gl_ctrl_change_easymode(void *data)
{
	GL_CHECK_VAL(data, -1);

	/* Remove ctxpopup */
	_gl_ctxpopup_del(data);
	/* Clear old view first */
	gl_pop_to_ctrlbar_ly(data);
	/* Initialize tab view */
	__gl_ctrl_reset_previous_tab(data);
	/* Reset tab view mode to re-entrance same tab view */
	_gl_ctrl_set_tab_mode(data, GL_CTRL_TAB_INIT);
	_gl_ctrl_show_view(data, GL_STR_ALBUMS);
	return 0;
}

int _gl_ctrl_enable_btns(void *data, Elm_Object_Item *nf_it)
{
	return __gl_ctrl_enable_btns(data, nf_it);
}

Evas_Object *_gl_ctrl_add_toolbar(Evas_Object *parent)
{
	Evas_Object *toolbar = elm_toolbar_add(parent);
	GL_CHECK_NULL(toolbar);
	elm_object_style_set(toolbar, "default");
	elm_toolbar_shrink_mode_set(toolbar, ELM_TOOLBAR_SHRINK_EXPAND);
	elm_toolbar_transverse_expanded_set(toolbar, EINA_TRUE);
	elm_toolbar_select_mode_set(toolbar, ELM_OBJECT_SELECT_MODE_NONE);
	return toolbar;
}

Elm_Object_Item *_gl_ctrl_append_item(Evas_Object *obj, const char *icon,
                                      const char *label, Evas_Smart_Cb func,
                                      const void *data)
{
	Elm_Object_Item *it = NULL;
	it = elm_toolbar_item_append(obj, icon, label, func, data);
	if (label) {
		_gl_ui_set_translatable_item(it, label);
	}
	return it;
}

int _gl_ctrl_disable_items(Elm_Object_Item *nf_it, bool b_disabled)
{
	GL_CHECK_VAL(nf_it, -1);
	Evas_Object *toolbar = NULL;

	toolbar = elm_object_item_part_content_get(nf_it, "toolbar");
	GL_CHECK_VAL(toolbar, -1);

	Elm_Object_Item *it = NULL;
	Elm_Object_Item *next_it = NULL;
	char *text = NULL;
	it = elm_toolbar_first_item_get(toolbar);
	while (it) {
		next_it = elm_toolbar_item_next_get(it);
		text = (char *)elm_object_item_text_get(it);
		if (text && !g_strcmp0(text, _gl_str(GL_STR_ID_CANCEL))) {
			it = next_it;
			continue;
		}
		_gl_ctrl_disable_item(it, b_disabled);
		it = next_it;
	}
	return 0;
}

int _gl_ctrl_disable_items_with_check(Elm_Object_Item *nf_it, bool b_disabled,
                                      char *check_str, bool b_check_disabled)
{
	GL_CHECK_VAL(nf_it, -1);
	Evas_Object *toolbar = NULL;
	toolbar = elm_object_item_part_content_get(nf_it, "toolbar");
	GL_CHECK_VAL(toolbar, -1);

	Elm_Object_Item *it = NULL;
	Elm_Object_Item *next_it = NULL;
	char *text = NULL;
	it = elm_toolbar_first_item_get(toolbar);
	while (it) {
		next_it = elm_toolbar_item_next_get(it);
		text = (char *)elm_object_item_text_get(it);
		if (text && !g_strcmp0(text, _gl_str(GL_STR_ID_CANCEL))) {
			it = next_it;
			continue;
		}
		if (check_str) {
			if (text && !g_strcmp0(text, _gl_str(check_str))) {
				_gl_ctrl_disable_item(it, b_check_disabled);
			} else {
				_gl_ctrl_disable_item(it, b_disabled);
			}
		} else {
			_gl_ctrl_disable_item(it, b_disabled);
		}
		it = next_it;
	}
	return 0;
}

int _gl_ctrl_disable_item_with_check(Elm_Object_Item *nf_it, char *check_str,
                                     bool b_disabled)
{
	GL_CHECK_VAL(check_str, -1);
	GL_CHECK_VAL(nf_it, -1);
	Evas_Object *toolbar = NULL;
	toolbar = elm_object_item_part_content_get(nf_it, "toolbar");
	GL_CHECK_VAL(toolbar, -1);

	Elm_Object_Item *it = NULL;
	Elm_Object_Item *next_it = NULL;
	char *text = NULL;
	it = elm_toolbar_first_item_get(toolbar);
	while (it) {
		next_it = elm_toolbar_item_next_get(it);
		text = (char *)elm_object_item_text_get(it);
		if (text && !g_strcmp0(text, _gl_str(check_str))) {
			_gl_ctrl_disable_item(it, b_disabled);
			return 0;
		}
		it = next_it;
	}
	return -1;
}

int _gl_ctrl_disable_item(Elm_Object_Item *it, Eina_Bool b_disabled)
{
	GL_CHECK_VAL(it, -1);
	/* dlog fatal is enabled. Elm_Object_Item cannot be NULL. */
	elm_object_item_disabled_set(it, b_disabled);
	return 0;
}

