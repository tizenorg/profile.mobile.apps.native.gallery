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
#include "gl-thumbs.h"
#include "gl-thumbs-edit.h"
#include "gl-ui-util.h"
#include "gl-util.h"
#include "gl-albums.h"
#include "gl-albums-sel.h"
#include "gl-data.h"
#include "gl-ext-ug-load.h"
#include "gl-ext-exec.h"
#include "gl-pinchzoom.h"
#include "gl-progressbar.h"
#include "gl-controlbar.h"
#include "gl-nocontents.h"
#include "gl-notify.h"
#include "gl-strings.h"
#include "gl-icons.h"
#include "gl-thumb.h"
#include "gl-button.h"
#include "gl-ctxpopup.h"
#include "gl-editfield.h"
#include "gl-fs.h"
#include "gl-gesture.h"
#include "gl-popup.h"
#include "gl-file-util.h"

static void __gl_thumbs_check_changed(void *data, Evas_Object *obj, void *ei);
void _gl_thumbs_open_file(void *data)
{
	gl_dbg("");
	GL_CHECK(data);
	gl_item *gitem = (gl_item *)data;
	GL_CHECK(gitem->item);
	GL_CHECK(gitem->item->file_url);
	GL_CHECK(strlen(gitem->item->file_url));
	GL_CHECK(gitem->ad);
	gl_appdata *ad = (gl_appdata *)gitem->ad;
	int view_mode = gl_get_view_mode(ad);
	if (view_mode != GL_VIEW_THUMBS) {
		gl_dbgE("Error view mode!");
		return;
	}

	gl_dbg("Loading UG-IMAGE-VIEWER");
	gl_ext_load_iv_ug(ad, gitem, GL_UG_IV);
}

void _gl_thumbs_open_file_select_mode(void *data)
{
	gl_dbg("");
	GL_CHECK(data);
	gl_item *gitem = (gl_item *)data;
	GL_CHECK(gitem->item);
	GL_CHECK(gitem->item->file_url);
	GL_CHECK(strlen(gitem->item->file_url));
	GL_CHECK(gitem->ad);
	gl_appdata *ad = (gl_appdata *)gitem->ad;

	gl_dbg("Loading UG-IMAGE-VIEWER-SELECT-MODE");
	gl_ext_load_iv_ug_select_mode(ad, gitem->item, GL_UG_IV);
}

static int __gl_thumbs_change_check(void *data, Elm_Object_Item *it)
{
	GL_CHECK_VAL(it, -1);
	GL_CHECK_VAL(data, -1);
	Evas_Object *ck = NULL;
	gl_dbg("");

	ck = elm_object_item_part_content_get(it, GL_THUMB_CHECKBOX);
	GL_CHECK_VAL(ck, -1);

	elm_check_state_set(ck, !elm_check_state_get(ck));
	__gl_thumbs_check_changed(data, ck, NULL);
	return 0;
}

static void __gl_thumbs_sel_cb(void *data, Evas_Object *obj, void *ei)
{
	GL_CHECK(data);
	gl_item *gitem = (gl_item *)data;
	GL_CHECK(gitem->ad);
	gl_appdata *ad = (gl_appdata *)gitem->ad;
	int view_mode = gl_get_view_mode(ad);

	elm_gengrid_item_selected_set((Elm_Object_Item *)ei, EINA_FALSE);
	if (view_mode != GL_VIEW_THUMBS) {
		__gl_thumbs_change_check(data, (Elm_Object_Item *)ei);
		return;
	}
	/* Save scroller position before clearing gengrid */
	_gl_ui_save_scroller_pos(obj);
	_gl_thumbs_open_file(data);
}

static void __gl_thumbs_realized(void *data, Evas_Object *obj, void *ei)
{
	gl_dbg("realized");
	GL_CHECK(ei);
	GL_CHECK(data); /* It's ad */
	Elm_Object_Item *it = (Elm_Object_Item *)ei;
	Elm_Gengrid_Item_Class *gic = NULL;
	gic = evas_object_data_get(obj, "gl_thumbs_item_style_key");
	/* Do nothing for date item */
	if (elm_gengrid_item_item_class_get(it) != gic) {
		return;
	}

	gl_item *gitem = elm_object_item_data_get(it);
	GL_CHECK(gitem);
	GL_CHECK(gitem->item);
	/* Checking for local files only */
	if (gitem->store_type == GL_STORE_T_MMC ||
	        gitem->store_type == GL_STORE_T_ALL ||
	        gitem->store_type == GL_STORE_T_PHONE) {
		/* Use default image */
		if (!GL_FILE_EXISTS(gitem->item->thumb_url)) {
			_gl_thumbs_create_thumb(gitem);
		}
	}
}

static void __gl_thumbs_unrealized(void *data, Evas_Object *obj, void *ei)
{
	gl_dbg("unrealized");
	GL_CHECK(ei);
	GL_CHECK(data); /* It's ad */
	Elm_Object_Item *it = (Elm_Object_Item *)ei;
	Elm_Gengrid_Item_Class *gic = NULL;
	gic = evas_object_data_get(obj, "gl_thumbs_item_style_key");
	/* Do nothing for date item */
	if (elm_gengrid_item_item_class_get(it) != gic) {
		return;
	}

	gl_item *gitem = elm_object_item_data_get(it);
	GL_CHECK(gitem);
	GL_CHECK(gitem->item);
	/* Checking for local files only */
	if (gitem->store_type == GL_STORE_T_MMC ||
	        gitem->store_type == GL_STORE_T_ALL ||
	        gitem->store_type == GL_STORE_T_PHONE) {
		if (gitem->item->b_create_thumb) {
			_gl_data_cancel_thumb(gitem->item);
		}
	}
}

static void __gl_thumbs_longpressed_cb(void *data, Evas_Object *obj, void *ei)
{
	gl_dbg("longpressed");
	GL_CHECK(ei);
	Elm_Gengrid_Item_Class *gic = NULL;
	gic = evas_object_data_get(obj, "gl_thumbs_item_style_key");
	/* Do nothing for date item */
	if (elm_gengrid_item_item_class_get((Elm_Object_Item *)ei) != gic) {
		return;
	}
	if (GL_VIEW_THUMBS != gl_get_view_mode(data) ||
	        GL_CTRL_TAB_ALBUMS != _gl_ctrl_get_tab_mode(data)) {
		return;
	}

#if 0
	gl_item *gitem = (gl_item *)elm_object_item_data_get(ei);
	GL_CHECK(gitem);
	GL_CHECK(gitem->item);
	GL_CHECK(gitem->item->display_name);

	_gl_data_selected_list_finalize(data);
	_gl_data_selected_list_append(data, gitem);
	Eina_List *medias_elist = NULL;
	_gl_thumbs_get_list(data, &medias_elist);
	_gl_data_selected_add_burstshot(data, gitem, medias_elist);
#endif
}

static void __gl_thumbs_lang_changed(void *data, Evas_Object *obj, void *ei)
{
	GL_CHECK(obj);
	Eina_List *its = NULL;
	Eina_List *l = NULL;
	Elm_Object_Item *it = NULL;
	const Elm_Gengrid_Item_Class *itc = NULL;

	its = elm_gengrid_realized_items_get(obj);
	EINA_LIST_FOREACH(its, l, it) {
		itc = elm_gengrid_item_item_class_get(it);
		if (itc && itc->func.text_get) { /* Date item */
			elm_gengrid_item_update(it);
		}
		it = NULL;
	}
}

static void __gl_thumbs_check_changed(void *data, Evas_Object *obj, void *ei)
{
	GL_CHECK(obj);
	GL_CHECK(data);
	gl_item *gitem = (gl_item *)data;
	GL_CHECK(gitem->ad);
	GL_CHECK(gitem->album);
	gl_appdata *ad = (gl_appdata *)gitem->ad;
	int view_mode = gl_get_view_mode(ad);
	Elm_Object_Item *nf_it = NULL;
	int (*update_text)(Elm_Object_Item * nf_it, int sel_cnt, bool b_lang);
	gl_dbg("");

	if (view_mode == GL_VIEW_THUMBS_EDIT) {
		nf_it = ad->gridinfo.nf_it;
		update_text = _gl_thumbs_update_label_text;
	} else if (view_mode == GL_VIEW_THUMBS_SELECT) {
		nf_it = ad->albuminfo.nf_it_select;
		update_text = _gl_ui_update_navi_title_text;
	} else {
		return;
	}

	Eina_Bool checked = elm_check_state_get(obj);
	gitem->checked = checked;
	if (checked) {
		if (strcmp(gitem->album->cluster->uuid, GL_ALBUM_FAVOURITE_ID) == 0) {
			Eina_List *sel_list1 = ad->selinfo.fav_elist;
			sel_list1 = eina_list_append(sel_list1, gitem);
			ad->selinfo.fav_elist = sel_list1;
		} else {
			_gl_data_selected_list_append(ad, gitem);
			Eina_List *medias_elist = NULL;
			_gl_thumbs_get_list(ad, &medias_elist);
			_gl_data_selected_add_burstshot(ad, gitem, medias_elist);
		}
		gitem->album->elist = eina_list_append(gitem->album->elist, gitem);
	} else {
		gl_sdbg("Remove:%s", gitem->item->file_url);
		if (strcmp(gitem->album->cluster->uuid, GL_ALBUM_FAVOURITE_ID) == 0) {
			_gl_data_selected_fav_list_remove(ad, gitem);
		} else {
			_gl_data_selected_list_remove(ad, gitem);
		}
		gitem->album->elist = eina_list_remove(gitem->album->elist, gitem);
	}
	/* Display selectioninfo */
	int sel_cnt = _gl_data_selected_list_count(ad);
	int fav_sel_cnt = eina_list_count(ad->selinfo.fav_elist);
	int total_sel_count = sel_cnt + fav_sel_cnt;
	int album_select_count = eina_list_count(gitem->album->elist);
	if (gitem->album->item) {
		char buf[GL_ALBUM_NAME_LEN_MAX] = { 0, };
		if (album_select_count > 0) {
			elm_object_item_signal_emit(gitem->album->item,
			                            "elm,state,elm.text.badge,visible",
			                            "elm");
			snprintf(buf, sizeof(buf), "%d", album_select_count);
		} else {
			elm_object_item_signal_emit(gitem->album->item,
			                            "elm,state,elm.text.badge,hidden",
			                            "elm");
		}
#if 0
		elm_gengrid_item_fields_update(gitem->album->item, "elm.text.badge", ELM_GENGRID_ITEM_FIELD_TEXT);
#endif
	}
	_gl_notify_check_selall(ad, nf_it, ad->gridinfo.count, album_select_count);
	/* Update the label text */
	//if(ad->selinfo.fav_elist!=NULL)
	gl_sdbg("sel_cnt:%d", total_sel_count);
	update_text(nf_it, total_sel_count, false);

}

#ifdef _USE_SHRINK_EFFECT


