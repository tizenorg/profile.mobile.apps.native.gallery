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

#include <errno.h>
#include "gl-albums.h"
#include "gl-albums-edit.h"
#include "gl-albums-rename.h"
#include "gl-debug.h"
#include "gl-strings.h"
#include "gl-ui-util.h"
#include "gl-util.h"
#include "gl-data.h"
#include "gl-icons.h"
#include "gl-controlbar.h"
#include "gl-ctxpopup.h"
#include "gl-button.h"
#include "gl-notify.h"
#include "gl-popup.h"
#include "gl-tile.h"
#include "gl-ext-exec.h"

/**
* @Brif Update checkbox state and album cover and update selectioninfo.
*
* @Param data  gl_appdata.
* @Param album_item  album item.
* @Param check_obj  object of checkbox.
* @Param b_ck_box  true: Checkbox selection, false: album cover selection.
*/
static int __gl_albums_edit_update_check_state(void *data,
        gl_cluster *album_item,
        Evas_Object *check_obj,
        bool b_ck_box)
{
	GL_CHECK_VAL(album_item, -1);
	GL_CHECK_VAL(album_item->cluster, -1);
	GL_CHECK_VAL(album_item->cluster->uuid, -1);
	GL_CHECK_VAL(check_obj, -1);
	GL_CHECK_VAL(data, -1);
	gl_appdata *ad = (gl_appdata *)data;
	gl_dbg("Select checkbox[1]/album_cover[0]: %d.", b_ck_box);

	/* Update state of checkbox */
	album_item->checked = !album_item->checked;
	gl_sdbg("album (%s) check state: %d", album_item->cluster->display_name,
	        album_item->checked);

	/**
	* If album cover selected, update checkbox icon manually,
	* If checkbox selected, its icon is updated by system automatically.
	*/
	Evas_Object *ck = NULL;
	if (b_ck_box == false)
		ck = elm_object_item_part_content_get(album_item->item,
		                                      GL_TILE_CHECKBOX);
	else
		ck = elm_object_item_part_content_get(album_item->item,
		                                      GL_TILE_CHECKBOX_GRID);
	elm_check_state_set(ck, album_item->checked);

	if (album_item->checked) {
		gl_sdbg("Append:%s, id:%s", album_item->cluster->display_name,
		        album_item->cluster->uuid);
		_gl_data_albums_selected_list_append(ad, album_item);
	} else {
		gl_sdbg("Remove:%s, id:%s", album_item->cluster->display_name,
		        album_item->cluster->uuid);
		_gl_data_albums_selected_list_remove(ad, album_item);
	}

	/* Display selectioninfo */
	int cnt = _gl_data_get_albums_selected_cnt(ad);
	_gl_notify_check_selall(ad, ad->ctrlinfo.nf_it,
	                        ad->albuminfo.elist->edit_cnt, cnt);
	/* Update the label text */
	_gl_ui_update_navi_title_text(ad->ctrlinfo.nf_it, cnt, false);
	return 0;
}

static void __gl_albums_edit_check_changed(void *data, Evas_Object *obj,
        void *ei)
{
	GL_CHECK(obj);
	GL_CHECK(data);
	gl_cluster *album_item = (gl_cluster *)data;
	GL_CHECK(album_item->ad);
	gl_appdata *ad = (gl_appdata *)album_item->ad;
	gl_dbg("");

	if (gl_get_view_mode(ad) != GL_VIEW_ALBUMS_EDIT) {
		gl_dbgE("Wrong view mode!");
		return;
	} else {
		/* gl_dbg("EditMode"); */
	}

	/* Checkbox selected, b_ck_box=true */
	__gl_albums_edit_update_check_state(ad, album_item, obj, true);
}

