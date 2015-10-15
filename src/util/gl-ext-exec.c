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
#include "gl-ext-exec.h"
#include "gl-debug.h"
#include "gallery.h"
#include "gl-data.h"
#include "gl-util.h"
#include "gl-popup.h"
#include "gl-controlbar.h"
#include "gl-albums.h"
#include "gl-thumbs.h"
#include "gl-strings.h"
#include "gl-notify.h"

#define GL_APP_PKG_VIDEOPLAYER	"org.tizen.videos-lite"
#define GL_APP_PKG_CAMERA_EFL "org.tizen.camera-app-lite"
#define GL_APP_PKG_IE "com.samsung.image-editor"
#define GL_SEPERATOR_IE '|'
#define GL_ARGV_VIDEO_LIST_TYPE "video_list_type"
#define GL_ARGV_VIDEO_ALL_FOLDER_VIDEO "all_folder_video"
#define GL_ARGV_VIDEO_FOLDER "folder"

#define GL_ARGV_VIDEO_ORDER_TYPE "order_type"
#define GL_ARGV_VIDEO_ORDER_DATE_DESC "date_desc"
#define GL_ARGV_VIDEO_START_POS "start_pos_time"

#define GL_ARGV_VIDEO_LAUNCH_APP "launching_application"
#define GL_ARGV_VIDEO_LAUNCH_GALLERY "gallery"

#define GL_ARGV_PATH "path"

#define GL_SHARE_FILE_PREFIX "file://"
#define GL_SHARE_OPERATION_SINGLE "http://tizen.org/appcontrol/operation/share"
#define GL_SHARE_OPERATION_MULTIPLE "http://tizen.org/appcontrol/operation/multi_share"
#define GL_SHARE_SVC_FILE_PATH "http://tizen.org/appcontrol/data/path"

//refer to imageviewer
static int __gl_ext_launch_video_player(void *data, app_control_h service)
{
	GL_CHECK_VAL(service, -1);
	GL_CHECK_VAL(data, -1);
	gl_item *gitem = (gl_item *)data;
	GL_CHECK_VAL(gitem->ad, -1);
	gl_appdata *ad = (gl_appdata *)gitem->ad;
	GL_CHECK_VAL(gitem->item, -1);
	GL_CHECK_VAL(gitem->item->uuid, -1);
	GL_CHECK_VAL(gitem->item->file_url, -1);

	int last_pos = 0;
	char last_pos_str[GL_INTERGER_LEN_MAX] = { 0, };

	GL_CHECK_VAL(gitem->item->video_info, -1);
	last_pos = gitem->item->video_info->last_played_pos;
	eina_convert_itoa(last_pos, last_pos_str);
	gl_dbg("last_pos : %d", last_pos);

	const char *video_path = gitem->item->file_url;
	/* Albums view */
	gl_cluster *cur_cluster = _gl_albums_get_current(ad);
	if (cur_cluster && cur_cluster->cluster) {
		gl_sdbg("Album: %s", cur_cluster->cluster->display_name);
		if (cur_cluster->cluster->type == GL_STORE_T_ALL) {
			gl_dbg("All album");
			app_control_add_extra_data(service,
					       GL_ARGV_VIDEO_LIST_TYPE,
					       GL_ARGV_VIDEO_ALL_FOLDER_VIDEO);
			app_control_add_extra_data(service,
					       GL_ARGV_VIDEO_ORDER_TYPE,
					       GL_ARGV_VIDEO_ORDER_DATE_DESC);
			app_control_add_extra_data(service,
					       GL_ARGV_VIDEO_LAUNCH_APP,
					       GL_ARGV_VIDEO_LAUNCH_GALLERY);
			app_control_add_extra_data(service,
					       GL_ARGV_VIDEO_START_POS,
					       last_pos_str);
		} else if (cur_cluster->cluster->type == GL_STORE_T_PHONE ||
			 cur_cluster->cluster->type == GL_STORE_T_MMC) {
			gl_dbg("Real album");
			app_control_add_extra_data(service,
					       GL_ARGV_VIDEO_LIST_TYPE,
					       GL_ARGV_VIDEO_FOLDER);
			app_control_add_extra_data(service,
					       GL_ARGV_VIDEO_ORDER_TYPE,
					       GL_ARGV_VIDEO_ORDER_DATE_DESC);
			app_control_add_extra_data(service,
					       GL_ARGV_VIDEO_LAUNCH_APP,
					       GL_ARGV_VIDEO_LAUNCH_GALLERY);
			app_control_add_extra_data(service,
					       GL_ARGV_VIDEO_START_POS,
					       last_pos_str);
		} else {
			gl_dbg("Web album? Under construction...");
		}
	} else {
		/* Fixme: return or not? */
		gl_dbgE("Invalid current album, return?");
	}

	int ret = 0;

	//              aul_open_file(video_path);
	app_control_add_extra_data(service, GL_ARGV_PATH, video_path);
	app_control_set_operation(service, APP_CONTROL_OPERATION_DEFAULT);
	if (APP_CONTROL_ERROR_NONE != app_control_set_app_id(service, GL_APP_PKG_VIDEOPLAYER)) {
		gl_dbgE("app_control_set_app_id failed");
	}
	ret = app_control_send_launch_request(service, NULL, NULL);

	if (ret == APP_CONTROL_ERROR_NONE)
		return 0;
	else
		return -1;
}

