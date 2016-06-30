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
#include "gl-albums-sel.h"
#include "gl-ui-util.h"
#include "gl-util.h"
#include "gl-data.h"
#include "gl-controlbar.h"
#include "gl-button.h"
#include "gl-nocontents.h"
#include "gl-notify.h"
#include "gl-strings.h"
#include "gl-albums.h"
#include "gl-timeline.h"
#include "gl-thumbs.h"
#include "gl-tile.h"
#include "gl-editfield.h"

static void _gl_albums_sel_update_album_sel_list(void *data);

static void __gl_albums_sel_trans_finished_cb(void *data, Evas_Object *obj,
			void *event_info)
{
	GL_CHECK(data);
	gl_appdata *ad = (gl_appdata *)data;
	int view_mode = gl_get_view_mode(ad);
	gl_dbgW("view_mode: %d", view_mode);
	evas_object_smart_callback_del(obj, GL_TRANS_FINISHED,
	                               __gl_albums_sel_trans_finished_cb);

	/* Clear previous view after animation finished */
	if (view_mode == GL_VIEW_ALBUMS_SELECT) {
		if (ad->gridinfo.nocontents == NULL) {
			elm_gengrid_clear(ad->gridinfo.view);
		}
	} else {
		gl_dbgE("Wrong view mode!");
	}
}

/* callback after user tap Cancel button in option header in select album view */
static Eina_Bool __gl_albums_sel_pop_cb(void *data, Elm_Object_Item *it)
{
	GL_CHECK_FALSE(data);
	gl_dbg("");
	gl_appdata *ad = (gl_appdata *)data;
	_gl_data_selected_list_finalize(data);
	/* If the back button is clicked or H/W Back key is pressed,
	this callback will be called right before the
	current view pop. User may implement application logic here. */

	/* To proceed the popping, please return the EINA_TRUE or not,
	popping will be canceled.
	If it needs to delete the current view without any transition effect,
	then call the elm_object_item_del() here and then return the EINA_FALSE */
	_gl_albums_sel_pop_view(data);
	_gl_albums_sel_update_album_sel_list(data);
	if (ad->albuminfo.selected_uuid) {
		free(ad->albuminfo.selected_uuid);
		ad->albuminfo.selected_uuid = NULL;
	}
	return EINA_TRUE;
}

/**
 *  Use naviframe api to push new album view to stack.
 *  @param obj is the content to be pushed.
 */
static int __gl_albums_sel_push_view(void *data, Evas_Object *parent,
                                     Evas_Object *obj, char *title)
{
	gl_dbg("GL_NAVI_ALBUMS_SELECT");
	GL_CHECK_VAL(obj, -1);
	GL_CHECK_VAL(parent, -1);
	GL_CHECK_VAL(data, -1);
	gl_appdata *ad = (gl_appdata *)data;
	Elm_Object_Item *nf_it = NULL;

	/* Add transition finished callback */
	evas_object_smart_callback_add(parent, GL_TRANS_FINISHED,
	                               __gl_albums_sel_trans_finished_cb, data);

	/* Push to stack */
	nf_it = elm_naviframe_item_push(parent, title, NULL, NULL, obj,
	                                NULL);
	/* use pop_cb_set to support HW key event */
	elm_naviframe_item_pop_cb_set(nf_it, __gl_albums_sel_pop_cb, data);
	ad->albuminfo.nf_it_select = nf_it;
	return 0;
}

/* Free data after layout deleted */
static void __gl_albums_sel_delete_layout_cb(void *data, Evas *e,
			Evas_Object *obj, void *ei)
{
	gl_dbg("Delete layout ---");
	GL_CHECK(data);
	gl_appdata *ad = (gl_appdata *)data;

	_gl_albums_clear_cbs(ad->albuminfo.select_view);
	_gl_ui_del_scroller_pos(ad->albuminfo.select_view);

	ad->albuminfo.b_new_album = false;
	ad->albuminfo.select_view = NULL;
	ad->albuminfo.select_layout = NULL;
	ad->albuminfo.nf_it_select = NULL;
	gl_dbg("Delete layout +++");
}