static void __gl_albums_edit_check_grid_changed(void *data, Evas_Object *obj,
        void *ei)
{
	GL_CHECK(obj);
	GL_CHECK(data);
	gl_cluster *album_item = (gl_cluster *)data;
	GL_CHECK(album_item->ad);
	gl_appdata *ad = (gl_appdata *)album_item->ad;
	gl_dbg("");

	if (gl_get_view_mode(ad) != GL_VIEW_ALBUMS_EDIT) {
		gl_dbgE("Wrong view mode!");
		return;
	} else {
		/* gl_dbg("EditMode"); */
	}

	/* Grid checkbox selected, b_ck_box=false */
	__gl_albums_edit_update_check_state(ad, album_item, obj, false);
}

#if 0
static void __gl_albums_edit_rename(gl_cluster *album)
{
	GL_CHECK(album);
	GL_CHECK(album->ad);
	gl_appdata *ad = (gl_appdata *)album->ad;
	/* save album selected for rename */
	if (_gl_albums_rename_create_view(album->ad, album) == 0) {
		ad->albuminfo.selected = album;
	}
}

static void __gl_albums_edit_rename_btn_cb(void *data, Evas_Object *obj,
        void *event_info)
{
	gl_dbg("");
	GL_CHECK(data);
	gl_cluster *album_item = (gl_cluster *)data;
	gl_appdata *ad = (gl_appdata *)album_item->ad;
	_gl_ctxpopup_del(ad);
	/* Save scroller position before clearing gengrid */
	_gl_ui_save_scroller_pos(ad->albuminfo.view);
	__gl_albums_edit_rename(album_item);
}
#endif

static void __gl_albums_edit_del_cb(void *data, Evas_Object *obj,
                                    void *ei)
{
	elm_toolbar_item_selected_set((Elm_Object_Item *)ei, EINA_FALSE);
	GL_CHECK(data);
	gl_dbg("");

	if ((gl_get_view_mode(data) == GL_VIEW_ALBUMS_EDIT)) {
		if (_gl_data_get_albums_selected_cnt(data) == 0) {
			gl_dbgW("No albums selected!");
			gl_popup_create_popup(data, GL_POPUP_NOBUT,
			                      GL_STR_NO_ALBUMS_SELECTED);
			return;
		}
		_gl_popup_create_del(data, GL_POPUP_DEL_ALBUM,
		                     _gl_data_get_albums_selected_cnt,
		                     _gl_del_albums, data);
	} else {
		gl_dbgE("Unknow mode!");
	}
}

static void __gl_albums_edit_cancel_cb(void *data, Evas_Object *obj, void *ei)
{
	elm_toolbar_item_selected_set((Elm_Object_Item *)ei, EINA_FALSE);
	GL_CHECK(data);
	gl_dbg("");
	_gl_albums_change_mode(data, false);
}

#ifdef GL_EXTENDED_FEATURES
static int __gl_albums_edit_get_cnt(void *data, gl_media_s *item)
{
	GL_CHECK_VAL(item, -1);
	GL_CHECK_VAL(item->file_url, -1);
	GL_CHECK_VAL(data, -1);

	int *count = (int *)data;
	*count += 1;
	return 0;
}

static int __gl_albums_edit_get_path_cb(void *data, char **files)
{
	GL_CHECK_VAL(data, 0);
	GL_CHECK_VAL(files, 0);
	return _gl_get_album_images_path(data, files, false);
}

void _gl_albums_edit_share(void *data)
{
	GL_CHECK(data);
	gl_appdata *ad = (gl_appdata *)data;
	gl_dbg("");

	if (ad->uginfo.ug || ad->uginfo.b_app_called) {
		gl_dbgW("UG or APP is already loaded!");
		return;
	}

	int cnt = 0;

	cnt = _gl_data_get_albums_selected_cnt(ad);
	if (cnt == 0) {
		gl_dbgW("No albums selected!");
		gl_popup_create_popup(ad, GL_POPUP_NOBUT,
		                      GL_STR_NO_ALBUMS_SELECTED);
		return;
	}
	/* Check albums are empty */
	if (_gl_data_is_albums_selected_empty(ad)) {
		gl_dbgW("No thumbs selected!");
		gl_popup_create_popup(ad, GL_POPUP_NOBUT,
		                      GL_STR_NO_FILES_SELECTED);
		return;
	}

	int media_cnt = 0;
	_gl_data_get_albums_selected_files(data, __gl_albums_edit_get_cnt,
	                                   (void *)(&media_cnt));
	gl_dbg("media_cnt: %d", media_cnt);
	/* Check albums are empty */
	if (media_cnt == 0) {
		gl_dbgW("No thumbs selected!");
		gl_popup_create_popup(ad, GL_POPUP_NOBUT,
		                      GL_STR_NO_FILES_SELECTED);
		return;
	}
	_gl_ext_launch_share(data, media_cnt,
	                     __gl_albums_edit_get_path_cb);
}