static int __gl_ext_compose_exec_cmd(void *data, gl_ext_app_type type,
				      char *path, char **pkg_name,
				      app_control_h service)
{
	GL_CHECK_VAL(data, -1);
	gl_sdbg("type:%d, path:%s", type, path);

	switch (type) {
	case GL_APP_VIDEOPLAYER:
		GL_CHECK_VAL(path, -1);
		*pkg_name = GL_APP_PKG_VIDEOPLAYER;
		break;
	default:
		*pkg_name = NULL;
	}

	return 0;
}

#ifdef GL_EXTENDED_FEATURES
static void __gl_ext_app_control_reply_cb(app_control_h request, app_control_h reply,
				      app_control_result_e result, void *user_data)
{
	gl_dbg("");
	switch (result) {
	case APP_CONTROL_RESULT_SUCCEEDED:
		gl_dbg("APP_CONTROL_RESULT_SUCCEEDED");
		break;
	case APP_CONTROL_RESULT_FAILED:
		gl_dbg("APP_CONTROL_RESULT_FAILED");
		break;
	case APP_CONTROL_RESULT_CANCELED:
		gl_dbg("APP_CONTROL_RESULT_CANCELED");
		break;
	default:
		gl_dbgE("Unhandled value: %d!", result);
		break;
	}
}
#endif

int gl_ext_exec(void *data, gl_ext_app_type type)
{
	GL_CHECK_VAL(data, -1);
	gl_item *gitem = (gl_item *)data;
	GL_CHECK_VAL(gitem->ad, -1);
	gl_appdata *ad = (gl_appdata *)gitem->ad;
	GL_CHECK_VAL(gitem->item, -1);
	GL_CHECK_VAL(gitem->item->file_url, -1);
	char *path = gitem->item->file_url;
	app_control_h service = NULL;
	char *pkg_name = NULL;

	if (ad->uginfo.b_app_called) {
		gl_dbgE("APP launched, return!");
		return GL_LAUNCH_FAIL;
	}

	app_control_create(&service);
	GL_CHECK_VAL(service, GL_LAUNCH_FAIL);

	__gl_ext_compose_exec_cmd(ad, type, path, &pkg_name, service);
	if (pkg_name == NULL) {
		app_control_destroy(service);
		return GL_LAUNCH_FAIL;
	}

	int ret = 0;
	if (type == GL_APP_VIDEOPLAYER) {
		if (path == NULL) {
			app_control_destroy(service);
			return GL_LAUNCH_FAIL;
		}
		if (__gl_ext_launch_video_player(data, service) != 0) {
			app_control_destroy(service);
			return GL_LAUNCH_FAIL;
		}
	} else {
		app_control_set_operation(service, APP_CONTROL_OPERATION_DEFAULT);
		app_control_set_app_id(service, pkg_name);
		ret = app_control_send_launch_request(service, NULL, NULL);
		if (ret != APP_CONTROL_ERROR_NONE) {
			app_control_destroy(service);
			gl_dbgE("app_control_send_launch_request failed[%d]!", ret);
			return GL_LAUNCH_FAIL;
		}
	}

	ret = app_control_destroy(service);
	if (ret != APP_CONTROL_ERROR_NONE) {
		ad->uginfo.b_app_called = true;
		return GL_LAUNCH_SUCCESS;
	} else {
		return GL_LAUNCH_FAIL;
	}
}

