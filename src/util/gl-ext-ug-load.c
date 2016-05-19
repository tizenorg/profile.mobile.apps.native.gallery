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

#include <Elementary.h>
#include "gl-ext-ug-load.h"
#include "gl-ext-exec.h"
#include "gl-debug.h"
#include "gl-ui-util.h"
#include "gl-util.h"
#include "gl-albums.h"
#include "gl-thumbs.h"
#include "gl-thumbs-edit.h"
#include "gallery.h"
#include "gl-popup.h"
#include "gl-strings.h"
#include "gl-controlbar.h"
#include "gl-db-update.h"
#include "gl-notify.h"
#include "gl-timeline.h"

#define GL_EXT_STR_BUNDLE_LEN 48

#define GL_UG_PKG_IV			"org.tizen.image-viewer"
#define GL_UG_PKG_GALLERY_SETTING	"setting-gallery-efl"

#define GL_ARGV_IV_VIEW_MODE			"View Mode"
#define GL_ARGV_IV_VIEW_GALLERY			"GALLERY"
#define GL_ARGV_IV_VIEW_SLIDESHOW		"SLIDESHOW"
#define GL_ARGV_IV_PATH				"Path"
#define GL_ARGV_IV_INDEX		"Index"
#define GL_ARGV_IV_VIEW_BY		"View By"
#define GL_ARGV_IV_VIEW_BY_FOLER	"By Folder"
#define GL_ARGV_IV_VIEW_BY_ALL		"All"
#define GL_ARGV_IV_MEDIA_TYPE			"Media type"
#define GL_ARGV_IV_MEDIA_ALL			"All"
#define GL_ARGV_IV_MEDIA_IMAGE			"Image"
#define GL_ARGV_IV_MEDIA_VIDEO			"Video"
#define GL_ARGV_IV_ALBUM_INDEX			"Album index"
#define GL_ARGV_IV_SORT_BY			"Sort By"
#define GL_ARGV_IV_SORT_NAME			"Name"
#define GL_ARGV_IV_SORT_NAMEDESC		"NameDesc"
#define GL_ARGV_IV_SORT_DATE			"Date"
#define GL_ARGV_IV_SORT_DATEDESC		"DateDesc"
#define GL_ARGV_IV_FOOTSTEPS			"Footsteps"
#define GL_ARGV_IV_TRUE				"TRUE"
#define GL_ARGV_IV_VIEW_BY_FAVORITES		"Favorites"

#define GL_ARGC_SELECTED_FILES "Selected index"
#define GL_ARGV_IV_INDEX_VALUE "1"