static void __gl_albums_edit_share_cb(void *data, Evas_Object *obj,
                                      void *ei)
{
	elm_toolbar_item_selected_set((Elm_Object_Item *)ei, EINA_FALSE);
	GL_CHECK(data);
	_gl_albums_edit_share(data);
	/*back to the albums view */
	_gl_albums_change_mode(data, false);
}
#endif

/* Select-all checkbox selected/deselected */
static void __gl_albums_edit_selall_cb(void *data, Evas_Object *obj, void *ei)
{
	GL_CHECK(data);
	gl_appdata *ad = (gl_appdata *)data;
	int item_cnt = 0;
	int sel_all_cnt = 0;

	ad->selinfo.ck_state = !ad->selinfo.ck_state;
	if (ad->selinfo.selectall_ck) {
		elm_check_state_set(ad->selinfo.selectall_ck, ad->selinfo.ck_state);
	}
	int state = ad->selinfo.ck_state;
	gl_dbg("Checkbox state: %d", state);

	gl_cluster *album = NULL;
	Elm_Object_Item *first_it = NULL;
	Elm_Object_Item *next_it = NULL;
	Elm_Object_Item *last_it = NULL;
	first_it = elm_gengrid_first_item_get(ad->albuminfo.view);
	last_it = elm_gengrid_last_item_get(ad->albuminfo.view);
	while (first_it) {
		album = NULL;
		next_it = elm_gengrid_item_next_get(first_it);
		/* Get data */
		album = (gl_cluster *)elm_object_item_data_get(first_it);
		if (album == NULL || album->cluster == NULL) {
			gl_dbgE("Invalid item data!");
			/* Get next item */
			first_it = next_it;
			continue;
		}

		if (album->cluster->type >= GL_STORE_T_ALL) {
			gl_dbgW("Uneditable album!");
			/* Get next item */
			first_it = next_it;
			continue;
		}

		item_cnt++;

		if (album->checked == state) {
			/* Get next item */
			first_it = next_it;
			continue;
		}

		/* Update checkbox state */
		album->checked = state;
		/* Update selected list */
		if (state == EINA_TRUE) {
			_gl_data_albums_selected_list_append(ad, album);
		} else {
			_gl_data_albums_selected_list_remove(ad, album);
		}

		if (last_it == first_it) {
			gl_dbg("Update done!");
			break;
		} else {
			first_it = next_it;
		}
	}
	/* Update all realized items */
	_gl_ui_update_realized_grid_ck(ad->albuminfo.view,
	                               GL_TILE_CHECKBOX_GRID, GL_TILE_CHECKBOX,
	                               state);

	/* Recreate selection info for both cases */
	if (state == EINA_FALSE) {
		sel_all_cnt = 0;
	} else {
		sel_all_cnt = item_cnt;
	}

	_gl_notify_check_selall(ad, ad->ctrlinfo.nf_it, item_cnt, sel_all_cnt);
	/* Update the label text */
	_gl_ui_update_navi_title_text(ad->ctrlinfo.nf_it, sel_all_cnt, false);
}

