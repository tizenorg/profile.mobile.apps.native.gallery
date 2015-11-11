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
#include "gl-thumbs-sel.h"
#include "gl-albums-sel.h"
#include "gl-thumbs.h"
#include "gl-thumb.h"
#include "gl-albums.h"
#include "gl-ui-util.h"
#include "gl-util.h"
#include "gl-data.h"
#include "gl-controlbar.h"
#include "gl-button.h"
#include "gl-pinchzoom.h"
#include "gl-popup.h"
#include "gl-notify.h"
#include "gl-strings.h"

#if 0
static void __gl_thumbs_sel_trans_finished_cb(void *data, Evas_Object *obj,
        void *event_info)
{
	GL_CHECK(data);
	gl_appdata *ad = (gl_appdata *)data;
	int view_mode = gl_get_view_mode(ad);
	gl_dbgW("view_mode: %d", view_mode);
	evas_object_smart_callback_del(obj, GL_TRANS_FINISHED,
	                               __gl_thumbs_sel_trans_finished_cb);

	/* Clear previous view after animation finished */
	if (view_mode == GL_VIEW_THUMBS_SELECT) {
		elm_gengrid_clear(ad->albuminfo.select_view);
	} else {
		gl_dbgE("Wrong view mode!");
	}

	if (ad->gridinfo.is_append) {
		_gl_thumbs_append_items(data);
		ad->gridinfo.is_append = false;
	}
}

static Eina_Bool __gl_thumbs_sel_pop_cb(void *data, Elm_Object_Item *it)
{
	GL_CHECK_FALSE(data);
	gl_appdata *ad = (gl_appdata *)data;
	gl_dbg("");
	ad->gridinfo.is_append = false;

	/* If the back button is clicked or H/W Back key is pressed,
	this callback will be called right before the
	current view pop. User may implement application logic here. */

	/* To proceed the popping, please return the EINA_TRUE or not,
	popping will be canceled.
	If it needs to delete the current view without any transition effect,
	then call the elm_object_item_del() here and then return the EINA_FALSE */
	_gl_thumbs_sel_pop_view(data, false);

	/* Clear selected list */
	_gl_data_clear_selected_info(data);

	return EINA_TRUE;
}
#endif

static void __gl_thumbs_sel_popup_cb(void *data, Evas_Object *obj, void *ei)
{
	GL_CHECK(obj);
	GL_CHECK(data);
	gl_appdata *ad = (gl_appdata *)data;
	const char *text = NULL;

	text = elm_object_text_get(obj);
	GL_CHECK(text);
	gl_dbg("Button: %s", text);

	if (!g_strcmp0(text, _gl_str(GL_STR_ID_CANCEL_ABB))) {
		GL_IF_DEL_OBJ(ad->popupinfo.popup);
		return;
	} else if (!g_strcmp0(text, _gl_str(GL_STR_ID_MOVE))) {
		ad->albuminfo.file_mc_mode = GL_MC_MOVE;
	} else {
		ad->albuminfo.file_mc_mode = GL_MC_COPY;
	}

	gl_move_copy_to_album(data);
	GL_IF_DEL_OBJ(ad->popupinfo.popup);
}

static void __gl_thumbs_sel_done_cb(void *data, Evas_Object *obj, void *ei)
{
	GL_CHECK(data);
	gl_appdata *ad = (gl_appdata *)data;
	ad->selinfo.elist = eina_list_merge(ad->selinfo.elist, ad->selinfo.fav_elist);
	ad->selinfo.fav_elist = NULL;
	int cnt = _gl_data_selected_list_count(ad);
	if (cnt == 0) {
		gl_dbgW("No thumbs selected!");
		gl_popup_create_popup(ad, GL_POPUP_NOBUT, GL_STR_NO_FILES_SELECTED);
		return;
	}
	/* For new album case */
	_gl_popup_create(data, GL_STR_ID_NEW_ALBUM,
	                 GL_STR_ID_NEW_ALBUM_CONFIRMATION, GL_STR_ID_CANCEL_ABB,
	                 __gl_thumbs_sel_popup_cb, data, GL_STR_ID_MOVE,
	                 __gl_thumbs_sel_popup_cb, data, GL_STR_ID_COPY,
	                 __gl_thumbs_sel_popup_cb, data);
}

