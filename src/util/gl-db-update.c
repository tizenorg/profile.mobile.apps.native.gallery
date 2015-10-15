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
#include "gallery.h"
#include "gl-util.h"
#include "gl-debug.h"
#include "gl-db-update.h"
#include "gl-data.h"

#define GL_MONITOE_TIME_DELAY 1.0f

typedef enum
{
	GL_DU_LOCK_NONE,
	GL_DU_LOCK_ONCE,
	GL_DU_LOCK_ALWAYS,
} gl_du_lock_e;

struct gl_db_noti_t {
	void *ad;
	Ecore_Timer *db_timer; /*For update db data*/
	/*Make ture calling db update callback after other operations complete*/
	Ecore_Idler *db_idl;
	//media_content_noti_h cloud_h; /* Notify handle fro cloud content updating in DB */
	gl_du_lock_e lock_state; /*For update db data. If delete/rename operation of gallery, doesn't update*/
	int count; /* All media count got from DB */

	media_content_db_update_item_type_e update_item;
	media_content_db_update_type_e update_type;
	GList *uuids;
};

static Eina_Bool __gl_db_update_idler(void *data)
{
	GL_CHECK_FALSE(data);
	gl_appdata *ad = (gl_appdata *)data;
	GL_CHECK_FALSE(ad->db_noti_d);
	gl_db_noti_s *db_noti = ad->db_noti_d;
	gl_update_view(ad, GL_UPDATE_VIEW_INOTIFY);
	evas_object_smart_callback_call(ad->maininfo.naviframe,
					"gallery,db,data,updated", data);
	GL_IF_DEL_IDLER(db_noti->db_idl);
	return ECORE_CALLBACK_CANCEL;
}

static Eina_Bool __gl_db_update_timer_cb(void *data)
{
	GL_CHECK_FALSE(data);
	gl_appdata *ad = (gl_appdata *)data;
	GL_CHECK_FALSE(ad->db_noti_d);
	gl_db_noti_s *db_noti = ad->db_noti_d;

	gl_dbg("db_update[%d]", db_noti->lock_state);
	if (db_noti->lock_state) {
		gl_dbgW("Don't update, locked!");
		if (db_noti->lock_state == GL_DU_LOCK_ONCE)
			db_noti->lock_state = GL_DU_LOCK_NONE;
	} else {
		GL_IF_DEL_IDLER(db_noti->db_idl);
		db_noti->db_idl = ecore_idler_add(__gl_db_update_idler, data);
	}

	GL_IF_DEL_TIMER(db_noti->db_timer);
	return ECORE_CALLBACK_CANCEL;
}

static int __gl_db_update_op(media_content_error_e error, int pid,
			     media_content_db_update_item_type_e update_item,
			     media_content_db_update_type_e update_type,
			     media_content_type_e media_type, char *uuid,
			     char *path, char *mime_type, void *data)
{
	GL_CHECK_VAL(data, -1);
	gl_appdata *ad = (gl_appdata *)data;
	GL_CHECK_VAL(ad->db_noti_d, -1);
	gl_db_noti_s *db_noti = ad->db_noti_d;

	if (MEDIA_CONTENT_ERROR_NONE != error) {
		gl_dbgE("Update db error[%d]!", error);
		return -1;
	}
	if (update_item == MEDIA_ITEM_FILE &&
	    MEDIA_CONTENT_TYPE_IMAGE != media_type &&
	    MEDIA_CONTENT_TYPE_VIDEO != media_type) {
		gl_dbg("Media type is wrong");
		return -1;
	} else if (update_item == MEDIA_ITEM_DIRECTORY) {
		/* Batch operation, DB wouldn't return media type  */
		int cnt = 0;
		int ret = -1;
		ret = _gl_data_get_item_cnt(GL_ALBUM_ALL_ID, GL_STORE_T_ALL, &cnt);
		if (ret != 0 || cnt == 0) {
			gl_dbgE("Empty!");
		}
		gl_dbg("old: %d, new: %d", db_noti->count, cnt);
		if (cnt == db_noti->count) {
			gl_dbg("Nothing changed");
			if (path) {
				gl_album_s *album = NULL;
				_gl_local_data_get_album_by_path(path, &album);
				if (!album) {
					return -1;
				} else {
					_gl_data_type_free_glitem((void **)&album);
					gl_dbgW("Updated album contains images");
				}
			} else {
				return -1;
			}
		}
	}

	db_noti->update_item = update_item;
	db_noti->update_type = update_type;
	if (uuid && update_item == MEDIA_ITEM_FILE &&
	    update_type == MEDIA_CONTENT_DELETE) {
		gl_dbgW("Append: %s", uuid);
		db_noti->uuids = g_list_append(db_noti->uuids, (gpointer)g_strdup(uuid));
	}

	_gl_db_update_add_timer(data);
	return 0;
}

static void __gl_db_update_cb(media_content_error_e error, int pid,
			      media_content_db_update_item_type_e update_item,
			      media_content_db_update_type_e update_type,
			      media_content_type_e media_type, char *uuid,
			      char *path, char *mime_type, void *data)
{
	gl_dbg("update_item[%d], update_type[%d], media_type[%d]", update_item,
	       update_type, media_type);
	GL_CHECK(data);
	gl_dbg("uuid[%s], path[%s]", uuid, path);
	__gl_db_update_op(error, pid, update_item, update_type, media_type,
			  uuid, path, mime_type, data);
}

