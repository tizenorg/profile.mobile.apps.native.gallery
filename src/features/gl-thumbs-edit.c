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

#include "gl-thumbs-edit.h"
#include "gl-thumbs.h"
#include "gl-thumb.h"
#include "gl-pinchzoom.h"
#include "gl-debug.h"
#include "gl-strings.h"
#include "gl-ui-util.h"
#include "gl-util.h"
#include "gl-data.h"
#include "gl-icons.h"
#include "gl-controlbar.h"
#include "gl-ctxpopup.h"
#include "gl-progressbar.h"
#include "gl-button.h"
#include "gl-notify.h"
#include "gl-popup.h"
#include "gl-lang.h"
#include "gl-ext-exec.h"
#include "gl-ext-ug-load.h"
#include "gl-fs.h"
#include "gl-albums.h"

int _gl_thumbs_edit_get_path(void *data, char **files)
{
	GL_CHECK_VAL(files, 0);
	GL_CHECK_VAL(data, 0);
	gl_appdata *ad = (gl_appdata *)data;
	GL_CHECK_VAL(ad->selinfo.elist, 0);
	int i = 0;
	gl_item *current = NULL;
	Eina_List *l = NULL;

	EINA_LIST_FOREACH(ad->selinfo.elist, l, current) {
		if (current == NULL || current->item == NULL ||
		        current->item->file_url == NULL) {
			gl_dbgE("Invalid gitem!");
			continue;
		}

		files[i++] = g_strdup(current->item->file_url);
		gl_sdbg("file_url: %s!", files[i - 1]);
	}
	return i;
}

#ifdef GL_EXTENDED_FEATURES
static void __gl_thumbs_edit_share_cb(void *data, Evas_Object *obj,
                                      void *ei)
{
	elm_toolbar_item_selected_set((Elm_Object_Item *)ei, EINA_FALSE);
	GL_CHECK(data);
	gl_appdata *ad = (gl_appdata *)data;
	gl_dbg("");

	if (ad->uginfo.ug || ad->uginfo.b_app_called) {
		gl_dbgW("UG or APP is already loaded!");
		return;
	}

	int view_mode = gl_get_view_mode(ad);
	int cnt = _gl_data_selected_list_count(ad);

	if (view_mode == GL_VIEW_THUMBS_EDIT) {
		if (cnt == 0) {
			gl_dbg("No thumbs selected, return!");
			gl_popup_create_popup(ad, GL_POPUP_NOBUT,
			                      GL_STR_NO_FILES_SELECTED);
			return;
		}
	} else {
		gl_dbgE("Unknow mode!");
		return;
	}
	_gl_ext_launch_share(data, cnt, _gl_thumbs_edit_get_path);
	/* Change to normal mode */
	_gl_thumbs_edit_pop_view(data);
}

/* To launch Image-editor*/
static void __gl_thumbs_edit_ie_cb(void *data, Evas_Object *obj,
                                   void *ei)
{
	GL_CHECK(data);
	gl_appdata *ad = (gl_appdata *)data;
	gl_dbg("");

	if (ad->uginfo.ug || ad->uginfo.b_app_called) {
		gl_dbgW("UG or APP is already loaded!");
		return;
	}
	_gl_ctxpopup_del(data);

	if (_gl_data_selected_list_count(ad) == 0) {
		gl_dbgW("No thumbs selected!");
		gl_popup_create_popup(ad, GL_POPUP_NOBUT,
		                      GL_STR_NO_FILES_SELECTED);
		return;
	}

	int view_mode = gl_get_view_mode(ad);
	if (view_mode == GL_VIEW_THUMBS_EDIT) {
		_gl_ext_load_ie(data, gl_get_selected_files_path_str);
	} else {
		gl_dbgE("Unkonw mode!");
	}
	/* Change to normal mode */
	_gl_thumbs_edit_pop_view(data);
}