static void __gl_thumbs_sel_cancel_cb(void *data, Evas_Object *obj, void *ei)
{
	elm_toolbar_item_selected_set((Elm_Object_Item *)ei, EINA_FALSE);
	GL_CHECK(data);
	gl_dbg("");
	gl_appdata *ad = (gl_appdata *)data;
	ad->selinfo.fav_elist = NULL;
	elm_naviframe_item_pop(ad->maininfo.naviframe);
}

#if 0
/**
 *  Use naviframe api to push thumbnails eidt view to stack.
 *  @param obj is the content to be pushed.
 */
static int __gl_thumbs_sel_push_view(void *data, Evas_Object *parent,
                                     Evas_Object *obj, char *title)
{
	gl_dbg("GL_NAVI_THUMBS_SELECT");
	GL_CHECK_VAL(obj, -1);
	GL_CHECK_VAL(parent, -1);
	GL_CHECK_VAL(data, -1);
	gl_appdata *ad = (gl_appdata *)data;
	Elm_Object_Item *nf_it = NULL;

	/* Add transition finished callback */
	evas_object_smart_callback_add(parent, GL_TRANS_FINISHED,
	                               __gl_thumbs_sel_trans_finished_cb, data);

	/* Push to stack */
	nf_it = elm_naviframe_item_push(parent, title, NULL, NULL, obj,
	                                NULL);
	/* use pop_cb_set to support HW key event */
	elm_naviframe_item_pop_cb_set(nf_it, __gl_thumbs_sel_pop_cb, data);
	_gl_ui_set_translatable_item(nf_it, title);
	ad->gridinfo.nf_it_select = nf_it;

	Elm_Object_Item *tb_it = NULL;
	Evas_Object *toolbar = _gl_ctrl_add_toolbar(parent);
	/* Add 'Cancel' button */
	_gl_ctrl_append_item(toolbar, NULL, GL_STR_ID_CANCEL,
	                     __gl_thumbs_sel_cancel_cb, data);
	char *str = GL_STR_ID_DONE;
	/* Add 'Done' button */
	tb_it = _gl_ctrl_append_item(toolbar, NULL, str,
	                             __gl_thumbs_sel_done_cb, data);
	_gl_ctrl_disable_item(tb_it, true);
	elm_object_item_part_content_set(nf_it, "toolbar", toolbar);
	return 0;
}
#endif

/* Free data after layout deleted */
static void __gl_thumbs_sel_delete_layout_cb(void *data, Evas *e,
        Evas_Object *obj, void *ei)
{
	gl_dbg("Delete layout ---");
	GL_CHECK(data);
	gl_appdata *ad = (gl_appdata *)data;

	Elm_Gengrid_Item_Class *gic = NULL;
	gic = evas_object_data_get(ad->gridinfo.select_view,
	                           "gl_thumbs_item_style_key");
	GL_CHECK(gic);
	gic->func.del = NULL;

	_gl_thumbs_clear_cbs(ad->gridinfo.select_view);
	elm_gengrid_clear(ad->gridinfo.select_view);
	_gl_ui_del_scroller_pos(ad->gridinfo.select_view);
	/* Remove idler */
	ad->gridinfo.is_append = false;
	ad->gridinfo.select_view = NULL;
	ad->gridinfo.select_layout = NULL;
	ad->gridinfo.nf_it_select = NULL;
	/* Clear selected list if it wasn't freed before */
	_gl_data_clear_selected_info(data);
	/* Clear all data */
	_gl_thumbs_clear_and_set_list(data, NULL);
	_gl_albums_set_current(data, NULL);
	/* Reset pinch zoom in/out flag */
	_gl_pinch_reset_flag(data);
	gl_dbg("Delete layout +++");
}

