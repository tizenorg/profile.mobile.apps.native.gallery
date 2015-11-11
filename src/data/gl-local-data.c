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

#include <media_content.h>
#include <media_info.h>
#include <string.h>
#include <glib.h>
#include "gl-local-data.h"
#include "gl-data-util.h"
#include "gl-debug.h"
#include "gl-thread-util.h"
#include "gl-fs.h"
#include "gl-file-util.h"

typedef struct _gl_transfer_data_t gl_transfer_data_s;
typedef struct _gl_foreach_data_t gl_foreach_data_s;

struct _gl_transfer_data_t {
	void **userdata;
	filter_h filter;
	char *album_id;
	bool with_meta;
};

struct _gl_foreach_data_t {
	void **userdata;
	filter_h filter;
	char *album_id;
	bool with_meta;
};

static int __gl_local_data_create_filter(gl_filter_s *condition, filter_h *filter)
{
	GL_CHECK_VAL(filter, -1);
	GL_CHECK_VAL(condition, -1);
	int ret = -1;
	filter_h tmp_filter = NULL;
	ret = media_filter_create(&tmp_filter);
	if (ret != MEDIA_CONTENT_ERROR_NONE) {
		gl_dbgE("Fail to create filter");
		return -1;
	}

	if (strlen(condition->cond) > 0) {
		ret = media_filter_set_condition(tmp_filter, condition->cond,
		                                 condition->collate_type);
		if (ret != MEDIA_CONTENT_ERROR_NONE) {
			gl_dbgE("Fail to set condition");
			goto GL_LOCAL_FAILED;
		}
	}

	if (strlen(condition->sort_keyword) > 0) {
		ret = media_filter_set_order(tmp_filter, condition->sort_type,
		                             condition->sort_keyword,
		                             condition->collate_type);
		if (ret != MEDIA_CONTENT_ERROR_NONE) {
			gl_dbgE("Fail to set order");
			goto GL_LOCAL_FAILED;
		}
	}

	if (condition->offset != -1 && condition->count != -1 &&
	        condition->count > 0) {
		ret = media_filter_set_offset(tmp_filter, condition->offset,
		                              condition->count);
		if (ret != MEDIA_CONTENT_ERROR_NONE) {
			gl_dbgE("Fail to set offset");
			goto GL_LOCAL_FAILED;
		}
	}

	*filter = tmp_filter;
	return 0;

GL_LOCAL_FAILED:

	if (tmp_filter) {
		media_filter_destroy(tmp_filter);
		tmp_filter = NULL;
		*filter = NULL;
	}
	return -1;
}

static int __gl_local_data_destroy_filter(filter_h filter)
{
	GL_CHECK_VAL(filter, -1);
	if (media_filter_destroy(filter) != MEDIA_CONTENT_ERROR_NONE) {
		gl_dbgE("Failed to destroy filter!");
		return -1;
	}

	return 0;
}

static bool __gl_local_data_get_album_list_cb(media_folder_h folder,
        void *data)
{
	GL_CHECK_FALSE(data);
	gl_transfer_data_s *tmp_data = (gl_transfer_data_s *)data;
	GL_CHECK_FALSE(tmp_data->userdata);
	GL_CHECK_FALSE(folder);

	Eina_List **elist = (Eina_List **)(tmp_data->userdata);
	gl_album_s *album = NULL;

	album = (gl_album_s *)calloc(1, sizeof(gl_album_s));
	GL_CHECK_FALSE(album);
	album->gtype = GL_TYPE_ALBUM;

	if (media_folder_clone(&(album->folder_h), folder) != MEDIA_CONTENT_ERROR_NONE) {
		gl_dbgE("Clone folder handle failed!");
		goto GL_LOCAL_FAILED;
	}

	if (media_folder_get_folder_id(folder, &(album->uuid)) != MEDIA_CONTENT_ERROR_NONE) {
		gl_dbgE("Get folder id failed!");
		goto GL_LOCAL_FAILED;
	}

	if (media_folder_get_modified_time(folder, &(album->mtime)) != MEDIA_CONTENT_ERROR_NONE) {
		gl_dbgE("Get modified time failed!");
		goto GL_LOCAL_FAILED;
	}

	media_content_storage_e storage_type;
	if (media_folder_get_storage_type(folder, &storage_type) != MEDIA_CONTENT_ERROR_NONE) {
		gl_dbgE("Get folder type failed!");
		goto GL_LOCAL_FAILED;
	}

	if (storage_type == MEDIA_CONTENT_STORAGE_INTERNAL) {/* The device's internal storage */
		album->type = GL_STORE_T_PHONE;
	} else if (storage_type == MEDIA_CONTENT_STORAGE_EXTERNAL) {/* The device's external storage */
		album->type = GL_STORE_T_MMC;
	} else {
		gl_dbgE("Undefined mode[%d]!", storage_type);
	}

	if (media_folder_get_name(folder, &(album->display_name)) != MEDIA_CONTENT_ERROR_NONE) {
		gl_dbgE("Get folder name failed!");
		goto GL_LOCAL_FAILED;
	}

	if (media_folder_get_path(folder, &(album->path)) != MEDIA_CONTENT_ERROR_NONE) {
		gl_dbgE("Get folder path failed!");
		goto GL_LOCAL_FAILED;
	}
	/* TMP: done */
	if (tmp_data->filter) {
		if (media_folder_get_media_count_from_db(album->uuid, tmp_data->filter, &(album->count)) != MEDIA_CONTENT_ERROR_NONE) {
			gl_dbgE("Failed to get count[uuid: %s]!", album->uuid);
			goto GL_LOCAL_FAILED;
		}
	}

	*elist = eina_list_append(*elist, album);

	return true;

GL_LOCAL_FAILED:

	_gl_data_type_free_glitem((void **)(&album));
	return false;
}

