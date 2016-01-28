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
#include "gl-albums.h"
#include "gl-thumbs.h"
#include "gl-main.h"
#include "gl-controlbar.h"
#include "gl-ui-util.h"
#include "gl-util.h"
#include "gl-button.h"
#include "gl-data.h"
#include "gl-popup.h"
#include "gl-ctxpopup.h"
#include "gl-progressbar.h"
#include "gl-pinchzoom.h"
#include "gl-tile.h"
#include "gl-ext-exec.h"
#include "gl-thumbs-sel.h"
#include "gl-thumbs-edit.h"
#include "gl-albums-sel.h"
#include "gl-albums-edit.h"
#include "gl-albums-rename.h"
#include "gl-albums-new.h"
#include "gl-strings.h"
#include "gl-nocontents.h"
#include "gl-notify.h"
#include "gl-ext-ug-load.h"
#ifdef _USE_ROTATE_BG
#include "gl-rotate-bg.h"
#include "gl-exif.h"
#endif
#include "gl-gesture.h"
#include "gl-file-util.h"

static int __gl_albums_create_thumb(gl_item *gitem, gl_cluster *album);
static Eina_Bool __gl_albums_pop_op(void *data);
#ifdef SUPPORT_SLIDESHOW
static int __gl_albums_slideshow(void *data, const char *label);
#endif

static void __gl_albums_realized(void *data, Evas_Object *obj, void *ei)
{
	GL_CHECK(data);
	/* Register idler if needed */
	_gl_main_add_reg_idler(data);

	GL_CHECK(ei);
	Elm_Object_Item *it = (Elm_Object_Item *)ei;
	gl_cluster *album = elm_object_item_data_get(it);
	GL_CHECK(album);
	GL_CHECK(album->cluster);
	GL_CHECK(album->cover);
	GL_CHECK(album->cover->item);

	gl_dbg("realized");
	if (!GL_FILE_EXISTS(album->cover->item->thumb_url) &&
	        GL_FILE_EXISTS(album->cover->item->file_url) &&
	        (album->cluster->type == GL_STORE_T_PHONE ||
	         album->cluster->type == GL_STORE_T_MMC ||
	         album->cluster->type == GL_STORE_T_ALL)) {
		__gl_albums_create_thumb(album->cover, album);
	}
}

static void __gl_albums_unrealized(void *data, Evas_Object *obj, void *ei)
{
	gl_dbg("unrealized");
	GL_CHECK(ei);
	Elm_Object_Item *it = (Elm_Object_Item *)ei;
	gl_cluster *album = elm_object_item_data_get(it);
	GL_CHECK(album);
	GL_CHECK(album->cluster);
	GL_CHECK(album->cover);
	GL_CHECK(album->cover->item);

	/* Checking for local files only */
	if (album->cluster->type == GL_STORE_T_PHONE ||
	        album->cluster->type == GL_STORE_T_MMC ||
	        album->cluster->type == GL_STORE_T_ALL) {
		if (album->cover->item->b_create_thumb) {
			_gl_data_cancel_thumb(album->cover->item);
		}
	}
}

static void __gl_split_albums_realized(void *data, Evas_Object *obj, void *ei)
{
	gl_dbg("split view realized");
	GL_CHECK(data);
	GL_CHECK(ei);
	Elm_Object_Item *it = (Elm_Object_Item *)ei;
	gl_cluster *album = elm_object_item_data_get(it);
	GL_CHECK(album);
	GL_CHECK(album->cluster);
	GL_CHECK(album->cover);
	GL_CHECK(album->cover->item);
	gl_appdata *ad = (gl_appdata *)album->ad;

	if (ad->albuminfo.grid_sel_item) {
		if (ad->albuminfo.selected_uuid) {
			gl_dbg("Current item UUID : %s ", album->cluster->uuid);
			gl_dbg("album view selected item UUID : %s ", ad->albuminfo.selected_uuid);
			if (!strcmp(album->cluster->uuid, ad->albuminfo.selected_uuid)) {
				ad->albuminfo.grid_sel_item = ei;
				elm_object_item_signal_emit((Elm_Object_Item *)ei, "elm,state,focused", "elm");
			} else {
				elm_object_item_signal_emit((Elm_Object_Item *)ei, "elm,state,unfocused", "elm");
			}
		}
	}
	gl_dbg("realized");
}

#if 0
static void __gl_albums_lang_changed(void *data, Evas_Object *obj, void *ei)
{
	GL_CHECK(data);
	gl_appdata *ad = (gl_appdata *)data;
	gl_albums_update_items(ad);
}
#endif

void __gl_albums_new_album_sel(void *data, Evas_Object *obj, void *ei)
{
	GL_CHECK(data);
	gl_cluster *album_item = (gl_cluster *)data;
	GL_CHECK(album_item->cluster);
	GL_CHECK(album_item->ad);
	gl_appdata *ad = (gl_appdata *)album_item->ad;
	int view_mode = gl_get_view_mode(ad);
	elm_gengrid_item_selected_set(ei, EINA_FALSE);
	_gl_data_selected_list_finalize(ad);

	gl_dbg("view mode: %d.", view_mode);
	if (view_mode != GL_VIEW_ALBUMS_EDIT) {
		if (album_item->cluster->count == 0) {
			gl_dbgW("Empty album, return!");
			return;
		}
		/* Save scroller position before clearing gengrid */
		_gl_ui_save_scroller_pos(obj);
		gl_albums_open_album(album_item);
		elm_object_item_signal_emit((Elm_Object_Item *)ei, "elm,state,focused", "elm");
		if (ad->albuminfo.grid_sel_item) {
			gl_cluster *album_data = NULL;
			album_data = elm_object_item_data_get(ad->albuminfo.grid_sel_item);
			if (album_data) {
				if (0 == strcmp(album_item->cluster->uuid, album_data->cluster->uuid)) {
					gl_dbgW("Item matched. No need to remove focus");
				} else {
					elm_object_item_signal_emit(ad->albuminfo.grid_sel_item, "elm,state,unfocused", "elm");
				}
			}
		}
		ad->albuminfo.grid_sel_item = ei;
		if (ad->albuminfo.selected_uuid) {
			free(ad->albuminfo.selected_uuid);
			ad->albuminfo.selected_uuid = NULL;
		}
		ad->albuminfo.selected_uuid = strdup(album_item->cluster->uuid);
	}
}

/* Callback of album item selection */
static void __gl_albums_sel_cb(void *data, Evas_Object *obj, void *ei)
{
	GL_CHECK(data);
	gl_cluster *album_item = (gl_cluster *)data;
	GL_CHECK(album_item->cluster);
	GL_CHECK(album_item->ad);
	gl_appdata *ad = (gl_appdata *)album_item->ad;
	int view_mode = gl_get_view_mode(ad);
	elm_gengrid_item_selected_set(ei, EINA_FALSE);

	gl_dbg("view mode: %d.", view_mode);
	if (view_mode != GL_VIEW_ALBUMS_EDIT && view_mode != GL_VIEW_ALBUM_REORDER) {
		if (album_item->cluster->count == 0) {
			gl_dbgW("Empty album, return!");
			return;
		}
		/* Save scroller position before clearing gengrid */
		_gl_ui_save_scroller_pos(obj);
		gl_albums_open_album(album_item);
		elm_object_item_signal_emit((Elm_Object_Item *)ei, "elm,state,focused", "elm");
		if (ad->albuminfo.grid_sel_item) {
			gl_cluster *album_data = NULL;
			album_data = elm_object_item_data_get(ad->albuminfo.grid_sel_item);
			if (album_data) {
				if (strcmp(album_item->cluster->uuid, album_data->cluster->uuid)) {
					elm_object_item_signal_emit(ad->albuminfo.grid_sel_item, "elm,state,unfocused", "elm");
				}
			}
		}
		ad->albuminfo.grid_sel_item = ei;
		if (ad->albuminfo.selected_uuid) {
			free(ad->albuminfo.selected_uuid);
			ad->albuminfo.selected_uuid = NULL;
		}
		ad->albuminfo.selected_uuid = strdup(album_item->cluster->uuid);
	}
}