#ifdef GL_EXTENDED_FEATURES
static void __gl_ext_popup_resp_cb(void *data, Evas_Object *obj, void *ei)
{
	GL_CHECK(obj);
	GL_CHECK(data);
	gl_appdata *ad = (gl_appdata *)data;
	GL_IF_DEL_OBJ(ad->popupinfo.popup);
}

static Eina_Bool __gl_ext_avoid_multi_click_timer_cb(void *data)
{
	GL_CHECK_FALSE(data);
	gl_appdata *ad = (gl_appdata *)data;
	gl_dbg("");
	GL_IF_DEL_TIMER(ad->ctrlinfo.avoid_multi_tap);
	return ECORE_CALLBACK_CANCEL;
}

/**
 * Launching Camera application
 */
int _gl_ext_load_camera(void *data)
{
	gl_dbg("");
	GL_CHECK_VAL(data, -1);
	gl_appdata *ad = (gl_appdata *)data;
	if (ad->ctrlinfo.avoid_multi_tap)
		return 0;
	Ecore_Timer *timer = NULL;
	timer = ecore_timer_add(2.0, __gl_ext_avoid_multi_click_timer_cb, data);
	ad->ctrlinfo.avoid_multi_tap = timer;

	int ret = -1;
	int destroy_ret = -1;
	app_control_h handle;

	ret = app_control_create(&handle);
	if (ret != APP_CONTROL_ERROR_NONE) {
		gl_dbgE("app_control_create failed[%d]!", ret);
		return GL_LAUNCH_FAIL;
	}

	ret = app_control_set_operation(handle, APP_CONTROL_OPERATION_DEFAULT);
	if (ret != APP_CONTROL_ERROR_NONE) {
		gl_dbgE("app_control_set_operation [%s] failed[%d]!", APP_CONTROL_OPERATION_DEFAULT, ret);
		goto GL_EXT_LOAD_CAMERA_FINISHED;
	}

	ret = app_control_set_app_id(handle, GL_APP_PKG_CAMERA_EFL);
	if (ret != APP_CONTROL_ERROR_NONE) {
		gl_dbgE("app_control_set_app_id [%s] failed[%d]!", GL_APP_PKG_CAMERA_EFL, ret);
		goto GL_EXT_LOAD_CAMERA_FINISHED;
	}

	ret = app_control_add_extra_data(handle, "CALLER", "gallery");
	if (ret != APP_CONTROL_ERROR_NONE) {
		gl_dbgE("app_control_add_extra_data failed[%d]!", ret);
		goto GL_EXT_LOAD_CAMERA_FINISHED;
	}

	ret = app_control_send_launch_request(handle, __gl_ext_app_control_reply_cb, NULL);
	if (ret != APP_CONTROL_ERROR_NONE) {
		gl_dbgE("app_control_send_launch_request failed[%d]!", ret);
		goto GL_EXT_LOAD_CAMERA_FINISHED;
	}

GL_EXT_LOAD_CAMERA_FINISHED:
	destroy_ret = app_control_destroy(handle);
	if (destroy_ret != APP_CONTROL_ERROR_NONE) {
		gl_dbgE("app_control_destroy failed[%d]!", destroy_ret);
		return GL_LAUNCH_FAIL;
	}

	return (ret == APP_CONTROL_ERROR_NONE ? GL_LAUNCH_SUCCESS : GL_LAUNCH_FAIL);

}