static bool __gl_local_data_get_media_list_cb(media_info_h media, void *data)
{
	GL_CHECK_FALSE(data);
	gl_transfer_data_s *tmp_data = (gl_transfer_data_s *)data;
	GL_CHECK_FALSE(tmp_data->userdata);
	GL_CHECK_FALSE(media);
	gl_media_s *item = NULL;
	gl_image_s *image_info = NULL;
	gl_video_s *video_info = NULL;
	image_meta_h image_handle = NULL;
	video_meta_h video_handle = NULL;
	char *ext = NULL;

	int ret = -1;
	Eina_List **elist = (Eina_List **)(tmp_data->userdata);

	item = (gl_media_s *)calloc(1, sizeof(gl_media_s));
	GL_CHECK_FALSE(item);
	item->gtype = GL_TYPE_MEDIA;

	if (media_info_clone(&(item->media_h), media) != MEDIA_CONTENT_ERROR_NONE) {
		gl_dbgE("Clone media handle failed!");
		goto GL_LOCAL_FAILED;
	}

	if (media_info_get_media_id(media, &(item->uuid)) != MEDIA_CONTENT_ERROR_NONE) {
		gl_dbgE("Get media id failed!");
		goto GL_LOCAL_FAILED;
	}

	if (media_info_get_display_name(media, &(item->display_name)) != MEDIA_CONTENT_ERROR_NONE) {
		gl_dbgE("Get media display name failed!");
		goto GL_LOCAL_FAILED;
	}

	if (media_info_get_file_path(media, &(item->file_url)) != MEDIA_CONTENT_ERROR_NONE) {
		gl_dbgE("Get media file path failed!");
		goto GL_LOCAL_FAILED;
	}

	if (media_info_get_media_type(media, (media_content_type_e *) & (item->type)) != MEDIA_CONTENT_ERROR_NONE) {
		gl_dbgE("Get media type failed!");
		goto GL_LOCAL_FAILED;
	}
	if (media_info_get_thumbnail_path(media, &(item->thumb_url)) != MEDIA_CONTENT_ERROR_NONE) {
		gl_dbgE("Get media thumbnail path failed!");
		goto GL_LOCAL_FAILED;
	}
	gl_sdbg("thumb_url: %s", item->thumb_url);

	if (media_info_get_timeline(media, &(item->mtime)) != MEDIA_CONTENT_ERROR_NONE) {
		gl_dbgE("Get media timeline failed!");
		goto GL_LOCAL_FAILED;
	}

//	if (media_info_get_mode(media, (media_content_mode_e *)&(item->mode)) != MEDIA_CONTENT_ERROR_NONE) {
//		gl_dbgE("Get media mode failed!");
//		goto GL_LOCAL_FAILED;
//	}

	media_content_storage_e storage_type = 0;
	if (media_info_get_storage_type(media, &storage_type) != MEDIA_CONTENT_ERROR_NONE) {
		gl_dbgE("Get storage type failed!");
		goto GL_LOCAL_FAILED;
	}
	if (storage_type == MEDIA_CONTENT_STORAGE_INTERNAL) { /* The device's internal storage */
		item->storage_type = GL_STORE_T_PHONE;
	} else if (storage_type == MEDIA_CONTENT_STORAGE_EXTERNAL) { /* The device's external storage */
		item->storage_type = GL_STORE_T_MMC;
	} else {
		gl_dbgE("Undefined mode[%d]!", storage_type);
	}
	/* Without meta */
	if (!tmp_data->with_meta) {
		goto GL_LOCAL_SUCCESS;
	}

	if (item->type == MEDIA_CONTENT_TYPE_IMAGE) {
		ret = media_info_get_image(media, &image_handle);
		if (ret != MEDIA_CONTENT_ERROR_NONE || image_handle == NULL) {
			gl_dbgE("Failed to get image handle[%d]!", ret);
			goto GL_LOCAL_FAILED;
		}

		image_info = (gl_image_s *)calloc(1, sizeof(gl_image_s));
		if (NULL == image_info) {
			gl_dbgE("Failed to calloc!");
			goto GL_LOCAL_FAILED;
		}
		item->image_info = image_info;

		if (image_meta_clone(&(image_info->image_h), image_handle) != MEDIA_CONTENT_ERROR_NONE) {
			gl_dbgE("Clone image handle failed!");
			goto GL_LOCAL_FAILED;
		}

		if (image_meta_get_media_id(image_handle, &(image_info->media_uuid)) != MEDIA_CONTENT_ERROR_NONE) {
			gl_dbgE("Get image id failed!");
			goto GL_LOCAL_FAILED;
		}

		if (image_meta_get_orientation(image_handle, (media_content_orientation_e *) & (image_info->orientation)) != MEDIA_CONTENT_ERROR_NONE) {
			gl_dbgE("Get image orientation failed!");
			goto GL_LOCAL_FAILED;
		}
		if (image_meta_get_burst_id(image_handle, &(image_info->burstshot_id)) != MEDIA_CONTENT_ERROR_NONE) {
			image_info->burstshot_id = NULL;
			gl_dbgE("Get burstshot_id failed!");
		}

		if (image_meta_destroy(image_handle) != MEDIA_CONTENT_ERROR_NONE) {
			gl_dbgE("Destroy image handle failed!");
			goto GL_LOCAL_FAILED;
		}
	} else if (item->type == MEDIA_CONTENT_TYPE_VIDEO) {
		ret = media_info_get_video(media, &video_handle);
		if (ret != MEDIA_CONTENT_ERROR_NONE || video_handle == NULL) {
			gl_dbgE("Failed to get video handle[%d]!", ret);
			goto GL_LOCAL_FAILED;
		}

		video_info = (gl_video_s *)calloc(1, sizeof(gl_video_s));
		if (NULL == video_info) {
			gl_dbgE("Failed to calloc!");
			goto GL_LOCAL_FAILED;
		}
		item->video_info = video_info;

		if (video_meta_clone(&(video_info->video_h), video_handle) != MEDIA_CONTENT_ERROR_NONE) {
			gl_dbgE("Clone video handle failed!");
			goto GL_LOCAL_FAILED;
		}

		if (video_meta_get_media_id(video_handle, &(video_info->media_uuid)) != MEDIA_CONTENT_ERROR_NONE) {
			gl_dbgE("Get video id failed!");
			goto GL_LOCAL_FAILED;
		}

//		if (video_meta_get_title(video_handle, &(video_info->title)) != MEDIA_CONTENT_ERROR_NONE) {
//			gl_dbgE("Get video title failed!");
//			goto GL_LOCAL_FAILED;
//		}

		if (video_meta_get_duration(video_handle, &(video_info->duration)) != MEDIA_CONTENT_ERROR_NONE) {
			gl_dbgE("Get video duration failed!");
			goto GL_LOCAL_FAILED;
		}

		if (video_meta_get_played_time(video_handle, &(video_info->last_played_pos)) != MEDIA_CONTENT_ERROR_NONE) {
			gl_dbgE("Get video last played position failed!");
			goto GL_LOCAL_FAILED;
		}

		if (video_meta_destroy(video_handle) != MEDIA_CONTENT_ERROR_NONE) {
			gl_dbgE("Destroy video handle failed!");
			goto GL_LOCAL_FAILED;
		}

		/* Get bookmark elist in case of video */
		if (video_info->media_uuid) {
			video_info->bookmarks = 0;
			int count = 0;
			ret = media_info_get_bookmark_count_from_db(video_info->media_uuid,
			        NULL,
			        &count);
			if (ret != MEDIA_CONTENT_ERROR_NONE) {
				gl_dbgE("Failed to get bookmark[%d]", ret);
			} else {
				video_info->bookmarks = count;
			}
		}
	} else {
		gl_dbgE("Wrong media type[%d]!", item->type);
	}

GL_LOCAL_SUCCESS:

	/* Get extension */
	ext = strrchr(item->file_url, '.');
	if (ext) {
		item->ext = strdup(ext + 1);
	} else {
		gl_dbgE("Extension is NULL!");
	}

	*elist = eina_list_append(*elist, item);
	return true;

GL_LOCAL_FAILED:

	if (image_handle) {
		image_meta_destroy(image_handle);
	}
	if (video_handle) {
		video_meta_destroy(video_handle);
	}

	_gl_data_type_free_glitem((void **)(&item));
	return false;
}