static char *__gl_albums_get_text(void *data, Evas_Object *obj, const char *part)
{
	GL_PROFILE_IN;
	GL_CHECK_NULL(part);
	GL_CHECK_NULL(strlen(part));
	GL_CHECK_NULL(data);
	gl_cluster *album_item = (gl_cluster *)data;
	GL_CHECK_NULL(album_item->cluster);
	GL_CHECK_NULL(album_item->cluster->display_name);
	GL_CHECK_NULL(album_item->cluster->uuid);
	GL_CHECK_NULL(album_item->ad);
	char buf[GL_ALBUM_NAME_LEN_MAX] = { 0, };

	if (!g_strcmp0(part, "elm.text.name")) {
		GL_PROFILE_F_OUT("__gl_albums_get_text(name)");
		GL_PROFILE_F_OUT("__gl_albums_get_text(count)");
	} else if (!g_strcmp0(part, "elm.text.date")) {
		if (album_item->cover) {
			_gl_data_util_free_gitem(album_item->cover);
			album_item->cover = NULL;
		}
		gl_item *gitem = NULL;
		_gl_data_get_album_cover(album_item, &gitem,
		                         MEDIA_CONTENT_ORDER_DESC);
		if (gitem == NULL || gitem->item == NULL) {
			gl_dbg("Empty album...");
			_gl_data_util_free_gitem(gitem);
			album_item->cover_thumbs_cnt = 0;
			return NULL;
		}

		album_item->cover_thumbs_cnt = GL_ALBUM_COVER_THUMB_NUM;
		album_item->cover = gitem;
		GL_PROFILE_F_OUT("__gl_albums_get_text(date)");
	} else if (!g_strcmp0(part, "elm.text.badge")) {
		if (gl_get_view_mode(album_item->ad) == GL_VIEW_THUMBS_EDIT ||
		        gl_get_view_mode(album_item->ad) == GL_VIEW_THUMBS_SELECT) {
			if (album_item->elist) {
				int sel_cnt = eina_list_count(album_item->elist);
				if (sel_cnt > 0) {
					elm_object_item_signal_emit(album_item->item,
					                            "elm,state,elm.text.badge,visible",
					                            "elm");
					snprintf(buf, sizeof(buf), "%d", sel_cnt);
				} else {
					elm_object_item_signal_emit(album_item->item,
					                            "elm,state,elm.text.badge,hidden",
					                            "elm");
				}
			} else {
				elm_object_item_signal_emit(album_item->item,
				                            "elm,state,elm.text.badge,hidden",
				                            "elm");
			}
		} else {
			elm_object_item_signal_emit(album_item->item,
			                            "elm,state,elm.text.badge,hidden",
			                            "elm");
		}
	}
	return strdup(buf);
}

/* Only for local medias */
static void __gl_albums_create_thumb_cb(media_content_error_e error,
                                        const char *path, void *user_data)
{
	GL_CHECK(user_data);
	gl_album_data_s *album_data = (gl_album_data_s *)user_data;
	GL_CHECK(album_data->ad);
	gl_appdata *ad = (gl_appdata *)album_data->ad;
	gl_cluster *album = album_data->album;
	GL_FREE(album_data);
	album->album_data = NULL;

	if (gl_get_view_mode(ad) != GL_VIEW_ALBUMS &&
	        gl_get_view_mode(ad) != GL_VIEW_ALBUMS_EDIT &&
	        gl_get_view_mode(ad) != GL_VIEW_ALBUMS_SELECT) {
		return;
	}

	if (error == MEDIA_CONTENT_ERROR_NONE && GL_FILE_EXISTS(path) &&
	        g_strcmp0(path, GL_ICON_DB_DEFAULT_THUMB)) {
		GL_CHECK(album);
		GL_CHECK(album->item);
		elm_gengrid_item_update(album->item);
	} else {
		gl_sdbgE("[%d]Invalid path[%s]!", error, path);
	}
}

/* Use file to create new thumb if possible */
static int __gl_albums_create_thumb(gl_item *gitem, gl_cluster *album)
{
	GL_CHECK_VAL(gitem, -1);
	GL_CHECK_VAL(gitem->item, -1);
	GL_CHECK_VAL(gitem->item->file_url, -1);
	GL_CHECK_VAL(album, -1);

	if (GL_FILE_EXISTS(gitem->item->file_url)) {
		gl_album_data_s *album_data = NULL;
		album_data = (gl_album_data_s *)calloc(1, sizeof(gl_album_data_s));
		GL_CHECK_VAL(album_data, -1);
		album_data->ad = album->ad;
		album_data->album = album;
		album->album_data = album_data;
		_gl_data_create_thumb(gitem->item, __gl_albums_create_thumb_cb,
		                      album_data);
		return 0;
	}
	return -1;
}

static int __gl_albums_set_bg_file(Evas_Object *bg, void *data)
{
	GL_CHECK_VAL(data, -1);
	gl_item *git = (gl_item *)data;
	GL_CHECK_VAL(git->album, -1);

	char *bg_path = GL_ICON_NO_THUMBNAIL;
	gl_icon_type_e ret_val = -1;

	if (git == NULL || git->item == NULL) {
		gl_dbgE("Invalid item :%p", git);
		goto GL_ALBUMS_FAILED;
	}

	ret_val = 0;
	bg_path = GL_ICON_NO_THUMBNAIL;
	if (GL_FILE_EXISTS(git->item->thumb_url)) {
		bg_path = git->item->thumb_url;
	} else {
		ret_val = -1;
	}

GL_ALBUMS_FAILED:

#ifdef _USE_ROTATE_BG
	_gl_rotate_bg_set_image_file(bg, bg_path);
#else
	elm_bg_file_set(bg, bg_path, NULL);
#endif

	return ret_val;
}

static Evas_Object *__gl_albums_get_type_icon(Evas_Object *obj, gl_cluster *album)
{
	GL_CHECK_NULL(album);
	GL_CHECK_NULL(album->cluster);
	GL_CHECK_NULL(obj);
	Evas_Object *_obj = NULL;

	if (_gl_data_is_camera_album(album->cluster))
		_obj = _gl_tile_show_part_type_icon(obj,
		                                    GL_TILE_TYPE_CAMERA);
	else if (_gl_data_is_default_album(GL_STR_ALBUM_DOWNLOADS, album->cluster))
		_obj = _gl_tile_show_part_type_icon(obj,
		                                    GL_TILE_TYPE_DOWNLOAD);
	else
		_obj = _gl_tile_show_part_type_icon(obj,
		                                    GL_TILE_TYPE_FOLDER);
	return _obj;
}

static void
_gl_count_set(Evas_Object *layout, const char *text)
{
	Evas_Object *edje;
	Edje_Message_Float *msg;
	if (text) {
		elm_layout_text_set(layout, "elm.sub.text", text);
	} else {
		elm_layout_text_set(layout, "elm.sub.text", "");
	}
	edje = elm_layout_edje_get(layout);
	msg = calloc(1, sizeof(Edje_Message_Float));
	if (msg) {
		msg->val = elm_config_scale_get();
		edje_object_message_send(edje, EDJE_MESSAGE_FLOAT, 0, msg);
	}
}