/**
 * Launching image-editor application
 */
int _gl_ext_load_ie(void *data, void *get_path_cb)
{
	GL_CHECK_VAL(get_path_cb, -1);
	GL_CHECK_VAL(data, -1);
	char *filepath_arg = NULL;

	int (*_get_path_cb) (void *data, gchar sep_c, char **path_str, int *sel_cnt);
	_get_path_cb = get_path_cb;

	_get_path_cb(data, GL_SEPERATOR_IE, &filepath_arg, NULL);
	if (filepath_arg == NULL) {
		gl_dbgE("Failed to get path string!");
		return GL_LAUNCH_FAIL;
	}

	gl_dbg("Launch Image-editor");
	int ret = GL_LAUNCH_FAIL;
	ret = _gl_ext_load(filepath_arg, APP_CONTROL_OPERATION_EDIT, GL_APP_PKG_IE,
			   NULL, NULL);
	if (filepath_arg) {
		g_free(filepath_arg);
		filepath_arg = NULL;
	}

	return ret;
}

int _gl_ext_load(const char *uri, const char *operation, const char *pkg,
		 const char *pkg_id, const char *mime)
{
	gl_dbg("operation: %s, pkg: %s, uri: %s", operation, pkg, uri);
	int ret = -1;
	int destroy_ret = -1;
	app_control_h handle;

	ret = app_control_create(&handle);
	if (ret != APP_CONTROL_ERROR_NONE) {
		gl_dbgE("app_control_create failed[%d]!", ret);
		return GL_LAUNCH_FAIL;
	}

	ret = app_control_set_operation(handle, operation);
	if (ret != APP_CONTROL_ERROR_NONE) {
		gl_dbgE("app_control_set_operation [%s] failed[%d]!", operation,
			ret);
		goto GL_EXT_FAILED;
	}

	if (mime) {
		ret = app_control_set_mime(handle, mime);
		if (ret != APP_CONTROL_ERROR_NONE) {
			gl_dbgE("app_control_set_mime [%s] failed[%d]!", mime, ret);
			goto GL_EXT_FAILED;
		}
	}

	if (uri) {
		ret = app_control_set_uri(handle, uri);
		if (ret != APP_CONTROL_ERROR_NONE) {
			gl_dbgE("app_control_set_uri [%s] failed[%d]!", uri, ret);
			goto GL_EXT_FAILED;
		}
	}

	if (pkg) {
		ret = app_control_set_app_id(handle, pkg);
		if (ret != APP_CONTROL_ERROR_NONE) {
			gl_dbgE("app_control_set_app_id [%s] failed[%d]!", pkg,
				ret);
			goto GL_EXT_FAILED;
		}
	}

	if (pkg_id) {
		ret = app_control_set_app_id(handle, pkg_id);
		if (ret != APP_CONTROL_ERROR_NONE) {
			gl_dbgE("app_control_set_app_id [%s] failed[%d]!", pkg_id,
				ret);
			goto GL_EXT_FAILED;
		}
	}

	ret = app_control_send_launch_request(handle, __gl_ext_app_control_reply_cb,
					  NULL);
	if (ret != APP_CONTROL_ERROR_NONE) {
		gl_dbgE("app_control_send_launch_request failed[%d]!", ret);
		goto GL_EXT_FAILED;
	}

 GL_EXT_FAILED:
	destroy_ret = app_control_destroy(handle);
	if (destroy_ret != APP_CONTROL_ERROR_NONE) {
		gl_dbgE("app_control_destroy failed[%d]!", destroy_ret);
		return GL_LAUNCH_FAIL;
	}

	return (ret == APP_CONTROL_ERROR_NONE ? GL_LAUNCH_SUCCESS : GL_LAUNCH_FAIL);
}