static void __gl_thumbs_shrink_cb(void *data, Evas_Object *obj,
                                  const char *emission, const char *source)
{
	GL_CHECK(data);
	gl_item *gitem = (gl_item *) data;
	GL_CHECK(gitem->ad);
	gl_appdata *ad = (gl_appdata *)gitem->ad;
	gl_dbg("");

	_gl_thumbs_open_file(data);

	edje_object_signal_callback_del(obj, "shrink,expand,done", "bg",
	                                __gl_thumbs_shrink_cb);
}
#endif

/* Only for local medias */
static void __gl_thumbs_create_thumb_cb(media_content_error_e error,
                                        const char *path, void *user_data)
{
	GL_CHECK(user_data);
	gl_thumb_data_s *thumb_data = (gl_thumb_data_s *)user_data;
	GL_CHECK(thumb_data->ad);
	gl_appdata *ad = (gl_appdata *)thumb_data->ad;
	gl_item *gitem = thumb_data->item;
	GL_FREE(thumb_data);
	gitem->thumb_data = NULL;

	if (gl_get_view_mode(ad) != GL_VIEW_THUMBS &&
	        gl_get_view_mode(ad) != GL_VIEW_THUMBS_EDIT &&
	        gl_get_view_mode(ad) != GL_VIEW_THUMBS_SELECT) {
		return;
	}

	if (error == MEDIA_CONTENT_ERROR_NONE && GL_FILE_EXISTS(path) &&
	        g_strcmp0(path, GL_ICON_DB_DEFAULT_THUMB)) {
		GL_CHECK(gitem);
		GL_CHECK(gitem->item);
		gl_dbg("Update item with new thumb path[%s]", path);
		/* Update thumb path */
		GL_FREEIF(gitem->item->thumb_url);
		gitem->item->thumb_url = strdup(path);
		elm_gengrid_item_update(gitem->elm_item);
	} else {
		gl_dbgE("[%d]Invalid path[%s]!", error, path);
	}
}

/* Use file to create new thumb if possible */
int _gl_thumbs_create_thumb(gl_item *gitem)
{
	GL_CHECK_VAL(gitem, -1);
	GL_CHECK_VAL(gitem->item, -1);
	GL_CHECK_VAL(gitem->item->file_url, -1);

	if (GL_FILE_EXISTS(gitem->item->file_url)) {
		gl_thumb_data_s *thumb_data = NULL;
		thumb_data = (gl_thumb_data_s *)calloc(1, sizeof(gl_thumb_data_s));
		GL_CHECK_VAL(thumb_data, -1);
		thumb_data->ad = gitem->ad;
		thumb_data->item = gitem;
		gitem->thumb_data = thumb_data;
		_gl_data_create_thumb(gitem->item, __gl_thumbs_create_thumb_cb,
		                      thumb_data);
		return 0;
	}
	return -1;
}

/*
static void  __gl_thumbs_item_deleted(void *data, Evas_Object *obj)
{
	GL_CHECK(data);
	gl_item *gitem = (gl_item *)data;
	gl_dbg("[%p]", gitem);
	if (gitem->b_deleted) {
		gl_dbgW("[%p]b_deleted : %d", gitem, gitem->b_deleted);
		//_gl_data_free_burstshot_items(gitem,);
		_gl_data_util_free_gitem(gitem);
	}
}
*/

void open_image_in_select_mode(void *data, Evas_Object *obj, void *event_info)
{
	GL_CHECK(data);
	_gl_thumbs_open_file_select_mode(data);
}

static Evas_Object *__gl_thumbs_get_content(void *data, Evas_Object *obj,
        const char *part)
{
	GL_CHECK_NULL(part);
	GL_CHECK_NULL(strlen(part));
	GL_CHECK_NULL(data);
	gl_item *gitem = (gl_item *)data;
	GL_CHECK_NULL(gitem->item);
	GL_CHECK_NULL(gitem->ad);
	gl_appdata *ad = (gl_appdata *)gitem->ad;
	int view_mode = gl_get_view_mode(ad);

	if (!g_strcmp0(part, GL_THUMB_ICON)) {
		Evas_Object *layout = _gl_thumbs_get_content(ad, obj, gitem,
		                      ad->gridinfo.icon_w,
		                      ad->gridinfo.icon_h);

#ifdef _USE_SHRINK_EFFECT
		evas_object_event_callback_add(layout, EVAS_CALLBACK_MOUSE_DOWN,
		                               _gl_thumbs_mouse_down, gitem);
		evas_object_event_callback_add(layout, EVAS_CALLBACK_MOUSE_UP,
		                               _gl_thumbs_mouse_up, gitem);
#endif
		return layout;
	} else if (!g_strcmp0(part, GL_THUMB_CHECKBOX)) {
		Evas_Object *ck = NULL;
		if (view_mode == GL_VIEW_THUMBS_EDIT ||
		        view_mode == GL_VIEW_THUMBS_SELECT) {
			bool checked = true;
			if (!gitem->checked) {
				checked = _gl_data_get_burstshot_status(ad, gitem);
			} else {
				checked = gitem->checked;
			}
			ck = _gl_thumb_show_checkbox(obj, checked,
			                             __gl_thumbs_check_changed, data);
			elm_object_item_signal_emit(gitem->elm_item, "show_image_icon", "elm_image_open_icon_rect");
		} else {
			ck = elm_object_item_part_content_get(gitem->elm_item,
			                                      GL_THUMB_CHECKBOX);
			elm_object_item_signal_emit(gitem->elm_item, "hide_image_icon", "elm_image_open_icon_rect");
			if (ck) {
				evas_object_del(ck);
				ck = NULL;
			}
		}
		return ck;
	} else if (!g_strcmp0(part, "elm_image_open_icon_swallow_blocker")) {
		Evas_Object *btn1 = NULL;
		if (view_mode == GL_VIEW_THUMBS_EDIT ||
		        view_mode == GL_VIEW_THUMBS_SELECT) {
			btn1 = evas_object_rectangle_add(evas_object_evas_get(obj));
			evas_object_color_set(btn1, 0, 255, 0, 0);
			evas_object_propagate_events_set(btn1, EINA_FALSE);
		} else {
			btn1 = elm_object_item_part_content_get(gitem->elm_item,
			                                        "elm_image_open_icon_swallow_blocker");
			if (btn1) {
				evas_object_del(btn1);
				btn1 = NULL;
			}
		}
		return btn1;
	} else if (!g_strcmp0(part, "elm_image_open_icon_swallow")) {
		Evas_Object *btn = NULL;
		if (view_mode == GL_VIEW_THUMBS_EDIT ||
		        view_mode == GL_VIEW_THUMBS_SELECT) {
			btn = elm_button_add(obj);
			elm_object_style_set(btn, "transparent");
			evas_object_show(btn);
			evas_object_propagate_events_set(btn, EINA_FALSE);
			evas_object_smart_callback_add(btn, "clicked", open_image_in_select_mode, gitem);
		} else {
			btn = elm_object_item_part_content_get(gitem->elm_item,
			                                       "elm_image_open_icon_swallow");
			if (btn) {
				evas_object_del(btn);
				btn = NULL;
			}
		}
		return btn;
	} else if (!g_strcmp0(part, GL_THUMB_MODE)) {
		return _gl_thumb_show_mode(obj, gitem->item->mode);
	}
	return NULL;
}

static char *__gl_thumbs_get_text(void *data, Evas_Object *obj,
                                  const char *part)
{
	GL_CHECK_NULL(part);
	GL_CHECK_NULL(strlen(part));
	GL_CHECK_NULL(data);
	gl_item *gitem = (gl_item *)data;
	GL_CHECK_NULL(gitem->item);

	struct tm time;
	localtime_r((time_t *) & (gitem->item->mtime), &time);

	if (!g_strcmp0(part, GL_THUMB_YEAR)) {
		char buf[GL_DATE_INFO_LEN_MAX] = {0};
		snprintf(buf, sizeof(buf), "%d", GL_DEFAULT_YEAR + time.tm_year);
		return strdup(buf);
	} else if (!g_strcmp0(part, GL_THUMB_MONTH)) {
		char *mon_name[12] = {
			GL_STR_ID_JAN, GL_STR_ID_FEB, GL_STR_ID_MAR,
			GL_STR_ID_APR, GL_STR_ID_MAY, GL_STR_ID_JUN,
			GL_STR_ID_JUL, GL_STR_ID_AUG, GL_STR_ID_SEP,
			GL_STR_ID_OCT, GL_STR_ID_NOV, GL_STR_ID_DEC
		};
		return g_ascii_strup(_gl_str(mon_name[time.tm_mon]), -1);
	}

	return NULL;
}

static int __gl_thumbs_set_type(void *data)
{
	GL_CHECK_VAL(data, -1);
	gl_appdata *ad = (gl_appdata *)data;

	ad->gridinfo.grid_type = GL_GRID_T_NONE;
	gl_cluster *cur_album = _gl_albums_get_current(data);
	GL_CHECK_VAL(cur_album, -1);
	GL_CHECK_VAL(cur_album->cluster, -1);

	switch (cur_album->cluster->type) {
	case GL_STORE_T_PHONE:
	case GL_STORE_T_MMC:
		ad->gridinfo.grid_type = GL_GRID_T_LOCAL;
		break;
	case GL_STORE_T_ALL:
		ad->gridinfo.grid_type = GL_GRID_T_ALLALBUMS;
		break;
	case GL_STORE_T_FAV:
		ad->gridinfo.grid_type = GL_GRID_T_FAV;
		break;
	default:
		gl_dbgE("Wrong cluster storage type!");
	}
	gl_dbg("Gridview type: %d", ad->gridinfo.grid_type);
	return 0;
}

static int __gl_thumbs_set_item_style(void *data)
{
	GL_CHECK_VAL(data, -1);
	gl_appdata *ad = (gl_appdata *)data;

	if (ad->gridinfo.date_gic.item_style == NULL ||
	        ad->gridinfo.date_gic.func.text_get == NULL) {
		ad->gridinfo.date_gic.item_style = GL_GENGRID_ITEM_STYLE_DATE;
		ad->gridinfo.date_gic.func.text_get = __gl_thumbs_get_text;
		ad->gridinfo.date_gic.func.content_get = NULL;
	}
	return 0;
}