/* Select-all checkbox selected/deselected */
static void __gl_thumbs_sel_selall_cb(void *data, Evas_Object *obj, void *ei)
{
	GL_CHECK(data);
	gl_appdata *ad = (gl_appdata *)data;
	int i = 0;
	int item_cnt = 0;
	int sel_all_cnt = 0;
	gl_item *gitem = NULL;

	ad->selinfo.ck_state = !elm_check_state_get(ad->selinfo.selectall_ck);
	if (ad->selinfo.selectall_ck) {
		elm_check_state_set(ad->selinfo.selectall_ck, ad->selinfo.ck_state);
	}
	int state = ad->selinfo.ck_state;
	gl_dbg("Checkbox state: %d.", state);
	Eina_List *medias_elist = NULL;
	_gl_thumbs_get_list(ad, &(medias_elist));
	/* Get all medias count of current album */
	item_cnt = eina_list_count(medias_elist);

	for (i = 0; i < item_cnt; i++) {
		gitem = eina_list_nth(medias_elist, i);
		GL_CHECK(gitem);
		GL_CHECK(gitem->item);

		if (gitem->checked == state) {
			continue;
		}

		gitem->checked = state;

		if (state == EINA_TRUE) {
			if (strcmp(gitem->album->cluster->uuid, GL_ALBUM_FAVOURITE_ID) == 0) {
				Eina_List *sel_list1 = ad->selinfo.fav_elist;
				sel_list1 = eina_list_append(sel_list1, gitem);
				ad->selinfo.fav_elist = sel_list1;
			} else {
				_gl_data_selected_list_append(ad, gitem);
			}
			gitem->album->elist = eina_list_append(gitem->album->elist, gitem);
		} else {
			if (strcmp(gitem->album->cluster->uuid, GL_ALBUM_FAVOURITE_ID) == 0) {
				_gl_data_selected_fav_list_remove(ad, gitem);
			} else {
				_gl_data_selected_list_remove(ad, gitem);
			}
			gitem->album->elist = eina_list_remove(gitem->album->elist, gitem);
		}
	}
	/* Update all realized items */
	_gl_ui_update_realized_grid_ck(ad->gridinfo.select_view,
	                               GL_THUMB_CHECKBOX, NULL,
	                               ad->selinfo.ck_state);
	/* Recreate selection info for both cases */
	if (state == EINA_FALSE) {
		sel_all_cnt = 0;
	} else {
		sel_all_cnt = item_cnt;
	}

	int sel_cnt = eina_list_count(ad->selinfo.elist);
	int fav_sel_cnt = eina_list_count(ad->selinfo.fav_elist);
	int total_sel_count = sel_cnt + fav_sel_cnt;
	_gl_notify_check_selall(ad, ad->albuminfo.nf_it_select, item_cnt,
	                        sel_all_cnt);
	if (gitem->album->item) {
		elm_gengrid_item_update(gitem->album->item);
	}
	/* Update the label text */
	_gl_ui_update_navi_title_text(ad->albuminfo.nf_it_select, total_sel_count, false);
}

void _gl_thumb_sel_add_album_data(gl_cluster *album, Eina_List *medias_elist)
{
	GL_CHECK(album);
	GL_CHECK(medias_elist);
	int cnt = eina_list_count(medias_elist);
	int i;
	gl_item *gitem = NULL;
	for (i = 0; i < cnt; i++) {
		gitem = eina_list_nth(medias_elist, i);
		if (gitem) {
			gitem->album = album;
		}
	}
}

/**
 * Album selected for adding tags to photo
 */