static bool __gl_local_data_get_fav_media_list_cb(media_info_h media, void *data)
{
	bool ret = false;
	ret = __gl_local_data_get_media_list_cb(media, data);
	return ret;
}

static bool __gl_local_data_get_album_cover_cb(media_info_h media, void *data)
{
	GL_CHECK_FALSE(data);
	GL_CHECK_FALSE(media);
	gl_media_s *item = NULL;
	Eina_List **elist = (Eina_List **)data;

	item = (gl_media_s *)calloc(1, sizeof(gl_media_s));
	GL_CHECK_FALSE(item);
	item->gtype = GL_TYPE_MEDIA;

	if (media_info_clone(&(item->media_h), media) != MEDIA_CONTENT_ERROR_NONE) {
		gl_dbgE("Clone media handle failed!");
		goto GL_LOCAL_FAILED;
	}
	if (media_info_get_file_path(media, &(item->file_url)) != MEDIA_CONTENT_ERROR_NONE) {
		gl_dbgE("Get media file path failed!");
		goto GL_LOCAL_FAILED;
	}
	if (media_info_get_media_type(media, (media_content_type_e *) & (item->type)) != MEDIA_CONTENT_ERROR_NONE) {
		gl_dbgE("Get media type failed!");
		goto GL_LOCAL_FAILED;
	}
	if (media_info_get_thumbnail_path(media, &(item->thumb_url)) != MEDIA_CONTENT_ERROR_NONE) {
		gl_dbgE("Get media thumbnail path failed!");
		goto GL_LOCAL_FAILED;
	}
	gl_sdbg("thumb_url: %s", item->thumb_url);

	*elist = eina_list_append(*elist, item);
	return true;

GL_LOCAL_FAILED:

	_gl_data_type_free_glitem((void **)(&item));
	return false;
}