static bool __gl_thumbs_create_items(void *data, Evas_Object *parent)
{
	GL_CHECK_FALSE(data);
	GL_CHECK_FALSE(parent);
	gl_appdata *ad = (gl_appdata *)data;
	int i = 0;
	int item_cnt = 0;
	gl_item *gitem = NULL;
	int ret = -1;
	char *burstshot_id = NULL;
	/* Get all medias count of current album */
	Eina_List *media_elist = NULL;
	_gl_thumbs_get_list(data, &media_elist);
	if (ad->gridinfo.media_display_order == ORDER_ASC) {
		media_elist = eina_list_reverse(media_elist);
	}

	int cnt = eina_list_count(media_elist);
	gl_dbg("List count : %d", cnt);
	Elm_Gengrid_Item_Class *gic = NULL;
	gic = evas_object_data_get(parent, "gl_thumbs_item_style_key");
	GL_CHECK_FALSE(gic);

	if (elm_gengrid_items_count(parent) > 0) {
		/* Save scroller position before clearing gengrid */
		_gl_ui_save_scroller_pos(parent);
		evas_object_smart_callback_del(parent, "unrealized",
		                               __gl_thumbs_unrealized);
		elm_gengrid_clear(parent);
		evas_object_smart_callback_add(parent, "unrealized",
		                               __gl_thumbs_unrealized, ad);
	}
	for (i = 0; i < cnt; i++) {
		gitem = eina_list_nth(media_elist, i);
		if (gitem == NULL || gitem->item == NULL ||
		        gitem->item->uuid == NULL) {
			gl_dbgE("Invalid gitem. continue...");
			continue;
		}

		if (!gitem->item->file_url) {
			gl_dbg("file_url is invalid.");
			ret = _gl_data_delete_media(ad, gitem->item);
			if (ret != 0) {
				continue;
			}
			_gl_data_selected_list_remove(ad, gitem);
			media_elist = eina_list_remove(media_elist, gitem);
			_gl_thumbs_set_list(data, media_elist);
			cnt--;
			i--;
			gitem = NULL;
			continue;
		}

		if (ad->gridinfo.back_to_normal) {
			gitem->checked = false;
		}
		if (_gl_thumbs_is_append(gitem, &burstshot_id)) {
			gitem->elm_item = elm_gengrid_item_append(parent, gic,
			                  gitem,
			                  __gl_thumbs_sel_cb,
			                  gitem);
			gitem->preappend = false;
		}
		item_cnt++;
		gitem->sequence = item_cnt;
	}
	if (ad->gridinfo.media_display_order == ORDER_ASC) {
		media_elist = eina_list_reverse(media_elist);
	}
	/* Restore previous position of scroller */
	_gl_ui_restore_scroller_pos(parent);

#ifdef _USE_SHRINK_EFFECT
	/* Disable grid item selection callback */
	elm_gengrid_select_mode_set(parent, ELM_OBJECT_SELECT_MODE_NONE);
#endif
	ad->gridinfo.count = item_cnt;
	ad->gridinfo.back_to_normal = false;

	if (item_cnt == 0) {
		return EINA_FALSE;
	} else {
		return EINA_TRUE;
	}
}

/*
* Set new view to all/image_grid_view.
*
* b_noc, true: it's nocontents view, false: normal view.
*/
static int __gl_thumbs_set_view(void *data, Evas_Object *view, bool b_noc)
{
	GL_CHECK_VAL(data, -1);
	gl_appdata *ad = (gl_appdata *)data;
	ad->gridinfo.view = view;
	if (b_noc) {
		_gl_thumbs_set_nocontents(ad, view);
	} else {
		_gl_thumbs_set_nocontents(ad, NULL);
	}
	return 0;
}

void _gl_thumbs_add_album_data(gl_cluster *album, Eina_List *medias_elist)
{
	GL_CHECK(album);
	GL_CHECK(medias_elist);
	int cnt = eina_list_count(medias_elist);
	int i;
	gl_item *gitem = NULL;
	for (i = 0; i < cnt; i++) {
		gitem = eina_list_nth(medias_elist, i);
		GL_CHECK(gitem);
		gitem->album = album;
	}
}

/*
* When select album, show first (GL_FIRST_VIEW_END_POS+1) medias.
* Then use idler to get other medias from DB and appened
* them to gridview.
* In order to reduce transit time of first show of thumbnails view.
* Cause most of time is spent for getting records from DB,
* so we get first (GL_FIRST_VIEW_END_POS+1) items and show thumbnails view,
* it will accelerate view show.
*/
static int __gl_thumbs_idler_append_items(void *data)
{
	GL_CHECK_VAL(data, -1);
	gl_appdata *ad = (gl_appdata *)data;
	GL_CHECK_VAL(_gl_albums_get_current(data), -1);
	int i = 0;
	int item_cnt = 0;
	gl_item *gitem = NULL;
	gl_cluster *album = NULL;
	int ret = -1;
	char *burstshot_id = NULL;
	Evas_Object *view = ad->gridinfo.view;

	if (gl_get_view_mode(data) == GL_VIEW_THUMBS_SELECT) {
		view = ad->gridinfo.select_view;
	}
	GL_CHECK_VAL(view, -1);

	Elm_Gengrid_Item_Class *gic = NULL;
	gic = evas_object_data_get(view, "gl_thumbs_item_style_key");
	GL_CHECK_VAL(gic, -1);

	/* Get all medias count of current album */
	Eina_List *media_elist = NULL;
	_gl_thumbs_get_list(data, &media_elist);
	int cnt = eina_list_count(media_elist);
	gl_dbg("First view medias count: %d", cnt);
	if (cnt != (GL_FIRST_VIEW_END_POS + 1)) {
		gl_dbg("No any more items, return!");
		return -1;
	}
	/* Get other items from DB */
	ret = _gl_data_get_items_album(ad, _gl_albums_get_current(data),
	                               (GL_FIRST_VIEW_END_POS + 1),
	                               GL_GET_UNTIL_LAST_RECORD, &media_elist);
	if (ret < 0) {
		gl_dbgE("Get items list failed[%d]!", ret);
		return ret;
	}

	_gl_thumbs_set_list(data, media_elist);
	cnt = eina_list_count(media_elist);
	gl_dbg("Grid view all medias count: %d", cnt);

	/* get album data associated with media item */
	gitem = eina_list_nth(media_elist, 0);
	if (gitem) {
		album = gitem->album;
	}

	/* From (GL_FIRST_VIEW_END_POS + 1)th item to last one */
	i = GL_FIRST_VIEW_END_POS + 1;

	/* Check the first item of previous appending, maybe it's a burst item */
	gitem = eina_list_nth(media_elist, i - 1);
	if (gitem && gitem->item &&
	        gitem->item->type == MEDIA_CONTENT_TYPE_IMAGE &&
	        gitem->item->image_info &&
	        gitem->item->image_info->burstshot_id) {
		burstshot_id = gitem->item->image_info->burstshot_id;
		gl_dbgW("Update burstshot item!");
		elm_gengrid_item_update(gitem->elm_item);
	}
	gitem = NULL;
	for (; i < cnt; i++) {
		gitem = eina_list_nth(media_elist, i);
		if (gitem == NULL || gitem->item == NULL ||
		        gitem->item->uuid == NULL) {
			gl_dbgE("Invalid gitem, continue...");
			continue;
		}

		if (!gitem->item->file_url) {
			gl_dbg("file_url is invalid.");
			_gl_data_delete_media(ad, gitem->item);
			_gl_data_selected_list_remove(ad, gitem);
			media_elist = eina_list_remove(media_elist, gitem);
			_gl_thumbs_set_list(data, media_elist);
			cnt--;
			i--;
			gitem = NULL;
			continue;
		}
		if (_gl_thumbs_is_append(gitem, &burstshot_id)) {
			gitem->elm_item = elm_gengrid_item_append(view, gic,
			                  gitem,
			                  __gl_thumbs_sel_cb,
			                  gitem);
			gitem->preappend = false;
		}

		item_cnt++;
		gitem->sequence = item_cnt + GL_FIRST_VIEW_END_POS + 1;
	}
	_gl_thumbs_add_album_data(album, media_elist);

	gl_dbgW("Use idler to append other medias--Done!");
	if (item_cnt == 0) {
		return -1;
	} else {
		ad->gridinfo.count = item_cnt + GL_FIRST_VIEW_END_POS + 1;
		gl_dbg("All count appended: %d", ad->gridinfo.count);
		return 0;
	}
}

static int __gl_thumbs_mode_reset_btns(void *data)
{
	GL_CHECK_VAL(data, -1);
	gl_appdata *ad = (gl_appdata *)data;

	/* none items, disable 'edit' button */
	if (_gl_thumbs_check_zero(data)) {
		_gl_thumbs_disable_share(ad, true);
		_gl_thumbs_disable_slideshow(ad, true);
		_gl_ui_disable_menu(ad->gridinfo.nf_it, true);
	} else {
		_gl_thumbs_disable_share(ad, false);
		_gl_thumbs_disable_slideshow(ad, false);
		_gl_ui_disable_menu(ad->gridinfo.nf_it, false);
	}
	return 0;
}

static void __gl_thumbs_trans_finished_cb(void *data, Evas_Object *obj, void *ei)
{
	GL_CHECK(data);
	gl_appdata *ad = (gl_appdata *)data;
	int view_mode = gl_get_view_mode(ad);
	gl_dbgW("view_mode: %d", view_mode);
	evas_object_smart_callback_del(obj, GL_TRANS_FINISHED,
	                               __gl_thumbs_trans_finished_cb);

	/* Clear previous view after animation finished */
	if (view_mode == GL_VIEW_THUMBS || view_mode == GL_VIEW_THUMBS_EDIT) {
		if (_gl_ctrl_get_tab_mode(ad) == GL_CTRL_TAB_ALBUMS) {
			elm_gengrid_clear(ad->albuminfo.view);
		}
	} else {
		gl_dbgE("Wrong view mode!");
	}

	if (ad->gridinfo.is_append) {
		_gl_thumbs_append_items(data);
		ad->gridinfo.is_append = false;
	}
}

static Eina_Bool __gl_thumbs_pop_cb(void *data, Elm_Object_Item *it)
{
	GL_CHECK_FALSE(data);
	gl_appdata *ad = (gl_appdata *)data;

	if (gl_get_view_mode(data) == GL_VIEW_THUMBS_EDIT) {
		gl_dbg("GL_VIEW_THUMBS_EDIT");
		_gl_thumbs_edit_pop_view(data);
		/* Keep current view unchanged */
		if (_gl_thumbs_get_edit_mode(data) < GL_THUMBS_EDIT_FIRST) {
			return EINA_FALSE;
		}
	}

	/* Call function to update previous view */
	Elm_Naviframe_Item_Pop_Cb func = NULL;
	func = (Elm_Naviframe_Item_Pop_Cb)elm_object_item_data_get(ad->gridinfo.nf_it);
	/* show albums view */
	gl_dbg("func: %p", func);
	if (func) {
		func(data, it);
	}
	/* Pop view(current naviframe item) */
	if (ad->albuminfo.selected_uuid) {
		free(ad->albuminfo.selected_uuid);
		ad->albuminfo.selected_uuid = NULL;
	}
	return EINA_TRUE;
}

static void __gl_thumbs_edit_cb(void *data, Evas_Object *obj, void *ei)
{
	GL_CHECK(data);
	gl_appdata *ad = (gl_appdata *)data;
	_gl_ctxpopup_del(data);
	if (ad->uginfo.ug || ad->gridinfo.is_append) {
		/**
		* Prevent changed to edit view in wrong way.
		* 1. When invoke imageviewer UG;
		* 2. First show thumbnails view, use idler to append other medias.
		*/
		gl_dbg("UG invoked or appending gridview.");
		return;
	}

	ad->gridinfo.b_slideshow_disable = false;

	int view_mode = gl_get_view_mode(data);
	gl_dbg("mode: %d", view_mode);
	if (view_mode == GL_VIEW_THUMBS) {
		_gl_thumbs_set_edit_mode(data, GL_THUMBS_EDIT_NORMAL);
		gl_item *gitem = eina_list_nth(ad->gridinfo.medias_elist, 0);
		gl_item *tmp = NULL;

		GL_CHECK(gitem);
		GL_CHECK(gitem->album);
		if (gitem && gitem->album && gitem->album->elist) {
			EINA_LIST_FREE(gitem->album->elist, tmp) {
				if (tmp) {
					tmp->checked = false;
				}
			}
		}
		_gl_thumbs_edit_create_view(data);
	}
}