static Evas_Object *__gl_albums_get_content(void *data, Evas_Object *obj,
					const char *part)
{
	GL_CHECK_NULL(part);
	GL_CHECK_NULL(strlen(part));
	GL_CHECK_NULL(data);
	gl_cluster *album_item = (gl_cluster *)data;
	GL_CHECK_NULL(album_item->cluster);
	GL_CHECK_NULL(album_item->ad);
	gl_appdata *ad = (gl_appdata *)album_item->ad;
	GL_CHECK_NULL(album_item->cluster->uuid);
	int view_mode = gl_get_view_mode(ad);

	if (view_mode != GL_VIEW_ALBUMS &&
	        view_mode != GL_VIEW_ALBUMS_EDIT &&
	        view_mode != GL_VIEW_ALBUMS_SELECT &&
	        view_mode != GL_VIEW_THUMBS &&
	        view_mode != GL_VIEW_THUMBS_EDIT &&
	        view_mode != GL_VIEW_THUMBS_SELECT &&
	        view_mode != GL_VIEW_ALBUM_REORDER) {
		gl_dbg("Album is empty, view mode is : % d", view_mode);
		return NULL;
	}
	Evas_Object *_obj = NULL;


	if (!g_strcmp0(part, "elm.text.name.swallow")) {
		if (ad->maininfo.b_paused) {
			ad->maininfo.lang_changed = true;
		}
		char cnt[GL_ALBUM_NAME_LEN_MAX] = { 0, };
		Evas_Object *layout = elm_layout_add(obj);
		char *path = _gl_get_edje_path();
		GL_CHECK_NULL(path);
		elm_layout_file_set(layout, path, "ceter_algined_text_layout");
		free(path);
		char *i18n_name = _gl_get_i18n_album_name(album_item->cluster);
		elm_layout_text_set(layout, "elm.text", elm_entry_utf8_to_markup(_gl_str(i18n_name)));
		snprintf(cnt, sizeof(cnt), "(%d)", (int)(album_item->cluster->count));
		_gl_count_set(layout, cnt);

		return layout;
	} else if (!g_strcmp0(part, GL_TILE_ICON)) {
		if (album_item->cover_thumbs_cnt <= 0) {
			gl_dbg("Empty album");
		}

		_obj = _gl_tile_show_part_icon(obj, part,
		                               __gl_albums_set_bg_file,
		                               (void *)(album_item->cover));
		int first_cnt = 2;
		int level = (int)evas_object_data_get(ad->albuminfo.view,
		                                      GL_GESTURE_KEY_PINCH_LEVEL);
		if (ad->maininfo.rotate_mode == APP_DEVICE_ORIENTATION_90 ||
		        ad->maininfo.rotate_mode == APP_DEVICE_ORIENTATION_270 ||
		        level == GL_ZOOM_NONE) {
			first_cnt = 3;
		}
		if (album_item->index < first_cnt && album_item->item)
			elm_object_item_signal_emit(album_item->item,
			                            "show_top_pad",
			                            "padding.top");
	} else if (!g_strcmp0(part, GL_TILE_TYPE_ICON)) {
		_obj = __gl_albums_get_type_icon(obj, album_item);
	} else if (!g_strcmp0(part, GL_TILE_SD_ICON)) {
		if (GL_STORE_T_MMC == album_item->cluster->type) {
			_obj = _gl_tile_show_part_type_icon(obj,
			                                    GL_TILE_TYPE_MMC_STORAGE);
		} else {
			elm_object_item_signal_emit(album_item->item,
			                            "hide_sd_icon",
			                            "elm.swallow.sd_icon");
		}
	} else if (view_mode == GL_VIEW_ALBUMS_EDIT) {
		_obj = _gl_albums_edit_add_content(data, obj, part);
	}
	return _obj;
}

static Evas_Object *__gl_albums_get_content_split_view(void *data, Evas_Object *obj,
					const char *part)
{
	GL_CHECK_NULL(part);
	GL_CHECK_NULL(strlen(part));
	GL_CHECK_NULL(data);
	gl_cluster *album_item = (gl_cluster *)data;
	GL_CHECK_NULL(album_item->cluster);
	GL_CHECK_NULL(album_item->ad);
	gl_appdata *ad = (gl_appdata *)album_item->ad;
	GL_CHECK_NULL(album_item->cluster->uuid);
	int view_mode = gl_get_view_mode(ad);

	if (view_mode != GL_VIEW_ALBUMS &&
	        view_mode != GL_VIEW_ALBUMS_EDIT &&
	        view_mode != GL_VIEW_ALBUMS_SELECT &&
	        view_mode != GL_VIEW_THUMBS &&
	        view_mode != GL_VIEW_THUMBS_EDIT &&
	        view_mode != GL_VIEW_THUMBS_SELECT &&
	        view_mode != GL_VIEW_ALBUM_REORDER) {
		gl_dbg("Album is empty, view mode is : % d", view_mode);
		return NULL;
	}
	Evas_Object *_obj = NULL;


	if (!g_strcmp0(part, "elm.text.name.swallow")) {
		char cnt[GL_ALBUM_NAME_LEN_MAX] = { 0, };
		Evas_Object *layout = elm_layout_add(obj);
		char *path = _gl_get_edje_path();
		GL_CHECK_NULL(path);
		elm_layout_file_set(layout, path, "ceter_algined_text_layout_split_view");
		free(path);
		char *i18n_name = _gl_get_i18n_album_name(album_item->cluster);
		elm_layout_text_set(layout, "elm.text", _gl_str(i18n_name));
		snprintf(cnt, sizeof(cnt), "(%d)", (int)(album_item->cluster->count));
		_gl_count_set(layout, cnt);

		return layout;
	} else if (!g_strcmp0(part, GL_TILE_ICON)) {
		if (album_item->cover_thumbs_cnt <= 0) {
			gl_dbg("Empty album");
		}

		_obj = _gl_tile_show_part_icon(obj, part,
		                               __gl_albums_set_bg_file,
		                               (void *)(album_item->cover));
		int first_cnt = 2;
		int level = (int)evas_object_data_get(ad->albuminfo.view,
		                                      GL_GESTURE_KEY_PINCH_LEVEL);
		if (ad->maininfo.rotate_mode == APP_DEVICE_ORIENTATION_90 ||
		        ad->maininfo.rotate_mode == APP_DEVICE_ORIENTATION_270 ||
		        level == GL_ZOOM_NONE) {
			first_cnt = 3;
		}
		if (album_item->index < first_cnt && album_item->item)
			elm_object_item_signal_emit(album_item->item,
			                            "show_top_pad",
			                            "padding.top");
	} else if (!g_strcmp0(part, GL_TILE_TYPE_ICON)) {
		_obj = __gl_albums_get_type_icon(obj, album_item);
	} else if (!g_strcmp0(part, GL_TILE_SD_ICON)) {
		if (GL_STORE_T_MMC == album_item->cluster->type) {
			_obj = _gl_tile_show_part_type_icon(obj,
			                                    GL_TILE_TYPE_MMC_STORAGE);
		} else {
			elm_object_item_signal_emit(album_item->item,
			                            "hide_sd_icon",
			                            "elm.swallow.sd_icon");
		}
	} else if (view_mode == GL_VIEW_ALBUMS_EDIT) {
		_obj = _gl_albums_edit_add_content(data, obj, part);
	}
	return _obj;
}


static Eina_Bool __gl_albums_close_album_cb(void *data, Elm_Object_Item *it)
{
	GL_CHECK_FALSE(data);
	gl_appdata *ad = (gl_appdata *)data;
	gl_dbg("");
	_gl_albums_close_album(ad);
	_gl_albums_check_btns(data);
	return EINA_TRUE;
}

static void __gl_albums_longpressed_cb(void *data, Evas_Object *obj, void *ei)
{
	gl_dbg("unrealized");
	GL_CHECK(ei);
	GL_CHECK(data);

	if (GL_VIEW_ALBUMS != gl_get_view_mode(data)) {
		return;
	}
	Elm_Object_Item *it = (Elm_Object_Item *)ei;
	gl_cluster *album = elm_object_item_data_get(it);
	GL_CHECK(album);
	_gl_albums_set_current(data, album);
#if 0
	_gl_data_finalize_albums_selected_list(data);
	_gl_data_albums_selected_list_append(data, album);
#endif
}

/* Free data after layout deleted */
static void __gl_albums_delete_layout_cb(void *data, Evas *e, Evas_Object *obj,
			void *ei)
{
	gl_dbg("Delete layout ---");
	GL_CHECK(data);
	gl_albums_free_data(data);
	_gl_data_clear_cluster_list(data, true);
	gl_dbg("Delete layout +++");
}

static void __gl_albums_newalbum_cb(void *data, Evas_Object *obj, void *ei)
{
	GL_CHECK(data);
	gl_appdata *ad = (gl_appdata *)data;
	_gl_ctxpopup_del(data);
	ad->albuminfo.b_new_album = true;
	gl_dbg("");
	_gl_albums_new_create_view(data, _gl_albums_sel_create_view);
}

static void __gl_reorder_soft_back_cb(void *data, Evas_Object *obj, void *ei)
{
	GL_CHECK(data);
	__gl_albums_pop_op(data);
}

static void __gl_albums_reorder_cb(void *data, Evas_Object *obj, void *ei)
{
	GL_CHECK(data);
	gl_appdata *ad = (gl_appdata *)data;
	_gl_ctxpopup_del(data);
	int view_mode = gl_get_view_mode(ad);
	if (view_mode == GL_VIEW_ALBUMS) {
		gl_set_view_mode(ad, GL_VIEW_ALBUM_REORDER);
		elm_gengrid_reorder_mode_set(ad->albuminfo.view, EINA_TRUE);
		_gl_ui_disable_menu(ad->ctrlinfo.nf_it, true);
		_gl_ui_change_navi_title(ad->ctrlinfo.nf_it, GL_STR_ID_REORDER, true);
		Evas_Object *btn = elm_button_add(ad->albuminfo.view);
		GL_CHECK(btn);
		elm_object_style_set(btn, "naviframe/end_btn/default");
		evas_object_smart_callback_add(btn, "clicked", __gl_reorder_soft_back_cb, data);
		elm_object_item_part_content_set(ad->ctrlinfo.nf_it, GL_NAVIFRAME_PREV_BTN, btn);
		evas_object_smart_callback_del(ad->albuminfo.view, "longpressed", __gl_albums_longpressed_cb);
	}
}