Evas_Object *_gl_albums_create_sel_gengrid(void *data)
{
	GL_CHECK_VAL(data, NULL);
	gl_appdata *ad = (gl_appdata *)data;
	Evas_Object *layout_inner = elm_gengrid_add(ad->maininfo.win);

	elm_gengrid_align_set(layout_inner, 0.5f, 0.0);
	elm_gengrid_horizontal_set(layout_inner, EINA_FALSE);
	elm_scroller_bounce_set(layout_inner, EINA_FALSE, EINA_TRUE);
	elm_scroller_policy_set(layout_inner, ELM_SCROLLER_POLICY_OFF,
	                        ELM_SCROLLER_POLICY_AUTO);
	elm_gengrid_multi_select_set(layout_inner, EINA_TRUE);
	evas_object_size_hint_weight_set(layout_inner, EVAS_HINT_EXPAND,
	                                 EVAS_HINT_EXPAND);
	return layout_inner;
}

int _gl_albums_create_split_album_sel_view(void *data, Evas_Object *layout_inner)
{
	GL_CHECK_VAL(data, -1);
	gl_appdata *ad = (gl_appdata *)data;
	if (_gl_split_albums_create_items(ad, layout_inner) < 0) {
		gl_dbgE("Failed to add Split album view!");
		return -1;
	}
	int w = -1, h = -1;
	evas_object_geometry_get(ad->maininfo.win, NULL, NULL, &w, &h);
	if (w < h) {
		elm_gengrid_item_size_set(layout_inner, (w / 3) - 10, (w / 3) - 10);
	} else {
		elm_gengrid_item_size_set(layout_inner, (h / 3) - 10, (h / 3) - 10);
	}
	return 0;
}

int _gl_albums_sel_create_view(void *data)
{
	GL_CHECK_VAL(data, -1);
	gl_appdata *ad = (gl_appdata *)data;
	GL_CHECK_VAL(ad->maininfo.naviframe, -1);
	gl_dbg("");

	/* Create tags view layout for Select album */
	Evas_Object *layout = NULL;
	layout = _gl_ctrl_add_layout(ad->maininfo.naviframe);
	GL_CHECK_VAL(layout, -1);
	evas_object_size_hint_weight_set(layout, EVAS_HINT_EXPAND,
	                                 EVAS_HINT_EXPAND);
	evas_object_size_hint_align_set(layout, EVAS_HINT_FILL, EVAS_HINT_FILL);
	/* Register delete callback */
	evas_object_event_callback_add(layout, EVAS_CALLBACK_DEL,
	                               __gl_albums_sel_delete_layout_cb, data);
	/* Save view mode */
	int pre_view_m = gl_get_view_mode(data);
	/* Set view mode of Select album */
	gl_set_view_mode(data, GL_VIEW_ALBUMS_SELECT);
	Evas_Object *view = NULL;
	/* Create tags view for Select album */
	view = _gl_albums_create_sel_gengrid(data);
	if (view == NULL) {
		evas_object_del(layout);
		/* Reset view mode */
		gl_set_view_mode(data, pre_view_m);
		gl_dbgW("None albums!");
		/* Add notification */
		_gl_notify_create_notiinfo(GL_STR_ADDED);
		return -1;
	}
	if (_gl_albums_create_split_album_sel_view(ad, view) == -1) {
		gl_dbgE("Failed to add album split view!");
		evas_object_del(layout);
		/* Reset view mode */
		gl_set_view_mode(data, pre_view_m);
		/* Add notification */
		_gl_notify_create_notiinfo(GL_STR_ADDED);
		return -1;
	}

	int length = eina_list_count(ad->albuminfo.elist->clist);
	gl_cluster *album_item = NULL;
	gl_dbg("Albums length: %d", length);
	if (length > 0) {
		album_item  = eina_list_nth(ad->albuminfo.elist->clist, 0);
		if (!album_item) {
			return -1;
		}
	} else {
		view = NULL;
		evas_object_del(layout);
		/* Reset view mode */
		gl_set_view_mode(data, pre_view_m);
		gl_dbgW("None albums!");
		/* Add notification */
		_gl_notify_create_notiinfo(GL_STR_ADDED);
		return -1;
	}
	__gl_albums_new_album_sel(album_item, view, (void *)album_item->item);

	/* Set view to layout */
	int w = -1, h = -1;
	evas_object_geometry_get(ad->maininfo.win, NULL, NULL, &w, &h);
	Edje_Message_Int *msg = (Edje_Message_Int *)malloc(sizeof(Edje_Message_Int) + sizeof(int));
	GL_CHECK_VAL(msg, -1);
	msg->val = ((w < h) ? (w / 3) : (h / 3));
	edje_object_message_send(elm_layout_edje_get(layout), EDJE_MESSAGE_INT, 1, msg);
	free(msg);
	elm_object_signal_emit(layout, "elm,splitview,state,visible", "elm");

	if (w < h) {
		elm_gengrid_item_size_set(view, (w / 3) - 10, (w / 3) - 10);
	} else {
		elm_gengrid_item_size_set(view, (h / 3) - 10, (h / 3) - 10);
	}
	elm_object_part_content_set(layout, "split.view", view);
	Evas_Object *gesture = _gl_tile_add_gesture(data, layout);
	if (gesture == NULL) {
		gl_dbgE("Failed to create gesture!");
	} else {
		elm_object_part_content_set(layout, "gesture", gesture);
	}
	ad->albuminfo.select_view = view;
	ad->albuminfo.select_layout = layout;
	__gl_albums_sel_push_view(data, ad->maininfo.naviframe, layout,
	                          "Select Item");
	__gl_albums_new_album_sel(album_item, view, (void *)album_item->item);
	/* Save pointer of View and Layout */
	elm_object_part_content_set(layout, "elm.swallow.view", ad->gridinfo.select_view);
	ad->albuminfo.select_view = view;
	ad->albuminfo.select_layout = layout;
	_gl_editfield_destroy_imf(data);
	gl_dbg("Done");
	return 0;
}