static void __gl_appcontrol_select_result_cb(app_control_h request, app_control_h reply, app_control_result_e result, void *user_data)
{
	GL_CHECK(user_data);
	gl_appdata *ad = (gl_appdata *)user_data;
	int i;
	bool in_list = false;
	char **select_result = NULL;
	int count = 0;
	Eina_List *l = NULL;
	gl_item *data = NULL;
	int sel_count = 0;
	int view_mode = gl_get_view_mode(ad);
	gl_item *gitem = NULL;

	gitem = eina_list_nth(ad->gridinfo.medias_elist, 0);
	if (gitem && strcmp(gitem->album->cluster->uuid, GL_ALBUM_FAVOURITE_ID) == 0) {
		app_control_get_extra_data_array(reply, "Selected index fav", &select_result, &count);
	} else {
		app_control_get_extra_data_array(reply, "Selected index", &select_result, &count);
	}

	int tab_mode = _gl_ctrl_get_tab_mode(ad);
	if (tab_mode == GL_CTRL_TAB_TIMELINE) {
		_gl_update_timeview_iv_select_mode_reply(ad, select_result, count);
		return;
	}

	if (select_result) {
		if (gitem && strcmp(gitem->album->cluster->uuid, GL_ALBUM_FAVOURITE_ID) == 0) {
			EINA_LIST_FOREACH(ad->gridinfo.medias_elist, l, data) {
				if (!data || !data->item) {
					continue;
				}
				in_list = false;
				for (i = 0; i < count; i++) {
					if (!strcmp(select_result[i], data->item->file_url)) {
						if (!data->checked) {
							Eina_List *sel_list1 = ad->selinfo.fav_elist;
							sel_list1 = eina_list_append(sel_list1, data);
							ad->selinfo.fav_elist = sel_list1;
							data->album->elist = eina_list_append(data->album->elist, data);
							data->checked = true;
						}
						in_list = true;
						sel_count++;
						break;
					}
				}
				if (!in_list) {
					if (data->checked) {
						_gl_data_selected_fav_list_remove(ad, data);
						data->album->elist = eina_list_remove(data->album->elist, data);
						data->checked = false;
					}
				}
			}
		} else {
			EINA_LIST_FOREACH(ad->gridinfo.medias_elist, l, data) {
				if (!data || !data->item) {
					continue;
				}
				in_list = false;
				for (i = 0; i < count; i++) {
					if (!strcmp(select_result[i], data->item->file_url)) {
						if (!data->checked) {
							_gl_data_selected_list_append(ad, data);
							data->album->elist = eina_list_append(data->album->elist, data);
							data->checked = true;
						}
						in_list = true;
						sel_count++;
						break;
					}
				}
				if (!in_list) {
					if (data->checked) {
						_gl_data_selected_list_remove(ad, data);
						data->album->elist = eina_list_remove(data->album->elist, data);
						data->checked = false;
					}
				}
			}
		}
	} else {
		if (gitem && strcmp(gitem->album->cluster->uuid, GL_ALBUM_FAVOURITE_ID) == 0) {
			EINA_LIST_FOREACH(ad->gridinfo.medias_elist, l, data) {
				if (!data || !data->item) {
					continue;
				}
				if (data->checked) {
					_gl_data_selected_fav_list_remove(ad, data);
					data->album->elist = eina_list_remove(data->album->elist, data);
					data->checked = false;
				}
			}
		} else {
			EINA_LIST_FOREACH(ad->gridinfo.medias_elist, l, data) {
				if (!data || !data->item) {
					continue;
				}
				if (data->checked) {
					_gl_data_selected_list_remove(ad, data);
					data->album->elist = eina_list_remove(data->album->elist, data);
					data->checked = false;
				}
			}
		}
	}

	data = eina_list_nth(ad->gridinfo.medias_elist, 0);
	if (data && data->album && data->album->item) {
		if (sel_count > 0) {
			elm_object_item_signal_emit(data->album->item,
			                            "elm,state,elm.text.badge,visible",
			                            "elm");
		} else {
			elm_object_item_signal_emit(data->album->item,
			                            "elm,state,elm.text.badge,hidden",
			                            "elm");
		}
		elm_gengrid_item_fields_update(data->album->item, "elm.text.badge", ELM_GENGRID_ITEM_FIELD_TEXT);
	}
	int total_selected_count = eina_list_count(ad->selinfo.elist) + eina_list_count(ad->selinfo.fav_elist);
	int item_cnt = eina_list_count(ad->gridinfo.medias_elist);
	if (view_mode == GL_VIEW_THUMBS_EDIT) {
		_gl_notify_check_selall(ad, ad->gridinfo.nf_it, item_cnt, sel_count);
		/* Update the label text */
		_gl_thumbs_update_label_text(ad->gridinfo.nf_it, total_selected_count, false);
		elm_gengrid_realized_items_update(ad->gridinfo.view);
	} else if (view_mode == GL_VIEW_THUMBS_SELECT) {
		_gl_notify_check_selall(ad, ad->albuminfo.nf_it_select, item_cnt,
		                        sel_count);
		/* Update the label text */
		_gl_ui_update_navi_title_text(ad->albuminfo.nf_it_select, total_selected_count, false);
		elm_gengrid_realized_items_update(ad->gridinfo.select_view);
	}

	for (i = 0; i < count; i++) {
		if (select_result[i]) {
			free(select_result[i]);
		}
	}
	if (select_result) {
		free(select_result);
	}
}