/* Copy media to album in edit view */
static void __gl_thumbs_edit_copy_cb(void *data, Evas_Object *obj,
                                     void *ei)
{
	GL_CHECK(data);
	_gl_ctxpopup_del(data);
	gl_appdata *ad = (gl_appdata *)data;
	int view_mode = gl_get_view_mode(data);
	gl_dbg("");

	if (ad->uginfo.ug || ad->uginfo.b_app_called) {
		gl_dbgW("UG or APP is already loaded!");
		return;
	}

	if (view_mode == GL_VIEW_THUMBS_EDIT) {
		if (_gl_data_selected_list_count(data) == 0) {
			gl_dbgW("No thumbs selected!");
			gl_popup_create_popup(data, GL_POPUP_NOBUT,
			                      GL_STR_NO_FILES_SELECTED);
			return;
		}

		ad->albuminfo.file_mc_mode = GL_MC_COPY;
		_gl_popup_create_copy(data, gl_move_copy_to_album);
	} else {
		gl_dbgE("Unknow mode!");
	}
}

/* move media to album in edit view*/
static void __gl_thumbs_edit_move_cb(void *data, Evas_Object *obj, void *ei)
{
	elm_toolbar_item_selected_set((Elm_Object_Item *)ei, EINA_FALSE);
	GL_CHECK(data);
	gl_appdata *ad = (gl_appdata *)data;
	int view_mode = gl_get_view_mode(data);
	gl_dbg("");

	if (ad->uginfo.ug || ad->uginfo.b_app_called) {
		gl_dbgW("UG or APP is already loaded!");
		return;
	}

	if (view_mode == GL_VIEW_THUMBS_EDIT) {
		if (_gl_data_selected_list_count(data) == 0) {
			gl_dbgW("No thumbs selected!");
			gl_popup_create_popup(data, GL_POPUP_NOBUT,
			                      GL_STR_NO_FILES_SELECTED);
			return;
		}

		ad->albuminfo.file_mc_mode = GL_MC_MOVE;
		_gl_popup_create_move(data, gl_move_copy_to_album);
	} else {
		gl_dbgE("Unknow mode!");
	}
}

#ifdef _USE_ROTATE_BG
static void __gl_thumbs_edit_rotate_left_cb(void *data, Evas_Object *obj,
			void *ei)
{
	GL_CHECK(data);
	if (_gl_data_selected_list_count(data) == 0) {
		gl_dbgW("No thumbs selected!");
		gl_popup_create_popup(data, GL_POPUP_NOBUT,
		                      GL_STR_NO_FILES_SELECTED);
		return;
	}
	_gl_ctxpopup_del(data);
	_gl_rotate_images(data, true);
}

static void __gl_thumbs_edit_rotate_right_cb(void *data, Evas_Object *obj,
			void *ei)
{
	GL_CHECK(data);
	if (_gl_data_selected_list_count(data) == 0) {
		gl_dbgW("No thumbs selected!");
		gl_popup_create_popup(data, GL_POPUP_NOBUT,
		                      GL_STR_NO_FILES_SELECTED);
		return;
	}
	_gl_ctxpopup_del(data);
	_gl_rotate_images(data, false);
}
#endif

static int __gl_thumbs_edit_ctxpopup_append(void *data, Evas_Object *parent)
{
	gl_dbg("");
	GL_CHECK_VAL(parent, -1);
	GL_CHECK_VAL(data, -1);
	gl_appdata *ad = (gl_appdata *)data;
	int share_m = gl_get_share_mode(ad);

	switch (ad->gridinfo.grid_type) {
	case GL_GRID_T_LOCAL:
	case GL_GRID_T_ALLALBUMS: /* BTN: More - Delete/Move/Cancel */
		/* More : Edit image/Add tag/Copy/Create collage/Hide items/Rotate left/Rotate right/Crete a story album*/
		/* Image-editor is available if only images are selected */
		if (share_m >= GL_SHARE_IMAGE_ONE &&
		        share_m <= GL_SHARE_IMAGE_MULTI_JPEG)
			_gl_ctxpopup_append(parent, GL_STR_ID_EDIT_IMAGE,
			                    __gl_thumbs_edit_ie_cb, data);
		_gl_ctxpopup_append(parent, GL_STR_ID_COPY,
		                    __gl_thumbs_edit_copy_cb, data);
#ifdef _USE_ROTATE_BG
		/* Rotation is available if only images are selected */
		if (share_m == GL_SHARE_IMAGE_ONE_JPEG ||
		        share_m == GL_SHARE_IMAGE_MULTI_JPEG) {
			_gl_ctxpopup_append(parent, GL_STR_ID_ROTATE_LEFT,
			                    __gl_thumbs_edit_rotate_left_cb,
			                    data);
			_gl_ctxpopup_append(parent, GL_STR_ID_ROTATE_RIGHT,
			                    __gl_thumbs_edit_rotate_right_cb,
			                    data);
		}
#endif
		break;
	default:
		return -1;
	}
	return 0;
}