static void __gl_albums_viewas_pop_cb(void *data, Evas_Object *obj, void *ei)
{
	gl_dbg("ENTRY");
	GL_CHECK(data);
	gl_appdata *ad = (gl_appdata *)data;
	int state_index = 1; // default is album
	int view_mode = gl_get_view_mode(ad);
	if (view_mode == GL_VIEW_TIMELINE) {
		state_index = 0;
	} else if (view_mode == GL_VIEW_ALBUMS) {
		state_index = 1;
	}
	_gl_list_pop_create(data, obj, ei, GL_STR_ID_VIEW_AS, GL_STR_TIME_VIEW, GL_STR_ALBUMS, state_index);
	gl_dbg("EXIT");
}
static void __gl_albums_edit_cb(void *data, Evas_Object *obj, void *ei)
{
	GL_CHECK(data);
	gl_appdata *ad = (gl_appdata *)data;
	_gl_ctxpopup_del(data);
	if (ad->uginfo.ug) {
		/**
		* Prevent changed to edit view in wrong way.
		* 1. When invoke imageviewer UG;
		*/
		gl_dbg("UG invoked or appending gridview.");
		return;
	}

	int view_mode = gl_get_view_mode(data);
	gl_dbg("mode: %d", view_mode);
	if (view_mode == GL_VIEW_ALBUMS) {
		_gl_albums_change_mode(data, true);
	}
}

#if 0
static void __gl_albums_share_cb(void *data, Evas_Object *obj, void *ei)
{
	GL_CHECK(data);
	gl_appdata *ad = (gl_appdata *)data;
	_gl_ctxpopup_del(data);
	if (ad->uginfo.ug) {
		/**
		* Prevent changed to edit view in wrong way.
		* 1. When invoke imageviewer UG;
		*/
		gl_dbg("UG invoked or appending gridview.");
		return;
	}
	ad->albuminfo.b_share_mode = GL_GRID_SHARE;
	__gl_albums_edit_cb(data, NULL, NULL);
}
#endif

#ifdef SUPPORT_SLIDESHOW
static int __gl_albums_start_slideshow(void *data)
{
	GL_CHECK_VAL(data, -1);
	gl_appdata *ad = (gl_appdata *)data;
	ad->gridinfo.grid_type = GL_GRID_T_ALLALBUMS;

	Eina_List *list = NULL;
	gl_item *gitem = NULL;
	gl_media_s *item = NULL;
	bool is_all = true;
	if (ad->albuminfo.sel_elist &&
	        eina_list_count(ad->albuminfo.sel_elist) > 0) {
		gl_cluster *album = NULL;
		album = eina_list_nth(ad->albuminfo.sel_elist, 0);
		_gl_data_get_items_album(data, album, 0, 0, &list);
		gitem = (gl_item *)eina_list_nth(list, 0);
		is_all = false;
	} else {
		_gl_data_get_items(GL_FIRST_VIEW_START_POS,
		                   GL_FIRST_VIEW_START_POS, &list);
		item = (gl_media_s *)eina_list_nth(list, 0);
		gitem = _gl_data_new_item_mitem(item);
	}
	if (list) {
		eina_list_free(list);
	}
	if (gitem) {
		GL_CHECK_VAL(gitem, -1);
		if (is_all) {
			gl_ext_load_iv_ug(data, gitem, GL_UG_IV_SLIDESHOW_ALL);
		} else {
			gl_ext_load_iv_ug(data, gitem, GL_UG_IV_SLIDESHOW);
		}
	}
	if (gitem) {
		_gl_data_util_free_gitem(gitem);
	}
	_gl_data_finalize_albums_selected_list(data);
	return 0;
}
#endif

#ifdef _USE_APP_SLIDESHOW
/* TO select thumb for slideshow */
static int __gl_albums_select_slideshow(void *data)
{
	GL_CHECK_VAL(data, -1);
	gl_appdata *ad = (gl_appdata *)data;
	int item_cnt = 0;
	int ret = -1;

	ret = _gl_data_get_item_cnt(GL_ALBUM_ALL_ID, GL_STORE_T_ALL, &item_cnt);
	if (ret != 0 || item_cnt == 0) {
		gl_dbgE("Empty!");
		return -1;
	}

	gl_cluster *cluster = NULL;
	_gl_data_get_cluster_by_id(data, GL_ALBUM_ALL_ID, &cluster);
	GL_CHECK_VAL(cluster, -1);
	_gl_albums_set_current(data, cluster);
	Eina_List *medias_elist = NULL;
	_gl_data_get_items_album(data, cluster, GL_FIRST_VIEW_START_POS,
	                         GL_FIRST_VIEW_END_POS, &medias_elist);
	_gl_thumbs_set_edit_mode(data, GL_THUMBS_EDIT_SLIDESHOW);
	_gl_thumbs_set_list(ad, medias_elist);
	_gl_thumbs_create_view(data, GL_NAVI_THUMBS, GL_STR_ID_ALL_ALBUMS, true,
	                       __gl_albums_close_album_cb);

	gl_dbg("Done edit");
	return 0;
}
#endif

#ifdef SUPPORT_SLIDESHOW
static int __gl_albums_slideshow(void *data, const char *label)
{
	GL_CHECK_VAL(label, -1);
	GL_CHECK_VAL(data, -1);
	gl_dbg("label : %s ", label);
	gl_appdata *ad = (gl_appdata *)data;

	if (!g_strcmp0(label, _gl_str(GL_STR_ID_ALL_ITEMS))) {
		__gl_albums_start_slideshow(data);
	} else if (!g_strcmp0(label, _gl_str(GL_STR_ID_SETTINGS))) {
		evas_object_data_set(ad->maininfo.naviframe,
		                     GL_NAVIFRAME_SLIDESHOW_DATA_KEY,
		                     __gl_albums_start_slideshow);
		gl_ext_load_ug(data, GL_UG_GALLERY_SETTING_SLIDESHOW);
	} else if (!g_strcmp0(label, _gl_str(GL_STR_ID_SELECT_ITEMS))) {
#ifdef _USE_APP_SLIDESHOW
		__gl_albums_select_slideshow(data);
		evas_object_data_set(ad->maininfo.naviframe,
		                     GL_NAVIFRAME_SELECTED_SLIDESHOW_KEY,
		                     gl_pop_to_ctrlbar_ly);
#else
		_gl_ext_launch_gallery_ug(data);
#endif
	} else {
		gl_dbgE("Invalid lable!");
		return -1;
	}
	return 0;
}

static void __gl_albums_slideshow_cb(void *data, Evas_Object *obj, void *ei)
{
	GL_CHECK(data);
	_gl_ctxpopup_del(data);
	_gl_popup_create_slideshow(data, __gl_albums_slideshow);
}
#endif

static int __gl_albums_ctxpopup_append(void *data, Evas_Object *parent)
{
	gl_dbg("");
	GL_CHECK_VAL(parent, -1);
	GL_CHECK_VAL(data, -1);
	gl_appdata *ad = (gl_appdata *)data;

	if (ad->albuminfo.elist->edit_cnt > 0) {
		/* 1.  View As*/
		_gl_ctxpopup_append(parent, GL_STR_ID_VIEW_AS,
		                    __gl_albums_viewas_pop_cb, data);
		/* 2. New album*/
		_gl_ctxpopup_append(parent, GL_STR_ID_NEW_ALBUM,
		                    __gl_albums_newalbum_cb, data);
		_gl_ctxpopup_append(parent, GL_STR_ID_REORDER,
		                    __gl_albums_reorder_cb, data);
		/* 3. Delete */
		_gl_ctxpopup_append(parent, GL_STR_ID_DELETE, __gl_albums_edit_cb,
		                    data);
#ifdef SUPPORT_SLIDESHOW
		/* 4. Slideshow */
		_gl_ctxpopup_append(parent, GL_STR_ID_SLIDESHOW,
		                    __gl_albums_slideshow_cb, data);
#endif
	}
	return 0;
}