static void __gl_appcontrol_result_cb(app_control_h request, app_control_h reply, app_control_result_e result, void *user_data)
{
	GL_CHECK(user_data);
	gl_appdata *ad = (gl_appdata *)user_data;
	char *delete_result = NULL;
	app_control_get_extra_data(reply, "delete file", &delete_result);
	if (delete_result) {
		_gl_db_update_lock_once(ad, true);
		gl_update_view(ad, GL_UPDATE_VIEW_INOTIFY);
		_gl_db_update_lock_once(ad, false);
		GL_FREE(delete_result);
	}

	if (ad->uginfo.ug_type == GL_UG_GALLERY_SETTING_SLIDESHOW) {
		char *start = NULL;
		app_control_get_extra_data(reply, "Result", &start);
		if (start) {
			gl_dbg("start string:%s", start);
			if (!g_strcmp0(start, "StartSlideShow")) {
				ad->uginfo.b_start_slideshow = true;
			} else {
				ad->uginfo.b_start_slideshow = false;
			}
			GL_FREE(start);
		}
	}
	void *cb = evas_object_data_get(ad->maininfo.naviframe,
	                                GL_NAVIFRAME_UG_RESULT_KEY);
	gl_sdbg("result_cb [%p]", cb);
	if (cb) {
		int (*result_cb)(void * data, app_control_h ret_h);
		result_cb = cb;
		result_cb(ad, reply);
	}

	GL_CHECK(ad->uginfo.ug);
	app_control_destroy(ad->uginfo.ug);
	ad->uginfo.ug = NULL;
	/* Clear type first */
	ad->uginfo.ug_type = -1;
	ad->uginfo.iv_type = -1;

	cb = evas_object_data_get(ad->maininfo.naviframe,
	                          GL_NAVIFRAME_UG_UPDATE_KEY);
	gl_sdbg("update_cb [%p]", cb);
	if (cb) {
		int (*update_cb)(void * data);
		update_cb = cb;
		update_cb(ad);
	}
	/*Enable the focus permission of the app layout,or else the app layout can't flick the highlight*/
	elm_object_tree_focus_allow_set(ad->maininfo.layout, EINA_TRUE);
	elm_object_focus_set(ad->maininfo.layout, EINA_TRUE);

	elm_win_indicator_mode_set(ad->maininfo.win, ELM_WIN_INDICATOR_SHOW);
	elm_win_indicator_opacity_set(ad->maininfo.win, ELM_WIN_INDICATOR_TRANSPARENT);
	_gl_ui_set_conform_overlap(ad->maininfo.naviframe);

	if (ad->uginfo.b_start_slideshow) {
		ad->uginfo.b_start_slideshow = false;
		ad->uginfo.ug = NULL;
		/* Clear type first */
		ad->uginfo.ug_type = -1;
		ad->uginfo.iv_type = -1;
		void *cb = evas_object_data_get(ad->maininfo.naviframe,
		                                GL_NAVIFRAME_SLIDESHOW_DATA_KEY);
		gl_sdbg("StartSlideShow[%p]", cb);
		if (cb) {
			evas_object_data_set(ad->maininfo.naviframe,
			                     GL_NAVIFRAME_SLIDESHOW_DATA_KEY,
			                     NULL);
			int (*start_slideshow)(void * data);
			start_slideshow = cb;
			start_slideshow(user_data);
		}
	}
#ifdef _USE_APP_SLIDESHOW
	else if (ad->uginfo.b_selected_slideshow) {
		ad->uginfo.b_selected_slideshow = false;
		cb = evas_object_data_get(ad->maininfo.naviframe,
		                          GL_NAVIFRAME_SELECTED_SLIDESHOW_KEY);
		gl_sdbg("selected_slideshow [%p]", cb);
		if (cb) {
			evas_object_data_set(ad->maininfo.naviframe,
			                     GL_NAVIFRAME_SELECTED_SLIDESHOW_KEY,
			                     NULL);
			int (*selected_slideshow_update)(void * data);
			selected_slideshow_update = cb;
			selected_slideshow_update(user_data);
		}
	}
#endif
	evas_object_data_set(ad->maininfo.naviframe,
	                     GL_NAVIFRAME_UG_RESULT_KEY, NULL);
	evas_object_data_set(ad->maininfo.naviframe,
	                     GL_NAVIFRAME_UG_UPDATE_KEY, NULL);
}

#ifdef _USE_APP_SLIDESHOW
static char **__gl_ext_get_select_index(void *data, int *size)
{
	GL_CHECK_NULL(data);
	gl_appdata *ad = (gl_appdata *)data;
	GL_CHECK_NULL(ad->selinfo.elist);
	gl_item *current = NULL;
	char *index = NULL;
	int i = 0;
	char **media_index = NULL;
	int pos = 0;
	int sel_cnt = _gl_data_selected_list_count(ad);

	media_index = (char **)calloc(sel_cnt, sizeof(char *));
	GL_CHECK_NULL(media_index);
	gl_dbg("select media count: %d", sel_cnt);

	for (i = 0; i < sel_cnt; ++i) {
		current = eina_list_nth(ad->selinfo.elist, i);
		index = (char *)calloc(1, GL_INTERGER_LEN_MAX);
		if (current == NULL || current->item == NULL || index == NULL) {
			for (pos = 0; pos < i; ++pos) {
				GL_FREEIF(media_index[pos]);
			}

			GL_FREEIF(index);
			GL_FREE(media_index);
			return NULL;
		}
		gl_dbg("Sequence: %d", current->sequence);
		snprintf(index, GL_INTERGER_LEN_MAX, "%d",
		         current->sequence - 1);
		media_index[i] = index;
		index = NULL;
	}

	if (size) {
		*size = sel_cnt;
	}

	/* Clear selected list */
	_gl_data_selected_list_finalize(ad);
	return media_index;
}