static void __gl_thumbs_edit_more_cb(void *data, Evas_Object *obj,
                                     void *ei)
{
	/* Add items */
	GL_CHECK(data);
	gl_appdata *ad = (gl_appdata *)data;
	Evas_Object *more = NULL;
	more = _gl_ui_get_btn(NULL, ad->gridinfo.nf_it, GL_NAVIFRAME_MORE);
	if (EINA_TRUE == elm_object_disabled_get(more)) {
		gl_dbg("Menu is disabled");
		return; /* Menu is disabled */
	}

	gl_dbg("Menu is clicked");
	_gl_ctxpopup_create(data, obj, __gl_thumbs_edit_ctxpopup_append);
}
#endif

static void __gl_thumbs_edit_del(void *data)
{
	GL_CHECK(data);
	gl_appdata *ad = (gl_appdata *)data;
	gl_dbg("");

	if (ad->uginfo.ug || ad->uginfo.b_app_called) {
		gl_dbgW("UG or APP is already loaded!");
		return;
	}

	int view_mode = gl_get_view_mode(ad);
	if (view_mode == GL_VIEW_THUMBS_EDIT) {
		int cnt = _gl_data_selected_list_count(ad);
		if (cnt == 0) {
			gl_dbgW("No thumbs selected!");
			gl_popup_create_popup(ad, GL_POPUP_NOBUT, GL_STR_NO_FILES_SELECTED);
			return;
		}

		_gl_popup_create_del(data, GL_POPUP_DEL_FILE,
		                     _gl_data_selected_list_count,
		                     gl_del_medias, data);
	} else {
		gl_dbgW("Unknow mode!");
	}
}

static void __gl_thumbs_edit_copy_move(void *data)
{
	GL_CHECK(data);
	gl_appdata *ad = (gl_appdata *)data;
	gl_dbg("");

	if (ad->uginfo.ug || ad->uginfo.b_app_called) {
		gl_dbgW("UG or APP is already loaded!");
		return;
	}

	int view_mode = gl_get_view_mode(ad);
	if (view_mode == GL_VIEW_THUMBS_EDIT) {
		int cnt = _gl_data_selected_list_count(ad);
		if (cnt == 0) {
			gl_dbgW("No thumbs selected!");
			gl_popup_create_popup(ad, GL_POPUP_NOBUT, GL_STR_NO_FILES_SELECTED);
			return;
		}

		ad->gridinfo.it = NULL;
		_gl_popup_create_copy_move(data, _gl_data_selected_list_count, data);
	} else {
		gl_dbgW("Unknow mode!");
	}
}

static void __gl_thumbs_edit_cancel_cb(void *data, Evas_Object *obj, void *ei)
{
	elm_toolbar_item_selected_set((Elm_Object_Item *)ei, EINA_FALSE);
	GL_CHECK(data);
	gl_dbg("");
	_gl_thumbs_edit_pop_view(data);
}

static void __gl_thumbs_edit_done_cb(void *data, Evas_Object *obj, void *ei)
{
	elm_toolbar_item_selected_set((Elm_Object_Item *)ei, EINA_FALSE);
	GL_CHECK(data);
	gl_appdata *ad = (gl_appdata *)data;

	if (_gl_thumbs_get_edit_mode(data) == GL_THUMBS_EDIT_COPY || _gl_thumbs_get_edit_mode(data) == GL_THUMBS_EDIT_MOVE) {
		__gl_thumbs_edit_copy_move(ad);
	} else {
		__gl_thumbs_edit_del(ad);
	}
}