int _gl_ext_launch_share(void *data, int total_cnt, void *get_path_cb)
{
#define GL_SHARE_CNT_MAX 500

	GL_CHECK_VAL(get_path_cb, -1);
	GL_CHECK_VAL(data, -1);
	gl_appdata *ad = (gl_appdata *)data;

	if (total_cnt == 0) {
		gl_dbgE("nothing selected!");
		return -1;
	} else if (total_cnt > GL_SHARE_CNT_MAX) {
		char *share_limit_noti = g_strdup_printf(GL_STR_ID_SHARE_LIMIT, GL_SHARE_CNT_MAX);
		_gl_notify_create_notiinfo(share_limit_noti);
		GL_FREEIF(share_limit_noti);
		return -1;
	}

	char **files = NULL;
	char *operation = NULL;
	int ret = -1;
	files = (char **)calloc(1, sizeof(char *) * total_cnt);
	GL_CHECK_VAL(files, -1);

	int (*_get_path_cb) (void *data, char **files);
	_get_path_cb = get_path_cb;
	int real_cnt = _get_path_cb(data, files);
	app_control_h service = NULL;
	if (real_cnt == 0) {
		gl_dbgE("nothing selected!");
		goto SHARE_PANEL_END;
	}

	ret = app_control_create(&service);
	if (ret != APP_CONTROL_ERROR_NONE) {
		gl_dbgE("Failed to create service[%d]", ret);
		goto SHARE_PANEL_END;
	}
	char prefix_file[GL_ARRAY_LEN_MAX] = {0,};
	snprintf(prefix_file, GL_ARRAY_LEN_MAX, "%s%s", GL_SHARE_FILE_PREFIX,
		 files[0]);
	ret = app_control_set_uri(service, prefix_file);
	if (ret != APP_CONTROL_ERROR_NONE) {
		gl_dbgE("app_control_set_uri failed[%d]", ret);
		goto SHARE_PANEL_END;
	}
	if (real_cnt == 1)
		operation = GL_SHARE_OPERATION_SINGLE;
	else
		operation = GL_SHARE_OPERATION_MULTIPLE;
	ret = app_control_set_operation(service, operation);
	if (ret != APP_CONTROL_ERROR_NONE) {
		gl_dbgE("app_control_set_operation failed[%d]", ret);
		goto SHARE_PANEL_END;
	}
	if (real_cnt > 1) {
		ret = app_control_add_extra_data_array(service, GL_SHARE_SVC_FILE_PATH,
						   (const char **)files,
						   real_cnt);
		if (ret != APP_CONTROL_ERROR_NONE) {
			gl_dbgE("app_control_add_extra_data_array failed[%d]", ret);
			goto SHARE_PANEL_END;
		}
	}
	/* set caller window for share panel */
	ret = app_control_set_window(service, elm_win_xwindow_get(ad->maininfo.win));
	if (ret != APP_CONTROL_ERROR_NONE) {
		gl_dbgE("app_control_set_window failed[%d]", ret);
		goto SHARE_PANEL_END;
	}
	ret = app_control_send_launch_request(service, NULL, NULL);
	if (ret != APP_CONTROL_ERROR_NONE)
		gl_dbgE("app_control_send_launch_request failed[%d]!", ret);

 SHARE_PANEL_END:

	if (service)
		app_control_destroy(service);
	if (files) {
		for (; real_cnt > 0; --real_cnt)
			GL_FREEIF(files[real_cnt - 1]);
		GL_FREE(files);
	}
	if (ret != APP_CONTROL_ERROR_NONE) {
		_gl_popup_create(data, NULL, GL_STR_ID_OPERATION_FAILED,
				 GL_STR_ID_CLOSE, __gl_ext_popup_resp_cb, data,
				 NULL, NULL, NULL, NULL, NULL, NULL);
		return -1;
	}
	return 0;
}