/* Slideshow selected images */
static int __gl_ext_slideshow_selected(void *data, app_control_h service)
{
	GL_CHECK_VAL(service, GL_UG_FAIL);
	GL_CHECK_VAL(data, GL_UG_FAIL);
	gl_appdata *ad = (gl_appdata *)data;
	char **media_index = NULL;
	int media_size = 0;

	media_index = __gl_ext_get_select_index(ad, &media_size);
	if (media_index == NULL) {
		gl_dbgE("Invalid select index!");
		return GL_UG_FAIL;
	}
	gl_dbg("Set selected medias, media_index[%p], size[%d]", media_index,
	       media_size);
	app_control_add_extra_data_array(service, GL_ARGC_SELECTED_FILES,
	                                 (const char **)media_index, media_size);
	/*free space of the medias index*/
	int i = 0;
	for (i = 0; i < media_size; ++i) {
		GL_FREEIF(media_index[i]);
	}
	GL_FREE(media_index);
	media_index = NULL;

	app_control_add_extra_data(service, GL_ARGV_IV_INDEX,
	                           GL_ARGV_IV_INDEX_VALUE);
	return GL_UG_SUCCESS;
}
#endif

static int __gl_ext_add_sortby(app_control_h service, int sort)
{
	GL_CHECK_VAL(service, GL_UG_FAIL);

	switch (sort) {
	case GL_SORT_BY_NAME_DESC:
		/* Sort by display name descending */
		app_control_add_extra_data(service, GL_ARGV_IV_SORT_BY,
		                           GL_ARGV_IV_SORT_NAMEDESC);
		gl_dbg("Name descending");
		break;
	case GL_SORT_BY_NAME_ASC:
		/* Sort by display name ascending */
		app_control_add_extra_data(service, GL_ARGV_IV_SORT_BY,
		                           GL_ARGV_IV_SORT_NAME);
		gl_dbg("Name ascending");
		break;
	case GL_SORT_BY_DATE_DESC:
		/* Sort by modified_date descending */
		app_control_add_extra_data(service, GL_ARGV_IV_SORT_BY,
		                           GL_ARGV_IV_SORT_DATEDESC);
		gl_dbg("Date descending");
		break;
	case GL_SORT_BY_DATE_ASC:
		/* Sort by modified_date ascending */
		app_control_add_extra_data(service, GL_ARGV_IV_SORT_BY,
		                           GL_ARGV_IV_SORT_DATE);
		gl_dbg("Date ascending");
		break;
	default:
		/* No Sort, use  GL_SORT_BY_NAME_ASC */
		app_control_add_extra_data(service, GL_ARGV_IV_SORT_BY,
		                           GL_ARGV_IV_SORT_NAME);
		gl_dbg("no sort, use default Name ascending");
		break;
	}
	return 0;
}

static void __gl_ext_popup_resp_cb(void *data, Evas_Object *obj, void *ei)
{
	GL_CHECK(obj);
	GL_CHECK(data);
	gl_appdata *ad = (gl_appdata *)data;
	GL_IF_DEL_OBJ(ad->popupinfo.popup);
}

static int __gl_ext_set_thumbs_mode(void *data, app_control_h service, int type)
{
	GL_CHECK_VAL(service, GL_UG_FAIL);
	GL_CHECK_VAL(data, GL_UG_FAIL);

	/* 2.0, Local file */
	if (GL_UG_IV_SLIDESHOW == type ||
#ifdef _USE_APP_SLIDESHOW
	        GL_UG_IV_SLIDESHOW_SELECTED == type ||
#endif
	        GL_UG_IV_SLIDESHOW_ALL == type)
		app_control_add_extra_data(service, GL_ARGV_IV_VIEW_MODE,
		                           GL_ARGV_IV_VIEW_SLIDESHOW);
	else
		app_control_add_extra_data(service, GL_ARGV_IV_VIEW_MODE,
		                           GL_ARGV_IV_VIEW_GALLERY);

	app_control_add_extra_data(service, GL_ARGV_IV_MEDIA_TYPE,
	                           GL_ARGV_IV_MEDIA_ALL);
	gl_dbg("Media type: All");
	return GL_UG_SUCCESS;
}
static int __gl_ext_albums_set_iv(void *data, app_control_h service, int type, gl_item *item)
{
	GL_CHECK_VAL(service, GL_UG_FAIL);
	GL_CHECK_VAL(data, GL_UG_FAIL);
	gl_cluster *cur_album = _gl_albums_get_current(data);
	gl_dbg("type: %d", type);

	/* 2.0, Local file */
	if (__gl_ext_set_thumbs_mode(data, service, type) == GL_UG_FAIL) {
		return GL_UG_FAIL;
	}

	if (type == GL_UG_IV_SLIDESHOW_ALL) {
		app_control_add_extra_data(service, GL_ARGV_IV_ALBUM_INDEX,
		                           GL_ALBUM_ALL_ID);
	} else if (cur_album && cur_album->cluster &&
	           cur_album->cluster->uuid) {
		gl_sdbg("album id: %s", cur_album->cluster->uuid);

		if (!strcmp(GL_ALBUM_FAVOURITE_ID, cur_album->cluster->uuid)) {
			app_control_add_extra_data(service, GL_ARGV_IV_VIEW_BY, GL_ARGV_IV_VIEW_BY_FAVORITES);
		} else {
			app_control_add_extra_data(service, GL_ARGV_IV_VIEW_BY, GL_ARGV_IV_VIEW_BY_FOLER);
		}

		app_control_add_extra_data(service, GL_ARGV_IV_ALBUM_INDEX,
		                           cur_album->cluster->uuid);
	}
	return GL_UG_SUCCESS;
}