/*
static void __gl_db_update_coud_cb(media_content_error_e error, int pid,
				   media_content_db_update_item_type_e update_item,
				   media_content_db_update_type_e update_type,
				   media_content_type_e media_type, char *uuid,
				   char *path, char *mime_type, void *data)
{
	gl_dbg("update_item[%d], update_type[%d], media_type[%d]", update_item,
	       update_type, media_type);
	GL_CHECK(data);
	__gl_db_update_op(error, pid, update_item, update_type, media_type,
			  uuid, path, mime_type, data);
}
*/

int _gl_db_update_add_timer(void *data)
{
	GL_CHECK_VAL(data, -1);
	gl_appdata *ad = (gl_appdata *)data;
	GL_CHECK_VAL(ad->db_noti_d, -1);
	gl_db_noti_s *db_noti = ad->db_noti_d;

	GL_IF_DEL_TIMER(db_noti->db_timer);
	db_noti->db_timer = ecore_timer_add(GL_MONITOE_TIME_DELAY,
					    __gl_db_update_timer_cb, data);
	gl_dbgW("TIMER[1.0f] added!");
	return 0;
}

bool _gl_db_update_lock_once(void *data, bool status)
{
	gl_dbg("");
	GL_CHECK_FALSE(data);
	gl_appdata *ad = (gl_appdata *)data;
	GL_CHECK_FALSE(ad->db_noti_d);
	gl_db_noti_s *db_noti = ad->db_noti_d;
	if (status)
		db_noti->lock_state = GL_DU_LOCK_ONCE;
	else
		db_noti->lock_state = GL_DU_LOCK_NONE;
	return true;
}

bool _gl_db_update_lock_always(void *data, bool status)
{
	gl_dbg("");
	GL_CHECK_FALSE(data);
	gl_appdata *ad = (gl_appdata *)data;
	GL_CHECK_FALSE(ad->db_noti_d);
	gl_db_noti_s *db_noti = ad->db_noti_d;
	if (status)
		db_noti->lock_state = GL_DU_LOCK_ALWAYS;
	else
		db_noti->lock_state = GL_DU_LOCK_NONE;
	return true;
}

bool _gl_db_update_set_count(void *data, int count)
{
	gl_dbg("count: %d", count);
	GL_CHECK_FALSE(data);
	gl_appdata *ad = (gl_appdata *)data;
	GL_CHECK_FALSE(ad->db_noti_d);
	gl_db_noti_s *db_noti = ad->db_noti_d;
	db_noti->count = count;
	return true;
}

int _gl_db_update_get_info(void *data,
			   media_content_db_update_item_type_e *update_item,
			   media_content_db_update_type_e *update_type,
			   GList **uuids)
{
	GL_CHECK_VAL(data, -1);
	gl_appdata *ad = (gl_appdata *)data;
	GL_CHECK_VAL(ad->db_noti_d, -1);

	if (update_item)
		*update_item = ad->db_noti_d->update_item;
	if (update_type)
		*update_type = ad->db_noti_d->update_type;
	if (ad->db_noti_d->uuids && uuids) {
		*uuids = ad->db_noti_d->uuids;
		ad->db_noti_d->uuids = NULL;
	}

	return 0;
}

/*Add media-content update callback*/
bool _gl_db_update_reg_cb(void *data)
{
	GL_CHECK_FALSE(data);
	gl_appdata *ad = (gl_appdata *)data;
	GL_CHECK_FALSE(ad->db_noti_d);
	int ret = -1;

	gl_dbg("Set db updated callback");
	ret = media_content_set_db_updated_cb(__gl_db_update_cb, data);
	if (ret != MEDIA_CONTENT_ERROR_NONE)
		gl_dbgE("Set db updated cb failed[%d]!", ret);
//	ret = media_content_set_db_updated_cloud_cb(&(ad->db_noti_d->cloud_h),
//						    __gl_db_update_coud_cb,
//						    data);
//	if (ret != MEDIA_CONTENT_ERROR_NONE)
//		gl_dbgE("Set db updated cloud cb failed[%d]!", ret);
	return true;
}

/* calloc memory */
bool _gl_db_update_init(void *data)
{
	GL_CHECK_FALSE(data);
	gl_appdata *ad = (gl_appdata *)data;
	gl_db_noti_s *db_noti = (gl_db_noti_s *)calloc(1, sizeof(gl_db_noti_s));
	GL_CHECK_FALSE(db_noti);

	ad->db_noti_d = db_noti;
	gl_dbg("DB updated intialization");
	return true;
}

bool _gl_db_update_finalize(void *data)
{
	int ret = -1;
	gl_dbg("Unset db updated callback");

	ret = media_content_unset_db_updated_cb();
	if (ret != MEDIA_CONTENT_ERROR_NONE)
		gl_dbgE("UNSet db updated cb failed[%d]!", ret);

	GL_CHECK_FALSE(data);
	gl_appdata *ad = (gl_appdata *)data;
	GL_CHECK_FALSE(ad->db_noti_d);
	gl_db_noti_s *db_noti = ad->db_noti_d;

//	if (ad->db_noti_d->cloud_h) {
//		ret = media_content_unset_db_updated_cloud_cb(ad->db_noti_d->cloud_h);
//		if (ret != MEDIA_CONTENT_ERROR_NONE)
//			gl_dbgE("UNSet db updated cloud cb failed[%d]!", ret);
//		ad->db_noti_d->cloud_h = NULL;
//	}
	if (db_noti->uuids) {
		g_list_free_full(db_noti->uuids, g_free);
		db_noti->uuids = NULL;
	}
	GL_IF_DEL_TIMER(db_noti->db_timer);
	GL_IF_DEL_IDLER(db_noti->db_idl);
	GL_FREE(ad->db_noti_d);
	return true;
}