int _gl_ext_is_larger_than_share_max(int total_cnt)
{
#define GL_SHARE_CNT_MAX 500

	if (total_cnt > GL_SHARE_CNT_MAX) {
		char limit_str[256] = {0};
		snprintf(limit_str, sizeof(limit_str), GL_STR_ID_SHARE_LIMIT,
			 GL_SHARE_CNT_MAX);
		_gl_notify_create_notiinfo(limit_str);
		return true;
	}
	return false;
}

int _gl_ext_launch_share_with_files(void *data, int total_cnt, char **files)
{
	GL_CHECK_VAL(files, -1);
	GL_CHECK_VAL(data, -1);
	gl_appdata *ad = (gl_appdata *)data;
	char *operation = NULL;
	int ret = -1;
	app_control_h service = NULL;

	if (total_cnt == 0) {
		gl_dbgE("nothing selected!");
		goto SHARE_PANEL_END;
	}

	ret = app_control_create(&service);
	if (ret != APP_CONTROL_ERROR_NONE) {
		gl_dbgE("Failed to create service[%d]", ret);
		goto SHARE_PANEL_END;
	}
	char prefix_file[GL_ARRAY_LEN_MAX] = {0,};
	snprintf(prefix_file, GL_ARRAY_LEN_MAX, "%s%s", GL_SHARE_FILE_PREFIX,
		 files[0]);
	ret = app_control_set_uri(service, prefix_file);
	if (ret != APP_CONTROL_ERROR_NONE) {
		gl_dbgE("app_control_set_uri failed[%d]", ret);
		goto SHARE_PANEL_END;
	}
	if (total_cnt == 1)
		operation = GL_SHARE_OPERATION_SINGLE;
	else
		operation = GL_SHARE_OPERATION_MULTIPLE;
	ret = app_control_set_operation(service, operation);
	if (ret != APP_CONTROL_ERROR_NONE) {
		gl_dbgE("app_control_set_operation failed[%d]", ret);
		goto SHARE_PANEL_END;
	}
	if (total_cnt > 1) {
		ret = app_control_add_extra_data_array(service, GL_SHARE_SVC_FILE_PATH,
						   (const char **)files,
						   total_cnt);
		if (ret != APP_CONTROL_ERROR_NONE) {
			gl_dbgE("app_control_add_extra_data_array failed[%d]", ret);
			goto SHARE_PANEL_END;
		}
	}
	/* set caller window for share panel */
	ret = app_control_set_window(service, elm_win_xwindow_get(ad->maininfo.win));
	if (ret != APP_CONTROL_ERROR_NONE) {
		gl_dbgE("app_control_set_window failed[%d]", ret);
		goto SHARE_PANEL_END;
	}
	ret = app_control_send_launch_request(service, NULL, NULL);
	if (ret != APP_CONTROL_ERROR_NONE)
		gl_dbgE("app_control_send_launch_request failed[%d]!", ret);

 SHARE_PANEL_END:

	if (service)
		app_control_destroy(service);
	if (ret != APP_CONTROL_ERROR_NONE) {
		_gl_popup_create(data, NULL, GL_STR_ID_OPERATION_FAILED,
				 GL_STR_ID_CLOSE, __gl_ext_popup_resp_cb, data,
				 NULL, NULL, NULL, NULL, NULL, NULL);
		return -1;
	}
	return 0;
}

int _gl_ext_launch_image_editor(const char *path_string)
{
	GL_CHECK_VAL(path_string, -1);

	gl_dbg("Launch Image-editor");
	int ret = GL_LAUNCH_FAIL;
	ret = _gl_ext_load(path_string, APP_CONTROL_OPERATION_EDIT, GL_APP_PKG_IE,
			   NULL, NULL);
	return ret;
}