static int __gl_ext_albums_set_iv_select_mode(void *data, app_control_h service, int type)
{
	GL_CHECK_VAL(service, GL_UG_FAIL);
	GL_CHECK_VAL(data, GL_UG_FAIL);
	gl_appdata *ad = (gl_appdata *)data;
	gl_cluster *cur_album = _gl_albums_get_current(data);
	gl_dbg("type: %d", type);

	/* 2.0, Local file */
	app_control_add_extra_data(service, GL_ARGV_IV_VIEW_MODE,
	                           "SELECT");

	app_control_add_extra_data(service, GL_ARGV_IV_MEDIA_TYPE,
	                           GL_ARGV_IV_MEDIA_ALL);

	int tab_mode = _gl_ctrl_get_tab_mode(ad);
	switch (tab_mode) {
	case GL_CTRL_TAB_ALBUMS:
		if (cur_album && cur_album->cluster &&
		        cur_album->cluster->uuid) {
			if (!strcmp(GL_ALBUM_ALL_ID, cur_album->cluster->uuid)) {
				app_control_add_extra_data(service, GL_ARGV_IV_VIEW_BY, GL_ARGV_IV_VIEW_BY_ALL);
			} else if (!strcmp(GL_ALBUM_FAVOURITE_ID, cur_album->cluster->uuid)) {
				app_control_add_extra_data(service, GL_ARGV_IV_VIEW_BY, GL_ARGV_IV_VIEW_BY_FAVORITES);
			} else {
				app_control_add_extra_data(service, GL_ARGV_IV_VIEW_BY, GL_ARGV_IV_VIEW_BY_FOLER);
			}

			app_control_add_extra_data(service, GL_ARGV_IV_ALBUM_INDEX,
			                           cur_album->cluster->uuid);
			if (ad->gridinfo.media_display_order == ORDER_ASC) {
				ad->uginfo.sort_type = GL_SORT_BY_DATE_ASC;
			} else if (ad->gridinfo.media_display_order == ORDER_DESC) {
				ad->uginfo.sort_type = GL_SORT_BY_DATE_DESC;
			}
		}
		break;
	case GL_CTRL_TAB_TIMELINE:
		app_control_add_extra_data(service, GL_ARGV_IV_VIEW_BY, GL_ARGV_IV_VIEW_BY_ALL);
		app_control_add_extra_data(service, GL_ARGV_IV_ALBUM_INDEX, GL_ALBUM_ALL_ID);
		break;
	default:
		gl_dbgE("Wrong tab mode!");
	}
	return GL_UG_SUCCESS;
}

static int _gl_ext_load_iv_selected_list(app_control_h service, void *data)
{
	GL_CHECK_VAL(data, GL_UG_FAIL);
	GL_CHECK_VAL(service, GL_UG_FAIL);
	gl_appdata *ad = (gl_appdata *)data;
	int i;
	gl_item *gitem = NULL;
	int count = eina_list_count(ad->selinfo.elist);
	int count_fav = eina_list_count(ad->selinfo.fav_elist);

	char **value = NULL;
	char **value_fav = NULL;
	if (count > 0) {
		(value) = (char**)malloc(count * sizeof(char *));
		if (!value) {
			return GL_UG_FAIL;
		}
	}
	if (count_fav > 0) {
		(value_fav) = (char**)malloc(count_fav * sizeof(char *));
		if (!value_fav) {
			return GL_UG_FAIL;
		}
	}

	for (i = 0; i < count; i++) {
		gitem = eina_list_nth(ad->selinfo.elist, i);
		if (gitem) {
			if (gitem->item) {
				(value)[i] = strdup(gitem->item->file_url);
			}
		}
	}
	for (i = 0; i < count_fav; i++) {
		gitem = eina_list_nth(ad->selinfo.fav_elist, i);
		if (gitem) {
			if (gitem->item) {
				(value_fav)[i] = strdup(gitem->item->file_url);
			}
		}
	}
	if (count > 0) {
		app_control_add_extra_data_array(service, "Selected index",
		                                 (const char **)value, count);
	}
	if (count_fav > 0) {
		app_control_add_extra_data_array(service, "Selected index fav",
		                                 (const char **)value_fav, count_fav);
	}


	if (value) {
		for (i = 0; i < count; i++) {
			free(value[i]);
		}
		free(value);
	}

	if (value_fav) {
		for (i = 0; i < count_fav; i++) {
			free(value_fav[i]);
		}
		free(value_fav);
	}

	return GL_UG_SUCCESS;
}