static bool __gl_local_data_get_fav_album_cover_cb(media_info_h media, void *data)
{
	GL_CHECK_FALSE(data);
	GL_CHECK_FALSE(media);
	gl_media_s *item = NULL;
	Eina_List **elist = (Eina_List **)data;

	item = (gl_media_s *)calloc(1, sizeof(gl_media_s));
	GL_CHECK_FALSE(item);
	item->gtype = GL_TYPE_MEDIA;

	if (media_info_clone(&(item->media_h), media) != MEDIA_CONTENT_ERROR_NONE) {
		gl_dbgE("Clone media handle failed!");
		goto GL_LOCAL_FAILED;
	}
	if (media_info_get_file_path(media, &(item->file_url)) != MEDIA_CONTENT_ERROR_NONE) {
		gl_dbgE("Get media file path failed!");
		goto GL_LOCAL_FAILED;
	}
	if (media_info_get_media_type(media, (media_content_type_e *) & (item->type)) != MEDIA_CONTENT_ERROR_NONE) {
		gl_dbgE("Get media type failed!");
		goto GL_LOCAL_FAILED;
	}
	if (media_info_get_thumbnail_path(media, &(item->thumb_url)) != MEDIA_CONTENT_ERROR_NONE) {
		gl_dbgE("Get media thumbnail path failed!");
		goto GL_LOCAL_FAILED;
	}
	gl_sdbg("thumb_url: %s", item->thumb_url);

	*elist = eina_list_append(*elist, item);
	return true;

GL_LOCAL_FAILED:

	_gl_data_type_free_glitem((void **)(&item));
	return false;
}

static bool __gl_local_data_delete_album_cb(media_info_h media, void *data)
{
	GL_CHECK_FALSE(data);
	GL_CHECK_FALSE(media);
	Eina_List **elist = (Eina_List **)data;
	gl_media_s *item = NULL;

	item = (gl_media_s *)calloc(1, sizeof(gl_media_s));
	GL_CHECK_FALSE(item);
	item->gtype = GL_TYPE_MEDIA;

	if (MEDIA_CONTENT_ERROR_NONE != media_info_get_file_path(media, &(item->file_url))) {
		gl_dbgE("media_info_get_file_path failed");
	}
	media_info_get_media_id(media, &(item->uuid));
	*elist = eina_list_append(*elist, item);
	return true;
}

/* Connect to media-content database */
int _gl_local_data_connect(void)
{
	int ret = -1;
	ret = media_content_connect();
	if (ret == MEDIA_CONTENT_ERROR_NONE) {
		gl_dbg("DB connection is success");
		return 0;
	} else {
		gl_dbgE("DB connection is failed[%d]!", ret);
		return -1;
	}
}

/* Disconnect from media-content database */
int _gl_local_data_disconnect(void)
{
	int ret = -1;
	ret = media_content_disconnect();
	if (ret == MEDIA_CONTENT_ERROR_NONE) {
		gl_dbg("DB disconnection is success");
		return 0;
	} else {
		gl_dbgE("DB disconnection is failed[%d]!", ret);
		return -1;
	}
}

int _gl_local_data_get_album_by_path(char *path, gl_album_s **album)
{
	GL_CHECK_VAL(path, -1);
	GL_CHECK_VAL(album, -1);
	int ret = -1;
	Eina_List *list = NULL;
	gl_filter_s condition;
	gl_album_s *_item = NULL;
	int i = 0;

	if (strlen(path) <= 0) {
		gl_dbgE("Invalid path!");
		return -1;
	}
	gl_sdbg("path: %s", path);

	memset(&condition, 0x00, sizeof(gl_filter_s));
	_gl_data_util_get_cond(condition.cond);
	g_strlcpy(condition.sort_keyword, GL_CONDITION_ORDER, KEYWORD_LENGTH);
	condition.collate_type = MEDIA_CONTENT_COLLATE_NOCASE;
	condition.sort_type = MEDIA_CONTENT_ORDER_DESC;
	condition.offset = -1;
	condition.count = -1;
	condition.with_meta = false;

	snprintf(condition.cond, CONDITION_LENGTH,
	         "(%s=0 OR %s=1) AND %s=\'%s\'", MEDIA_TYPE, MEDIA_TYPE,
	         FOLDER_PATH, path);

	ret = _gl_local_data_get_album_list(&condition, &list);
	if (ret != 0 || NULL == list) {
		gl_dbgE("Failed to get album list[%d]!", ret);
		ret = -1;
	} else if (NULL != list) {
		*album = eina_list_nth(list, 0);
		i = 1;
		ret = 0;
	}

	/* Free other items */
	if (list) {
		int len = eina_list_count(list);
		gl_dbg("len: %d", len);

		for (; i < len; i++) {
			_item = eina_list_nth(list, i);
			_gl_data_type_free_glitem((void **)(&_item));
		}

		eina_list_free(list);
	}

	return ret;
}