static void __gl_albums_more_btn_cb(void *data, Evas_Object *obj, void *ei)
{
	gl_dbg("more button clicked");
	GL_CHECK(data);
	gl_appdata *ad = (gl_appdata *)data;
	Evas_Object *more = NULL;
	more = _gl_ui_get_btn(NULL, ad->ctrlinfo.nf_it, GL_NAVIFRAME_MORE);
	if (EINA_TRUE == elm_object_disabled_get(more)) {
		gl_dbg("Menu is disabled");
		return; /* Menu is disabled */
	}

	_gl_ctxpopup_create(data, obj, __gl_albums_ctxpopup_append);
}

/*static void __gl_albums_camera_cb(void *data, Evas_Object *obj, void *ei)
{
	GL_CHECK(data);
	gl_dbg("");
	_gl_ext_load_camera(data);
}*/

void _gl_update_list_data(void *data)
{
	GL_CHECK(data);
	gl_appdata *ad = (gl_appdata *)data;
	Elm_Object_Item *l_it = elm_gengrid_last_item_get(ad->albuminfo.view);
	Elm_Object_Item *f_it = elm_gengrid_first_item_get(ad->albuminfo.view);
	gl_cluster *album_item = NULL;
	Eina_List *list = NULL;
	if (f_it) {
		album_item = elm_object_item_data_get(f_it);
		if (album_item) {
			list = eina_list_append(list, album_item);
			album_item = NULL;
		}
	}
	if (l_it && f_it) {
		while (f_it != l_it) {
			f_it = elm_gengrid_item_next_get(f_it);
			if (f_it) {
				album_item = elm_object_item_data_get(f_it);
				if (album_item) {
					list = eina_list_append(list, album_item);
					album_item = NULL;
				}
			}
		}
	}
	if (list) {
		ad->albuminfo.elist->clist = list;
	} else if (list == NULL) {
		gl_dbg("new list is null");
	}
}

static Eina_Bool __gl_albums_pop_op(void *data)
{
	GL_CHECK_FALSE(data);
	if (gl_get_view_mode(data) == GL_VIEW_ALBUMS_EDIT) {
		gl_dbg("EDIT");
		_gl_albums_change_mode(data, false);
		/* Just cancel edit view, dont lower app to background */
		return EINA_TRUE;
	} else if (gl_get_view_mode(data) == GL_VIEW_ALBUM_REORDER) {
		gl_appdata *ad = (gl_appdata *)data;
		gl_set_view_mode(ad, GL_VIEW_ALBUMS);
		elm_gengrid_reorder_mode_set(ad->albuminfo.view, EINA_FALSE);
		Evas_Object *unset = elm_object_item_part_content_unset(ad->ctrlinfo.nf_it, GL_NAVIFRAME_PREV_BTN);
		evas_object_hide(unset);
		_gl_ui_disable_menu(ad->ctrlinfo.nf_it, false);
		_gl_ui_change_navi_title(ad->ctrlinfo.nf_it, GL_STR_ID_ALBUM, true);
		_gl_update_list_data(data);
		evas_object_smart_callback_add(ad->albuminfo.view, "longpressed", __gl_albums_longpressed_cb, data);
		return EINA_TRUE;
	}
	return EINA_FALSE;
}

/**
 * When push albums view for the first time, albums list is empty.
 * After albums list got from DB then update edit item state.
 */
static int __gl_albums_add_btns(void *data)
{
	gl_dbg("");
	GL_CHECK_VAL(data, -1);
	gl_appdata *ad = (gl_appdata *)data;
	GL_CHECK_VAL(ad->maininfo.naviframe, -1);
	GL_CHECK_VAL(ad->ctrlinfo.nf_it, -1);
	Evas_Object *parent = ad->maininfo.naviframe;
	Elm_Object_Item *nf_it = ad->ctrlinfo.nf_it;
	Evas_Object *btn = NULL;

	/* Remove btns */
	_gl_ui_clear_btns(data);
	/* More */
	btn = _gl_but_create_but(parent, NULL, NULL, GL_BUTTON_STYLE_NAVI_MORE,
	                         __gl_albums_more_btn_cb, data);
	GL_CHECK_VAL(btn, -1);

	elm_object_item_part_content_set(nf_it, GL_NAVIFRAME_MORE, btn);
	_gl_albums_check_btns(data);
	return 0;
}

/* From thumbnails view to albums view */
int _gl_albums_close_album(void *data)
{
	gl_dbg("");
	GL_CHECK_VAL(data, -1);
	gl_appdata *ad = (gl_appdata *)data;

	gl_set_view_mode(ad, GL_VIEW_ALBUMS);
	_gl_albums_set_current(data, NULL);
	gl_albums_update_items(ad);
	return 0;
}

/* Update albums view */
int gl_albums_update_items(void *data)
{
	GL_PROFILE_IN;
	GL_CHECK_VAL(data, -1);
	gl_appdata *ad = (gl_appdata *)data;
	GL_CHECK_VAL(ad->albuminfo.elist, -1);
	int view_mode = gl_get_view_mode(ad);
	ad->albuminfo.albums_cnt = 0;
	gl_dbg("");

	/* Changed to show no contents if needed */
	elm_naviframe_item_title_enabled_set(ad->ctrlinfo.nf_it, EINA_TRUE, EINA_TRUE);
	if (gl_check_gallery_empty(ad)) {

GL_ALBUMS_SHOW_NOCONTENTS:

		if (ad->albuminfo.view) {
			/* Clear callbacks for gengrid view */
			if (ad->albuminfo.nocontents == NULL) {
				_gl_albums_clear_cbs(ad->albuminfo.view);
				_gl_ui_del_scroller_pos(ad->albuminfo.view);
			}
			evas_object_del(ad->albuminfo.view);
		}

		Evas_Object *noc = _gl_nocontents_create(ad->ctrlinfo.ctrlbar_view_ly);
		ad->albuminfo.nocontents = noc;
		ad->albuminfo.view = noc;
		evas_object_show(noc);

		elm_naviframe_item_title_enabled_set(ad->ctrlinfo.nf_it, EINA_FALSE, EINA_FALSE);
		Evas_Object *old_view = NULL;
		old_view = elm_object_part_content_unset(ad->ctrlinfo.ctrlbar_view_ly,
		           "elm.swallow.view");
		GL_IF_DEL_OBJ(old_view);
		elm_object_part_content_set(ad->ctrlinfo.ctrlbar_view_ly,
		                            "elm.swallow.view", noc);
	} else {
		if (view_mode == GL_VIEW_ALBUMS_EDIT) {
			_gl_albums_create_items(ad, ad->albuminfo.view);
			/* Display selectioninfo */
			int cnt = _gl_data_get_albums_selected_cnt(ad);
			_gl_notify_check_selall(ad, ad->ctrlinfo.nf_it,
			                        ad->albuminfo.albums_cnt, cnt);
			/* Update the label text */
			_gl_ui_update_navi_title_text(ad->ctrlinfo.nf_it, cnt, false);
		} else {
			if (ad->albuminfo.nocontents) {
				/**
				* current view is nocontents,
				* unset it first then create albums view.
				*/
				gl_albums_remove_nocontents(ad);
			} else {
				if (_gl_albums_create_items(ad, ad->albuminfo.view) < 0) {
					gl_dbgW("To show nocontents!");
					goto GL_ALBUMS_SHOW_NOCONTENTS;
				}
			}
		}
	}
	GL_PROFILE_OUT;
	return 0;
}

/* Add albums view and append nothing */
int _gl_albums_create_view(void *data, Evas_Object *parent)
{
	GL_PROFILE_IN;
	GL_CHECK_VAL(parent, -1);
	GL_CHECK_VAL(data, -1);
	gl_appdata *ad = (gl_appdata *)data;

	Evas_Object *layout_inner = NULL;
	layout_inner = _gl_albums_add_gengrid(ad, parent);
	GL_CHECK_VAL(layout_inner, -1);
	elm_object_part_content_set(parent, "elm.swallow.view", layout_inner);
	ad->albuminfo.view = layout_inner;
	/* Register delete callback */
	evas_object_event_callback_add(parent, EVAS_CALLBACK_DEL,
	                               __gl_albums_delete_layout_cb, data);
	GL_PROFILE_OUT;
	return 0;
}

/* Update albums list and refresh albums view, remove invalid widgets */
int gl_albums_update_view(void *data)
{
	GL_CHECK_VAL(data, -1);
	gl_appdata *ad = (gl_appdata *)data;
	int view_mode = gl_get_view_mode(ad);
	gl_dbg("view_mode: %d.", view_mode);

	ad->albuminfo.albums_cnt = 0;

	if (view_mode == GL_VIEW_ALBUMS) {
		gl_albums_update_items(data);
		_gl_albums_check_btns(data);
	} else if (view_mode == GL_VIEW_ALBUMS_EDIT) {
		_gl_albums_edit_update_view(data);
	} else if (view_mode == GL_VIEW_ALBUMS_RENAME) {
		_gl_albums_rename_update_view(data);
	}
	return 0;
}