int gl_ext_load_iv_time_ug_select_mode(void *data, gl_media_s *item, gl_ext_iv_type type, int sort_type)
{
	GL_CHECK_VAL(data, GL_UG_FAIL);
	gl_appdata *ad = (gl_appdata *)data;

	int ret;
	if (sort_type == ORDER_ASC) {
		ad->uginfo.sort_type = GL_SORT_BY_DATE_ASC;
	} else if (sort_type == ORDER_DESC) {
		ad->uginfo.sort_type = GL_SORT_BY_DATE_DESC;
	}
	ret = gl_ext_load_iv_ug_select_mode(data, item, type);

	return ret;
}

int gl_ext_load_iv_ug_select_mode(void *data, gl_media_s *item, gl_ext_iv_type type)
{
	GL_CHECK_VAL(item, GL_UG_FAIL);
	GL_CHECK_VAL(data, GL_UG_FAIL);
	gl_appdata *ad = (gl_appdata *)data;
	app_control_h service = NULL;

	if (ad->uginfo.ug) {
		gl_dbgE("Already exits some UG called by me, type(%d)!", type);
		return GL_UG_FAIL;
	}

	app_control_create(&service);
	GL_CHECK_VAL(service, GL_UG_FAIL);

	ad->uginfo.ug_type = GL_UG_IMAGEVIEWER;
	ad->uginfo.iv_type = type;

	int ret = GL_UG_FAIL;
	ret = __gl_ext_albums_set_iv_select_mode(data, service, type);

	if (ret == GL_UG_FAIL) {
		gl_dbgW("Service data setting failed!");
		app_control_destroy(service);
		return GL_UG_FAIL;
	}

	app_control_add_extra_data(service, GL_ARGV_IV_PATH, item->file_url);
	__gl_ext_add_sortby(service, ad->uginfo.sort_type);

	int tab_mode = _gl_ctrl_get_tab_mode(ad);
	switch (tab_mode) {
	case GL_CTRL_TAB_ALBUMS:
		_gl_ext_load_iv_selected_list(service, ad);
		break;
	case GL_CTRL_TAB_TIMELINE:
		_gl_ext_load_time_iv_selected_list(service, ad);
		break;
	default:
		gl_dbgE("Wrong tab mode!");
	}

	ret = app_control_set_launch_mode(service, APP_CONTROL_LAUNCH_MODE_GROUP);
	if (ret != APP_CONTROL_ERROR_NONE) {
		gl_dbgE("set launch mode failed");
	}
	ret = app_control_set_app_id(service, GL_UG_PKG_IV);
	if (ret != APP_CONTROL_ERROR_NONE) {
		gl_dbgE("set appid failed");
	}
	ret = app_control_send_launch_request(service, __gl_appcontrol_select_result_cb, (void *)ad);

	//ad->uginfo.ug = service;
	app_control_destroy(service);

	if (ret != APP_CONTROL_ERROR_NONE) {
		gl_dbgE("ug_create failed!");
		_gl_popup_create(data, NULL, GL_STR_ID_OPERATION_FAILED,
		                 GL_STR_ID_CLOSE, __gl_ext_popup_resp_cb, data,
		                 NULL, NULL, NULL, NULL, NULL, NULL);
		return GL_UG_FAIL;
	} else {
		elm_object_tree_focus_allow_set(ad->maininfo.layout, EINA_FALSE);
		ad->uginfo.b_ug_launched = true;
		return GL_UG_SUCCESS;
	}
}