#if 0
static int __gl_albums_edit_ctxpopup_append(void *data, Evas_Object *parent)
{
	gl_dbg("");
	GL_CHECK_VAL(parent, -1);
	GL_CHECK_VAL(data, -1);
	gl_appdata *ad = (gl_appdata *)data;
	GL_CHECK_VAL(ad->albuminfo.sel_elist, -1);
	gl_cluster *album_item = NULL;
	album_item = (gl_cluster *)eina_list_nth(ad->albuminfo.sel_elist, 0);
	GL_CHECK_VAL(album_item, -1);

	/* Rename */
	_gl_ctxpopup_append(parent, GL_STR_ID_RENAME,
	                    __gl_albums_edit_rename_btn_cb, album_item);
	return 0;
}

static void __gl_albums_edit_more_btn_cb(void *data, Evas_Object *obj, void *ei)
{
	gl_dbgE("more button clicked");
	GL_CHECK(data);
	gl_appdata *ad = (gl_appdata *)data;

	if (GL_GRID_EDIT != ad->albuminfo.b_share_mode) {
		return;
	}
	GL_CHECK(ad->albuminfo.sel_elist);
	gl_cluster *album_item = NULL;
	album_item = (gl_cluster *)eina_list_nth(ad->albuminfo.sel_elist, 0);
	GL_CHECK(album_item);
	if (1 != _gl_data_get_albums_selected_cnt(ad) ||
	        album_item->cluster->type > GL_STORE_T_MMC ||
	        _gl_data_is_camera_album(album_item->cluster)) {
		return;
	}
	_gl_ctxpopup_create(data, obj, __gl_albums_edit_ctxpopup_append);
}
#endif

Evas_Object *_gl_albums_edit_add_content(void *data, Evas_Object *obj,
        const char *part)
{
	GL_CHECK_NULL(part);
	GL_CHECK_NULL(data);
	gl_cluster *album = (gl_cluster *)data;
	GL_CHECK_NULL(album->cluster);
	Evas_Object *_obj = NULL;

	if (!g_strcmp0(part, GL_TILE_CHECKBOX_GRID)) {
		if (album->cluster->type >= GL_STORE_T_ALL) {
			return NULL;
		}

		_obj = _gl_tile_show_part_checkbox_grid(obj, album->checked,
		                                        __gl_albums_edit_check_grid_changed,
		                                        data);
	} else if (!g_strcmp0(part, GL_TILE_CHECKBOX)) {
		if (album->cluster->type >= GL_STORE_T_ALL) {
			return NULL;
		}

		_obj = _gl_tile_show_part_checkbox(obj, album->checked,
		                                   __gl_albums_edit_check_changed,
		                                   data);
	} else if (!g_strcmp0(part, GL_TILE_DIM) &&
	           album->cluster->type >= GL_STORE_T_ALL) {
		_obj = _gl_tile_show_part_dim(obj); /* Disable item */
	}

	return _obj;
}

/**
 *  Use naviframe api to push albums edit view to stack.
 *  @param obj is the content to be pushed.
 */
int _gl_albums_edit_add_btns(void *data, Evas_Object *parent)
{
	gl_dbg("GL_NAVI_ALBUMS_EDIT");
	GL_CHECK_VAL(parent, -1);
	GL_CHECK_VAL(data, -1);
	gl_appdata *ad = (gl_appdata *)data;
	Elm_Object_Item *nf_it = elm_naviframe_top_item_get(parent);
	GL_CHECK_VAL(nf_it, -1);

	_gl_ui_clear_btns(data);

	_gl_ui_disable_menu(nf_it, true);

	Evas_Object *btn1 = elm_button_add(parent);
	Evas_Object *btn2 = elm_button_add(parent);
	GL_CHECK_VAL(btn1, -1);
	GL_CHECK_VAL(btn2, -1);
	/* Title Cancel Button */
	elm_object_style_set(btn1, "naviframe/title_left");
	elm_object_item_part_content_set(ad->ctrlinfo.nf_it, GL_NAVIFRAME_TITLE_LEFT_BTN, btn1);
	_gl_ui_set_translate_str(btn1, GL_STR_ID_CANCEL_CAP);
	evas_object_smart_callback_add(btn1, "clicked", __gl_albums_edit_cancel_cb, ad);

	/* Title Done Button */
	elm_object_style_set(btn2, "naviframe/title_right");
	elm_object_item_part_content_set(ad->ctrlinfo.nf_it, GL_NAVIFRAME_TITLE_RIGHT_BTN, btn2);
	_gl_ui_set_translate_str(btn2, GL_STR_ID_DONE_CAP);
	evas_object_smart_callback_add(btn2, "clicked", __gl_albums_edit_del_cb, ad);
	elm_object_disabled_set(btn2, EINA_TRUE);

	/* Select-all */
	ad->selinfo.ck_state = false;
	_gl_ui_add_selall_ck(ad->ctrlinfo.ctrlbar_view_ly, "select.all.area.check", "select.all.area.check",
	                     __gl_albums_edit_selall_cb, data);
	return 0;
}