/* Select-all checkbox selected/deselected */
static void __gl_thumbs_edit_selall_cb(void *data, Evas_Object *obj, void *ei)
{
	GL_CHECK(data);
	gl_appdata *ad = (gl_appdata *)data;
	int i = 0;
	int item_cnt = 0;
	int sel_all_cnt = 0;
	int view_mode = gl_get_view_mode(ad);
	gl_dbg("view_mode: %d.", view_mode);

	ad->selinfo.ck_state = !elm_check_state_get(ad->selinfo.selectall_ck);
	if (ad->selinfo.selectall_ck) {
		elm_check_state_set(ad->selinfo.selectall_ck, ad->selinfo.ck_state);
	}
	gl_dbg("Checkbox state: %d", ad->selinfo.ck_state);
	Eina_List *medias_elist = NULL;
	_gl_thumbs_get_list(ad, &(medias_elist));
	/* Get all medias count of current album */
	item_cnt = eina_list_count(medias_elist);
	sel_all_cnt = item_cnt;
	gl_item *gitem = NULL;
	for (i = 0; i < item_cnt; i++) {
		gitem = eina_list_nth(medias_elist, i);
		GL_CHECK(gitem);
		GL_CHECK(gitem->item);

		if (gitem->checked == ad->selinfo.ck_state) {
			continue;
		}

		gitem->checked = ad->selinfo.ck_state;
		if (ad->selinfo.ck_state == EINA_TRUE) {
			_gl_data_selected_list_append(ad, gitem);
			gitem->album->elist = eina_list_append(gitem->album->elist, gitem);
		} else {
			_gl_data_selected_list_remove(ad, gitem);
			gitem->album->elist = eina_list_remove(gitem->album->elist, gitem);
		}
	}
	/* Update all realized items */
	if (view_mode == GL_VIEW_THUMBS_EDIT)
		_gl_ui_update_realized_grid_ck(ad->gridinfo.view,
		                               GL_THUMB_CHECKBOX, NULL,
		                               ad->selinfo.ck_state);
	/* Recreate selection info for both cases */
	if (ad->selinfo.ck_state) {
		item_cnt = sel_all_cnt;
	} else {
		sel_all_cnt = 0;
	}

	int total_selected_count = eina_list_count(ad->selinfo.elist);
	_gl_notify_check_selall(ad, ad->gridinfo.nf_it, item_cnt, sel_all_cnt);
	if (gitem->album->item) {
		char buf[GL_ALBUM_NAME_LEN_MAX] = { 0, };
		if (sel_all_cnt > 0) {
			elm_object_item_signal_emit(gitem->album->item,
			                            "elm,state,elm.text.badge,visible",
			                            "elm");
			snprintf(buf, sizeof(buf), "%d", sel_all_cnt);
		} else {
			elm_object_item_signal_emit(gitem->album->item,
			                            "elm,state,elm.text.badge,hidden",
			                            "elm");
		}
		elm_gengrid_item_fields_update(gitem->album->item, "elm.text.badge", ELM_GENGRID_ITEM_FIELD_TEXT);
	}
	/* Update the label text */
	_gl_thumbs_update_label_text(ad->gridinfo.nf_it, total_selected_count, false);
}

#ifdef _USE_APP_SLIDESHOW
static void __gl_thumbs_edit_selected_slideshow_cb(void *data, Evas_Object *obj,
			void *ei)
{
	elm_toolbar_item_selected_set((Elm_Object_Item *)ei, EINA_FALSE);
	GL_CHECK(data);
	gl_appdata *ad = (gl_appdata *)data;
	gl_item *cur_item = NULL;
	media_content_type_e type = MEDIA_CONTENT_TYPE_OTHERS;

	_gl_data_get_first_item(type, ad->selinfo.elist, &cur_item);
	GL_CHECK_VAL(cur_item, -1);
	gl_ext_load_iv_ug(ad, cur_item, GL_UG_IV_SLIDESHOW_SELECTED);
}
#endif