/* From albums edit view to albums view */
int gl_albums_change_to_view(void *data)
{
	GL_CHECK_VAL(data, -1);
	gl_appdata *ad = (gl_appdata *)data;
	GL_CHECK_VAL(ad->albuminfo.elist, -1);
	gl_dbg("");

	ad->albuminfo.albums_cnt = 0;
	GL_CHECK_VAL(ad->albuminfo.view, -1);
	gl_set_view_mode(ad, GL_VIEW_ALBUMS);
	/* changed to show no contents if needed */
	elm_naviframe_item_title_enabled_set(ad->ctrlinfo.nf_it, EINA_TRUE, EINA_TRUE);
	if (gl_check_gallery_empty(ad)) {
		if (ad->albuminfo.view) {
			/* Clear callbacks for gengrid view */
			if (ad->albuminfo.nocontents == NULL) {
				_gl_albums_clear_cbs(ad->albuminfo.view);
				_gl_ui_del_scroller_pos(ad->albuminfo.view);
			}
			evas_object_del(ad->albuminfo.view);
		}

		Evas_Object *noc = _gl_nocontents_create(ad->ctrlinfo.ctrlbar_view_ly);
		ad->albuminfo.nocontents = noc;
		ad->albuminfo.view = noc;
		evas_object_show(noc);

		elm_naviframe_item_title_enabled_set(ad->ctrlinfo.nf_it, EINA_FALSE, EINA_FALSE);
		Evas_Object *old_view = NULL;
		old_view = elm_object_part_content_unset(ad->ctrlinfo.ctrlbar_view_ly,
		           "elm.swallow.view");
		evas_object_hide(old_view);
		elm_object_part_content_set(ad->ctrlinfo.ctrlbar_view_ly,
		                            "elm.swallow.view", noc);
	} else {
		_gl_albums_create_items(ad, ad->albuminfo.view);
	}
	return 0;
}

/**
* Remove nocontents view and show albums view.
* Case 1, gallery is empty->home key tapped
*	->take photos with camera->back to gallery;
* Case 2, gallery is empty->add web album
*	->update albums view.
*/
int gl_albums_remove_nocontents(void *data)
{
	GL_CHECK_VAL(data, -1);
	gl_appdata *ad = (gl_appdata *)data;
	Evas_Object *view = NULL;
	gl_dbg("");

	if (ad->albuminfo.nocontents && !gl_check_gallery_empty(ad)) {
		gl_dbg("Remove nocontents view then create albums view.");
		evas_object_del(ad->albuminfo.nocontents);
		ad->albuminfo.nocontents = NULL;

		view = _gl_albums_add_view(ad, ad->maininfo.naviframe);
		elm_object_part_content_set(ad->ctrlinfo.ctrlbar_view_ly,
		                            "elm.swallow.view", view);
		evas_object_show(view);
		ad->albuminfo.view = view;
		_gl_albums_check_btns(data);
	} else if (ad->albuminfo.nocontents) {
		gl_dbg("Gallery is empty!");
		bool b_update = false;
		b_update = _gl_nocontents_update_label(ad->albuminfo.nocontents,
		                                       GL_STR_NO_ALBUMS);
		/* Update toolbar state */
		if (b_update) {
			_gl_albums_check_btns(data);
		}
	} else {
		gl_dbg("Nocontents was removed!");
	}

	return 0;
}

/* Free resources allocated for albums view */
int gl_albums_free_data(void *data)
{
	gl_dbg("");
	GL_CHECK_VAL(data, -1);
	gl_appdata *ad = (gl_appdata *)data;

	_gl_albums_clear_cbs(ad->albuminfo.view);
	_gl_ui_del_scroller_pos(ad->albuminfo.view);
	GL_FREEIF(ad->albuminfo.path);
	return 0;
}

int _gl_albums_show_view_tab(void *data, Evas_Object *parent)
{
	GL_CHECK_VAL(parent, -1);
	GL_CHECK_VAL(data, -1);
	gl_appdata *ad = (gl_appdata *)data;

	/* Set pop callback for operating when button back clicked */
	evas_object_data_set(ad->maininfo.naviframe, GL_NAVIFRAME_POP_CB_KEY,
	                     (void *)__gl_albums_pop_op);

	gl_set_view_mode(data, GL_VIEW_ALBUMS);
	_gl_ctrl_show_title(data, GL_CTRL_TAB_ALBUMS);

	if (!ad->albuminfo.view) {
		gl_dbg("Add gengrid first");
		_gl_albums_create_view(ad, ad->ctrlinfo.ctrlbar_view_ly);
	}

	gl_albums_update_items(data);

	if (!ad->albuminfo.nocontents) {
		evas_object_show(ad->albuminfo.view);
		elm_object_part_content_set(parent, "elm.swallow.view",
		                            ad->albuminfo.view);
	} else {
		/* Register service if nocontents created for the first launching */
		_gl_main_add_reg_idler(data);
	}

	/* add items */
	__gl_albums_add_btns(data);
	return 0;
}

/* Terminate EDIT mode */
int _gl_albums_hide_view_tab(void *data)
{
	GL_CHECK_VAL(data, -1);
	gl_appdata *ad = (gl_appdata *)data;
	/* Reset flags */
	ad->albuminfo.selected = NULL;
	_gl_albums_set_current(data, NULL);
	int view_m = gl_get_view_mode(data);

	if (view_m == GL_VIEW_ALBUMS_EDIT) {
		gl_dbg("GL_VIEW_ALBUMS_EDIT");
		_gl_data_finalize_albums_selected_list(data);
		/* CLEAR GENGRID */
		if (ad->albuminfo.view &&
		        ad->albuminfo.view != ad->albuminfo.nocontents) {
			gl_dbg("elm_gengrid_clear view");
			elm_gengrid_clear(ad->albuminfo.view);
		}
	} else if (view_m == GL_VIEW_ALBUMS) {
		gl_dbg("GL_VIEW_ALBUMS");
		/* CLEAR GENGRID */
		if (ad->albuminfo.view &&
		        ad->albuminfo.view != ad->albuminfo.nocontents) {
			gl_dbg("elm_gengrid_clear view");
			elm_gengrid_clear(ad->albuminfo.view);
		}
	}
	/* Hide previous view */
	Evas_Object *pre_view = NULL;
	pre_view = elm_object_part_content_unset(ad->ctrlinfo.ctrlbar_view_ly,
	           "elm.swallow.view");
	evas_object_hide(pre_view);
	return 0;
}

int _gl_albums_change_mode(void *data, bool b_edit)
{
	GL_CHECK_VAL(data, -1);
	gl_appdata *ad = (gl_appdata *)data;
	/* Reset flags */
	ad->albuminfo.selected = NULL;
	_gl_albums_set_current(data, NULL);

	if (b_edit) {
		gl_set_view_mode(ad, GL_VIEW_ALBUMS_EDIT);
		_gl_ui_change_navi_title(ad->ctrlinfo.nf_it, GL_STR_ID_SELECT_ITEM, false);
		elm_object_signal_emit(ad->ctrlinfo.ctrlbar_view_ly, "elm,selectall,state,visible,bg", "elm");
		elm_object_signal_emit(ad->ctrlinfo.ctrlbar_view_ly, "elm,selectall,state,visible", "elm");
		_gl_albums_edit_add_btns(data, ad->maininfo.naviframe);
#ifdef _USE_GRID_CHECK
		_gl_show_grid_checks_dual(ad->albuminfo.view, GL_TILE_CHECKBOX, GL_TILE_CHECKBOX_GRID);
#else
		evas_object_smart_callback_del(ad->albuminfo.view, "unrealized",
		                               __gl_albums_unrealized);
		elm_gengrid_realized_items_update(ad->albuminfo.view);
		evas_object_smart_callback_add(ad->albuminfo.view, "unrealized",
		                               __gl_albums_unrealized, data);
#endif
	} else {
		_gl_ui_change_navi_title(ad->ctrlinfo.nf_it, GL_STR_ID_ALBUM, true);
		__gl_albums_add_btns(data);
		_gl_albums_edit_pop_view(data);
		gl_dbg("Sending signal to EDC 1");
		elm_object_signal_emit(ad->ctrlinfo.ctrlbar_view_ly, "elm,selectall,state,default", "elm");
	}
	gl_albums_update_view(data);
	return 0;
}