#ifndef _USE_APP_SLIDESHOW
int _gl_ext_launch_gallery_ug(void *data)
{
#define GL_GALLERY_UG  "gallery-efl-lite"
#define GL_FILE_TYPE "file-type"
#define GL_VIEW_BY "view-by"
#define GL_EXT_STR_BUNDLE_LEN 48
	GL_CHECK_VAL(data, -1);
	gl_appdata *ad = (gl_appdata *)data;
	int ret = -1;
	app_control_h service = NULL;

	ret = app_control_create(&service);
	if (ret != APP_CONTROL_ERROR_NONE) {
		gl_dbgE("app_control_create failed: 0x%x", ret);
		return false;
	}
	/* set caller window */
	ret = app_control_set_window(service, elm_win_xwindow_get(ad->maininfo.win));
	if (ret != APP_CONTROL_ERROR_NONE) {
		gl_dbgE("set_window failed[%d]", ret);
		goto GL_EXT_FAILED;
	}
	ret = app_control_set_app_id(service, GL_GALLERY_UG);
	if (ret != APP_CONTROL_ERROR_NONE) {
		gl_dbgE("set_app_id Failed: 0x%x", ret);
		goto GL_EXT_FAILED;
	}
	ret = app_control_add_extra_data(service, "launch-type",
				     "select-slideshow");
	if (ret != APP_CONTROL_ERROR_NONE) {
		gl_dbgE("Add launched type failed: 0x%x", ret);
		goto GL_EXT_FAILED;
	}
	ret = app_control_add_extra_data(service, "indicator-state", "hide");
	if (ret != APP_CONTROL_ERROR_NONE) {
		gl_dbgE("Add indicator state failed: 0x%x", ret);
		goto GL_EXT_FAILED;
	}

	int view_mode = gl_get_view_mode(ad);
	gl_dbg("view_mode: %d", view_mode);
	ret = app_control_add_extra_data(service, GL_FILE_TYPE, "all");
	gl_dbg("Media type: All");
	if (ret != APP_CONTROL_ERROR_NONE) {
		gl_dbgE("Add media type failed: 0x%x", ret);
		goto GL_EXT_FAILED;
	}

	int tab_mode = _gl_ctrl_get_tab_mode(ad);
	gl_dbg("tab_mode: %d", tab_mode);
	switch (tab_mode) {
	case GL_CTRL_TAB_ALBUMS:
	{
		char *album_id = NULL;
		gl_cluster *cur_album = _gl_albums_get_current(data);
		if (cur_album && cur_album->cluster &&
		   cur_album->cluster->uuid) {
			album_id = g_strdup(cur_album->cluster->uuid);
		}
		if (album_id) {
			ret = app_control_add_extra_data(service, GL_VIEW_BY,
						     "albums1");
			if (ret != APP_CONTROL_ERROR_NONE) {
				gl_dbgE("Add view by failed: 0x%x", ret);
				goto GL_EXT_FAILED;
			}
			ret = app_control_add_extra_data(service, "album-id",
						     album_id);
			if (ret != APP_CONTROL_ERROR_NONE) {
				gl_dbgE("Add album id failed: 0x%x", ret);
				goto GL_EXT_FAILED;
			}
			GL_GFREE(album_id);
		} else {
			gl_dbg("Albums2");
			ret = app_control_add_extra_data(service, GL_VIEW_BY,
						     "all");
			if (ret != APP_CONTROL_ERROR_NONE) {
				gl_dbgE("Add view by failed: 0x%x", ret);
				goto GL_EXT_FAILED;
			}
		}
		break;
	}
	case GL_CTRL_TAB_TIMELINE:
		ret = app_control_add_extra_data(service, GL_VIEW_BY, "all");
		if (ret != APP_CONTROL_ERROR_NONE) {
			gl_dbgE("Add view by failed: 0x%x", ret);
			goto GL_EXT_FAILED;
		}
		break;
	default:
		gl_dbgE("Wrong tab mode!");
		goto GL_EXT_FAILED;
	}

	ret = app_control_send_launch_request(service, __gl_ext_app_control_reply_cb,
					  NULL);
	if (ret != APP_CONTROL_ERROR_NONE) {
		gl_dbgE("app_control_send_launch_request Failed: 0x%x", ret);
		goto GL_EXT_FAILED;
	}

	app_control_destroy(service);
	return 0;

 GL_EXT_FAILED:

	app_control_destroy(service);
	return -1;
}
#endif
#endif