#ifdef GL_EXTENDED_FEATURES
int _gl_thumbs_edit_add_more(void *data, Evas_Object *parent,
                             Elm_Object_Item *nf_it)
{
	Evas_Object *btn = NULL;
	/* More */
	btn = _gl_but_create_but(parent, NULL, NULL, GL_BUTTON_STYLE_NAVI_MORE,
	                         __gl_thumbs_edit_more_cb, data);
	GL_CHECK_VAL(btn, -1);

	elm_object_item_part_content_set(nf_it, GL_NAVIFRAME_MORE, btn);
	_gl_ui_disable_btn(btn);
	return 0;
}
#endif

/* Delete/Copy/Move -- unavailable for Facebook(Facebook Team confirmed) */
int _gl_thumbs_edit_add_btns(void *data, Evas_Object *parent,
                             Elm_Object_Item *nf_it)
{
	GL_CHECK_VAL(nf_it, -1);
	GL_CHECK_VAL(parent, -1);
	GL_CHECK_VAL(data, -1);
	gl_appdata *ad = (gl_appdata *)data;

	_gl_ui_clear_btns(data);

	int w = -1, h = -1;
	evas_object_geometry_get(ad->maininfo.win, NULL, NULL, &w, &h);
	Edje_Message_Int *msg = (Edje_Message_Int *)malloc(sizeof(Edje_Message_Int) + sizeof(int));
	GL_CHECK_VAL(msg, -1);
	if (ad->gridinfo.split_view_mode == DETAIL_VIEW) {
		msg->val = w;
	} else {
		msg->val = ((w < h) ? (w - (w / 3)) : (w - (h / 3)));
	}
	edje_object_message_send(elm_layout_edje_get(ad->gridinfo.layout), EDJE_MESSAGE_INT, 2, msg);
	free(msg);
	elm_object_signal_emit(ad->gridinfo.layout, "elm,selectall,state,visible", "elm");
	_gl_ui_add_selall_ck(ad->gridinfo.layout, "select.all.area.check", "select.all.area.check",
	                     __gl_thumbs_edit_selall_cb, data);

#ifdef _USE_APP_SLIDESHOW
	/*only display the done button,ux:[Tizen] Gallery UI(T01)_v5.2_20130524*/
	if (_gl_thumbs_get_edit_mode(data) == GL_THUMBS_EDIT_SLIDESHOW) {
		gl_dbg("only display the done button for selected slideshow");
		tb_it = _gl_ctrl_append_item(toolbar, NULL, GL_STR_ID_START,
		                             __gl_thumbs_edit_selected_slideshow_cb,
		                             data);
		_gl_ctrl_disable_item(tb_it, true);
		/* elm_object_item_part_content_set(nf_it, "toolbar", toolbar); */
		return 0;
	}
#endif

#ifdef GL_EXTENDED_FEATURES
	/********* SHARE ***********/
	if (_gl_thumbs_get_edit_mode(data) == GL_THUMBS_EDIT_SHARE) {
		/* BTN: Cancel/Share */
		/* Cancel */
		Evas_Object *btn1 = elm_button_add(layout);
		Evas_Object *btn2 = elm_button_add(layout);
		elm_object_style_set(btn1, "transparent");
		elm_object_style_set(btn2, "transparent");
		evas_object_size_hint_weight_set(btn1, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
		evas_object_size_hint_weight_set(btn2, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
		evas_object_size_hint_align_set(btn1, 1, 1);
		evas_object_size_hint_align_set(btn2, 1, 1);
		evas_object_show(btn1);
		evas_object_show(btn2);
		elm_object_part_content_set(layout, "elm.swallow.left", btn1);
		elm_object_part_content_set(layout, "elm.swallow.right", btn2);
		evas_object_smart_callback_add(btn1, "clicked", __gl_thumbs_edit_cancel_cb, ad);
		evas_object_smart_callback_add(btn2, "clicked", __gl_thumbs_edit_share_cb, ad);
		_gl_ui_set_translate_part_str(layout, "title_left_text", GL_STR_ID_CANCEL);
		_gl_ui_set_translate_part_str(layout, "title_right_text", GL_STR_ID_SHARE);
		_gl_ctrl_append_item(toolbar, NULL, GL_STR_ID_CANCEL,
		                     __gl_thumbs_edit_cancel_cb, data);
		/* Share - to launch Share panel(app) */
		tb_it = _gl_ctrl_append_item(toolbar, NULL, GL_STR_ID_SHARE,
		                             __gl_thumbs_edit_share_cb, data);
		_gl_ctrl_disable_item(tb_it, true);
		/* elm_object_item_part_content_set(nf_it, "toolbar", toolbar); */
		return 0;
	}
#endif

	Evas_Object *btn1 = elm_button_add(parent);
	Evas_Object *btn2 = elm_button_add(parent);
	GL_CHECK_VAL(btn1, -1);
	GL_CHECK_VAL(btn2, -1);
	/* Title Cancel Button */
	elm_object_style_set(btn1, "naviframe/title_left");
	elm_object_item_part_content_set(nf_it, GL_NAVIFRAME_TITLE_LEFT_BTN, btn1);
	_gl_ui_set_translate_str(btn1, GL_STR_ID_CANCEL_CAP);
	evas_object_smart_callback_add(btn1, "clicked", __gl_thumbs_edit_cancel_cb, ad);

	/* Title Done Button */
	elm_object_style_set(btn2, "naviframe/title_right");
	elm_object_item_part_content_set(nf_it, GL_NAVIFRAME_TITLE_RIGHT_BTN, btn2);
	_gl_ui_set_translate_str(btn2, GL_STR_ID_DONE_CAP);
	evas_object_smart_callback_add(btn2, "clicked", __gl_thumbs_edit_done_cb, ad);
	elm_object_disabled_set(btn2, EINA_TRUE);

	return 0;
}

int _gl_thumbs_edit_create_view(void *data)
{
	GL_CHECK_VAL(data, -1);
	gl_appdata *ad = (gl_appdata *)data;
	int view_mode = gl_get_view_mode(data);
	gl_dbg("mode: %d", view_mode);

	if (view_mode == GL_VIEW_THUMBS_EDIT &&
	        _gl_thumbs_get_edit_mode(data) < GL_THUMBS_EDIT_FIRST) {
		/* Fixme: maybe we should disable edit button until grid append idler is done */
		/**
		* Happen in quickly tap edit button,
		* it will invoke edit_cb several time
		* and push some unuseful views.
		*/
		gl_dbg("Already in edit mode, return.");
		return -1;
	}

	ad->albuminfo.file_mc_mode = GL_MC_NONE;

	gl_set_view_mode(ad, GL_VIEW_THUMBS_EDIT);
	Evas_Object *gv = NULL;
	bool b_view = false;
	b_view = _gl_thumbs_get_view(ad, &gv);
	gl_dbg("no content view is : %d.", b_view);
	GL_CHECK_FALSE(gv);
	_gl_thumbs_set_size(ad, gv);

	_gl_thumbs_edit_add_btns(data, ad->maininfo.naviframe,
	                         ad->gridinfo.nf_it);
	/* Update the label text */
	_gl_thumbs_update_label(ad->gridinfo.nf_it, GL_STR_ID_SELECT_ITEM);
	/* Update realized items */
	if (_gl_thumbs_get_edit_mode(data) < GL_THUMBS_EDIT_FIRST)
#ifdef _USE_GRID_CHECK
		_gl_show_grid_checks(ad->gridinfo.view, GL_THUMB_CHECKBOX);
#else
		_gl_thumbs_update_realized_items(data);
#endif
	_gl_thumb_update_split_view(data);
	gl_dbg("Done");
	return 0;
}

/* Get new data and refresh view */
int _gl_thumbs_edit_update_view(void *data)
{
	GL_CHECK_VAL(data, -1);
	gl_appdata *ad = (gl_appdata *)data;
	gl_sdbg("grid_type: %d", ad->gridinfo.grid_type);

	switch (ad->gridinfo.grid_type) {
	case GL_GRID_T_LOCAL:
	case GL_GRID_T_ALLALBUMS:
		/* Albums list should be updated first */
		_gl_thumbs_update_items(ad);
		break;
	default:
		gl_dbgE("Wrong grid type!");
		return -1;
	}
	return 0;
}

int _gl_thumbs_edit_pop_view(void *data)
{
	GL_CHECK_VAL(data, -1);
	gl_appdata *ad = (gl_appdata *)data;

	/* Make sure it's in correct naviframe item, to pop upside naviframe items */
	elm_naviframe_item_pop_to(ad->gridinfo.nf_it);

	_gl_data_selected_list_finalize(data);
	/* Clear buttons for edit view first */
	_gl_ui_clear_btns(data);
	_gl_thumbs_set_edit_mode(data, GL_THUMBS_EDIT_NONE);

	gl_set_view_mode(data, GL_VIEW_THUMBS);
	if (_gl_thumbs_get_edit_mode(data) < GL_THUMBS_EDIT_FIRST) {
		/* Update the label text */
		_gl_thumbs_update_label(ad->gridinfo.nf_it, ad->gridinfo.title);
		/* enable the grid pinch */
		edje_object_signal_emit(_EDJ(ad->gridinfo.layout),
		                        GL_SIGNAL_GESTURE_DEFAULT, "elm");

		elm_object_signal_emit(ad->gridinfo.layout, "elm,selectall,state,default", "elm");
#ifdef _USE_GRID_CHECK
		_gl_show_grid_checks(ad->gridinfo.view, GL_THUMB_CHECKBOX);
#else
		_gl_thumbs_update_realized_items(data);
#endif
		_gl_thumbs_add_btns(data);
		ad->gridinfo.update_split_view = true;
		_gl_thumb_update_split_view(data);
		if (_gl_thumbs_check_zero(data)) {
			gl_dbg("show nocontents");
			_gl_thumbs_show_nocontents(ad);
		}
	}
	return 0;
}

int _gl_thumbs_edit_update_lang(void *data)
{
	GL_CHECK_VAL(data, -1);
	gl_appdata *ad = (gl_appdata *)data;
	GL_CHECK_VAL(ad->gridinfo.nf_it, -1);
	int cnt = _gl_data_selected_list_count(ad);
	/* Update the label text */
	if (cnt == 0) {
		_gl_ui_change_navi_title(ad->gridinfo.nf_it, GL_STR_ID_SELECT_ITEM, true);
	}
	_gl_thumbs_update_label_text(ad->gridinfo.nf_it, cnt, true);
	return 0;
}

/*thumbnail view: more-share/delete-Download */
/* Delete in unavaliable for Facebook */
/* Download only for Video files in Cloud album */
int _gl_thumbs_edit_disable_btns(void *data, bool b_disable)
{
	GL_CHECK_VAL(data, -1);
	gl_appdata *ad = (gl_appdata *)data;
	bool b_disable_more = false; /* No permission for FB */
	gl_dbg("b_disable: %d", b_disable);
	Elm_Object_Item *nf_it = elm_naviframe_top_item_get(ad->maininfo.naviframe);

	if (!b_disable) {
#ifdef _USE_APP_SLIDESHOW
		if (_gl_thumbs_get_edit_mode(data) == GL_THUMBS_EDIT_SLIDESHOW) {
			_gl_ctrl_disable_items(nf_it, false);
		} else
#endif

			_gl_ctrl_disable_items(nf_it, false);
		gl_dbg("b_disable_more: %d", b_disable_more);
		_gl_ui_disable_menu(nf_it, b_disable_more);
	} else {
		_gl_ui_disable_menu(nf_it, true);
		_gl_ctrl_disable_items(nf_it, true);
	}
	return 0;
}