static void __gl_thumbs_copy_cb(void *data, Evas_Object *obj, void *ei)
{
	GL_CHECK(data);
	gl_appdata *ad = (gl_appdata *)data;
	_gl_ctxpopup_del(data);
	if (ad->uginfo.ug || ad->gridinfo.is_append) {
		gl_dbg("UG invoked or appending gridview.");
		return;
	}

	ad->gridinfo.b_slideshow_disable = false;

	int view_mode = gl_get_view_mode(data);
	gl_dbg("mode: %d", view_mode);
	if (view_mode == GL_VIEW_THUMBS) {
		_gl_thumbs_set_edit_mode(data, GL_THUMBS_EDIT_COPY);
		gl_item *gitem = eina_list_nth(ad->gridinfo.medias_elist, 0);
		gl_item *tmp = NULL;
		if (gitem && gitem->album) {
			EINA_LIST_FREE(gitem->album->elist, tmp) {
				if (tmp) {
					tmp->checked = false;
				}
			}
		}
		_gl_thumbs_edit_create_view(data);
	}
}

static void __gl_thumbs_move_cb(void *data, Evas_Object *obj, void *ei)
{
	GL_CHECK(data);
	gl_appdata *ad = (gl_appdata *)data;
	_gl_ctxpopup_del(data);
	if (ad->uginfo.ug || ad->gridinfo.is_append) {
		gl_dbg("UG invoked or appending gridview.");
		return;
	}

	ad->gridinfo.b_slideshow_disable = false;

	int view_mode = gl_get_view_mode(data);
	gl_dbg("mode: %d", view_mode);
	if (view_mode == GL_VIEW_THUMBS) {
		_gl_thumbs_set_edit_mode(data, GL_THUMBS_EDIT_MOVE);
		gl_item *gitem = eina_list_nth(ad->gridinfo.medias_elist, 0);
		gl_item *tmp = NULL;
		if (gitem && gitem->album) {
			EINA_LIST_FREE(gitem->album->elist, tmp) {
				if (tmp) {
					tmp->checked = false;
				}
			}
		}
		_gl_thumbs_edit_create_view(data);
	}
}

static void __gl_thumbs_sortby_cb(void *data, Evas_Object *obj, void *ei)
{
	gl_dbg("ENTRY");
	GL_CHECK(data);
	gl_appdata *ad = (gl_appdata *)data;
	int state_index = -1;
	state_index = ad->gridinfo.media_display_order == ORDER_ASC ? 1 : 0;
	_gl_list_pop_create(data, obj, ei, GL_STR_SORT, GL_STR_DATE_MOST_RECENT, GL_STR_DATE_OLDEST, state_index);
	gl_dbg("EXIT");
}

#if 0
static void __gl_thumbs_share_cb(void *data, Evas_Object *obj, void *ei)
{
	GL_CHECK(data);
	gl_appdata *ad = (gl_appdata *)data;
	if (ad->uginfo.ug || ad->gridinfo.is_append) {
		/**
		* Prevent changed to edit view in wrong way.
		* 1. When invoke imageviewer UG;
		* 2. First show thumbnails view, use idler to append other medias.
		*/
		gl_dbg("UG invoked or appending gridview.");
		return;
	}

	int view_mode = gl_get_view_mode(data);
	gl_dbg("mode: %d", view_mode);
	if (view_mode == GL_VIEW_THUMBS) {
		_gl_thumbs_set_edit_mode(data, GL_THUMBS_EDIT_SHARE);
		_gl_thumbs_edit_create_view(data);
	}
}
#endif

static void __gl_thumbs_soft_back_cb(void *data, Evas_Object *obj, void *ei)
{
	GL_CHECK(data);
	gl_appdata *ad = (gl_appdata *)data;
	GL_CHECK(ad->maininfo.naviframe);
	elm_naviframe_item_pop(ad->maininfo.naviframe);
}

#ifdef SUPPORT_SLIDESHOW
static int __gl_thumbs_slideshow_op(void *data)
{
	GL_CHECK_VAL(data, -1);
	gl_appdata *ad = (gl_appdata *)data;
	Eina_List *medias_elist = NULL;
	gl_item *cur_item = NULL;
	media_content_type_e type = MEDIA_CONTENT_TYPE_OTHERS;

	_gl_thumbs_get_list(data, &medias_elist);
	_gl_data_get_first_item(type, medias_elist, &cur_item);
	GL_CHECK_VAL(cur_item, -1);
	return gl_ext_load_iv_ug(ad, cur_item, GL_UG_IV_SLIDESHOW);
}

static int __gl_thumbs_slideshow(void *data, const char *label)
{
	GL_CHECK_VAL(label, -1);
	GL_CHECK_VAL(data, -1);
	gl_dbg("label : %s ", label);
	gl_appdata *ad = (gl_appdata *)data;

	if (!g_strcmp0(label, _gl_str(GL_STR_ID_ALL_ITEMS))) {
		__gl_thumbs_slideshow_op(data);
	} else if (!g_strcmp0(label, _gl_str(GL_STR_ID_SETTINGS))) {
		evas_object_data_set(ad->maininfo.naviframe,
		                     GL_NAVIFRAME_SLIDESHOW_DATA_KEY,
		                     __gl_thumbs_slideshow_op);
		gl_ext_load_ug(data, GL_UG_GALLERY_SETTING_SLIDESHOW);
	} else if (!g_strcmp0(label, _gl_str(GL_STR_ID_SELECT_ITEMS))) {
#ifdef _USE_APP_SLIDESHOW
		_gl_thumbs_edit_create_view(data);
		evas_object_data_set(ad->maininfo.naviframe,
		                     GL_NAVIFRAME_SELECTED_SLIDESHOW_KEY,
		                     _gl_thumbs_edit_pop_view);
#else
		_gl_ext_launch_gallery_ug(data);
#endif
	} else {
		gl_dbgE("Invalid lable!");
		return -1;
	}
	return 0;
}

static void __gl_thumbs_slideshow_cb(void *data, Evas_Object *obj, void *ei)
{
	GL_CHECK(data);
	_gl_ctxpopup_del(data);
	_gl_popup_create_slideshow(data, __gl_thumbs_slideshow);
}
#endif

/*
  * Local : New album/Edit/View/Slideshow/Show hidden items/Tag buddy
  * Nearby : Edit/Refresh/Slideshow
  * PTP: Edit/Slideshow
  * Places: Edit/Footstep
  * Tags: Eidt/View/Slideshow
  * People: Edit/Slideshow/Tag buddy
  * Easymode: New album/Edit/Slideshow
  */
static int __gl_thumbs_ctxpopup_append(void *data, Evas_Object *parent)
{
	GL_CHECK_VAL(parent, -1);
	GL_CHECK_VAL(data, -1);
#ifdef SUPPORT_SLIDESHOW
	gl_appdata *ad = (gl_appdata *)data;
#endif
	gl_appdata *ad = (gl_appdata *)data;
	/* 1. New album -- local album */

	if (ad->gridinfo.grid_type == GL_GRID_T_FAV) {
		/* 3. Sort by */
		if (!_gl_thumbs_check_zero(data))
			_gl_ctxpopup_append(parent, GL_STR_SORT,
			                    __gl_thumbs_sortby_cb, data);
		return 0;
	}
	/* 2. Edit */
	/* No 'edit' for facebook files, only share */
	if (!_gl_thumbs_check_zero(data))
		_gl_ctxpopup_append(parent, GL_STR_ID_DELETE,
		                    __gl_thumbs_edit_cb, data);
	/* 3. Sort by */
	if (!_gl_thumbs_check_zero(data))
		_gl_ctxpopup_append(parent, GL_STR_SORT,
		                    __gl_thumbs_sortby_cb, data);
	/* 4. Copy to album*/
	if (!_gl_thumbs_check_zero(data))
		_gl_ctxpopup_append(parent, GL_STR_ID_COPY_TO_ALBUM,
		                    __gl_thumbs_copy_cb, data);
	/* 5. Move to album*/
	if (!_gl_thumbs_check_zero(data))
		_gl_ctxpopup_append(parent, GL_STR_ID_MOVE_TO_ALBUM,
		                    __gl_thumbs_move_cb, data);
#ifdef SUPPORT_SLIDESHOW
	/* 6. Slideshow */
	if (!ad->gridinfo.b_slideshow_disable)
		_gl_ctxpopup_append(parent, GL_STR_ID_SLIDESHOW,
		                    __gl_thumbs_slideshow_cb, data);
#endif
	return 0;
}

static void __gl_thumbs_more_btn_cb(void *data, Evas_Object *obj, void *ei)
{
	gl_dbg("more button clicked");
	GL_CHECK(data);
	gl_appdata *ad = (gl_appdata *)data;
	Evas_Object *more = NULL;
	more = _gl_ui_get_btn(NULL, ad->gridinfo.nf_it, GL_NAVIFRAME_MORE);
	if (EINA_TRUE == elm_object_disabled_get(more)) {
		gl_dbg("Menu is disabled");
		return; /* Menu is disabled */
	}

	_gl_ctxpopup_create(data, obj, __gl_thumbs_ctxpopup_append);
}

#if 0
static void __gl_thumbs_camera_cb(void *data, Evas_Object *obj, void *ei)
{
	GL_CHECK(data);
	gl_dbg("");
	_gl_ext_load_camera(data);
}
#endif

/**
 *  Use naviframe api to push thumbnails view to stack.
 *  @param obj is the content to be pushed.
 */
static int __gl_thumbs_add_btns(void *data, Evas_Object *parent,
                                Elm_Object_Item *nf_it)
{
	gl_dbg("GL_NAVI_THUMBS");
	GL_CHECK_VAL(nf_it, -1);
	GL_CHECK_VAL(parent, -1);
	GL_CHECK_VAL(data, -1);
	Evas_Object *btn = NULL;

	/* More */
	btn = _gl_but_create_but(parent, NULL, NULL, GL_BUTTON_STYLE_NAVI_MORE,
	                         __gl_thumbs_more_btn_cb, data);
	GL_CHECK_VAL(btn, -1);

	elm_object_item_part_content_set(nf_it, GL_NAVIFRAME_MORE, btn);

	btn = elm_button_add(parent);
	elm_object_style_set(btn, "naviframe/end_btn/default");
	evas_object_smart_callback_add(btn, "clicked", __gl_thumbs_soft_back_cb, data);
	GL_CHECK_VAL(btn, -1);
	elm_object_item_part_content_set(nf_it, GL_NAVIFRAME_PREV_BTN, btn);
	__gl_thumbs_mode_reset_btns(data);
	return 0;
}