int _gl_local_data_get_album_list(gl_filter_s *condition, Eina_List **elist)
{
	GL_CHECK_VAL(elist, -1);
	GL_CHECK_VAL(condition, -1);
	int ret = -1;
	filter_h filter = NULL;

	ret = __gl_local_data_create_filter(condition, &filter);
	if (ret != 0) {
		gl_dbgE("Create filter failed[%d]!", ret);
		return -1;
	}

	filter_h media_filter = NULL;
	gl_filter_s media_condition;
	memset(&media_condition, 0x00, sizeof(gl_filter_s));
	/* Get all contents(including local and cloud) for albums list */
	g_strlcpy(media_condition.cond, GL_CONDITION_IMAGE_VIDEO,
	          CONDITION_LENGTH);
	media_condition.sort_type = MEDIA_CONTENT_ORDER_DESC;
	g_strlcpy(media_condition.sort_keyword, GL_CONDITION_ORDER,
	          KEYWORD_LENGTH);
	media_condition.collate_type = MEDIA_CONTENT_COLLATE_NOCASE;
	media_condition.offset = -1;
	media_condition.count = -1;
	media_condition.with_meta = false;

	ret = __gl_local_data_create_filter(&media_condition, &media_filter);
	if (ret != 0) {
		__gl_local_data_destroy_filter(filter);
		gl_dbgE("Create filter failed[%d]!", ret);
		return -1;
	}

	gl_transfer_data_s tran_data;
	memset(&tran_data, 0x00, sizeof(gl_transfer_data_s));
	tran_data.userdata = (void **)elist;
	tran_data.filter = media_filter;
	tran_data.album_id = NULL;
	tran_data.with_meta = false;

	gl_dbg("Get folders--start");
	ret = media_folder_foreach_folder_from_db(filter,
	        __gl_local_data_get_album_list_cb,
	        &tran_data);
	gl_dbg("Get folders---over");

	__gl_local_data_destroy_filter(media_filter);
	__gl_local_data_destroy_filter(filter);

	if (ret != MEDIA_CONTENT_ERROR_NONE) {
		gl_dbgE("Failed to get all folders[%d]!", ret);
		return -1;
	}

	return 0;
}

int _gl_local_data_get_media_by_id(char *media_id, gl_media_s **mitem)
{
	GL_CHECK_VAL(mitem, -1);

	if (media_id == NULL) {
		//gl_dbg("Create a empty media");
		_gl_data_type_new_media(mitem);
		return 0;
	}

	int ret = -1;
	Eina_List *list = NULL;
	gl_media_s *_mitem = NULL;
	media_info_h media_h = NULL;
	int i = 0;
	gl_sdbg("media id: %s", media_id);

	ret = media_info_get_media_from_db(media_id, &media_h);
	if (ret != MEDIA_CONTENT_ERROR_NONE || media_h == NULL) {
		gl_dbgE("Failed to get media handle[%d]!", ret);
		if (media_h) {
			media_info_destroy(media_h);
		}
		return -1;
	}

	gl_transfer_data_s tran_data;
	memset(&tran_data, 0x00, sizeof(gl_transfer_data_s));
	tran_data.userdata = (void **)&list;
	tran_data.filter = NULL;
	tran_data.album_id = NULL;
	tran_data.with_meta = false;

	bool b_ret = __gl_local_data_get_media_list_cb(media_h, &tran_data);

	media_info_destroy(media_h);

	if (b_ret && list) {
		*mitem = eina_list_nth(list, 0);
		i = 1;
		ret = 0;
	} else {
		gl_dbgE("Failed to get media list!");
		ret = -1;
	}

	/* Free other items */
	if (list) {
		int len = eina_list_count(list);
		gl_dbg("len: %d", len);

		for (; i < len; i++) {
			_mitem = eina_list_nth(list, i);
			_gl_data_type_free_glitem((void **)(&_mitem));
		}

		eina_list_free(list);
	}

	return ret;
}

int _gl_local_data_get_media_by_path(const char *path, gl_media_s **mitem)
{
	GL_CHECK_VAL(mitem, -1);
	GL_CHECK_VAL(path, -1);
	int ret = -1;
	Eina_List *list = NULL;
	gl_filter_s condition;
	gl_media_s *_mitem = NULL;
	int i = 0;

	if (strlen(path) <= 0) {
		gl_dbgE("Invalid path!");
		return -1;
	}
	gl_sdbg("path: %s", path);

	memset(&condition, 0x00, sizeof(gl_filter_s));
	_gl_data_util_get_cond(condition.cond);
	g_strlcpy(condition.sort_keyword, GL_CONDITION_ORDER, KEYWORD_LENGTH);
	condition.collate_type = MEDIA_CONTENT_COLLATE_NOCASE;
	condition.sort_type = MEDIA_CONTENT_ORDER_DESC;
	condition.offset = -1;
	condition.count = -1;
	condition.with_meta = true;

	snprintf(condition.cond, CONDITION_LENGTH,
	         "(%s=0 OR %s=1) AND %s=\'%s\'", MEDIA_TYPE, MEDIA_TYPE,
	         MEDIA_PATH, path);
	ret = _gl_local_data_get_all_albums_media_list(&condition, &list);
	if (ret != 0 || NULL == list) {
		gl_dbgE("Failed to get all albums[%d]!", ret);
		ret = -1;
	} else if (NULL != list) {
		*mitem = eina_list_nth(list, 0);
		i = 1;
		ret = 0;
	}

	/* Free other items */
	if (list) {
		int len = eina_list_count(list);
		gl_dbg("len: %d", len);

		for (; i < len; i++) {
			_mitem = eina_list_nth(list, i);
			_gl_data_type_free_glitem((void **)(&_mitem));
		}

		eina_list_free(list);
	}

	return ret;
}