int _gl_albums_check_btns(void *data)
{
	GL_CHECK_VAL(data, -1);
	gl_appdata *ad = (gl_appdata *)data;
	GL_CHECK_VAL(ad->ctrlinfo.nf_it, -1);
	bool b_disabled = false;

	/* Disable button share */
	if (gl_check_gallery_empty(data) || ad->albuminfo.elist->edit_cnt <= 0) {
		b_disabled = true;
	}
	gl_dbg("b_disable: %d", b_disabled);

	_gl_ui_disable_menu(ad->ctrlinfo.nf_it, false);
	return 0;
}

int _gl_albums_rotate_view(void *data)
{
	GL_CHECK_VAL(data, -1);
	gl_appdata *ad = (gl_appdata *)data;

	if (ad->albuminfo.view == ad->albuminfo.nocontents) {
		return -1;
	}
	Elm_Object_Item *item = elm_gengrid_first_item_get(ad->albuminfo.view);
	if (item) {
		item = elm_gengrid_item_next_get(item);
		if (item) {
			item = elm_gengrid_item_next_get(item);
			if (item)
				elm_object_item_signal_emit(item,
				                            "show_top_pad",
				                            "padding.top");
		}
	}
	return _gl_tile_update_item_size(data, ad->albuminfo.view, true);
}

int _gl_albums_set_current(void *data, gl_cluster *current)
{
	GL_CHECK_VAL(data, -1);
	gl_appdata *ad = (gl_appdata *)data;
	ad->albuminfo.current = current;
	return 0;
}

gl_cluster *_gl_albums_get_current(void *data)
{
	GL_CHECK_NULL(data);
	gl_appdata *ad = (gl_appdata *)data;
	return ad->albuminfo.current;
}

/* Free resources allocated for albums view */
int _gl_albums_clear_cbs(Evas_Object *view)
{
	gl_dbg("");
	GL_CHECK_VAL(view, -1);
	evas_object_smart_callback_del(view, "realized", __gl_albums_realized);
	evas_object_smart_callback_del(view, "unrealized",
	                               __gl_albums_unrealized);
	evas_object_smart_callback_del(view, "longpressed",
	                               __gl_albums_longpressed_cb);
	return 0;
}

void _gl_add_album_data(gl_cluster *album, Eina_List *medias_elist)
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

int gl_albums_open_album(gl_cluster *album)
{
	GL_CHECK_VAL(album, -1);
	GL_CHECK_VAL(album->cluster, -1);
	GL_CHECK_VAL(album->cluster->uuid, -1);
	GL_CHECK_VAL(album->ad, -1);
	gl_appdata *ad = (gl_appdata *)album->ad;
	GL_CHECK_VAL(ad->albuminfo.elist, -1);
	GL_CHECK_VAL(ad->albuminfo.elist->clist, -1);
	gl_item *gitem = NULL;
	gl_item *data = NULL;
	int view_mode = gl_get_view_mode(ad);
	int i;

	if (album->cover) {
		_gl_data_util_free_gitem(album->cover);
		album->cover = NULL;
	}

	if (view_mode == GL_VIEW_ALBUMS) {
		gl_dbg("View mode!");
	} else if (view_mode == GL_VIEW_ALBUMS_SELECT || view_mode == GL_VIEW_THUMBS_SELECT) {
		gl_dbg("Select mode!");
		_gl_albums_set_current(ad, album);
		_gl_thumbs_sel_create_view(ad, album);
		return 0;
	} else if (view_mode == GL_VIEW_THUMBS) {
		gl_dbg("Split view!");
		char *i18n_name = _gl_get_i18n_album_name(album->cluster);
		gl_sdbg("Album: %s, UUID: %s", i18n_name, album->cluster->uuid);
		_gl_albums_set_current(ad, album);
		Eina_List *medias_elist = NULL;
		_gl_data_get_items_album(ad, album, GL_FIRST_VIEW_START_POS,
		                         GL_GET_UNTIL_LAST_RECORD - 1, &medias_elist);
		_gl_add_album_data(album, medias_elist);
		EINA_LIST_FREE(album->elist, data) {
			if (data) {
				data->checked = false;
			}
		}
		_gl_thumbs_set_list(ad, medias_elist);
		_gl_thumbs_update_split_view(ad, GL_NAVI_THUMBS, i18n_name, true,
		                             __gl_albums_close_album_cb);
		return 0;
	} else if (view_mode == GL_VIEW_THUMBS_EDIT) {
		gl_dbg("Split edit view!");
		char *i18n_name = _gl_get_i18n_album_name(album->cluster);
		gl_sdbg("Album: %s, UUID: %s", i18n_name, album->cluster->uuid);
		_gl_albums_set_current(ad, album);

		Eina_List *medias_elist = NULL;
		_gl_data_get_items_album(ad, album, GL_FIRST_VIEW_START_POS,
		                         GL_GET_UNTIL_LAST_RECORD - 1, &medias_elist);
		_gl_add_album_data(album, medias_elist);
		EINA_LIST_FREE(album->elist, data) {
			if (data) {
				data->checked = false;
			}
		}

		_gl_thumbs_set_list(ad, medias_elist);
		int count = eina_list_count(medias_elist);
		for (i = 0; i < count ; i++) {
			gitem = eina_list_nth(medias_elist, i);
			_gl_data_restore_selected(ad->selinfo.elist, gitem);
		}
		_gl_thumbs_update_edit_split_view(ad, GL_NAVI_THUMBS, i18n_name, true,
		                                  __gl_albums_close_album_cb);
		return 0;
	} else {
		gl_dbgW("Wrong view mode");
		return -1;
	}
	char *i18n_name = _gl_get_i18n_album_name(album->cluster);
	gl_sdbg("Album: %s, UUID: %s", i18n_name, album->cluster->uuid);
	ad->gridinfo.title = album->cluster->uuid;
	_gl_albums_set_current(ad, album);
	Eina_List *medias_elist = NULL;
	_gl_data_get_items_album(ad, album, GL_FIRST_VIEW_START_POS,
	                         GL_FIRST_VIEW_END_POS, &medias_elist);
	_gl_add_album_data(album, medias_elist);
	EINA_LIST_FREE(album->elist, data) {
		if (data) {
			data->checked = false;
		}
	}
	_gl_thumbs_set_list(ad, medias_elist);
	_gl_thumbs_create_view(ad, GL_NAVI_THUMBS, i18n_name, true,
	                       __gl_albums_close_album_cb);
	gl_dbg("albums_view 0x%x cleared", ad->albuminfo.view);
	gl_dbg("Done albums selected");
	return 0;
}

Evas_Object *_gl_albums_add_gengrid(void *data, Evas_Object *parent)
{
	GL_PROFILE_IN;
	GL_CHECK_NULL(parent);
	GL_CHECK_NULL(data);
	Evas_Object *grid = _gl_tile_add_gengrid(parent);
	evas_object_smart_callback_add(grid, "realized", __gl_albums_realized,
	                               data);
	evas_object_smart_callback_add(grid, "unrealized",
	                               __gl_albums_unrealized, data);
#if 0
	evas_object_smart_callback_add(grid, "language,changed",
	                               __gl_albums_lang_changed, data);
#endif
	evas_object_smart_callback_add(grid, "longpressed",
	                               __gl_albums_longpressed_cb, data);
	evas_object_show(grid);
	_gl_ui_reset_scroller_pos(grid);
	GL_PROFILE_OUT;
	return grid;
}

Evas_Object * _gl_albums_create_album_sel_gengrid(void *data)
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

Evas_Object *_gl_albums_sel_add_view(void *data, Evas_Object *parent)
{
	GL_CHECK_NULL(parent);
	GL_CHECK_NULL(data);
	gl_appdata *ad = (gl_appdata *)data;
	GL_CHECK_NULL(ad->albuminfo.elist);
	gl_dbg("");

	ad->albuminfo.albums_cnt = 0;

	Evas_Object *grid = _gl_albums_create_album_sel_gengrid(data);
	GL_CHECK_NULL(grid);
	int ret = _gl_albums_create_items(ad, grid);

	int view_mode = gl_get_view_mode(ad);
	gl_dbg("view_mode: %d", view_mode);
	if (view_mode == GL_VIEW_ALBUMS_SELECT) {
		if (ad->albuminfo.albums_cnt == 0) {
			gl_dbgE("None albums!");
			evas_object_del(grid);
			grid = NULL;
		} else {
			evas_object_show(grid);
		}
		return grid;
	}

	/* show no contents if none albums appended or none album exists */
	if (ret < 0 || gl_check_gallery_empty(ad)) {
		_gl_ui_del_scroller_pos(grid);
		evas_object_del(grid);
		grid = NULL;

		Evas_Object *noc = _gl_nocontents_create(ad->ctrlinfo.ctrlbar_view_ly);
		evas_object_show(noc);
		ad->albuminfo.nocontents = noc;
		elm_naviframe_item_title_enabled_set(ad->ctrlinfo.nf_it, EINA_FALSE, EINA_FALSE);

		return noc;
	} else {
		ad->albuminfo.nocontents = NULL;
		elm_naviframe_item_title_enabled_set(ad->ctrlinfo.nf_it, EINA_TRUE, EINA_TRUE);
	}

	gl_dbg("done");
	return grid;
}