/* Invoke Imageviewer UG */
int gl_ext_load_iv_ug(void *data, gl_item *item, gl_ext_iv_type type)
{
	GL_CHECK_VAL(item, GL_UG_FAIL);
	GL_CHECK_VAL(item->item, GL_UG_FAIL);
	GL_CHECK_VAL(data, GL_UG_FAIL);
	gl_appdata *ad = (gl_appdata *)data;
	//GL_CHECK_VAL(ad->maininfo.win, GL_UG_FAIL);
	//Evas_Object *win = ad->maininfo.win;
	app_control_h service = NULL;
	gl_dbg("type: %d", type);

	if (ad->uginfo.ug) {
		gl_dbgE("Already exits some UG called by me, type(%d)!", type);
		return GL_UG_FAIL;
	}

	app_control_create(&service);
	GL_CHECK_VAL(service, GL_UG_FAIL);

	//UG_INIT_EFL(win, UG_OPT_INDICATOR_ENABLE);
	ad->uginfo.ug_type = GL_UG_IMAGEVIEWER;
	ad->uginfo.iv_type = type;

	int ret = GL_UG_FAIL;
	int tab_mode = _gl_ctrl_get_tab_mode(ad);
	switch (tab_mode) {
	case GL_CTRL_TAB_ALBUMS:
		ret = __gl_ext_albums_set_iv(data, service, type, item);
		break;
	case GL_CTRL_TAB_TIMELINE:
		ret = __gl_ext_albums_set_iv(data, service, type, item);
		break;
	default:
		gl_dbgE("Wrong tab mode!");
	}

	if (ret == GL_UG_FAIL) {
		gl_dbgW("Service data setting failed!");
		app_control_destroy(service);
		return GL_UG_FAIL;
	}

	app_control_add_extra_data(service, GL_ARGV_IV_PATH, item->item->file_url);

	/* Sort by type */
	if (ad->gridinfo.media_display_order == ORDER_ASC) {
		ad->uginfo.sort_type = GL_SORT_BY_DATE_ASC;
	} else if (ad->gridinfo.media_display_order == ORDER_DESC) {
		ad->uginfo.sort_type = GL_SORT_BY_DATE_DESC;
	}
	__gl_ext_add_sortby(service, ad->uginfo.sort_type);

#ifdef _USE_APP_SLIDESHOW
	if (type == GL_UG_IV_SLIDESHOW_SELECTED) {
		if (__gl_ext_slideshow_selected(ad, service) == GL_UG_FAIL) {
			gl_dbgE("Failed to slideshow selected files!");
			app_control_destroy(service);
			return GL_UG_FAIL;
		}
		ad->uginfo.b_selected_slideshow = true;
	} else
#endif
		if (type == GL_UG_IV_SLIDESHOW_ALL ||
		        GL_VIEW_ALBUMS == gl_get_view_mode(data)) {
			app_control_add_extra_data(service, GL_ARGV_IV_INDEX,
			                           GL_ARGV_IV_INDEX_VALUE);
		} else if (type == GL_UG_IV || type == GL_UG_IV_SLIDESHOW) {
			char sequence_str[GL_INTERGER_LEN_MAX] = { 0, };
			eina_convert_itoa(item->sequence, sequence_str);
			gl_dbg("sequence : %s", sequence_str);
			app_control_add_extra_data(service, GL_ARGV_IV_INDEX, sequence_str);
		}
	ret = app_control_set_launch_mode(service, APP_CONTROL_LAUNCH_MODE_GROUP);
	if (ret != APP_CONTROL_ERROR_NONE) {
		gl_dbgE("set launch mode failed");
	}
	ret = app_control_set_app_id(service, GL_UG_PKG_IV);
	if (ret != APP_CONTROL_ERROR_NONE) {
		gl_dbgE("set appid failed");
	}
	ret = app_control_send_launch_request(service, __gl_appcontrol_result_cb, (void *)ad);

	//ad->uginfo.ug = service;
	app_control_destroy(service);

	if (ret != APP_CONTROL_ERROR_NONE) {
		gl_dbgE("ug_create failed!");
		_gl_popup_create(data, NULL, GL_STR_ID_OPERATION_FAILED,
		                 GL_STR_ID_CLOSE, __gl_ext_popup_resp_cb, data,
		                 NULL, NULL, NULL, NULL, NULL, NULL);
		return GL_UG_FAIL;
	} else {
		elm_object_tree_focus_allow_set(ad->maininfo.layout, EINA_FALSE);
		ad->uginfo.b_ug_launched = true;
		return GL_UG_SUCCESS;
	}
}