int _gl_local_data_get_media_count(const char *cluster_id, gl_filter_s *condition,
                                   int *item_cnt)
{
	GL_CHECK_VAL(cluster_id, -1);
	GL_CHECK_VAL(condition, -1);
	GL_CHECK_VAL(item_cnt, -1);
	int ret = -1;
	filter_h filter = NULL;

	ret = __gl_local_data_create_filter(condition, &filter);
	if (ret != 0) {
		gl_dbgE("Create filter failed[%d]!", ret);
		return -1;
	}

	gl_dbg("Get media count--start");
	ret = media_folder_get_media_count_from_db(cluster_id, filter, item_cnt);
	gl_dbg("Get media count---over");

	__gl_local_data_destroy_filter(filter);

	if (ret != MEDIA_CONTENT_ERROR_NONE) {
		gl_dbgE("Failed to get media count[%d]!", ret);
		return -1;
	}

	return 0;
}

int _gl_local_data_get_all_media_count(gl_filter_s *condtion, int *item_cnt)
{
	GL_CHECK_VAL(condtion, -1);
	GL_CHECK_VAL(item_cnt, -1);
	int ret = -1;
	filter_h filter = NULL;

	ret = __gl_local_data_create_filter(condtion, &filter);
	if (ret != 0) {
		gl_dbgE("Create filter failed[%d]!", ret);
		return -1;
	}

	gl_dbg("Get media count--start");
	GL_PROFILE_F_IN("media_info_get_media_count_from_db");
	ret = media_info_get_media_count_from_db(filter, item_cnt);
	GL_PROFILE_F_OUT("media_info_get_media_count_from_db");
	gl_dbg("Get media count---over");

	__gl_local_data_destroy_filter(filter);

	if (ret != MEDIA_CONTENT_ERROR_NONE) {
		gl_dbgE("Failed to get media count[%d]", ret);
		return -1;
	}

	return 0;
}

int _gl_local_data_get_album_media_list(gl_filter_s *condition,
                                        const char *album_id, Eina_List **elist)
{
	GL_CHECK_VAL(elist, -1);
	GL_CHECK_VAL(album_id, -1);
	GL_CHECK_VAL(condition, -1);
	gl_sdbg("album id: %s", album_id);

	int ret = -1;
	filter_h filter = NULL;
	ret = __gl_local_data_create_filter(condition, &filter);
	if (ret != 0) {
		gl_dbgE("Create filter failed!");
		return -1;
	}

	gl_transfer_data_s tran_data;
	memset(&tran_data, 0x00, sizeof(gl_transfer_data_s));
	tran_data.userdata = (void **)elist;
	tran_data.filter = NULL;
	tran_data.album_id = (char *)album_id;
	tran_data.with_meta = condition->with_meta;

	gl_dbg("Get medias--start");
	ret = media_folder_foreach_media_from_db(album_id, filter,
	        __gl_local_data_get_media_list_cb,
	        &tran_data);
	gl_dbg("Get medias--start");

	__gl_local_data_destroy_filter(filter);

	if (ret != MEDIA_CONTENT_ERROR_NONE) {
		gl_dbgE("Failed to get medias[%d]!", ret);
		return -1;
	}

	return 0;
}

int _gl_local_data_get_album_cover(gl_filter_s *condition, const char *album_id,
                                   Eina_List **elist)
{
	GL_CHECK_VAL(elist, -1);
	GL_CHECK_VAL(album_id, -1);
	GL_CHECK_VAL(condition, -1);
	gl_sdbg("album id: %s", album_id);

	int ret = -1;
	filter_h filter = NULL;
	ret = __gl_local_data_create_filter(condition, &filter);
	if (ret != 0) {
		gl_dbgE("Create filter failed!");
		return -1;
	}

	gl_dbg("Get medias--start");
	ret = media_folder_foreach_media_from_db(album_id, filter,
	        __gl_local_data_get_album_cover_cb,
	        elist);
	gl_dbg("Get medias--start");

	__gl_local_data_destroy_filter(filter);

	if (ret != MEDIA_CONTENT_ERROR_NONE) {
		gl_dbgE("Failed to get medias[%d]!", ret);
		return -1;
	}

	return 0;
}

int _gl_local_data_get_all_albums_media_list(gl_filter_s *condition,
        Eina_List **elist)
{
	GL_CHECK_VAL(elist, -1);
	GL_CHECK_VAL(condition, -1);
	int ret = -1;
	filter_h filter = NULL;

	ret = __gl_local_data_create_filter(condition, &filter);
	if (ret != 0) {
		gl_dbgE("Create filter failed!");
		return -1;
	}

	gl_transfer_data_s tran_data;
	memset(&tran_data, 0x00, sizeof(gl_transfer_data_s));
	tran_data.userdata = (void **)elist;
	tran_data.filter = NULL;
	tran_data.album_id = NULL;
	tran_data.with_meta = condition->with_meta;

	gl_dbg("Get all medias--start");
	ret = media_info_foreach_media_from_db(filter,
	                                       __gl_local_data_get_media_list_cb,
	                                       &tran_data);
	gl_dbg("Get all medias--over");

	__gl_local_data_destroy_filter(filter);

	if (ret != MEDIA_CONTENT_ERROR_NONE) {
		gl_dbgE("Failed to get medias[%d]!", ret);
		return -1;
	}

	return 0;
}