/* Clear data for thumbs view */
static int __gl_thumbs_clear_data(void *data)
{
	GL_CHECK_VAL(data, -1);
	gl_appdata *ad = (gl_appdata *)data;

	/* destroy timer */
	GL_IF_DEL_TIMER(ad->gridinfo.clicked_timer);

	if (ad->gridinfo.view) {
		if (ad->gridinfo.view != ad->gridinfo.nocontents) {
			_gl_thumbs_clear_cbs(ad->gridinfo.view);
			_gl_ui_del_scroller_pos(ad->gridinfo.view);
			/* elm_gengrid_clear should be called after callbacks are unregistered */
			if (ad->gridinfo.medias_elist &&
			        eina_list_count(ad->gridinfo.medias_elist) > 0) {
				elm_gengrid_clear(ad->gridinfo.view);
			}
		}
		ad->gridinfo.view = NULL;
	}
	ad->gridinfo.grid_type = GL_GRID_T_NONE;
	ad->gridinfo.nocontents = NULL;
	ad->gridinfo.layout = NULL;
	ad->gridinfo.b_slideshow_disable = false;
	ad->gridinfo.nf_it = NULL;
	ad->gridinfo.count = 0;
	_gl_thumbs_set_edit_mode(data, GL_THUMBS_EDIT_NONE);
	/* Free title */
	GL_FREEIF(ad->gridinfo.title);
	return 0;
}

/* Free data after layout deleted */
static void __gl_thumbs_delete_layout_cb(void *data, Evas *e, Evas_Object *obj,
        void *ei)
{
	gl_dbg("Delete layout ---");
	GL_CHECK(data);
	gl_appdata *ad = (gl_appdata *)data;
	/* Clear view data */
	__gl_thumbs_clear_data(data);
	/* Free selected list */
	_gl_data_clear_selected_info(data);
	/* Clear all data */
	_gl_data_util_clear_item_list(&(ad->gridinfo.medias_elist));
	gl_dbg("Delete layout +++");
}

static int __gl_thumbs_push_view(void *data, Evas_Object *parent,
                                 Evas_Object *obj, gl_navi_mode mode,
                                 char *title)
{
	GL_CHECK_VAL(obj, -1);
	GL_CHECK_VAL(parent, -1);
	GL_CHECK_VAL(data, -1);
	gl_appdata *ad = (gl_appdata *)data;
	Elm_Object_Item *nf_it = NULL;

	/* Add transition finished callback */
	evas_object_smart_callback_add(parent, GL_TRANS_FINISHED,
	                               __gl_thumbs_trans_finished_cb, data);
	/* Push to stack with basic transition */
	nf_it = elm_naviframe_item_push(parent, title, NULL, NULL, obj, NULL);
	elm_object_item_part_text_set(nf_it, "elm.text.title", _gl_str(title));
	ad->gridinfo.nf_it = nf_it;
	elm_naviframe_item_pop_cb_set(nf_it, __gl_thumbs_pop_cb, data);

	if (_gl_thumbs_get_edit_mode(data) > GL_THUMBS_EDIT_FIRST) {
		_gl_thumbs_edit_create_view(data);
		return 0;
	}
	GL_FREEIF(ad->gridinfo.title);
	if (title) {
		ad->gridinfo.title = strdup(title);
	}

	__gl_thumbs_add_btns(data, parent, nf_it);
	gl_dbg("Done");
	return 0;
}

/* Free data after layout deleted */
static void __gl_thumbs_del_view_cb(void *data, Evas *e, Evas_Object *obj,
                                    void *ei)
{
	Elm_Gengrid_Item_Class *gic = NULL;
	gic = evas_object_data_get(obj, "gl_thumbs_item_style_key");
	evas_object_data_set(obj, "gl_thumbs_item_style_key", (void *)NULL);
	if (gic) {
		elm_gengrid_item_class_free(gic);
	}
}

Eina_Bool _gl_thumbs_append_items(void *data)
{
	GL_CHECK_CANCEL(data);

	/* Try to get other medias from DB and append them to gridview */
	if (__gl_thumbs_idler_append_items(data) < 0) {
		gl_dbgE("Failed to append grid items!");
	} else {
		gl_dbg("Successful to append grid items!");
	}
	/* Need to check size and alignment again, cuz using burstshot mode */
	_gl_thumbs_set_size(data, NULL);
	return ECORE_CALLBACK_CANCEL;
}

Elm_Gengrid_Item_Class *_gl_thumbs_new_item_style(Evas_Object *parent)
{
	GL_CHECK_NULL(parent);

	Elm_Gengrid_Item_Class *gic = NULL;

	gic = elm_gengrid_item_class_new();
	GL_CHECK_NULL(gic);
	/* Register delete callback */
	evas_object_event_callback_add(parent, EVAS_CALLBACK_DEL,
	                               __gl_thumbs_del_view_cb, gic);
	evas_object_data_set(parent, "gl_thumbs_item_style_key", (void *)gic);
	gic->item_style = GL_GENGRID_ITEM_STYLE_THUMBNAIL;
	gic->func.text_get = NULL;
	gic->func.content_get = __gl_thumbs_get_content;
	gic->func.del = NULL;
	return gic;
}

int _gl_thumbs_update_realized_items(void *data)
{
	GL_CHECK_VAL(data, -1);
	gl_appdata *ad = (gl_appdata *)data;

	/* Clear old view */
	evas_object_smart_callback_del(ad->gridinfo.view, "unrealized",
	                               __gl_thumbs_unrealized);
	_gl_thumbs_set_size(ad, NULL);
	elm_gengrid_realized_items_update(ad->gridinfo.view);
	evas_object_smart_callback_add(ad->gridinfo.view, "unrealized",
	                               __gl_thumbs_unrealized, ad);
	return 0;
}

Evas_Object *_gl_thumbs_get_content(void *data, Evas_Object *parent,
                                    gl_item *gitem, int w, int h)
{
	GL_CHECK_NULL(gitem);
	GL_CHECK_NULL(gitem->item);
	GL_CHECK_NULL(parent);
	GL_CHECK_NULL(data);
	gl_appdata *ad = (gl_appdata *)data;
	char *path = NULL;
	Evas_Object *layout = NULL;

	if (GL_FILE_EXISTS(gitem->item->thumb_url)) {
		path = gitem->item->thumb_url;
	} else {
		/* Use default image */
		path = GL_ICON_NO_THUMBNAIL;
	}

	int zoom_level = ad->pinchinfo.zoom_level;

	if (gitem->item->type == MEDIA_CONTENT_TYPE_VIDEO) {
		unsigned int v_dur = 0;
		if (gitem->item->video_info) {
			v_dur = gitem->item->video_info->duration;
		}
		layout = _gl_thumb_show_video(parent, path, v_dur, w, h,
		                              zoom_level);
	} else {
		if (gitem->item->image_info &&
		        gitem->item->image_info->burstshot_id) {
			layout = _gl_thumb_show_image(parent, path, 0, w, h,
			                              zoom_level);
			gitem->item->mode = GL_CM_MODE_BURSTSHOT;
		} else {
			layout = _gl_thumb_show_image(parent, path, 0, w, h,
			                              zoom_level);
		}
	}
	return layout;
}

int _gl_thumbs_add_btns(void *data)
{
	GL_CHECK_VAL(data, -1);
	gl_appdata *ad = (gl_appdata *)data;
	__gl_thumbs_add_btns(data, ad->maininfo.naviframe, ad->gridinfo.nf_it);
	return 0;
}

void _gl_thumbs_mouse_down(void *data, Evas *e, Evas_Object *obj, void *ei)
{
	GL_CHECK(obj);
	GL_CHECK(ei);
	GL_CHECK(data);
	gl_item *gitem = (gl_item *) data;
	GL_CHECK(gitem->ad);
	gl_appdata *ad = (gl_appdata *)gitem->ad;
	Evas_Event_Mouse_Down *ev = (Evas_Event_Mouse_Down *)ei;

	ad->gridinfo.mouse.x = ev->output.x;
	ad->gridinfo.mouse.y = ev->output.y;

#ifdef _USE_SHRINK_EFFECT
	if (ad->uginfo.ug == NULL) {
		edje_object_signal_emit(_EDJ(obj), "mouse,down,shrink", "bg");
		ad->gridinfo.mouse.b_pressed = true;
	}
#endif
}

void _gl_thumbs_mouse_up(void *data, Evas *e, Evas_Object *obj, void *ei)
{
	GL_CHECK(data);
	gl_item *gitem = (gl_item *)data;
	GL_CHECK(gitem->ad);
	gl_appdata *ad = (gl_appdata *)gitem->ad;

	if (!ei || !obj) {
		goto RET_WO_EXEC;
	}

	Evas_Event_Mouse_Up *ev = (Evas_Event_Mouse_Up *)ei;
	if (ev->event_flags & EVAS_EVENT_FLAG_ON_HOLD ||
	        ev->event_flags & EVAS_EVENT_FLAG_ON_SCROLL) {
		goto RET_WO_EXEC;
	}

	if ((abs(ad->gridinfo.mouse.x - ev->output.x) > GL_MOUSE_RANGE) ||
	        (abs(ad->gridinfo.mouse.y - ev->output.y) > GL_MOUSE_RANGE)) {
		goto RET_WO_EXEC;
	}

	gl_dbg("");

#ifdef _USE_SHRINK_EFFECT
	if (ad->gridinfo.mouse.b_pressed) {
		edje_object_signal_emit(_EDJ(obj), "mouse,up,expand", "bg");
		edje_object_signal_callback_add(_EDJ(obj), "shrink,expand,done",
		                                "bg", __gl_thumbs_shrink_cb,
		                                data);
		ad->gridinfo.mouse.b_pressed = false;
	}
	return;

RET_WO_EXEC:

	if (ad->gridinfo.mouse.b_pressed) {
		edje_object_signal_emit(_EDJ(obj), "mouse,up,expand", "bg");
		ad->gridinfo.mouse.b_pressed = false;
	}
#else
	_gl_thumbs_open_file(data);
	return;

RET_WO_EXEC:
	return;
#endif
}

Evas_Object *_gl_thumbs_add_grid(void *data, Evas_Object *parent,
                                 gl_thumb_mode mode)
{
	GL_CHECK_NULL(parent);
	GL_CHECK_NULL(data);
	gl_appdata *ad = (gl_appdata *)data;
	Evas_Object *grid = NULL;

	int view_mode = gl_get_view_mode(ad);
	grid = _gl_thumb_add_gengrid(parent);
	evas_object_smart_callback_add(grid, "realized", __gl_thumbs_realized,
	                               data);
	evas_object_smart_callback_add(grid, "unrealized",
	                               __gl_thumbs_unrealized, data);
	evas_object_smart_callback_add(grid, "longpressed",
	                               __gl_thumbs_longpressed_cb, data);
	evas_object_smart_callback_add(grid, "language,changed",
	                               __gl_thumbs_lang_changed, NULL);
	/* Set grid view type */
	__gl_thumbs_set_type(ad);
	_gl_ui_reset_scroller_pos(grid);
	/* Set gengrid item style first */
	__gl_thumbs_set_item_style(data);
	_gl_thumbs_new_item_style(grid);

	if (!__gl_thumbs_create_items(ad, grid)) {
		_gl_ui_del_scroller_pos(grid);
		evas_object_del(grid);
		grid = NULL;
		Evas_Object *noc = _gl_nocontents_create(parent);
		_gl_thumbs_set_nocontents(ad, noc);
		evas_object_show(noc);
		return noc;
	}

	gl_dbg("thumbs_cnt : %d", ad->gridinfo.count);
	_gl_thumbs_set_size(ad, grid);
	if (view_mode != GL_VIEW_THUMBS_SELECT) {
		_gl_thumbs_set_nocontents(ad, NULL);
	}
	evas_object_show(grid);

	return grid;
}