/* Invoke Imageviewer UG from timeline view */
int _gl_ext_load_iv_timeline(void *data, const char *path, int sequence, int sort_type)
{
	GL_CHECK_VAL(data, GL_UG_FAIL);
	int ret = GL_UG_FAIL;
	gl_appdata *ad = (gl_appdata *)data;
	//GL_CHECK_VAL(ad->maininfo.win, GL_UG_FAIL);
	//Evas_Object *win = ad->maininfo.win;
	app_control_h service = NULL;

	if (ad->uginfo.ug) {
		gl_dbgE("Already exits some UG called by me!");
		return GL_UG_FAIL;
	}

	app_control_create(&service);
	GL_CHECK_VAL(service, GL_UG_FAIL);

	//UG_INIT_EFL(win, UG_OPT_INDICATOR_ENABLE);
	ad->uginfo.ug_type = GL_UG_IMAGEVIEWER;
	ad->uginfo.iv_type = GL_UG_IV;

	/* Local file */
	app_control_add_extra_data(service, GL_ARGV_IV_MEDIA_TYPE,
	                           GL_ARGV_IV_MEDIA_ALL);
	gl_dbg("Media type: All");
	/* 'All' album->thumbnails view */
	app_control_add_extra_data(service, GL_ARGV_IV_VIEW_BY,
	                           GL_ARGV_IV_VIEW_BY_ALL);
	gl_dbg("View By: All");

	app_control_add_extra_data(service, GL_ARGV_IV_PATH, path);

	/* Sort by type */
	if (sort_type == ORDER_ASC) {
		ad->uginfo.sort_type = GL_SORT_BY_DATE_ASC;
	} else if (sort_type == ORDER_DESC) {
		ad->uginfo.sort_type = GL_SORT_BY_DATE_DESC;
	}
	__gl_ext_add_sortby(service, ad->uginfo.sort_type);
	char sequence_str[GL_INTERGER_LEN_MAX] = { 0, };
	eina_convert_itoa(sequence, sequence_str);
	gl_dbg("sequence: %s", sequence_str);
	app_control_add_extra_data(service, GL_ARGV_IV_INDEX, sequence_str);
	ret = app_control_set_launch_mode(service, APP_CONTROL_LAUNCH_MODE_GROUP);
	if (ret != APP_CONTROL_ERROR_NONE) {
		gl_dbgE("set launch mode failed");
	}

	ret = app_control_set_app_id(service, GL_UG_PKG_IV);
	if (ret != APP_CONTROL_ERROR_NONE) {
		gl_dbgE("set appid failed");
	}
	ret = app_control_send_launch_request(service, __gl_appcontrol_result_cb, (void *)ad);

	//ad->uginfo.ug = service;
	app_control_destroy(service);

	if (ret != APP_CONTROL_ERROR_NONE) {
		gl_dbgE("ug_create failed!");
		_gl_popup_create(data, NULL, GL_STR_ID_OPERATION_FAILED,
		                 GL_STR_ID_CLOSE, __gl_ext_popup_resp_cb, data,
		                 NULL, NULL, NULL, NULL, NULL, NULL);
		return GL_UG_FAIL;

	} else {
		/*Disable the focus permission of the app layout*/
		/*or else the highlight will fall under the ug layout */
		elm_object_tree_focus_allow_set(ad->maininfo.layout, EINA_FALSE);
		ad->uginfo.b_ug_launched = true;
		return GL_UG_SUCCESS;

	}
}

int gl_ext_load_ug(void *data, gl_ext_ug_type type)
{
	GL_CHECK_VAL(data, GL_UG_FAIL);
	int ret = GL_UG_FAIL;
	gl_appdata *ad = (gl_appdata *)data;
	GL_CHECK_VAL(ad->maininfo.win, GL_UG_FAIL);
	app_control_h service = NULL;

	if (ad->uginfo.ug) {
		gl_dbg("Already exits some UG, Request type(%d)", type);
		return GL_UG_FAIL;
	}

	ad->uginfo.ug_type = type;
	ad->uginfo.data = NULL;

	app_control_create(&service);
	GL_CHECK_VAL(service, GL_UG_FAIL);

	switch (type) {
	case GL_UG_GALLERY_SETTING_SLIDESHOW:
		ad->uginfo.b_start_slideshow = false;
		app_control_add_extra_data(service, "ShowStartBtn", "TRUE");
		ret = app_control_set_app_id(service, GL_UG_PKG_GALLERY_SETTING);
		if (ret != APP_CONTROL_ERROR_NONE) {
			gl_dbgE("service create failed");
		}
		ret = app_control_send_launch_request(service, __gl_appcontrol_result_cb, (void *)ad);
		break;
	case GL_UG_GALLERY_SETTING:
		ret = app_control_set_app_id(service, GL_UG_PKG_GALLERY_SETTING);
		if (ret != APP_CONTROL_ERROR_NONE) {
			gl_dbgE("service create failed");
		}
		ret = app_control_send_launch_request(service, __gl_appcontrol_result_cb, (void *)ad);
		break;
	default:
		gl_dbgE("Wrong UG type!");
		goto EXT_UG_FAILED;
	}
	ad->uginfo.ug = service;

	if (ret != APP_CONTROL_ERROR_NONE) {
		gl_dbgE("ug_create failed!");
		_gl_popup_create(data, NULL, GL_STR_ID_OPERATION_FAILED,
		                 GL_STR_ID_CLOSE, __gl_ext_popup_resp_cb, data,
		                 NULL, NULL, NULL, NULL, NULL, NULL);
		return GL_UG_FAIL;
	} else {
		gl_dbgW("ug_create success!");
		/*Disable the focus permission of the app layout*/
		/*or else the highlight will fall under the ug layout */
		elm_object_tree_focus_allow_set(ad->maininfo.layout, EINA_FALSE);
		return GL_UG_SUCCESS;
	}

EXT_UG_FAILED:

	gl_dbgE("EXT_UG_FAILED!");
	app_control_destroy(service);
	_gl_popup_create(data, NULL, GL_STR_ID_OPERATION_FAILED,
	                 GL_STR_ID_CLOSE, __gl_ext_popup_resp_cb, data,
	                 NULL, NULL, NULL, NULL, NULL, NULL);
	return GL_UG_FAIL;
}