int _gl_local_data_get_fav_albums_media_list(gl_filter_s *condition,
        Eina_List **elist)
{
	GL_CHECK_VAL(elist, -1);
	GL_CHECK_VAL(condition, -1);
	int ret = -1;
	filter_h filter = NULL;

	ret = __gl_local_data_create_filter(condition, &filter);
	if (ret != 0) {
		gl_dbgE("Create filter failed!");
		return -1;
	}

	gl_transfer_data_s tran_data;
	memset(&tran_data, 0x00, sizeof(gl_transfer_data_s));
	tran_data.userdata = (void **)elist;
	tran_data.filter = NULL;
	tran_data.album_id = NULL;
	tran_data.with_meta = condition->with_meta;

	gl_dbg("Get all medias--start");
	ret = media_info_foreach_media_from_db(filter,
	                                       __gl_local_data_get_fav_media_list_cb, &tran_data);
	gl_dbg("Get all medias--over");

	__gl_local_data_destroy_filter(filter);

	if (ret != MEDIA_CONTENT_ERROR_NONE) {
		gl_dbgE("Failed to get medias[%d]!", ret);
		return -1;
	}

	return 0;
}

int _gl_local_data_get_all_albums_cover(gl_filter_s *condition,
                                        Eina_List **elist)
{
	GL_CHECK_VAL(elist, -1);
	GL_CHECK_VAL(condition, -1);
	int ret = -1;
	filter_h filter = NULL;

	ret = __gl_local_data_create_filter(condition, &filter);
	if (ret != 0) {
		gl_dbgE("Create filter failed!");
		return -1;
	}

	gl_dbg("Get all medias--start");
	ret = media_info_foreach_media_from_db(filter,
	                                       __gl_local_data_get_album_cover_cb,
	                                       elist);
	gl_dbg("Get all medias--over");

	__gl_local_data_destroy_filter(filter);

	if (ret != MEDIA_CONTENT_ERROR_NONE) {
		gl_dbgE("Failed to get medias[%d]!", ret);
		return -1;
	}

	return 0;
}

int _gl_local_data_get_fav_albums_cover(gl_filter_s *condition,
                                        Eina_List **elist)
{
	GL_CHECK_VAL(elist, -1);
	GL_CHECK_VAL(condition, -1);
	int ret = -1;
	filter_h filter = NULL;

	ret = __gl_local_data_create_filter(condition, &filter);
	if (ret != 0) {
		gl_dbgE("Create filter failed!");
		return -1;
	}

	gl_dbg("Get all medias--start");
	ret = media_info_foreach_media_from_db(filter,
	                                       __gl_local_data_get_fav_album_cover_cb, elist);
	gl_dbg("Get all medias--over");

	__gl_local_data_destroy_filter(filter);

	if (ret != MEDIA_CONTENT_ERROR_NONE) {
		gl_dbgE("Failed to get medias[%d]!", ret);
		return -1;
	}

	return 0;
}

int _gl_local_data_delete_album(gl_album_s *cluster, void *cb, void *data, bool is_hide)
{
	GL_CHECK_VAL(cluster, -1);
	GL_CHECK_VAL(cluster->uuid, -1);
	gl_filter_s condition;
	filter_h filter = NULL;
	int ret = -1;
	gl_sdbg("album id: %s", cluster->uuid);
	Eina_List *itemlist = NULL;
	gl_media_s *item = NULL;

	if (!is_hide) {
		memset(&condition, 0x00, sizeof(gl_filter_s));
		_gl_data_util_get_cond(condition.cond);
		condition.collate_type = MEDIA_CONTENT_COLLATE_NOCASE;
		condition.sort_type = MEDIA_CONTENT_ORDER_ASC;
		g_strlcpy(condition.sort_keyword, MEDIA_ADDED_TIME, KEYWORD_LENGTH);
		condition.offset = -1;
		condition.count = -1;
		condition.with_meta = false;

		ret = __gl_local_data_create_filter(&condition, &filter);
		if (ret != 0) {
			gl_dbgE("Create filter failed!");
			return -1;
		}
		ret = media_folder_foreach_media_from_db(cluster->uuid, filter,
		        __gl_local_data_delete_album_cb,
		        &itemlist);
		__gl_local_data_destroy_filter(filter);
	}

	if (ret != MEDIA_CONTENT_ERROR_NONE) {
		gl_dbgE("Failed to get medias[%d]!", ret);
		return -1;
	}

	EINA_LIST_FREE(itemlist, item) {
		if (item == NULL || item->file_url == NULL) {
			gl_dbgE("Invalid item!");
			continue;
		}

		if (!gl_file_unlink(item->file_url)) {
			gl_dbgE("file_unlink failed!");
		}
		if (!is_hide) {
			media_info_delete_from_db(item->uuid);
			if (cb) {
				int (*delete_cb)(void * data, char * uuid);
				delete_cb = cb;
				delete_cb(data, item->uuid);
			}
		}
		_gl_data_type_free_glitem((void **)&item);
		int pbar_cancel = false;
		gl_thread_get_cancel_state(data, &pbar_cancel);
		if (pbar_cancel > GL_PB_CANCEL_NORMAL) {
			gl_dbgW("Cancelled[%d]!", pbar_cancel);
			return 0;
		}
	}

	return 0;
}