int _gl_thumbs_sel_create_view(void *data, gl_cluster *album_item)
{
	GL_CHECK_VAL(album_item, -1);
	GL_CHECK_VAL(data, -1);
	gl_appdata *ad = (gl_appdata *)data;
	ad->gridinfo.is_append = false;
	gl_item *gitem = NULL;
	EINA_LIST_FREE(album_item->elist, gitem) {
	}

	/* Get items list without current tag */
	Eina_List *medias_elist = NULL;
	_gl_data_get_items_album(ad, album_item, GL_FIRST_VIEW_START_POS,
	                         GL_GET_UNTIL_LAST_RECORD, &medias_elist);
	ad->gridinfo.is_append = true;
	_gl_thumbs_set_list(ad, medias_elist);
	_gl_thumb_sel_add_album_data(album_item, medias_elist);
	int count = eina_list_count(medias_elist);
	int i;

	if (strcmp(album_item->cluster->uuid, GL_ALBUM_FAVOURITE_ID) == 0) {
		for (i = 0; i < count ; i++) {
			gitem = eina_list_nth(medias_elist, i);
			_gl_data_restore_selected(ad->selinfo.fav_elist, gitem);
		}
	} else {
		for (i = 0; i < count ; i++) {
			gitem = eina_list_nth(medias_elist, i);
			_gl_data_restore_selected(ad->selinfo.elist, gitem);
		}
	}
	if (eina_list_count(medias_elist) <= 0) {
		gl_dbgW("All medias are tagged!");
		/* Add notification */
		_gl_notify_create_notiinfo(GL_STR_ADDED);
		return -1;
	}

	Evas_Object *layout = NULL;
	/* Add layout */
	layout = _gl_ctrl_add_layout(ad->maininfo.naviframe);
	GL_CHECK_VAL(layout, -1);
	evas_object_size_hint_weight_set(layout, EVAS_HINT_EXPAND,
	                                 EVAS_HINT_EXPAND);
	evas_object_size_hint_align_set(layout, EVAS_HINT_FILL, EVAS_HINT_FILL);
	/* Register delete callback */
	evas_object_event_callback_add(layout, EVAS_CALLBACK_DEL,
	                               __gl_thumbs_sel_delete_layout_cb, data);
	/* Save view mode */
	int pre_view_m = gl_get_view_mode(data);
	/* Set new view mode */
	gl_set_view_mode(data, GL_VIEW_THUMBS_SELECT);
	/* Add pinch effect before adding view */
	_gl_pinch_add_event(data, layout);
	/* Add view */
	Evas_Object *view = NULL;
	view = _gl_thumbs_add_grid(data, layout, GL_THUMB_ALL);
	if (view == NULL) {
		evas_object_del(layout);
		gl_popup_create_popup(data, GL_POPUP_NOBUT, GL_STR_NO_ALBUMS);
		/* Reset view mode */
		gl_set_view_mode(data, pre_view_m);
		return -1;
	}

	/* Set view to layout */
	elm_object_part_content_set(ad->albuminfo.select_layout, "elm.swallow.view", view);
	/* Push view to naviframe */
	elm_object_item_part_text_set(ad->albuminfo.nf_it_select, "elm.text.title", _gl_str(GL_STR_ID_SELECT_ITEM));

	Evas_Object *btn1 = elm_button_add(layout);
	Evas_Object *btn2 = elm_button_add(layout);

	GL_CHECK_VAL(btn1, -1);
	GL_CHECK_VAL(btn2, -1);

	/* Title Cancel Button */
	elm_object_style_set(btn1, "naviframe/title_left");
	elm_object_item_part_content_set(ad->albuminfo.nf_it_select, GL_NAVIFRAME_TITLE_LEFT_BTN, btn1);
	_gl_ui_set_translate_str(btn1, GL_STR_ID_CANCEL_CAP);
	evas_object_smart_callback_add(btn1, "clicked", __gl_thumbs_sel_cancel_cb, ad);

	/* Title Done Button */
	elm_object_style_set(btn2, "naviframe/title_right");
	elm_object_item_part_content_set(ad->albuminfo.nf_it_select, GL_NAVIFRAME_TITLE_RIGHT_BTN, btn2);
	_gl_ui_set_translate_str(btn2, GL_STR_ID_DONE_CAP);
	evas_object_smart_callback_add(btn2, "clicked", __gl_thumbs_sel_done_cb, ad);
	elm_object_disabled_set(btn2, EINA_TRUE);

	ad->gridinfo.select_view = view;
	ad->gridinfo.select_layout = layout;

	/* Show select all widget */
	int w = -1;
	int h = -1;
	evas_object_geometry_get(ad->maininfo.win, NULL, NULL, &w, &h);
	Edje_Message_Int *msg = (Edje_Message_Int *)malloc(sizeof(Edje_Message_Int) + sizeof(int));
	msg->val = ((w < h) ? (w - (w / 3)) : (w - (h / 3)));
	gl_dbgE("msg value : %d", msg->val);
	edje_object_message_send(elm_layout_edje_get(ad->albuminfo.select_layout), EDJE_MESSAGE_INT, 2, msg);
	free(msg);
	elm_object_signal_emit(ad->albuminfo.select_layout, "elm,selectall,state,visible,bg", "elm");
	elm_object_signal_emit(ad->albuminfo.select_layout, "elm,selectall,state,visible", "elm");
	_gl_ui_add_selall_ck(ad->albuminfo.select_layout, "select.all.area.check", "select.all.area.check",
	                     __gl_thumbs_sel_selall_cb, data);

	int sel_cnt = _gl_data_selected_list_count(ad);

	int fav_sel_cnt = eina_list_count(ad->selinfo.fav_elist);
	int total_sel_count = sel_cnt + fav_sel_cnt;
	/* Update the label text */
	_gl_thumbs_update_label_text(ad->albuminfo.nf_it_select, total_sel_count, false);

	gl_item *gitem_tmp = eina_list_nth(ad->gridinfo.medias_elist, 0);
	GL_CHECK_VAL(gitem_tmp->album->elist, -1);
	if (gitem_tmp->album->elist) {
		int album_select_count = eina_list_count(gitem_tmp->album->elist);
		gl_dbgW("album sel cont : %d !", album_select_count);
		_gl_notify_check_selall(ad, ad->albuminfo.nf_it_select, ad->gridinfo.count, album_select_count);
	}
	gl_dbg("Done");
	return 0;
}