/* Add albums view and doesn't push it to naviframe */
Evas_Object *_gl_albums_add_view(void *data, Evas_Object *parent)
{
	GL_CHECK_NULL(parent);
	GL_CHECK_NULL(data);
	gl_appdata *ad = (gl_appdata *)data;
	GL_CHECK_NULL(ad->albuminfo.elist);
	gl_dbg("");

	ad->albuminfo.albums_cnt = 0;

	Evas_Object *grid = _gl_albums_add_gengrid(data, parent);
	GL_CHECK_NULL(grid);
	int ret = _gl_albums_create_items(ad, grid);

	int view_mode = gl_get_view_mode(ad);
	gl_dbg("view_mode: %d", view_mode);
	if (view_mode == GL_VIEW_ALBUMS_SELECT) {
		if (ad->albuminfo.albums_cnt == 0) {
			gl_dbgE("None albums!");
			evas_object_del(grid);
			grid = NULL;
		} else {
			evas_object_show(grid);
		}
		return grid;
	}

	/* show no contents if none albums appended or none album exists */
	if (ret < 0 || gl_check_gallery_empty(ad)) {
		_gl_ui_del_scroller_pos(grid);
		evas_object_del(grid);
		grid = NULL;

		Evas_Object *noc = _gl_nocontents_create(ad->ctrlinfo.ctrlbar_view_ly);
		evas_object_show(noc);
		ad->albuminfo.nocontents = noc;
		elm_naviframe_item_title_enabled_set(ad->ctrlinfo.nf_it, EINA_FALSE, EINA_FALSE);

		return noc;
	} else {
		ad->albuminfo.nocontents = NULL;
		elm_naviframe_item_title_enabled_set(ad->ctrlinfo.nf_it, EINA_TRUE, EINA_TRUE);
	}

	gl_dbg("done");
	return grid;
}

int _gl_split_albums_create_items(void *data, Evas_Object *parent)
{
	GL_PROFILE_IN;
	GL_CHECK_VAL(parent, -1);
	GL_CHECK_VAL(data, -1);
	gl_appdata *ad = (gl_appdata *)data;
	int i = 0;
	gl_cluster *album_item = NULL;
	int length = 0;
	gl_dbg("");
	evas_object_smart_callback_add(parent, "realized", __gl_split_albums_realized,
	                               data);
	if (elm_gengrid_items_count(parent) > 0) {
		/* Save scroller position before clearing gengrid */
		_gl_ui_save_scroller_pos(parent);
		/* Clear albums view */
		elm_gengrid_clear(parent);
	}
	ad->albuminfo.gic.item_style = GL_GENGRID_STYLE_ALBUM_SPLIT_VIEW;
	ad->albuminfo.gic.func.text_get = __gl_albums_get_text;
	ad->albuminfo.gic.func.content_get = __gl_albums_get_content_split_view;
	Elm_Gengrid_Item_Class *pgic = &(ad->albuminfo.gic);
	GL_CHECK_VAL(ad->albuminfo.elist, -1);
	GL_CHECK_VAL(ad->albuminfo.elist->clist, -1);
	Eina_List *clist = ad->albuminfo.elist->clist;
	length = eina_list_count(clist);
	gl_dbg("Albums length: %d", length);
	ad->albuminfo.elist->edit_cnt = 0;
	int item_cnt = 0;

	for (i = 0; i < length; i++) {
		album_item = eina_list_nth(clist, i);
		GL_CHECK_VAL(album_item, -1);
		GL_CHECK_VAL(album_item->cluster, -1);
		GL_CHECK_VAL(album_item->cluster->display_name, -1);
		GL_CHECK_VAL(album_item->cluster->uuid, -1);
		if (album_item->cluster->type < GL_STORE_T_ALL) {
			ad->albuminfo.elist->edit_cnt++;
		} else {
			int view_mode = gl_get_view_mode(ad);
			if (view_mode == GL_VIEW_ALBUMS
			        || view_mode == GL_VIEW_ALBUMS_EDIT
			        || view_mode == GL_VIEW_TIMELINE
			        || view_mode == GL_VIEW_THUMBS_EDIT
			        || view_mode == GL_VIEW_THUMBS_SELECT
			   ) {
				gl_dbg("skip the item");
				continue;
			}
		}
		album_item->item = elm_gengrid_item_append(parent, pgic,
		                   album_item,
		                   __gl_albums_sel_cb,
		                   album_item);
		album_item->index = item_cnt;
		item_cnt++;
		gl_sdbg("Append[%s], id=%s.", album_item->cluster->display_name,
		        album_item->cluster->uuid);
	}
	/* Restore previous position of scroller */
	_gl_ui_restore_scroller_pos(parent);
	ad->albuminfo.albums_cnt = item_cnt;
	GL_PROFILE_OUT;
	if (item_cnt) {
		gl_sdbg("Item count is 0");
		return 0;
	} else {
		return -1;
	}
}

int _gl_albums_create_items(void *data, Evas_Object *parent)
{
	GL_PROFILE_IN;
	GL_CHECK_VAL(parent, -1);
	GL_CHECK_VAL(data, -1);
	gl_appdata *ad = (gl_appdata *)data;
	int i = 0;
	gl_cluster *album_item = NULL;
	int length = 0;
	gl_dbg("");

	if (elm_gengrid_items_count(parent) > 0) {
		/* Save scroller position before clearing gengrid */
		_gl_ui_save_scroller_pos(parent);
		evas_object_smart_callback_del(parent, "unrealized",
		                               __gl_albums_unrealized);
		/* Clear albums view */
		elm_gengrid_clear(parent);
		evas_object_smart_callback_add(parent, "unrealized",
		                               __gl_albums_unrealized, data);
	}
	ad->albuminfo.gic.item_style = GL_GENGRID_STYLE_ALBUM_VIEW;
	ad->albuminfo.gic.func.text_get = __gl_albums_get_text;
	ad->albuminfo.gic.func.content_get = __gl_albums_get_content;
	Elm_Gengrid_Item_Class *pgic = &(ad->albuminfo.gic);
	GL_CHECK_VAL(ad->albuminfo.elist, -1);
	GL_CHECK_VAL(ad->albuminfo.elist->clist, -1);
	Eina_List *clist = ad->albuminfo.elist->clist;
	length = eina_list_count(clist);
	gl_dbg("Albums length: %d", length);
	ad->albuminfo.elist->edit_cnt = 0;
	int item_cnt = 0;

	for (i = 0; i < length; i++) {
		album_item = eina_list_nth(clist, i);
		GL_CHECK_VAL(album_item, -1);
		GL_CHECK_VAL(album_item->cluster, -1);
		GL_CHECK_VAL(album_item->cluster->display_name, -1);
		GL_CHECK_VAL(album_item->cluster->uuid, -1);

		if (album_item->cluster->type < GL_STORE_T_ALL) {
			ad->albuminfo.elist->edit_cnt++;
		} else {
			if (gl_get_view_mode(ad) != GL_VIEW_ALBUMS) {
				continue;
			}
		}
		album_item->item = elm_gengrid_item_append(parent, pgic,
		                   album_item,
		                   __gl_albums_sel_cb,
		                   album_item);
		album_item->index = item_cnt;
		item_cnt++;
		gl_sdbg("Append[%s], id=%s.", album_item->cluster->display_name,
		        album_item->cluster->uuid);
	}
	/* Restore previous position of scroller */
	_gl_ui_restore_scroller_pos(parent);

	ad->albuminfo.albums_cnt = item_cnt;
	_gl_tile_update_item_size(data, parent, false);
	GL_PROFILE_OUT;
	if (item_cnt) {
		return 0;
	} else {
		return -1;
	}
}