int _gl_local_data_hide_album_media(void *data, gl_album_s *cluster, char *new_path)
{
	GL_CHECK_VAL(cluster, -1);
	GL_CHECK_VAL(cluster->uuid, -1);
	gl_filter_s condition;
	filter_h filter = NULL;
	int ret = -1;
	gl_dbgE("album id: %s", cluster->uuid);
	Eina_List *itemlist = NULL;
	gl_media_s *item = NULL;

	memset(&condition, 0x00, sizeof(gl_filter_s));
	_gl_data_util_get_cond(condition.cond);
	condition.collate_type = MEDIA_CONTENT_COLLATE_NOCASE;
	condition.sort_type = MEDIA_CONTENT_ORDER_ASC;
	g_strlcpy(condition.sort_keyword, MEDIA_ADDED_TIME, KEYWORD_LENGTH);
	condition.offset = -1;
	condition.count = -1;
	condition.with_meta = false;

	ret = __gl_local_data_create_filter(&condition, &filter);
	if (ret != 0) {
		gl_dbgE("Create filter failed!");
		return -1;
	}
	ret = media_folder_foreach_media_from_db(cluster->uuid, filter,
	        __gl_local_data_delete_album_cb,
	        &itemlist);
	__gl_local_data_destroy_filter(filter);

	if (ret != MEDIA_CONTENT_ERROR_NONE) {
		gl_dbgE("Failed to get medias[%d]!", ret);
		return -1;
	}

	char new_file[GL_FILE_PATH_LEN_MAX] = {0};
	char *name = NULL;
	EINA_LIST_FREE(itemlist, item) {
		if (item == NULL || item->file_url == NULL) {
			gl_dbgE("Invalid item!");
			continue;
		}

		name = strrchr(item->file_url, '/');
		if (name) {
			snprintf(new_file, GL_FILE_PATH_LEN_MAX, "%s/%s",
			         new_path, name + 1);
			gl_dbgE("new file: %s", new_file);
			if (!_gl_fs_move(data, item->file_url, new_file)) {
				gl_dbgE("rename failed!");
			}
		}
		_gl_data_type_free_glitem((void **)&item);
	}
	return 0;
}

int _gl_local_data_add_media(const char *file_url, media_info_h *info)
{
	GL_CHECK_VAL(file_url, -1);
	int ret = -1;
	media_info_h item = NULL;
	gl_sdbg("file_url is %s", file_url);

	ret = media_info_insert_to_db(file_url, &item);
	if (ret != MEDIA_CONTENT_ERROR_NONE) {
		gl_dbgE("Failed to insert media to DB[%d]!", ret);
		if (item) {
			media_info_destroy(item);
		}
		return -1;
	}

	if (info) {
		*info = item;
	} else {
		gl_dbgW("Destroy media_info item!");
		media_info_destroy(item);
	}

	return 0;
}

int _gl_local_data_get_thumb(gl_media_s *mitem, char **thumb)
{
	GL_CHECK_VAL(mitem, -1);
	GL_CHECK_VAL(mitem->media_h, -1);

	if (media_info_get_thumbnail_path(mitem->media_h, thumb) != MEDIA_CONTENT_ERROR_NONE) {
		gl_dbgE("Get media thumbnail path failed!");
		return -1;
	}

	return 0;
}

int _gl_local_data_move_media(gl_media_s *mitem, const char *dst)
{
	GL_CHECK_VAL(dst, -1);
	GL_CHECK_VAL(mitem, -1);
	GL_CHECK_VAL(mitem->media_h, -1);

	if (media_info_move_to_db(mitem->media_h, dst) != MEDIA_CONTENT_ERROR_NONE) {
		gl_dbgE("Move media thumbnail failed!");
		return -1;
	}

	return 0;
}

bool _gl_local_data_is_album_exists(char *path)
{
	GL_CHECK_FALSE(path);
	gl_album_s *album = NULL;
	gl_sdbgW("path: %s", path);

	_gl_local_data_get_album_by_path(path, &album);
	if (album) {
		_gl_data_type_free_glitem((void **)(&album));
		return true;
	}
	return false;
}

int _gl_local_data_get_path_by_id(const char *uuid, char **path)
{
	GL_CHECK_VAL(path, 0);
	GL_CHECK_VAL(uuid, 0);
	media_info_h media_h = NULL;
	int ret = -1;
	char *_path = NULL;

	ret = media_info_get_media_from_db(uuid, &media_h);
	if (ret != MEDIA_CONTENT_ERROR_NONE) {
		gl_dbgE("Get media failed[%d]!", ret);
		goto GL_LD_FAILED;
	}
	ret = media_info_get_file_path(media_h, &_path);
	if (ret != MEDIA_CONTENT_ERROR_NONE) {
		gl_dbgE("Get media file path failed!");
		goto GL_LD_FAILED;
	}

	media_info_destroy(media_h);
	media_h = NULL;

	if (!_path) {
		gl_dbgE("Invalid file path!");
		goto GL_LD_FAILED;
	}

	*path = _path;

GL_LD_FAILED:

	if (media_h) {
		media_info_destroy(media_h);
		media_h = NULL;
	}
	return 0;
}