int _gl_albums_edit_update_view(void *data)
{
	GL_CHECK_VAL(data, -1);
	gl_appdata *ad = (gl_appdata *)data;
	gl_dbg("");

	if (gl_check_gallery_empty(data) ||
	        ad->albuminfo.elist->edit_cnt == 0) {
		/* Remove invalid widgets */
		gl_del_invalid_widgets(data, GL_INVALID_NONE);
		gl_dbgW("None editable albums, show albums view!");
		_gl_albums_edit_pop_view(data);
		return 0;
	}

	int sel_cnt = _gl_data_get_albums_selected_cnt(data);
	int invalid_m = GL_INVALID_NONE;
	/* Album selected for rename was deleted */
	if (sel_cnt == 0) {
		/* None albums selected */
		gl_dbgW("None albums selected!");
		/* Disable toolbar buttons */
		_gl_albums_edit_disable_btns(data, true);
		/* Remove invalid widgets */
		gl_del_invalid_widgets(data, invalid_m);
	}

	/* Update edit view, other update view in cancel callback */
	_gl_albums_create_items(data, ad->albuminfo.view);
	/* Display selectioninfo */
	_gl_notify_check_selall(data, ad->ctrlinfo.nf_it,
	                        ad->albuminfo.elist->edit_cnt, sel_cnt);
	/* Update the label text */
	_gl_ui_update_navi_title_text(ad->ctrlinfo.nf_it, sel_cnt, false);
	return 0;
}

int _gl_albums_edit_pop_view(void *data)
{
	GL_CHECK_VAL(data, -1);
	gl_appdata *ad = (gl_appdata *)data;

	/* Pop rename entry view */
	elm_naviframe_item_pop_to(ad->ctrlinfo.nf_it);
	ad->albuminfo.selected = NULL;
	ad->albuminfo.b_share_mode = GL_GRID_EDIT;
	_gl_data_finalize_albums_selected_list(data);

	gl_albums_change_to_view(data);
	_gl_albums_check_btns(data);
	return 0;
}

int _gl_albums_edit_update_lang(void *data)
{
	GL_CHECK_VAL(data, -1);
	gl_appdata *ad = (gl_appdata *)data;
	GL_CHECK_VAL(ad->ctrlinfo.nf_it, -1);

	/*Reset select info text*/
	int cnt = _gl_data_get_albums_selected_cnt(ad);
	/* Update the label text */
	_gl_ui_update_navi_title_text(ad->ctrlinfo.nf_it, cnt, true);
	return 0;
}

/* albums view: more-delete-add to home */
/* Delete in unavaliable for Facebook */
int _gl_albums_edit_disable_btns(void *data, bool b_disable)
{
	GL_CHECK_VAL(data, -1);
	gl_appdata *ad = (gl_appdata *)data;
	gl_dbg("b_disable: %d", b_disable);

	if (!b_disable) {
		/* Enable items */
		_gl_ctrl_disable_items(ad->ctrlinfo.nf_it, false);
	} else {
		_gl_ctrl_disable_items(ad->ctrlinfo.nf_it, true);
	}
	return 0;
}