int _gl_thumbs_show_edit_view(void *data)
{
	GL_CHECK_VAL(data, -1);
	gl_appdata *ad = (gl_appdata *)data;
	int view_mode = gl_get_view_mode(ad);
	bool b_ret = false;
	Evas_Object *view = NULL;
	gl_dbg("view_mode: %d", view_mode);

	if (view_mode == GL_VIEW_THUMBS_EDIT) {
		view = ad->gridinfo.view;
	} else if (view_mode == GL_VIEW_THUMBS_SELECT) {
		view = ad->gridinfo.select_view;
	} else {
		gl_dbgE("Wrong view mode!");
		return -1;
	}
	GL_CHECK_VAL(view, -1);
	b_ret = __gl_thumbs_create_items(ad, view);
	_gl_thumbs_set_size(ad, NULL);

	if (!b_ret) {
		gl_dbgE("Create items failed!");
		return -1;
	} else {
		return 0;
	}
}

int _gl_thumbs_show_view(void *data)
{
	GL_CHECK_VAL(data, -1);
	gl_appdata *ad = (gl_appdata *)data;
	bool b_ret = false;
	gl_dbg("");

	if (ad->gridinfo.view == NULL) {
		gl_dbg("Error : Any gridview doesn't exist");
		return -1;
	}

	/* Come back from edit mode, save state */
	ad->gridinfo.back_to_normal = true;
	b_ret = __gl_thumbs_create_items(ad, ad->gridinfo.view);
	_gl_thumbs_set_size(ad, NULL);

	if (!b_ret) {
		gl_dbgE("Create items failed!");
		return -1;
	} else {
		return 0;
	}
}

Eina_Bool _gl_thumbs_show_items(void *data)
{
	GL_CHECK_FALSE(data);
	gl_appdata *ad = (gl_appdata *)data;
	Eina_Bool res = EINA_FALSE;
	Evas_Object *gv = NULL;
	bool b_view = false;
	gl_dbg("");

	b_view = _gl_thumbs_get_view(ad, &gv);
	GL_CHECK_FALSE(gv);
	if (b_view) {
		res = __gl_thumbs_create_items(ad, gv);
		if (!res) {
			gl_dbgW("create items failed!");
			elm_object_part_content_unset(ad->gridinfo.layout,
			                              "elm.swallow.view");
			_gl_thumbs_delete_view(data);
			__gl_thumbs_set_view(ad, NULL, false);
			Evas_Object *noc = _gl_nocontents_create(ad->gridinfo.layout);
			evas_object_show(noc);
			__gl_thumbs_set_view(ad, noc, true);

			elm_object_part_content_set(ad->gridinfo.layout,
			                            "elm.swallow.view", noc);
		} else {
			_gl_thumbs_set_size(ad, gv);
		}
	} else {
		gl_dbg("Remove nocontents view.");
		elm_object_part_content_unset(ad->gridinfo.layout,
		                              "elm.swallow.view");
		evas_object_hide(gv);
		evas_object_del(gv);
		__gl_thumbs_set_view(ad, NULL, false);
		bool b_noc = false;

		Evas_Object *view = NULL;
		view = _gl_thumbs_add_grid(ad, ad->gridinfo.layout, 0);
		if (ad->gridinfo.nocontents) {
			b_noc = true;
		}
		/* No nocotents removed, gridview created */
		__gl_thumbs_set_view(ad, view, b_noc);

		elm_object_part_content_set(ad->gridinfo.layout,
		                            "elm.swallow.view", view);
	}

	return res;
}

int _gl_thumbs_set_list(void *data, Eina_List *elist)
{
	GL_CHECK_VAL(data, -1);
	gl_appdata *ad = (gl_appdata *)data;
	ad->gridinfo.medias_elist = elist;
	return 0;
}

int _gl_thumbs_clear_and_set_list(void *data, Eina_List *elist)
{
	GL_CHECK_VAL(data, -1);
	gl_appdata *ad = (gl_appdata *)data;
	int view_mode = gl_get_view_mode(data);
	if (view_mode == GL_VIEW_THUMBS_SELECT && ad->gridinfo.select_view) {
		_gl_ui_save_scroller_pos(ad->gridinfo.select_view);
		elm_gengrid_clear(ad->gridinfo.select_view);
	} else if (ad->gridinfo.view && (ad->gridinfo.view != ad->gridinfo.nocontents)) {
		_gl_ui_save_scroller_pos(ad->gridinfo.view);
		elm_gengrid_clear(ad->gridinfo.view);
	}
	_gl_data_util_clear_item_list(&(ad->gridinfo.medias_elist));
	ad->gridinfo.medias_elist = elist;
	return 0;
}

int _gl_thumbs_get_list(void *data, Eina_List **p_elist)
{
	GL_CHECK_VAL(p_elist, -1);
	GL_CHECK_VAL(data, -1);
	gl_appdata *ad = (gl_appdata *)data;
	*p_elist = ad->gridinfo.medias_elist;
	return 0;
}

int _gl_thumbs_update_edit_split_view(void *data, int nf_mode, char *title, bool b_idler,
                                      Elm_Naviframe_Item_Pop_Cb func)
{
	GL_CHECK_VAL(data, -1);
	gl_appdata *ad = (gl_appdata *)data;
	GL_CHECK_VAL(ad->maininfo.naviframe, -1);
	Evas_Object *layout = NULL;
	Evas_Object *view = NULL;
	/* Add view */
	gl_set_view_mode(data, GL_VIEW_THUMBS);
	view = _gl_thumbs_add_grid(ad, ad->gridinfo.layout, GL_THUMB_ALL);
	if (view == NULL) {
		evas_object_del(layout);
		gl_dbgE("Failed to add gridview!");
		return -1;
	}
	ad->gridinfo.update_split_view = false;
	elm_object_part_content_set(ad->gridinfo.layout, "elm.swallow.view", view);
	ad->gridinfo.view = view;
	elm_object_item_data_set(ad->gridinfo.nf_it, (void *)func);
	elm_object_item_text_set(ad->gridinfo.nf_it, title);
	GL_FREEIF(ad->gridinfo.title);
	if (title) {
		ad->gridinfo.title = strdup(title);
	}
	if (_gl_thumbs_get_edit_mode(data) == GL_THUMBS_EDIT_NORMAL) {
		__gl_thumbs_edit_cb(ad, ad->gridinfo.view, NULL);
	} else if (_gl_thumbs_get_edit_mode(data) == GL_THUMBS_EDIT_COPY) {
		__gl_thumbs_copy_cb(ad, ad->gridinfo.view, NULL);
	} else if (_gl_thumbs_get_edit_mode(data) == GL_THUMBS_EDIT_MOVE) {
		__gl_thumbs_move_cb(ad, ad->gridinfo.view, NULL);
	}

	int sel_cnt = _gl_data_selected_list_count(ad);
	/* Update the label text */
	_gl_thumbs_update_label_text(ad->gridinfo.nf_it, sel_cnt, false);
	gl_item *gitem = NULL;
	int i;
	int count = eina_list_count(ad->gridinfo.medias_elist);
	for (i = 0; i < count ; i++) {
		gitem = eina_list_nth(ad->gridinfo.medias_elist, i);
		if (gitem) {
			_gl_data_restore_selected(ad->selinfo.elist, gitem);
		}
	}
	if (gitem && gitem->album->elist) {
		int album_select_count = eina_list_count(gitem->album->elist);
		_gl_notify_check_selall(ad, ad->gridinfo.nf_it, ad->gridinfo.count, album_select_count);
	}

	return 0;
}

int _gl_thumbs_update_split_view(void *data, int nf_mode, char *title, bool b_idler,
                                 Elm_Naviframe_Item_Pop_Cb func)
{
	GL_CHECK_VAL(data, -1);
	gl_appdata *ad = (gl_appdata *)data;
	GL_CHECK_VAL(ad->maininfo.naviframe, -1);
	Evas_Object *layout = NULL;
	Evas_Object *view = NULL;
	/* Add view */
	view = _gl_thumbs_add_grid(ad, ad->gridinfo.layout, GL_THUMB_ALL);
	if (view == NULL) {
		evas_object_del(layout);
		gl_dbgE("Failed to add gridview!");
		return -1;
	}
	elm_object_part_content_set(ad->gridinfo.layout, "elm.swallow.view", view);
	ad->gridinfo.view = view;
	gl_dbgE("Title is : %s", title);
	elm_object_item_data_set(ad->gridinfo.nf_it, (void *)func);
	elm_object_item_text_set(ad->gridinfo.nf_it, _gl_str(title));
	GL_FREEIF(ad->gridinfo.title);
	if (title) {
		ad->gridinfo.title = strdup(title);
	}
	return 0;
}

Evas_Object *
_create_gengrid(void *data)
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

int
_create_split_album_view(void *data, Evas_Object *layout_inner)
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
		elm_gengrid_item_size_set(layout_inner, (w / 3) - 40, (w / 3) - 20);
	} else {
		elm_gengrid_item_size_set(layout_inner, (h / 3) - 20, (h / 3) - 40);
	}
	return 0;
}