int _gl_albums_sel_update_view(void *data)
{
	GL_CHECK_VAL(data, -1);
	gl_appdata *ad = (gl_appdata *)data;
	/* Append griditems */
	_gl_albums_create_items(data, ad->albuminfo.select_view);
	return 0;
}

/* callback after user tap Cancel button in option header in select album view */
int _gl_albums_sel_pop_view(void *data)
{
	GL_CHECK_VAL(data, -1);
	gl_appdata *ad = (gl_appdata *)data;

	/* In Albums TAB */
	if (_gl_ctrl_get_tab_mode(ad) == GL_CTRL_TAB_ALBUMS) {
		elm_naviframe_item_pop_to(ad->ctrlinfo.nf_it);
		_gl_albums_close_album(ad);
	} else if (_gl_ctrl_get_tab_mode(ad) == GL_CTRL_TAB_TIMELINE) {
		elm_naviframe_item_pop_to(ad->ctrlinfo.nf_it);
		gl_set_view_mode(data, GL_VIEW_TIMELINE);
		_gl_timeline_update_view(ad);
	}
	return 0;
}

int _gl_albums_sel_rotate_view(void *data)
{
	GL_CHECK_VAL(data, -1);
	return 0;
}

static void  _gl_albums_sel_update_album_sel_list(void *data)
{
	GL_CHECK(data);
	gl_appdata *ad = (gl_appdata *)data;
	GL_CHECK(ad->albuminfo.elist);
	GL_CHECK(ad->albuminfo.elist->clist);
	gl_cluster *album_item = NULL;
	gl_item *item = NULL;
	int length, i;
	Eina_List *clist = ad->albuminfo.elist->clist;
	length = eina_list_count(clist);

	for (i = 0; i < length; i++) {
		album_item = eina_list_nth(clist, i);
		if (album_item && album_item->elist) {
			EINA_LIST_FREE(album_item->elist, item) {
				if (item) {
					item->checked = false;
				}
			}
		}
	}
}