/* Update in select thunb view for Tags */
int _gl_thumbs_sel_update_view(void *data)
{
	GL_CHECK_VAL(data, -1);
	gl_appdata *ad = (gl_appdata *)data;
	int view_mode = gl_get_view_mode(ad);

	gl_dbg("view_mode: %d", view_mode);
	if (view_mode != GL_VIEW_THUMBS_SELECT) {
		gl_dbg("Not in Select view.");
		return -1;
	}

#ifdef _USE_ROTATE_BG
	bool is_select = true;
	if (_gl_ctrl_get_tab_mode(data) == GL_CTRL_TAB_ALBUMS ||
	        _gl_ctrl_get_tab_mode(data) == GL_CTRL_TAB_TIMELINE) {
		is_select = false;
	}
	_gl_refresh_albums_list(data, false, is_select);
#else
	/* Checkme: albums list is not updated */
	Eina_List *sel_id_list = NULL;
	Eina_List *media_elist = NULL;
	_gl_data_save_selected_str_ids(data, &sel_id_list);
	_gl_data_clear_selected_info(ad);
	_gl_data_update_item_list(ad, &sel_id_list, &media_elist);
	_gl_thumbs_clear_and_set_list(ad, media_elist);
#endif

	if (_gl_thumbs_check_zero(data)) {
		int err = -1;
		int item_cnt = 0;
		err = _gl_data_get_item_cnt(GL_ALBUM_ALL_ID, GL_STORE_T_ALL, &item_cnt);
		/* Destroy select albums view */
		if (err != 0 || item_cnt == 0) {
			gl_dbg("None local albums.");
			_gl_albums_sel_pop_view(ad);
		} else {
			gl_dbg("Empty album.");
			_gl_thumbs_sel_pop_view(ad, true);
		}
		return 0;
	} else {
		_gl_thumbs_show_edit_view(ad);

		/* Get selected medias count */
		int sel_cnt = _gl_data_selected_list_count(ad);

		gl_item *gitem = NULL;
		gitem = eina_list_nth(ad->gridinfo.medias_elist, 0);
		int album_sel_count = eina_list_count(gitem->album->elist);

		/* Display selectioninfo */
		_gl_notify_check_selall(ad, ad->albuminfo.nf_it_select,
		                        ad->gridinfo.count, album_sel_count);

		/* Update the label text */
		_gl_ui_update_label_text(ad->albuminfo.nf_it_select, sel_cnt,
		                         false);
	}

	return 0;
}

int _gl_thumbs_sel_pop_view(void *data, bool b_pop)
{
	GL_CHECK_VAL(data, -1);
	gl_appdata *ad = (gl_appdata *)data;

	if (b_pop) {
		/* Pop naviframe item*/
		elm_naviframe_item_pop(ad->maininfo.naviframe);
	}
	/* Reset view mode */
	gl_set_view_mode(data, GL_VIEW_ALBUMS_SELECT);
	/* Update view */
	_gl_albums_sel_update_view(data);
	return 0;
}

int _gl_thumbs_sel_update_lang(void *data)
{
	GL_CHECK_VAL(data, -1);
	gl_appdata *ad = (gl_appdata *)data;
	gl_dbg("");
	int sel_cnt = _gl_data_selected_list_count(ad);
	/* Update the label text */
	_gl_ui_update_navi_title_text(ad->albuminfo.nf_it_select, sel_cnt, false);
	return 0;
}

int _gl_thumbs_sel_disable_btns(void *data, bool b_disabled)
{
	GL_CHECK_VAL(data, -1);
	gl_appdata *ad = (gl_appdata *)data;
	gl_dbg("disabled: %d", b_disabled);
	_gl_ctrl_disable_items(ad->gridinfo.nf_it_select, b_disabled);
	return 0;
}