int _gl_thumbs_create_view(void *data, int nf_mode, char *title, bool b_idler,
                           Elm_Naviframe_Item_Pop_Cb func)
{
	GL_CHECK_VAL(data, -1);
	gl_appdata *ad = (gl_appdata *)data;
	GL_CHECK_VAL(ad->maininfo.naviframe, -1);
	Evas_Object *layout = NULL;
	Evas_Object *view = NULL;
	Evas_Object *layout_inner = NULL;
	layout_inner = _create_gengrid(ad);
	GL_CHECK_VAL(layout_inner, -1);

	if (_gl_thumbs_get_edit_mode(data) > GL_THUMBS_EDIT_FIRST) {
		gl_set_view_mode(data, GL_VIEW_THUMBS_EDIT);
	} else {
		gl_set_view_mode(data, GL_VIEW_THUMBS);
	}

	if (_create_split_album_view(ad, layout_inner) == -1) {
		gl_dbgE("Failed to add album split view!");
		return -1;
	}

	ad->gridinfo.update_split_view = true;
	ad->gridinfo.media_display_order = ORDER_DESC;
	layout = _gl_ctrl_add_layout(ad->maininfo.naviframe);
	GL_CHECK_VAL(layout, -1);
	evas_object_size_hint_weight_set(layout, EVAS_HINT_EXPAND,
	                                 EVAS_HINT_EXPAND);
	evas_object_size_hint_align_set(layout, EVAS_HINT_FILL, EVAS_HINT_FILL);

	if (ad->gridinfo.split_view_mode) {
		int w = -1, h = -1;
		evas_object_geometry_get(ad->maininfo.win, NULL, NULL, &w, &h);
		Edje_Message_Int *msg = (Edje_Message_Int *)malloc(sizeof(Edje_Message_Int) + sizeof(int));
		GL_CHECK_VAL(msg, -1);
		msg->val = ((w < h) ? (w / 3) : (h / 3));
		edje_object_message_send(elm_layout_edje_get(layout), EDJE_MESSAGE_INT, 1, msg);
		free(msg);
		elm_object_signal_emit(layout, "elm,splitview,state,visible", "elm");
	} else {
		elm_object_signal_emit(layout, "elm,splitview,state,default", "elm");
	}

	/* Add pinch event before adding view */
	_gl_pinch_add_event(ad, layout);
	ad->gridinfo.is_append = b_idler;
	/* Add view */
	view = _gl_thumbs_add_grid(ad, layout, GL_THUMB_ALL);
	if (view == NULL) {
		evas_object_del(layout);
		gl_dbgE("Failed to add gridview!");
		return -1;
	}
	elm_object_part_content_set(layout, "elm.swallow.view", view);
	ad->gridinfo.view = view;
	ad->gridinfo.layout = layout;

	elm_object_part_content_set(layout, "split.view", layout_inner);
	__gl_thumbs_push_view(ad, ad->maininfo.naviframe, layout, nf_mode,
	                      title);
	elm_object_item_data_set(ad->gridinfo.nf_it, (void *)func);
	/* Register delete callback */
	evas_object_event_callback_add(layout, EVAS_CALLBACK_DEL,
	                               __gl_thumbs_delete_layout_cb, data);
	gl_dbg("Done");
	return 0;
}

/**
* @param: b_update_albums_list
*	True: Update albums list and medias list, then update normal view.
*	False: Get new medias for current view from DB
*	          then update view to synchronize data with Myfile.
*	          Use case:
*		1, Update gridview/listview created from Map;
*		2, Update edit gridview/listview.
*/
int _gl_thumbs_update_items(void *data)
{
	GL_CHECK_VAL(data, -1);
	gl_appdata *ad = (gl_appdata *)data;
	int view_mode = gl_get_view_mode(ad);
	bool b_edit_m = false;

	gl_dbg("view_mode: %d", view_mode);
	if (view_mode == GL_VIEW_THUMBS_EDIT) {
		gl_dbg("Edit view.");
		b_edit_m = true;
	}
	if (_gl_thumbs_check_zero(data)) {
		/* All items deleted, change to albms view */
		gl_pop_to_ctrlbar_ly(ad);
	} else {
		if (_gl_thumbs_check_zero(data)) {
			gl_dbg("none video/image, show nocontents");
			_gl_thumbs_show_nocontents(ad);
			/* Remove invalid widgets */
			gl_del_invalid_widgets(ad, GL_INVALID_NEW_ENTRY_NOC);
		} else if (view_mode == GL_VIEW_THUMBS) {
			_gl_thumbs_show_items(ad);
			/* Update buttons state */
			_gl_thumbs_check_btns(data);
		} else if (view_mode == GL_VIEW_THUMBS_EDIT) {
			_gl_thumbs_show_edit_view(ad);
		}

		/* Normal view, return */
		if (!b_edit_m) {
			return 0;
		}

		/* Get selected medias count */
		int sel_cnt = _gl_data_selected_list_count(ad);
		/* Remove invalid widgets */
		if (sel_cnt == 0) {
			gl_del_invalid_widgets(ad, GL_INVALID_NEW_ENTRY);
		}

		/* Display selectioninfo */
		gl_item *gitem = NULL;
		gitem = eina_list_nth(ad->gridinfo.medias_elist, 0);
		int album_sel_count = eina_list_count(gitem->album->elist);
		_gl_notify_check_selall(ad, ad->gridinfo.nf_it,
		                        ad->gridinfo.count, album_sel_count);
		_gl_thumbs_update_label_text(ad->gridinfo.nf_it, sel_cnt, false);
	}

	return 0;
}

int _gl_thumbs_update_view(void *data)
{
	GL_CHECK_VAL(data, -1);
	gl_appdata *ad = (gl_appdata *)data;
	gl_sdbg("grid_type: %d", ad->gridinfo.grid_type);

	switch (ad->gridinfo.grid_type) {
	case GL_GRID_T_LOCAL:
	case GL_GRID_T_ALLALBUMS:
	case GL_GRID_T_FAV:
		/* Albums list should be updated first */
	{
		gl_dbgE(" creating split album view!");
		Evas_Object *layout_inner = elm_object_part_content_get(ad->gridinfo.layout, "split.view");
		if (!layout_inner) {
			gl_dbgE("Failed to add split album view!");
		} else {
			_create_split_album_view(ad, layout_inner);
			elm_object_part_content_set(ad->gridinfo.layout, "split.view", layout_inner);
		}

		_gl_thumbs_update_items(ad);
	}
	break;
	default:
		gl_dbgE("Wrong grid type!");
		return -1;
	}
	return 0;
}

int _gl_thumbs_destroy_view(void *data, bool b_pop)
{
	GL_CHECK_VAL(data, -1);
	gl_appdata *ad = (gl_appdata *)data;
	int view_mode = gl_get_view_mode(ad);
	gl_dbg("view_mode: %d.", view_mode);

	gl_del_invalid_widgets(ad, GL_INVALID_NONE);
	_gl_albums_set_current(data, NULL);

	/* Pop to controlbar layout */
	if (b_pop) {
		/* To launch __gl_thumbs_pop_cb in thumb selected mode */
		elm_naviframe_item_pop_to(ad->gridinfo.nf_it);
		/* To skip operation about edit mode */
		if (view_mode == GL_VIEW_THUMBS_EDIT) {
			gl_set_view_mode(data, GL_VIEW_THUMBS);
		}
		elm_naviframe_item_pop_to(ad->ctrlinfo.nf_it);
	}
	return 0;
}

int _gl_thumbs_destroy_view_with_cb(void *data, void *cb)
{
	GL_CHECK_VAL(data, -1);
	gl_appdata *ad = (gl_appdata *)data;
	gl_dbg("start");

	/* Delete naviframe item dierctly, instead of poping them */
	_gl_thumbs_destroy_view(data, false);
	/* Delete naviframe item dierctly to launch layout_delete_cb immediately */
	_gl_ui_del_nf_items(ad->maininfo.naviframe, ad->ctrlinfo.nf_it);
	if (cb) {
		int (*close_cb)(void * data);
		close_cb = cb;
		close_cb(data);
	}
	gl_dbg("done");
	return 0;
}

/**
* Get all/image_grid_view.
*
* True returned if it's normal view, if it's nocontents view return false.
*/
bool _gl_thumbs_get_view(void *data, Evas_Object **view)
{
	GL_CHECK_FALSE(data);
	gl_appdata *ad = (gl_appdata *)data;
	Evas_Object *gv = NULL;
	bool b_view = false;

	gv = ad->gridinfo.view;
	if (gv && ad->gridinfo.nocontents == NULL) {
		b_view = true;
	}

	/* Nocontents */
	if (!b_view) {
		gl_dbg("Previous view is Nocontents...");
	}

	/* Return view */
	if (view) {
		*view = gv;
	}

	return b_view;
}

/**
 * Show Nocontents in All/Images/Videos segment.
 */
bool _gl_thumbs_show_nocontents(void *data)
{
	GL_CHECK_FALSE(data);
	gl_appdata *ad = (gl_appdata *)data;

	/* Clear invalid objects if needed */
	_gl_thumbs_delete_view(ad);
	/* Create nocontents widget */
	Evas_Object *noc = NULL;
	noc = _gl_nocontents_create(ad->gridinfo.layout);
	ad->gridinfo.view = noc;
	ad->gridinfo.nocontents = noc;
	elm_object_part_content_unset(ad->gridinfo.layout, "elm.swallow.view");
	evas_object_show(ad->gridinfo.view);
	elm_object_part_content_set(ad->gridinfo.layout, "elm.swallow.view",
	                            ad->gridinfo.view);
	_gl_thumbs_check_btns(ad);
	return true;
}

/* Assign new value to all/image_grid_nocontents */
int _gl_thumbs_set_nocontents(void *data, Evas_Object *noc)
{
	GL_CHECK_VAL(data, -1);
	gl_appdata *ad = (gl_appdata *)data;
	ad->gridinfo.nocontents = noc;
	return 0;
}

int _gl_thumbs_set_size(void *data, Evas_Object *gridview)
{
	GL_CHECK_VAL(data, -1);
	gl_appdata *ad = (gl_appdata *)data;
	Evas_Object *gv = NULL;

	if (gridview) {
		gv = gridview;
	} else {
		bool b_view = false;
		if (gl_get_view_mode(data) == GL_VIEW_THUMBS_SELECT) {
			b_view = true;
			gv = ad->gridinfo.select_view;
		} else {
			b_view = _gl_thumbs_get_view(ad, &gv);
		}

		if (!b_view || !gv) {
			gl_dbgE("gridview is invalid!");
			return -1;
		}
	}
	if (ad->gridinfo.split_view_mode == DETAIL_VIEW) {
		_gl_thumb_set_size(ad, gv, &(ad->gridinfo.icon_w),
		                   &(ad->gridinfo.icon_h));
	} else if (ad->gridinfo.split_view_mode == SPLIT_VIEW) {
		_gl_thumb_split_set_size(ad, gv);
	}

	if (gl_get_view_mode(ad) == GL_VIEW_THUMBS_SELECT) {
		_gl_thumb_split_set_size(ad, gv);
	}

	return 0;
}

int _gl_thumbs_clear_cbs(Evas_Object *grid)
{
	GL_CHECK_VAL(grid, -1);
	evas_object_smart_callback_del(grid, "realized", __gl_thumbs_realized);
	evas_object_smart_callback_del(grid, "unrealized",
	                               __gl_thumbs_unrealized);
	evas_object_smart_callback_del(grid, "longpressed",
	                               __gl_thumbs_longpressed_cb);
	return 0;
}

int _gl_thumbs_delete_view(void *data)
{
	GL_CHECK_VAL(data, -1);
	gl_appdata *ad = (gl_appdata *)data;

	if (ad->gridinfo.view) {
		_gl_thumbs_clear_cbs(ad->gridinfo.view);
		_gl_ui_del_scroller_pos(ad->gridinfo.view);
		evas_object_del(ad->gridinfo.view);
		ad->gridinfo.view = NULL;
	}
	return 0;
}

