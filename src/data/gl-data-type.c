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

#include "gl-data-type.h"
#include "gl-debug.h"

int _gl_data_type_new_media(gl_media_s **item)
{
	GL_CHECK_VAL(item, -1);
	gl_media_s *tmp_item = (gl_media_s *)calloc(1, sizeof(gl_media_s));
	GL_CHECK_VAL(tmp_item, -1);
	tmp_item->gtype = GL_TYPE_MEDIA;
	*item =  tmp_item;
	return 0;
}

int _gl_data_type_new_album(gl_album_s **album)
{
	GL_CHECK_VAL(album, -1);
	gl_album_s *tmp_item = (gl_album_s *)calloc(1, sizeof(gl_album_s));
	GL_CHECK_VAL(tmp_item, -1);
	tmp_item->gtype = GL_TYPE_ALBUM;
	*album =  tmp_item;
	return 0;
}


static int __gl_data_type_free_media(gl_media_s **item)
{
	GL_CHECK_VAL(item, -1);
	GL_CHECK_VAL(*item, -1);
	gl_media_s *tmp_item = *item;

	/* For local medias */
	if (tmp_item->media_h) {
		if (tmp_item->b_create_thumb) {
			media_info_cancel_thumbnail(tmp_item->media_h);
			tmp_item->b_create_thumb = false;
		}
		media_info_destroy(tmp_item->media_h);
		tmp_item->media_h = NULL;
	}

	GL_FREEIF(tmp_item->uuid);
	GL_FREEIF(tmp_item->thumb_url);
	GL_FREEIF(tmp_item->file_url);
	GL_FREEIF(tmp_item->display_name);
	GL_FREEIF(tmp_item->ext);

	if (MEDIA_CONTENT_TYPE_IMAGE == tmp_item->type &&
	        tmp_item->image_info) {
		/* For local medias */
		if (tmp_item->image_info->image_h) {
			image_meta_destroy(tmp_item->image_info->image_h);
			tmp_item->image_info->image_h = NULL;
		}
		GL_FREEIF(tmp_item->image_info->burstshot_id);
		GL_FREEIF(tmp_item->image_info->media_uuid);
		GL_FREE(tmp_item->image_info);
	} else if (MEDIA_CONTENT_TYPE_VIDEO == tmp_item->type &&
	           tmp_item->video_info) {
		/* For local medias */
		if (tmp_item->video_info->video_h) {
			video_meta_destroy(tmp_item->video_info->video_h);
			tmp_item->video_info->video_h = NULL;
		}

		GL_FREEIF(tmp_item->video_info->media_uuid);
		GL_FREEIF(tmp_item->video_info->title);
		GL_FREE(tmp_item->video_info);
	}

	GL_FREE(tmp_item);
	*item = NULL;
	return 0;
}

int _gl_data_type_free_media_list(Eina_List **list)
{
	GL_CHECK_VAL(list, -1);
	GL_CHECK_VAL(*list, -1);
	gl_media_s *item = NULL;
	Eina_List *tmp_list = *list;
	EINA_LIST_FREE(tmp_list, item) {
		if (item) {
			__gl_data_type_free_media(&item);
		}
	}
	*list = NULL;
	return 0;
}

static int __gl_data_type_free_album(gl_album_s **album)
{
	GL_CHECK_VAL(album, -1);
	GL_CHECK_VAL(*album, -1);
	gl_album_s *tmp_album = *album;

	GL_FREEIF(tmp_album->uuid);
	GL_FREEIF(tmp_album->display_name);
	GL_FREEIF(tmp_album->path);

	if (tmp_album->folder_h) {
		media_folder_destroy(tmp_album->folder_h);
	}
	GL_FREE(tmp_album);
	*album = NULL;
	return 0;
}

int _gl_data_type_free_album_list(Eina_List **list)
{
	GL_CHECK_VAL(list, -1);
	GL_CHECK_VAL(*list, -1);
	gl_album_s *item = NULL;
	Eina_List *tmp_list = *list;
	EINA_LIST_FREE(tmp_list, item) {
		if (item) {
			__gl_data_type_free_album(&item);
		}
	}
	*list = NULL;
	return 0;
}

int _gl_data_type_free_glitem(void **item)
{
	GL_CHECK_VAL(item, -1);
	GL_CHECK_VAL(*item, -1);
	int ret = -1;

	if (((gl_album_s *)*item)->gtype == GL_TYPE_ALBUM) {
		ret = __gl_data_type_free_album((gl_album_s **)item);
	} else if (((gl_media_s *)*item)->gtype == GL_TYPE_MEDIA ||
	           ((gl_media_s *)*item)->gtype == GL_TYPE_WEB_MEDIA) {
		ret = __gl_data_type_free_media((gl_media_s **)item);
	}

	if (ret < 0) {
		return -1;
	} else {
		return 0;
	}
}