/* Update griditem size */
int _gl_thumbs_update_size(void *data, Evas_Object *view)
{
	GL_CHECK_VAL(data, -1);
	gl_appdata *ad = (gl_appdata *)data;
	Evas_Object *_view = NULL;
	int view_mode = gl_get_view_mode(ad);
	gl_dbg("");

	if (view) {
		_view = view;
	} else {
		bool b_view = false;
		if (view_mode == GL_VIEW_THUMBS_SELECT) {
			b_view = true;
			_view = ad->gridinfo.select_view;
		} else {
			b_view = _gl_thumbs_get_view(ad, &_view);
		}

		if (!b_view || !_view) {
			gl_dbgE("gridview is invalid!");
			return -1;
		}
	}

	_gl_thumbs_set_size(data, _view);

	_gl_thumb_update_gengrid(_view);

	return 0;
}

/**
 * Check to append a item to gengrid or not,
 * if it's a normal item or it's the first burstshot item then append it to view
 */
bool _gl_thumbs_is_append(gl_item *gitem, char **burstshot_id)
{
	GL_CHECK_FALSE(gitem);
	GL_CHECK_FALSE(gitem->item);
	GL_CHECK_FALSE(burstshot_id);
	bool b_to_append = false;

	if (gitem->item->type == MEDIA_CONTENT_TYPE_IMAGE &&
	        gitem->item->image_info &&
	        gitem->item->image_info->burstshot_id) {
		/* First birstshot item,  two jacent birstshot items  */
		if (*burstshot_id == NULL ||
		        g_strcmp0(*burstshot_id, gitem->item->image_info->burstshot_id)) {
			*burstshot_id = gitem->item->image_info->burstshot_id;
			b_to_append = true;
		}
	} else {
		/* Normal item(not burstshot) */
		b_to_append = true;
		*burstshot_id = NULL;
	}
	return b_to_append;
}

bool _gl_thumbs_check_zero(void *data)
{
	GL_CHECK_VAL(data, -1);
	Eina_List *medias_elist = NULL;

	_gl_thumbs_get_list(data, &medias_elist);
	return (eina_list_count(medias_elist) <= 0);
}

/* Update the label text, recreate label to disable effect in edit view */
int _gl_thumbs_update_label(Elm_Object_Item *nf_it, char *title)
{
	GL_CHECK_VAL(nf_it, -1);
	elm_object_item_part_text_set(nf_it, "elm.text.title", _gl_str(title));
	if (_gl_is_str_id(title)) {
		_gl_ui_set_translatable_item(nf_it, title);
	}
	return 0;
}

/* Update the label text */
int _gl_thumbs_update_label_text(Elm_Object_Item *nf_it, int sel_cnt,
                                 bool b_lang)
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
		text = g_strdup_printf(pd_selected, sel_cnt);
		_gl_thumbs_update_label(nf_it, text);
		GL_GFREEIF(text);
	} else if (!b_lang) {
		/* Don't need to update text if it's called by language_changed_cb*/
		_gl_thumbs_update_label(nf_it, GL_STR_ID_SELECT_ITEM);
	}
	return 0;
}

int _gl_thumbs_update_lang(void *data)
{
	GL_CHECK_VAL(data, -1);
	return 0;
}

int _gl_thumbs_disable_slideshow(void *data, bool b_disabled)
{
	GL_CHECK_VAL(data, -1);
	gl_appdata *ad = (gl_appdata *)data;
	gl_dbg("b_disable: %d", b_disabled);
	ad->gridinfo.b_slideshow_disable = b_disabled;
	return 0;
}

int _gl_thumbs_disable_share(void *data, bool b_disabled)
{
	GL_CHECK_VAL(data, -1);
	gl_appdata *ad = (gl_appdata *)data;
	gl_dbg("b_disable: %d", b_disabled);
	GL_CHECK_VAL(ad->gridinfo.nf_it, -1);
	Evas_Object *btn = NULL;
	btn = elm_object_item_part_content_get(ad->gridinfo.nf_it,
	                                       GL_NAVIFRAME_TITLE_LEFT_BTN);
	if (btn) {
		elm_object_disabled_set(btn, b_disabled);
	} else {
		gl_dbgE("Failed to get share button!");
	}
	return 0;
}

/* Check buttons state when coming back from albums-select view */
int _gl_thumbs_check_btns(void *data)
{
	GL_CHECK_VAL(data, -1);
	__gl_thumbs_mode_reset_btns(data);
	return 0;
}

int _gl_thumbs_update_sequence(void *data)
{
	GL_CHECK_VAL(data, -1);
	gl_appdata *ad = (gl_appdata *)data;
	gl_item *gitem = NULL;
	Elm_Object_Item *first_it = NULL;
	Elm_Object_Item *next_it = NULL;
	Elm_Object_Item *last_it = NULL;

	Evas_Object *view = ad->gridinfo.view;
	if (gl_get_view_mode(data) == GL_VIEW_THUMBS_SELECT) {
		view = ad->gridinfo.select_view;
	}
	GL_CHECK_VAL(view, -1);
	first_it = elm_gengrid_first_item_get(view);
	last_it = elm_gengrid_last_item_get(view);

	int sequence = 1;
	while (first_it) {
		next_it = elm_gengrid_item_next_get(first_it);

		if (elm_gengrid_item_item_class_get(first_it) == &(ad->gridinfo.date_gic)) {
			goto GL_THUMBS_NEXT;
		}

		gitem = elm_object_item_data_get(first_it);
		if (gitem) {
			gitem->sequence = sequence++;
		}
		gitem = NULL;

GL_THUMBS_NEXT:

		if (last_it == first_it) {
			gl_dbg("done!");
			break;
		} else {
			first_it = next_it;
		}
	}
	return 0;
}

int _gl_thumbs_set_edit_mode(void *data, int mode)
{
	GL_CHECK_VAL(data, -1);
	gl_appdata *ad = (gl_appdata *)data;
	ad->gridinfo.edit_mode = mode;
	return 0;
}

int _gl_thumbs_get_edit_mode(void *data)
{
	GL_CHECK_FALSE(data);
	gl_appdata *ad = (gl_appdata *)data;
	return ad->gridinfo.edit_mode;
}

int _gl_thumbs_rotate_view(void *data)
{
	GL_CHECK_VAL(data, -1);
	gl_appdata *ad = (gl_appdata *)data;
	Eina_List *its = NULL;
	Eina_List *l = NULL;
	Elm_Object_Item *it = NULL;
	gl_item *gitem = NULL;

	if (ad->gridinfo.view == ad->gridinfo.nocontents) {
		return -1;
	}

	int w = -1;
	int h = -1;
	if (gl_get_view_mode(ad) == GL_VIEW_THUMBS_EDIT) {
		evas_object_geometry_get(ad->maininfo.win, NULL, NULL, &w, &h);
		Edje_Message_Int *msg = (Edje_Message_Int *)malloc(sizeof(Edje_Message_Int) + sizeof(int));
		GL_CHECK_VAL(msg, -1);
		if (ad->gridinfo.split_view_mode == DETAIL_VIEW) {
			msg->val = h;
		} else {
			msg->val = ((w < h) ? (h - (w / 3)) : (w - (h / 3)));
		}
		gl_dbgE("msg value : %d", msg->val);
		edje_object_message_send(elm_layout_edje_get(ad->gridinfo.layout), EDJE_MESSAGE_INT, 2, msg);
		free(msg);
		elm_object_signal_emit(ad->gridinfo.layout, "elm,selectall,state,visible", "elm");
	}
	_gl_thumbs_set_size(data, NULL);

	its = elm_gengrid_realized_items_get(ad->gridinfo.view);
	GL_CHECK_VAL(its, -1);

	EINA_LIST_FOREACH(its, l, it) {
		if (it == NULL) {
			gl_dbgE("Invalid item!");
			continue;
		}

		gitem = elm_object_item_data_get(it);
		if (gitem && gitem->item && gitem->item->image_info &&
		        gitem->item->image_info->burstshot_id) {
			/* Realize burstshot item again */
			elm_gengrid_item_update(it);
		}
	}
	return 0;
}

void _gl_thumbs_change_view(void *data, int prev_x, int current_x)
{
	GL_CHECK(data);
	gl_appdata *ad = (gl_appdata *)data;
	int diff = 0;
	int w = -1;
	int h = -1;

	evas_object_geometry_get(ad->maininfo.win, NULL, NULL, &w, &h);
	int offset = ((w < h) ? (w / 8) : (h / 8));
	diff = (current_x - prev_x);
	int view_mode = gl_get_view_mode(ad);
	if (view_mode == GL_VIEW_THUMBS_EDIT || view_mode == GL_VIEW_THUMBS) {
		if ((diff + offset) < 0) {
			elm_object_signal_emit(ad->gridinfo.layout, "elm,splitview,state,default", "elm");
			ad->gridinfo.split_view_mode = DETAIL_VIEW;
			_gl_thumbs_set_size(ad, NULL);
		} else if ((diff - offset) > 0) {
			if (ad->gridinfo.split_view_mode != SPLIT_VIEW) {
				evas_object_geometry_get(ad->maininfo.win, NULL, NULL, &w, &h);
				Edje_Message_Int *msg = (Edje_Message_Int *)malloc(sizeof(Edje_Message_Int) + sizeof(int));
				GL_CHECK(msg);
				msg->val = ((w < h) ? (w / 3) : (h / 3));
				edje_object_message_send(elm_layout_edje_get(ad->gridinfo.layout), EDJE_MESSAGE_INT, 1, msg);
				free(msg);
				elm_object_signal_emit(ad->gridinfo.layout, "elm,splitview,state,default", "elm");
				elm_object_signal_emit(ad->gridinfo.layout, "elm,splitview,state,visible", "elm");
				ad->gridinfo.split_view_mode = SPLIT_VIEW;
				_gl_thumbs_set_size(ad, NULL);
			}
		}
		if (view_mode == GL_VIEW_THUMBS_EDIT) {
			Edje_Message_Int *msg = (Edje_Message_Int *)malloc(sizeof(Edje_Message_Int) + sizeof(int));
			GL_CHECK(msg);
			if (ad->gridinfo.split_view_mode == DETAIL_VIEW) {
				msg->val = w;
			} else {
				msg->val = ((w < h) ? (w - (w / 3)) : (w - (h / 3)));
			}
			gl_dbgE("msg value : %d", msg->val);
			edje_object_message_send(elm_layout_edje_get(ad->gridinfo.layout), EDJE_MESSAGE_INT, 2, msg);
			free(msg);
			elm_object_signal_emit(ad->gridinfo.layout, "elm,selectall,state,visible", "elm");
		}
	}
}

void _gl_thumb_update_split_view_badge(void *data)
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

void _gl_thumb_update_split_view(void *data)
{
	GL_CHECK(data);
	gl_appdata *ad = (gl_appdata *)data;
	if (ad->gridinfo.update_split_view) {

		if (ad->gridinfo.layout) {
			Evas_Object *layout_inner = elm_object_part_content_get(ad->gridinfo.layout, "split.view");
			if (!layout_inner) {
				gl_dbgE("Failed to add split album view!");
			} else {
				_create_split_album_view(ad, layout_inner);
				elm_object_part_content_set(ad->gridinfo.layout, "split.view", layout_inner);
				_gl_thumb_update_split_view_badge(data);
			}
		}
	} else {
		ad->gridinfo.update_split_view = true;
	}
}
