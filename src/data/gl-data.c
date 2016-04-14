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

#include <errno.h>
#include <string.h>
#include <media_content.h>
#include "gl-data.h"
#include "gl-controlbar.h"
#include "gl-albums.h"
#include "gallery.h"
#include "gl-debug.h"
#include "gl-util.h"
#include "gl-strings.h"
#include "gl-icons.h"
#include "gl-exif.h"
#include "gl-db-update.h"
#include "gl-file-util.h"
#include "gl-data-util.h"
#include "gl-util.h"

static int __gl_data_append_gitems(void *data, Eina_List *elist, int store_type,
                                   Eina_List **p_elist)
{
	GL_CHECK_VAL(p_elist, -1);
	GL_CHECK_VAL(elist, -1);
	GL_CHECK_VAL(data, -1);
	gl_media_s *item = NULL;
	gl_item *gitem = NULL;

	EINA_LIST_FREE(elist, item) {
		if (item == NULL || item->uuid == NULL) {
			gl_dbgE("Invalid item!");
			continue;
		}

		gitem = _gl_data_util_calloc_gitem();
		if (gitem == NULL) {
			gl_dbgE("calloc gitem failed!");
			_gl_data_type_free_glitem((void **)&item);
			continue;
		}

		gitem->ad = data;
		gitem->item = item;
		gitem->store_type = store_type;
		*p_elist = eina_list_append(*p_elist, gitem);
		item = NULL;
		gitem = NULL;
	}
	return 0;
}

int _gl_get_fav_image_count(int *count)
{
	gl_filter_s filter;
	memset(&filter, 0x00, sizeof(gl_filter_s));
	_gl_data_util_get_fav_cond(filter.cond);
	filter.sort_type = MEDIA_CONTENT_ORDER_DESC;
	g_strlcpy(filter.sort_keyword, GL_CONDITION_ORDER, KEYWORD_LENGTH);
	filter.collate_type = MEDIA_CONTENT_COLLATE_NOCASE;
	filter.offset = GL_GET_ALL_RECORDS;
	filter.count = GL_GET_ALL_RECORDS;
	filter.with_meta = false;

	int err = _gl_local_data_get_all_media_count(&filter, count);
	if (err < 0) {
		gl_dbg("Failed to get item count[err: %d]!", err);
		return -1;
	}

	return err;
}

static int __gl_data_get_cluster_list(void *data, bool b_update)
{
	GL_PROFILE_IN;
	GL_CHECK_VAL(data, -1);
	gl_appdata *ad = (gl_appdata *)data;
	int length = 0;
	int all_item_cnt = 0;
	int phone_cnt = 0;
	Eina_List *item_list = NULL;
	gl_album_s *f_data = NULL;
	gl_cluster *default_album = NULL;
	int err = -1;
	int edit_album_cnt = 0;
	gl_filter_s filter;
	gl_album_s *new_mc = NULL;
	Eina_List *sel_id_list = NULL;
	bool b_selected = false;
	char new_mc_id[GL_MTYPE_ITEN_ID_LEN_MAX] = { 0, };
	char sel_id[GL_MTYPE_ITEN_ID_LEN_MAX] = { 0, };
	gl_dbg("");

	memset(&filter, 0x00, sizeof(gl_filter_s));

	if (b_update) {
		gl_dbg("Update mode.");
		gl_cluster *album_item = ad->albuminfo.selected;
		if (album_item && album_item->cluster &&
		        album_item->cluster->uuid) {
			g_strlcpy(sel_id, album_item->cluster->uuid,
			          GL_MTYPE_ITEN_ID_LEN_MAX);
			gl_sdbg("Save selected album ID: %s.", sel_id);
		}
		ad->albuminfo.selected = NULL;
		/* Get selected cluster IDs list */
		_gl_data_util_get_selected_cluster_id_list(ad, &sel_id_list);
		/* Free old list */
		_gl_data_finalize_albums_selected_list(ad);
		/* Clear cluster list */
		_gl_data_clear_cluster_list(ad, false);
	}


	//ad->new_album_name is the cluster name of newly created album
	//in moving medias to new album case.
	if (strlen(ad->albuminfo.new_name) && !ad->albuminfo.b_new_album) {
		memset(ad->albuminfo.new_name, 0x00, GL_ALBUM_NAME_LEN_MAX);
		//get gl_media_s from DB via folder full path(URL).
		err = _gl_local_data_get_album_by_path(ad->albuminfo.dest_folder,
		                                       &new_mc);
		if (err != 0) {
			gl_dbgE("Faild to get album[%d]!", err);
		} else if (new_mc->count == 0) {
			//media records of this cluster havn't been inserted to DB.
			//save the cluster ID.
			//update cluster item count while refresh albums view in _gl_albums_get_label().
			gl_sdbg("Get newly created gl_media_s, url: %s.",
			        ad->albuminfo.dest_folder);
			g_strlcpy(new_mc_id, new_mc->uuid,
			          GL_MTYPE_ITEN_ID_LEN_MAX);
			_gl_data_type_free_glitem((void **)&new_mc);
		}
	}

	//get real albums
	memset(filter.cond, 0x00, CONDITION_LENGTH);
	/* Get all contents(including local and cloud) for albums list */
	g_strlcpy(filter.cond, GL_CONDITION_IMAGE_VIDEO, CONDITION_LENGTH);
	filter.sort_type = MEDIA_CONTENT_ORDER_ASC;
	g_strlcpy(filter.sort_keyword, FOLDER_NAME, KEYWORD_LENGTH);
	filter.collate_type = MEDIA_CONTENT_COLLATE_NOCASE;
	filter.offset = GL_GET_ALL_RECORDS;
	filter.count = GL_GET_ALL_RECORDS;
	filter.with_meta = false;

	gl_dbg("Get_album_list:start");
	err = _gl_local_data_get_album_list(&filter, &item_list);
	gl_dbg("Get_album_list:end, %d", err);
	if (err != 0) {
		gl_dbg("No record!");
		ad->albuminfo.elist->clist = NULL;
		if (item_list) {
			_gl_data_util_clear_gtype_item_list(&item_list);
		}
		/* Continue to add AllShare album */
	}

	gl_cluster *gcluster = NULL;
	EINA_LIST_FREE(item_list, f_data) {
		if (f_data == NULL || f_data->uuid == NULL) {
			gl_dbgE("Invalid gl_media_s!");
			continue;
		}
		gl_sdbg("folder id: %s.", f_data->uuid);

		/**
		* There are Phone and web filters in libmedia-info,
		* but the are seperated, if we want to get  phone and web photos
		* only we can use is WMINFO_CLUSTER_TYPE_ALL,
		* and add condition checking to skip MMC albums.
		*
		* To skip MMC albums if MMC already unmounted,
		* in case of delay updating in DB.
		*/
		if ((ad->maininfo.mmc_state == GL_MMC_STATE_REMOVED_MOVING ||
		        ad->maininfo.mmc_state == GL_MMC_STATE_REMOVED) &&
		        f_data->type == GL_STORE_T_MMC) {
			gl_dbgW("MMC was unmounted, skip MMC album!");
			_gl_data_type_free_glitem((void **)&f_data);
			continue;
		}

		if (f_data->type == GL_STORE_T_PHONE ||
		        f_data->type == GL_STORE_T_MMC) {
			if (f_data->count == 0) {
				if (!g_strcmp0(f_data->uuid, new_mc_id)) {
					/* append the newly created cluster to list */
					gl_dbg("empty local album, but it's newly created...");
				} else {
					/* Skip empty album */
					gl_dbg("empty local album, skipping it...");
					_gl_data_type_free_glitem((void **)&f_data);
					continue;
				}
			}
			all_item_cnt += f_data->count;
			if (f_data->type == GL_STORE_T_PHONE ||
			        f_data->type == GL_STORE_T_MMC) {
				phone_cnt++;
			}
		}

		gcluster = NULL;
		gcluster = _gl_data_util_calloc_gcluster();
		if (gcluster == NULL) {
			gl_dbgE("_gl_data_util_calloc_gcluster failed!");
			_gl_data_type_free_glitem((void **)&f_data);
			continue;
		}

		gcluster->ad = ad;
		gcluster->cluster = f_data;

		length += f_data->count;

		if (_gl_data_is_camera_album(f_data)) {
			/**
			* Default album: Camera
			* Now Camera Shot is located in Phone/MMC.
			* If user can determine the location of default album,
			* here we should get the path and check it's in Phone or MMC.
			*/
			if (default_album) {
				/* album 'Camera' is in phone, set it before MMC album 'Camera' */
				char *phone_root_path = _gl_get_root_directory_path(STORAGE_TYPE_INTERNAL);
				if (phone_root_path && _gl_data_check_root_type(f_data->path, phone_root_path)) {
					ad->albuminfo.elist->clist = eina_list_prepend(ad->albuminfo.elist->clist,
					                             gcluster);
				} else {
					ad->albuminfo.elist->clist = eina_list_append_relative(ad->albuminfo.elist->clist,
					                             gcluster,
					                             default_album);
					default_album = gcluster;
				}
				GL_FREEIF(phone_root_path);
			} else {
				default_album = gcluster;
				ad->albuminfo.elist->clist = eina_list_prepend(ad->albuminfo.elist->clist,
				                             gcluster);
			}
		} else if (_gl_data_is_default_album(GL_STR_ALBUM_DOWNLOADS, f_data) || _gl_data_is_screenshot_album(GL_STR_ALBUM_SCREENSHOTS, f_data)) {
			if (default_album)
				ad->albuminfo.elist->clist = eina_list_append_relative(ad->albuminfo.elist->clist,
				                             gcluster,
				                             default_album);
			else
				ad->albuminfo.elist->clist = eina_list_prepend(ad->albuminfo.elist->clist,
				                             gcluster);
		} else {
			ad->albuminfo.elist->clist = eina_list_append(ad->albuminfo.elist->clist,
			                             gcluster);
		}
		/* Default album 'Camera shots' showed in edit view */
		edit_album_cnt++;

		if (sel_id_list) {
			b_selected = _gl_data_util_check_selected_str_id(&sel_id_list,
			             f_data->uuid);
			if (b_selected) {
				b_selected = false;
				/* Set checkbox state */
				gcluster->checked = true;
				/* Append gcluster to selected list */
				_gl_data_albums_selected_list_append(ad, gcluster);
			}
		}
		if (strlen(sel_id) > 0 && !g_strcmp0(sel_id, f_data->uuid)) {
			gl_dbgW("Found selected_album.");
			ad->albuminfo.selected = gcluster;
		}
	}

	/* Clear list of selected ID */
	if (sel_id_list) {
		b_selected = _gl_data_util_check_selected_str_id(&sel_id_list,
		             GL_ALBUM_ALL_ID);
		_gl_data_util_free_selected_str_list(&sel_id_list);
	}

	ad->albuminfo.elist->phone_cnt = phone_cnt;
	ad->albuminfo.elist->edit_cnt = edit_album_cnt;
#if 0
	/**
	* add "all" album, only inclduing local albums temporarily,
	* web album would be contained in the future .
	*/
	if (all_item_cnt) {
		_gl_db_update_set_count(ad, all_item_cnt);
		/* Create "All albums" album if any local file exists */
		gcluster = _gl_data_util_new_gcluster_all(ad, all_item_cnt);
		if (gcluster == NULL) {
			gl_dbgE("_gl_data_util_new_gcluster_all failed!");
			_gl_data_clear_cluster_list(ad, false);
			return -1;
		}
		if (b_selected) {
			b_selected = false;
			/* Set checkbox state */
			gcluster->checked = true;
			/* Append gcluster to selected list */
			_gl_data_albums_selected_list_append(ad, gcluster);
		}

		if (default_album)
			ad->albuminfo.elist->clist = eina_list_append_relative(ad->albuminfo.elist->clist,
			                             gcluster,
			                             default_album);
		else
			ad->albuminfo.elist->clist = eina_list_prepend(ad->albuminfo.elist->clist,
			                             gcluster);
		default_album = gcluster;

		/* Save all count */
		ad->maininfo.all_medias_cnt = all_item_cnt;
	}
#endif

	/* add "favourite" album */
	int fav_count = 0;
	err = _gl_get_fav_image_count(&fav_count);
	gl_dbg("fav image count is : %d", fav_count);

	if (fav_count) {
		/* Create "favourite albums" album if any favourite file exists */
		gcluster = _gl_data_util_new_gcluster_fav(ad, fav_count);
		if (gcluster == NULL) {
			gl_dbgE("_gl_data_util_new_gcluster_all failed!");
			_gl_data_clear_cluster_list(ad, false);
			return -1;
		}
		if (b_selected) {
			b_selected = false;
			/* Set checkbox state */
			gcluster->checked = true;
			/* Append gcluster to selected list */
			_gl_data_albums_selected_list_append(ad, gcluster);
		}

		if (default_album) {
			ad->albuminfo.elist->clist = eina_list_append_relative(
			                                 ad->albuminfo.elist->clist, gcluster, default_album);
		} else {
			ad->albuminfo.elist->clist = eina_list_prepend(
			                                 ad->albuminfo.elist->clist, gcluster);
		}
		default_album = gcluster;
	}

	if (all_item_cnt) {
		_gl_db_update_set_count(ad, all_item_cnt);
		/* Save all count */
		ad->maininfo.all_medias_cnt = all_item_cnt;
	}

	gl_dbg("Cluster Done.");
	GL_PROFILE_OUT;
	return length;
}

/* get a new gitem by mitem id*/
gl_item *_gl_data_new_gitem(void *data, const char *item_id)
{
	GL_CHECK_NULL(data);

	gl_item *gitem = _gl_data_util_calloc_gitem();
	GL_CHECK_NULL(gitem);

	_gl_local_data_get_media_by_id((char *)item_id, &(gitem->item));
	if (gitem->item == NULL) {
		_gl_data_util_free_gitem(gitem);
		return NULL;
	}
	return gitem;
}

gl_item *_gl_data_new_item_mitem(gl_media_s *mitem)
{
	gl_item *gitem = _gl_data_util_calloc_gitem();
	GL_CHECK_NULL(gitem);
	gitem->item = mitem;

	return gitem;
}

/*
*   Check it's default album under root path
*/
bool _gl_data_is_default_album(const char *match_folder, gl_album_s *album)
{
	GL_CHECK_FALSE(album);
	GL_CHECK_FALSE(album->display_name);
	GL_CHECK_FALSE(album->uuid);
	GL_CHECK_FALSE(match_folder);

	/* Name is 'Camera' folder locates in Phone */
	if (!g_strcmp0(album->display_name, match_folder) &&
	        album->type == GL_STORE_T_PHONE) {
		char parent_path[GL_DIR_PATH_LEN_MAX] = { 0, };

		gl_sdbg("Full path: %s", album->path);

		_gl_data_util_get_file_dir_name(album->path, NULL, NULL,
		                                parent_path);
		gl_sdbg("Parent path: %s.", parent_path);

		/* And parent folder is Phone root path, it's default folder */
		char *phone_root_path = _gl_get_root_directory_path(STORAGE_TYPE_INTERNAL);
		if (phone_root_path && !g_strcmp0(parent_path, phone_root_path)) {
			GL_FREE(phone_root_path);
			return true;
		}
		GL_FREEIF(phone_root_path);
	}

	return false;
}

/*
*   Check it's screenshot album under Image path
*/
bool _gl_data_is_screenshot_album(const char *match_folder, gl_album_s *album)
{
	GL_CHECK_FALSE(album);
	GL_CHECK_FALSE(album->display_name);
	GL_CHECK_FALSE(album->uuid);
	GL_CHECK_FALSE(match_folder);

	if (!g_strcmp0(album->display_name, match_folder) &&
	        album->type == GL_STORE_T_PHONE) {
		char parent_path[GL_DIR_PATH_LEN_MAX] = { 0, };

		_gl_data_util_get_file_dir_name(album->path, NULL, NULL,
		                                parent_path);
		/* And parent folder is Phone Image path, it's default folder */
		char *default_picture_path = _gl_get_directory_path(STORAGE_DIRECTORY_IMAGES);
		if (default_picture_path && !g_strcmp0(parent_path, default_picture_path)) {
			GL_FREE(default_picture_path);
			return true;
		}
		GL_FREEIF(default_picture_path);
	}

	return false;
}

/*
*   Check it's default album camera
*/
bool _gl_data_is_camera_album(gl_album_s *album)
{
	GL_CHECK_FALSE(album);
	GL_CHECK_FALSE(album->display_name);

#define GL_DCIM "DCIM"
	/* Name is 'Camera' folder locates in Phone */
	if (!g_strcmp0(album->display_name, GL_STR_ALBUM_CAMERA)) {
		char *parent_path = (char *)calloc(1, GL_DIR_PATH_LEN_MAX);
		GL_CHECK_FALSE(parent_path);

		char *root = NULL;
		if (album->type == GL_STORE_T_PHONE) {
			root = _gl_get_root_directory_path(STORAGE_TYPE_INTERNAL);
		} else {
			root = _gl_get_root_directory_path(STORAGE_TYPE_EXTERNAL);
		}

		if (!root) {
			GL_GFREE(parent_path);
			return false;
		}
		gl_sdbg("Full path: %s", album->path);
		_gl_data_util_get_file_dir_name(album->path, NULL, NULL,
		                                parent_path);
		gl_sdbg("Parent path: %s.", parent_path);

		char *dcim_path = g_strdup_printf("%s/%s", root, GL_DCIM);
		if (dcim_path == NULL) {
			GL_GFREE(parent_path);
			return false;
		}
		/* And parent folder is Phone root path, it's default folder */
		bool ret = false;
		ret = !g_strcmp0(dcim_path, parent_path);

		GL_GFREE(root);
		GL_GFREE(parent_path);
		GL_GFREE(dcim_path);
		return ret;
	}

	return false;
}

int _gl_data_cp_item_list(void *data, Eina_List *elist, Eina_List **p_elist)
{
	GL_CHECK_VAL(p_elist, -1);
	GL_CHECK_VAL(elist, -1);
	GL_CHECK_VAL(data, -1);

	Eina_List *l = NULL;
	gl_item *gitem = NULL;
	gl_item *ngitem = NULL;
	EINA_LIST_FOREACH(elist, l, gitem) {
		if (!gitem || !gitem->item) {
			continue;
		}
		ngitem = _gl_data_new_gitem(data, gitem->item->uuid);
		if (ngitem) {
			*p_elist = eina_list_append(*p_elist, ngitem);
		}
	}
	return 0;
}

int _gl_data_clear_cluster_list(void *data, bool b_force)
{
	GL_CHECK_VAL(data, -1);
	gl_appdata *ad = (gl_appdata *)data;
	/* To reset current album when clear cluster list */
	_gl_albums_set_current(data, NULL);

	if (ad->albuminfo.elist) {
		_gl_data_util_clear_cluster_list(&(ad->albuminfo.elist->clist));
		if (b_force) {
			GL_FREE(ad->albuminfo.elist);
		}
	}
	return 0;
}

int _gl_data_copy_cluster_list(void *data, bool b_force, Eina_List **list)
{
	GL_CHECK_VAL(data, -1);
	gl_appdata *ad = (gl_appdata *)data;
	GL_CHECK_VAL(ad->albuminfo.elist, -1);

	/* To reset current album when clear cluster list */
	_gl_albums_set_current(data, NULL);
	gl_cluster *current = NULL;
	int count = eina_list_count(ad->albuminfo.elist->clist);
	int i;
	for (i = 0; i < count; i++) {
		current = eina_list_nth(ad->albuminfo.elist->clist, i);
		if (current) {
			(*list) = eina_list_append((*list), current);
		}
	}

	ad->albuminfo.elist->clist = NULL;
	if (b_force) {
		GL_FREE(ad->albuminfo.elist);
	}

	return 0;
}

bool _gl_data_get_cluster_list(void *data)
{
	GL_PROFILE_IN;
	GL_CHECK_FALSE(data);
	gl_appdata *ad = (gl_appdata *)data;
	int n_entire_items = 0;

	/* Free old list */
	_gl_data_finalize_albums_selected_list(ad);
	_gl_data_clear_cluster_list(ad, true);

	ad->albuminfo.elist = _gl_data_util_calloc_cluster_list();
	GL_CHECK_FALSE(ad->albuminfo.elist);
	n_entire_items = __gl_data_get_cluster_list(ad, false);
	if (n_entire_items <= 0) { /* if error code is returned, negative value is possible */
		return false;
	}

	gl_dbg("cluster-length:%d", n_entire_items);
	GL_PROFILE_OUT;
	return true;
}

void _gl_update_album_selected_data(void *data, Eina_List **list)
{
	GL_CHECK(data);
	GL_CHECK(list);
	gl_appdata *ad = (gl_appdata *)data;
	gl_cluster *current = NULL;
	gl_cluster *sit = NULL;
	Eina_List *tmp_elist = NULL;
	if ((*list) && ad->albuminfo.elist && ad->albuminfo.elist->clist) {
		int count = eina_list_count(ad->albuminfo.elist->clist);
		int i;
		for (i = 0; i < count; i++) {
			current = eina_list_nth(ad->albuminfo.elist->clist, i);
			if (current && current->cluster) {
				EINA_LIST_FOREACH((*list), tmp_elist, sit) {
					if (sit && sit->cluster) {
						if (!g_strcmp0(current->cluster->uuid, sit->cluster->uuid)) {
							current->elist = sit->elist;
							current->checked = sit->checked;
							if(sit->checked) {
								_gl_data_albums_selected_list_append(ad, current);
							}
						}
					}
				}
			}
		}
		_gl_data_util_clear_cluster_list(list);

	} else {
		if (list) {
			gl_dbg("List is not NULL");
		}
	}
}

bool _gl_data_get_cluster_list_edit_mode(void *data)
{
	GL_PROFILE_IN;
	GL_CHECK_FALSE(data);
	gl_appdata *ad = (gl_appdata *)data;
	int n_entire_items = 0;
	Eina_List *list = NULL;

	/* Free old list */
	_gl_data_copy_cluster_list(ad, true, &list);
	if (!list) {
		gl_dbg("list is null");
	}
	ad->albuminfo.elist = _gl_data_util_calloc_cluster_list();
	GL_CHECK_FALSE(ad->albuminfo.elist);
	n_entire_items = __gl_data_get_cluster_list(ad, false);
	if (n_entire_items <= 0) { /* if error code is returned, negative value is possible */
		return false;
	}

	_gl_update_album_selected_data(data, &list);
	GL_PROFILE_OUT;
	return true;
}


int _gl_data_update_cluster_list(void *data)
{
	GL_CHECK_VAL(data, -1);
	gl_appdata *ad = (gl_appdata *)data;
	int len = 0;

	len = __gl_data_get_cluster_list(ad, true);
	gl_dbg("Cluster list length: %d.", len);
	if (len <= 0) {
		return -1;
	}

	return 0;
}

int _gl_data_cluster_list_remove(void *data, gl_cluster *item)
{
	GL_CHECK_VAL(item, -1);
	GL_CHECK_VAL(item->cluster, -1);
	GL_CHECK_VAL(item->cluster->uuid, -1);
	GL_CHECK_VAL(data, -1);
	gl_appdata *ad = (gl_appdata *)data;
	gl_cluster *current = NULL;
	Eina_List *l = NULL;

	GL_CHECK_VAL(ad->albuminfo.elist, -1);
	GL_CHECK_VAL(ad->albuminfo.elist->clist, -1);
	Eina_List *cluster_list = ad->albuminfo.elist->clist;
	EINA_LIST_FOREACH(cluster_list, l, current) {
		if (current == NULL || current->cluster == NULL ||
		        current->cluster->uuid == NULL) {
			gl_dbgE("Invalid album item!");
			continue;
		}

		if (!g_strcmp0(current->cluster->uuid, item->cluster->uuid)) {
			ad->albuminfo.elist->clist = eina_list_remove(ad->albuminfo.elist->clist,
			                             current);
			_gl_data_util_free_gcluster(current);
			break;
		}
		current = NULL;
	}

	return 0;
}

int _gl_data_get_cluster_by_id(void *data, const char *cluster_id,
                               gl_cluster **cluster)
{
	GL_CHECK_VAL(data, -1);
	GL_CHECK_VAL(cluster, -1);
	GL_CHECK_VAL(cluster_id, -1);
	gl_appdata *ad = (gl_appdata *)data;
	int i = 0;
	gl_cluster *current = NULL;

	*cluster = NULL;

	GL_CHECK_VAL(ad->albuminfo.elist, -1);
	int length = eina_list_count(ad->albuminfo.elist->clist);
	gl_sdbg("album length: %d, current album id: %s", length, cluster_id);

	for (i = 0; i < length; i++) {
		current = eina_list_nth(ad->albuminfo.elist->clist, i);
		if (current && current->cluster && current->cluster->uuid) {
			gl_sdbg("cluster : %s", current->cluster->display_name);
		} else {
			gl_dbgE("cluster is NULL");
			break;
		}

		if (!g_strcmp0(current->cluster->uuid, cluster_id)) {
			*cluster = current;
			gl_dbgW("Found!");
			return 0;
		}
	}
	return -1;
}

int _gl_data_get_cluster_by_path(void *data, const char *path,
                                 gl_cluster **cluster)
{
	GL_CHECK_VAL(data, -1);
	GL_CHECK_VAL(cluster, -1);
	GL_CHECK_VAL(path, -1);
	gl_appdata *ad = (gl_appdata *)data;
	int i = 0;
	gl_cluster *current = NULL;

	*cluster = NULL;

	GL_CHECK_VAL(ad->albuminfo.elist, -1);
	int length = eina_list_count(ad->albuminfo.elist->clist);
	gl_sdbg("album length: %d, path: %s", length, path);

	for (i = 0; i < length; i++) {
		current = eina_list_nth(ad->albuminfo.elist->clist, i);
		if (current && current->cluster && current->cluster->path) {
			gl_sdbg("cluster : %s", current->cluster->display_name);
		} else {
			gl_dbgE("cluster is NULL");
			continue;
		}

		if (!g_strcmp0(current->cluster->path, path)) {
			*cluster = current;
			gl_dbgW("Found!");
			return 0;
		}
	}
	return -1;
}

/*
*   append album to selected_albums_elist
*/
int _gl_data_albums_selected_list_append(void *data, gl_cluster *item)
{
	GL_CHECK_VAL(item, -1);
	GL_CHECK_VAL(item->cluster, -1);
	GL_CHECK_VAL(data, -1);
	gl_appdata *ad = (gl_appdata *)data;
	int img_cnt = 0;
	int sel_cnt = 0;
	int web_cnt = 0;

	if (GL_STORE_T_PHONE != item->cluster->type ||
	        _gl_data_is_camera_album(item->cluster) ||
	        _gl_data_is_default_album(GL_STR_ALBUM_DOWNLOADS, item->cluster)) {
		++ad->selinfo.disable_hide_cnt;
	}
	_gl_data_util_check_album_selected_files(item, &img_cnt,
	        &sel_cnt, &web_cnt);

	ad->selinfo.images_cnt = ad->selinfo.images_cnt + img_cnt;
	ad->selinfo.sel_cnt = ad->selinfo.sel_cnt + sel_cnt;
	ad->albuminfo.sel_elist = eina_list_append(ad->albuminfo.sel_elist, item);

	return 0;
}

/* Get new record from DB to check mitem exists or was already removed */
bool _gl_data_exists_item(void *data, const char *id)
{
	GL_CHECK_NULL(data);
	GL_CHECK_FALSE(id);
	gl_media_s *new_item = NULL;

	_gl_local_data_get_media_by_id((char *)id, &(new_item));
	if (new_item == NULL) {
		gl_dbgW("gl_media_s doesn't exist!");
		return false;
	} else {
		_gl_data_type_free_glitem((void **)&new_item);
		new_item = NULL;
		return true;
	}
}

/**
* Get medias count of album
*/
int _gl_data_get_item_cnt(const char *cluster_id, int album_type, int *item_cnt)
{
	GL_CHECK_VAL(item_cnt, -1);
	int err = -1;
	gl_sdbg("cluster_id: %s", cluster_id);

	gl_filter_s filter;
	memset(&filter, 0x00, sizeof(gl_filter_s));
	_gl_data_util_get_cond(filter.cond);
	filter.sort_type = MEDIA_CONTENT_ORDER_DESC;
	g_strlcpy(filter.sort_keyword, GL_CONDITION_ORDER, KEYWORD_LENGTH);
	filter.collate_type = MEDIA_CONTENT_COLLATE_NOCASE;
	filter.offset = GL_GET_ALL_RECORDS;
	filter.count = GL_GET_ALL_RECORDS;
	filter.with_meta = false;

	GL_CHECK_VAL(cluster_id, -1);
	if (album_type == GL_STORE_T_PHONE ||
	        album_type == GL_STORE_T_MMC) {
		/*It's normal album*/
		err = _gl_local_data_get_media_count(cluster_id, &filter,
		                                     item_cnt);
	} else if (album_type == GL_STORE_T_ALL) {
		/* All albums */
		gl_dbg("All albums media count.");
		err = _gl_local_data_get_all_media_count(&filter, item_cnt);
	} else if (album_type == GL_STORE_T_FAV) {
		/* Favorite albums */
		gl_dbg("favorite albums media count.");
		err = _gl_get_fav_image_count(item_cnt);
	} else {
		gl_dbgE("Invalid cluster_id!");
		return -1;
	}

	if (err < 0) {
		gl_dbg("Failed to get item count[err: %d]!", err);
		return -1;
	}

	gl_dbg("Item count: %d.", *item_cnt);
	return 0;
}

int _gl_data_update_item_cnt(gl_cluster *album)
{
	GL_CHECK_VAL(album, -1);
	GL_CHECK_VAL(album->cluster, -1);
	int item_count = 0;
	int err = -1;
	GL_CHECK_VAL(album->cluster->uuid, -1);
	err = _gl_data_get_item_cnt(album->cluster->uuid, album->cluster->type, &item_count);
	if (err != 0) {
		gl_dbg("_gl_data_get_item_cnt failed!");
		album->cluster->count = 0;
		return -1;
	}

	gl_dbg("Media count: old=%d, new=%d", album->cluster->count,
	       item_count);
	album->cluster->count = item_count;

	return 0;
}

int _gl_data_get_items(int start_pos, int end_pos, Eina_List **list)
{
	GL_CHECK_VAL(list, -1);
	int err = -1;
	gl_filter_s filter;

	memset(&filter, 0x00, sizeof(gl_filter_s));
	_gl_data_util_get_cond(filter.cond);
	filter.sort_type = MEDIA_CONTENT_ORDER_DESC;
	g_strlcpy(filter.sort_keyword, GL_CONDITION_ORDER, KEYWORD_LENGTH);
	filter.collate_type = MEDIA_CONTENT_COLLATE_NOCASE;
	if (start_pos != GL_GET_ALL_RECORDS) {
		filter.offset = start_pos;
		filter.count = end_pos - start_pos + 1;
	} else {
		filter.offset = GL_GET_ALL_RECORDS;
		filter.count = GL_GET_ALL_RECORDS;
	}
	filter.with_meta = true;

	/* Get "All" album medias */
	gl_dbg("_gl_data_get_items--all media");
	err = _gl_local_data_get_all_albums_media_list(&filter, list);
	gl_dbg("Error code: %d", err);
	return err;
}

int _gl_data_get_items_album(void *data, gl_cluster *album, int start_pos,
                             int end_pos, Eina_List **p_elist)
{
	GL_CHECK_VAL(data, -1);
	gl_appdata *ad = (gl_appdata *)data;
	Eina_List *itemlist = NULL;
	int err = -1;
	int store_type = GL_STORE_T_PHONE;
	gl_filter_s filter;

	memset(&filter, 0x00, sizeof(gl_filter_s));
	_gl_data_util_get_cond(filter.cond);
	filter.sort_type = MEDIA_CONTENT_ORDER_DESC;
	g_strlcpy(filter.sort_keyword, GL_CONDITION_ORDER, KEYWORD_LENGTH);
	filter.collate_type = MEDIA_CONTENT_COLLATE_NOCASE;
	filter.offset = start_pos;
	filter.count = end_pos - start_pos + 1;
	filter.with_meta = true;


	GL_CHECK_VAL(album, -1);
	GL_CHECK_VAL(album->cluster, -1);
	gl_dbg("folder start_pos[%d], end_pos[%d]", start_pos, end_pos);

	GL_CHECK_VAL(album->cluster->uuid, -1);
	store_type = album->cluster->type;

	/* Real album */
	if (store_type == GL_STORE_T_PHONE ||
	        store_type == GL_STORE_T_MMC) {
		ad->uginfo.sort_type = filter.sort_type;

		if (start_pos == (GL_FIRST_VIEW_END_POS + 1) &&
		        end_pos == GL_GET_UNTIL_LAST_RECORD) {
			/* Keep medias_elist and medias_cnt unchanged */
			gl_dbg("Gridview append idler; Keep medias_elist unchanged.");
		}
		err = _gl_local_data_get_album_media_list(&filter,
		        album->cluster->uuid,
		        &itemlist);
	} else if (store_type == GL_STORE_T_ALL) {
		/* Get "All" album medias */
		gl_dbg("All albums");
		ad->uginfo.sort_type = filter.sort_type;

		if (start_pos == (GL_FIRST_VIEW_END_POS + 1) &&
		        end_pos == GL_GET_UNTIL_LAST_RECORD) {
			/* Keep medias_elist and medias_cnt unchanged */
			gl_dbg("Keep medias_elist unchanged");
		}

		err = _gl_local_data_get_all_albums_media_list(&filter,
		        &itemlist);
	} else if (store_type == GL_STORE_T_FAV) {
		/* Get "All" album medias */
		gl_dbg("fav albums");
		ad->uginfo.sort_type = filter.sort_type;

		if (start_pos == (GL_FIRST_VIEW_END_POS + 1) &&
		        end_pos == GL_GET_UNTIL_LAST_RECORD) {
			/* Keep medias_elist and medias_cnt unchanged */
			gl_dbg("Keep medias_elist unchanged");
		}

		gl_filter_s filter_fav;

		memset(&filter_fav, 0x00, sizeof(gl_filter_s));
		_gl_data_util_get_fav_cond(filter_fav.cond);
		filter_fav.sort_type = MEDIA_CONTENT_ORDER_DESC;
		g_strlcpy(filter_fav.sort_keyword, GL_CONDITION_ORDER, KEYWORD_LENGTH);
		filter_fav.collate_type = MEDIA_CONTENT_COLLATE_NOCASE;
		filter_fav.offset = start_pos;
		filter_fav.count = end_pos - start_pos + 1;
		filter_fav.with_meta = true;

		err = _gl_local_data_get_fav_albums_media_list(&filter_fav,
		        &itemlist);
	} else {
		gl_dbgE("Wrong cluster type!");
		return -1;
	}

	gl_dbg("Error code: %d", err);
	if ((err == 0) && (itemlist != NULL)) {
		__gl_data_append_gitems(data, itemlist, store_type, p_elist);
	} else if (itemlist) {
		_gl_data_util_clear_gtype_item_list(&itemlist);
	}

	gl_dbg("done");
	return err;
}

int _gl_data_update_item_list(void *data, Eina_List **sel_id_list,
                              Eina_List **p_elist)
{
	GL_CHECK_VAL(data, -1);
	gl_appdata *ad = (gl_appdata *)data;
	Eina_List *itemlist = NULL;
	int err = -1;
	int view_mode = gl_get_view_mode(ad);
	gl_item *gitem = NULL;
	int store_type = GL_STORE_T_PHONE;
	gl_filter_s filter;

	memset(&filter, 0x00, sizeof(gl_filter_s));
	_gl_data_util_get_cond(filter.cond);
	filter.sort_type = MEDIA_CONTENT_ORDER_DESC;
	g_strlcpy(filter.sort_keyword, GL_CONDITION_ORDER, KEYWORD_LENGTH);
	filter.collate_type = MEDIA_CONTENT_COLLATE_NOCASE;
	filter.offset = GL_GET_ALL_RECORDS;
	filter.count = GL_GET_ALL_RECORDS;
	filter.with_meta = true;

	/* In albums tab, or album select view from tag, current album is available */
	if (_gl_ctrl_get_tab_mode(ad) == GL_CTRL_TAB_ALBUMS ||
	        view_mode == GL_VIEW_THUMBS_SELECT) {
		gl_cluster *cur_album = _gl_albums_get_current(data);
		GL_CHECK_VAL(cur_album, -1);
		GL_CHECK_VAL(cur_album->cluster, -1);
		GL_CHECK_VAL(cur_album->cluster->uuid, -1);
		store_type = cur_album->cluster->type;
		ad->uginfo.sort_type = filter.sort_type;

		if (store_type == GL_STORE_T_ALL) {
			gl_dbg("All albums medias");
			err = _gl_local_data_get_all_albums_media_list(&filter,
			        &itemlist);
		} else if (store_type == GL_STORE_T_FAV) {
			gl_filter_s filter_fav;

			memset(&filter_fav, 0x00, sizeof(gl_filter_s));
			_gl_data_util_get_fav_cond(filter_fav.cond);
			filter_fav.sort_type = MEDIA_CONTENT_ORDER_DESC;
			g_strlcpy(filter_fav.sort_keyword, GL_CONDITION_ORDER, KEYWORD_LENGTH);
			filter_fav.collate_type = MEDIA_CONTENT_COLLATE_NOCASE;
			filter_fav.offset = GL_GET_ALL_RECORDS;
			filter_fav.count = GL_GET_ALL_RECORDS;
			filter_fav.with_meta = true;
			err = _gl_local_data_get_fav_albums_media_list(&filter_fav, &itemlist);
		} else {
			gl_dbg("Local album medias");
			err = _gl_local_data_get_album_media_list(&filter,
			        cur_album->cluster->uuid,
			        &itemlist);
		}
	}

	gl_dbg("Error code: %d", err);
	if ((err != 0) || (itemlist == NULL)) {
		gl_dbgE("(err != 0) || (itemlist == NULL)");
		/* Clear list of selected ID */
		if (sel_id_list && *sel_id_list) {
			eina_list_free(*sel_id_list);
			*sel_id_list = NULL;
		}

		if (itemlist) {
			_gl_data_util_clear_gtype_item_list(&itemlist);
		}

		return err;
	}

	bool b_selected = false;
	gl_media_s *item = NULL;
	EINA_LIST_FREE(itemlist, item) {
		if (item == NULL || item->uuid == NULL) {
			gl_dbgE("Invalid item!");
			continue;
		}
		gitem = _gl_data_util_calloc_gitem();
		if (gitem == NULL) {
			gl_dbgE("_gl_data_util_calloc_gitem failed!");
			_gl_data_type_free_glitem((void **)&item);
			continue;
		}
		gl_cluster *cur_album = _gl_albums_get_current(data);
		if (cur_album == NULL) {
			free(gitem);
			return -1;
		}
		gitem->ad = ad;
		gitem->item = item;
		gitem->store_type = store_type;
		gitem->album = cur_album;
		/* Append item to medias_elist */
		*p_elist = eina_list_append(*p_elist, gitem);

		if (sel_id_list && *sel_id_list) {
			b_selected = _gl_data_util_check_selected_str_id(sel_id_list,
			             item->uuid);
			if (b_selected) {
				b_selected = false;
				/* Set checkbox state */
				gitem->checked = true;
				/* Append gitem to selected list */
				_gl_data_selected_list_append(ad, gitem);
			}
		}
	}

	/* Clear list of selected ID */
	if (sel_id_list && *sel_id_list) {
		_gl_data_util_free_selected_str_list(sel_id_list);
	}
	gl_dbg("done");
	return err;
}

int _gl_data_get_album_cover(gl_cluster *album, gl_item **pgitem,
                             media_content_order_e sort_type)
{
	GL_CHECK_VAL(pgitem, -1);
	GL_CHECK_VAL(album, -1);
	GL_CHECK_VAL(album->cluster, -1);
	GL_CHECK_VAL(album->cluster->uuid, -1);
	GL_CHECK_VAL(album->ad, -1);
	Eina_List *item_list = NULL;
	gl_media_s *item = NULL;
	gl_item *gitem = NULL;
	int err = -1;
	gl_filter_s filter;

	memset(&filter, 0x00, sizeof(gl_filter_s));
	_gl_data_util_get_cond(filter.cond);
	filter.sort_type = sort_type;
	g_strlcpy(filter.sort_keyword, GL_CONDITION_ORDER, KEYWORD_LENGTH);
	filter.collate_type = MEDIA_CONTENT_COLLATE_NOCASE;
	filter.offset = GL_FIRST_VIEW_START_POS;
	filter.count = 1;
	filter.with_meta = false;

	if (album->cluster->type == GL_STORE_T_PHONE ||
	        album->cluster->type == GL_STORE_T_MMC) {
		/* real album */
		err = _gl_local_data_get_album_cover(&filter,
		                                     album->cluster->uuid,
		                                     &item_list);
		if (err != 0 || item_list == NULL) {
			gl_dbgE("Get album medias failed[%d]", err);
			if (item_list) {
				_gl_data_util_clear_gtype_item_list(&item_list);
			}
			return -1;
		}

		EINA_LIST_FREE(item_list, item) {
			if (item == NULL) {
				gl_dbgE("Invalid gl_media_s!");
				return -1;
			}
			gitem = _gl_data_util_calloc_gitem();
			if (gitem == NULL) {
				gl_dbgE("_gl_data_util_calloc_gitem failed!");
				_gl_data_type_free_glitem((void **)&item);
				return -1;
			}
			gitem->item = item;
			gitem->album = album;
			*pgitem = gitem;
			break;
		}
	} else if (album->cluster->type == GL_STORE_T_ALL) {
		/* add "All" album */
		gl_appdata *ad = (gl_appdata *)album->ad;
		gl_dbg("db_get_item_list--all media");
		/* use new api to get all local files, mmc status checking would be done in new apis */
		err = _gl_local_data_get_all_albums_cover(&filter, &item_list);
		if (err != 0 || item_list == NULL) {
			gl_dbgE("Failed to get all medias[%d]!", err);
			if (item_list) {
				_gl_data_util_clear_gtype_item_list(&item_list);
			}

			return -1;
		}

		EINA_LIST_FREE(item_list, item) {
			if (item == NULL) {
				gl_dbgE("Invalid gl_media_s!");
				return -1;
			}
			gitem = _gl_data_util_calloc_gitem();
			if (gitem == NULL) {
				gl_dbgE("_gl_data_util_calloc_gitem failed!");
				_gl_data_type_free_glitem((void **)&item);
				return -1;
			}
			gitem->item = item;
			gitem->album = album;
			gitem->store_type = album->cluster->type;
			*pgitem = gitem;
			/* Get latest item for saving lastest modified time */
			ad->maininfo.last_mtime = item->mtime;
			break;
		}
	} else if (album->cluster->type == GL_STORE_T_FAV) {
		/* add "fav" album */
		gl_filter_s filter_fav;

		memset(&filter_fav, 0x00, sizeof(gl_filter_s));
		_gl_data_util_get_fav_cond(filter_fav.cond);
		filter_fav.sort_type = sort_type;
		g_strlcpy(filter_fav.sort_keyword, GL_CONDITION_ORDER, KEYWORD_LENGTH);
		filter_fav.collate_type = MEDIA_CONTENT_COLLATE_NOCASE;
		filter_fav.offset = GL_GET_ALL_RECORDS;
		filter_fav.count = GL_GET_ALL_RECORDS;
		filter_fav.with_meta = false;
		gl_dbg("db_get_item_list--fav media");
		/* use new api to get all local files, mmc status checking would be done in new apis */
		err = _gl_local_data_get_fav_albums_cover(&filter_fav, &item_list);
		if (err != 0 || item_list == NULL) {
			gl_dbgE("Failed to get fav medias[%d]!", err);
			if (item_list) {
				_gl_data_util_clear_gtype_item_list(&item_list);
			}
			return -1;
		}

		EINA_LIST_FREE(item_list, item) {
			if (item == NULL) {
				gl_dbgE("Invalid gl_media_s!");
				return -1;
			}
			gitem = _gl_data_util_calloc_gitem();
			if (gitem == NULL) {
				gl_dbgE("_gl_data_util_calloc_gitem failed!");
				_gl_data_type_free_glitem((void **)&item);
				return -1;
			}
			gitem->item = item;
			gitem->album = album;
			gitem->store_type = album->cluster->type;
			*pgitem = gitem;
			break;
		}
	} else {
		gl_dbgE("Wrong cluster type!");
		return -1;
	}

	if (*pgitem)
		gl_sdbg("Album(%s) cover[%p]", album->cluster->display_name,
		        *pgitem);
	return 0;
}

bool _gl_data_is_item_cnt_zero(void *data, media_content_type_e type,
                               Eina_List *elist)
{
	GL_CHECK_VAL(data, false);
	gl_item *gitem = NULL;
	Eina_List *l = NULL;

	EINA_LIST_FOREACH(elist, l, gitem) {
		if (gitem && gitem->item) {
			if (gitem->item->type == type) {
				return false;
			}
		}
	}

	gl_dbg(" item_cnt = 0 ");
	return true;
}

int _gl_data_get_first_item(media_content_type_e type, Eina_List *elist,
                            gl_item **p_gitem)
{
	GL_CHECK_VAL(p_gitem, -1);
	GL_CHECK_VAL(elist, -1);

	/* Get first item for 'All' */
	if (type == MEDIA_CONTENT_TYPE_OTHERS) {
		*p_gitem = eina_list_nth(elist, 0);
		return 0;
	}

	/* Get first item for "Images" or "Videos" */
	Eina_List *l = NULL;
	gl_item *gitem = NULL;
	EINA_LIST_FOREACH(elist, l, gitem) {
		if (gitem && gitem->item && gitem->item->type == type) {
			*p_gitem = gitem;
			return 0;
		}
	}

	gl_dbgW("Not found!");
	return -1;
}

int _gl_data_get_orig_item_by_index(void *data, gl_item **gitem, int idx)
{
	return 0;
}

/* Remove file from DB and file system */
int _gl_data_delete_file(char *path, char *uuid, bool is_hide)
{
	GL_CHECK_VAL(uuid, -1);
	GL_CHECK_VAL(path, -1);
	int ret = -1;

	if (!gl_file_unlink(path)) {
		char *error_msg = calloc(1, GL_ARRAY_LEN_MAX);
		GL_CHECK_VAL(error_msg, -1);
		strerror_r(errno, error_msg, GL_ARRAY_LEN_MAX);
		gl_dbgE("file_unlink failed[%s]!", error_msg);
		GL_FREE(error_msg);
		//return -1;
	}

	/* Delete record from DB then delete media from file system */
	ret = media_info_delete_from_db(uuid);

	if (ret != 0) {
		gl_dbgE("media_info_delete_from_db failed[%d]!", ret);
		return -1;
	}

	return 0;
}

gl_item *_gl_data_selected_list_get_nth(void *data, int idx)
{
	GL_CHECK_NULL(data);
	gl_appdata *ad = (gl_appdata *)data;

	return eina_list_nth(ad->selinfo.elist, idx);
}

int _gl_data_free_burstshot_items(gl_item *gitem, Eina_List *elist)
{
	GL_CHECK_VAL(elist, -1);
	GL_CHECK_VAL(gitem, -1);
	GL_CHECK_VAL(gitem->item, -1);

	if (gitem->item->type != MEDIA_CONTENT_TYPE_IMAGE ||
	        gitem->item->image_info == NULL ||
	        gitem->item->image_info->burstshot_id == NULL) {
		return -1;
	}

	int i = gitem->sequence;
	int cnt = eina_list_count(elist);
	gl_item *cur_gitem = NULL;
	char *burstshot_id = gitem->item->image_info->burstshot_id;
	for (; i < cnt; i++) {
		cur_gitem = eina_list_nth(elist, i);
		if (cur_gitem == NULL || cur_gitem->item == NULL ||
		        cur_gitem->item->uuid == NULL) {
			gl_dbgE("Invalid gitem. continue!");
			continue;
		} else if (cur_gitem->item->type != MEDIA_CONTENT_TYPE_IMAGE ||
		           cur_gitem->item->image_info == NULL ||
		           cur_gitem->item->image_info->burstshot_id == NULL) {
			/* No more same burstshot items */
			break;
		}

		if (!g_strcmp0(burstshot_id, cur_gitem->item->image_info->burstshot_id)) {
			_gl_data_util_free_gitem(cur_gitem);
		} else {
			/* No more same burstshot items */
			break;
		}
		cur_gitem = NULL;
	}
	return 0;
}

int _gl_data_selected_add_burstshot(void *data, gl_item *gitem, Eina_List *elist)
{
	GL_CHECK_VAL(elist, -1);
	GL_CHECK_VAL(gitem, -1);
	GL_CHECK_VAL(gitem->item, -1);
	GL_CHECK_VAL(data, -1);
	gl_appdata *ad = (gl_appdata *)data;

	if (gitem->item->type != MEDIA_CONTENT_TYPE_IMAGE ||
	        gitem->item->image_info == NULL ||
	        gitem->item->image_info->burstshot_id == NULL) {
		return -1;
	}

	int i = gitem->sequence;
	int cnt = eina_list_count(elist);
	gl_item *cur_gitem = NULL;
	char *burstshot_id = gitem->item->image_info->burstshot_id;
	for (; i < cnt; i++) {
		cur_gitem = eina_list_nth(elist, i);
		if (cur_gitem == NULL || cur_gitem->item == NULL ||
		        cur_gitem->item->uuid == NULL) {
			gl_dbgE("Invalid gitem. continue!");
			continue;
		} else if (cur_gitem->item->type != MEDIA_CONTENT_TYPE_IMAGE ||
		           cur_gitem->item->image_info == NULL ||
		           cur_gitem->item->image_info->burstshot_id == NULL) {
			/* No more same burstshot items */
			break;
		}

		if (!g_strcmp0(burstshot_id, cur_gitem->item->image_info->burstshot_id)) {
			cur_gitem->checked = gitem->checked;
			ad->selinfo.images_cnt++;
#ifdef _USE_ROTATE_BG
			if (cur_gitem->item->ext &&
			        !strcasecmp(cur_gitem->item->ext, GL_JPEG_FILE_EXT))
				if (_gl_exif_check_img_type(cur_gitem->item->file_url)) {
					ad->selinfo.jpeg_cnt++;
				}
#endif
			if (gitem->item->storage_type != GL_STORE_T_PHONE) {
				++ad->selinfo.disable_hide_cnt;
			}
			ad->selinfo.elist = eina_list_append(ad->selinfo.elist,
			                                     cur_gitem);
		} else {
			/* No more same burstshot items */
			break;
		}
		cur_gitem = NULL;
	}
	return 0;
}

bool _gl_data_get_burstshot_status(void *data, gl_item *gitem)
{
	GL_CHECK_FALSE(gitem);
	if (gitem->item == NULL ||
	        gitem->item->type != MEDIA_CONTENT_TYPE_IMAGE ||
	        gitem->item->image_info == NULL ||
	        gitem->item->image_info->burstshot_id == NULL) {
		return gitem->checked;
	}

	GL_CHECK_FALSE(data);
	gl_appdata *ad = (gl_appdata *)data;
	GL_CHECK_FALSE(ad->selinfo.elist);
	Eina_List *sel_list = ad->selinfo.elist;

	Eina_List *l = NULL;
	gl_item *current = NULL;
	EINA_LIST_FOREACH(sel_list, l, current) {
		if (current == NULL || current->item == NULL ||
		        current->item->type != MEDIA_CONTENT_TYPE_IMAGE ||
		        current->item->image_info == NULL ||
		        current->item->image_info->burstshot_id == NULL) {
			gl_dbgE("Invalid gitem!");
			continue;
		}
		if (!g_strcmp0(gitem->item->image_info->burstshot_id,
		               current->item->image_info->burstshot_id)) {
			return true;
		}
	}
	return false;
}

/* b_selall, append item for select-all case */
int _gl_data_selected_list_append(void *data, gl_item *gitem)
{
	GL_CHECK_VAL(gitem, -1);
	GL_CHECK_VAL(gitem->item, -1);
	GL_CHECK_VAL(data, -1);
	gl_appdata *ad = (gl_appdata *)data;

	/* Update selected images count */
	if (gitem->item->type == MEDIA_CONTENT_TYPE_IMAGE) {
		ad->selinfo.images_cnt++;
#ifdef _USE_ROTATE_BG
		gl_dbg("Ext: %s", gitem->item->ext);
		if (gitem->item->ext &&
		        !strcasecmp(gitem->item->ext, GL_JPEG_FILE_EXT)) {
			if (_gl_exif_check_img_type(gitem->item->file_url)) {
				ad->selinfo.jpeg_cnt++;
			}
		}
#endif
	}
	if (gitem->item->storage_type != GL_STORE_T_PHONE) {
		++ad->selinfo.disable_hide_cnt;
	}

	Eina_List *sel_list = ad->selinfo.elist;
	sel_list = eina_list_append(sel_list, gitem);
	ad->selinfo.elist = sel_list;
	return 0;
}

int _gl_data_selected_fav_list_remove(void *data, gl_item *gitem)
{
	GL_CHECK_VAL(gitem, -1);
	GL_CHECK_VAL(gitem->item, -1);
	GL_CHECK_VAL(gitem->item->uuid, -1);
	GL_CHECK_VAL(data, -1);
	gl_appdata *ad = (gl_appdata *)data;
	gl_item *current = NULL;
	Eina_List *l = NULL;

	EINA_LIST_FOREACH(ad->selinfo.fav_elist, l, current) {
		if (current == NULL || current->item == NULL ||
		        current->item->uuid == NULL) {
			gl_dbgE("Invalid gitem!");
			continue;
		}
		if (!g_strcmp0(current->item->uuid, gitem->item->uuid)) {
			ad->selinfo.fav_elist = eina_list_remove(ad->selinfo.fav_elist,
			                        current);
		}
		current = NULL;
	}
	return 0;
}

/* b_selall, remove item for select-all case */
int _gl_data_selected_list_remove(void *data, gl_item *gitem)
{
	GL_CHECK_VAL(gitem, -1);
	GL_CHECK_VAL(gitem->item, -1);
	GL_CHECK_VAL(gitem->item->uuid, -1);
	GL_CHECK_VAL(data, -1);
	gl_appdata *ad = (gl_appdata *)data;
	gl_item *current = NULL;
	Eina_List *l = NULL;
	bool b_removed = false;
	char *burstshot_id = NULL;
	if (gitem->item->type == MEDIA_CONTENT_TYPE_IMAGE &&
	        gitem->item->image_info) {
		burstshot_id = gitem->item->image_info->burstshot_id;
	}

	EINA_LIST_FOREACH(ad->selinfo.elist, l, current) {
		if (current == NULL || current->item == NULL ||
		        current->item->uuid == NULL) {
			gl_dbgE("Invalid gitem!");
			continue;
		}
		if (!g_strcmp0(current->item->uuid, gitem->item->uuid) ||
		        (burstshot_id &&
		         current->item->type == MEDIA_CONTENT_TYPE_IMAGE &&
		         current->item->image_info &&
		         current->item->image_info->burstshot_id &&
		         !g_strcmp0(burstshot_id, current->item->image_info->burstshot_id))) {
			if (gitem->item->storage_type != GL_STORE_T_PHONE) {
				ad->selinfo.disable_hide_cnt--;
			}
			/* Update selected images count */
			if (current->item->type == MEDIA_CONTENT_TYPE_IMAGE &&
			        ad->selinfo.images_cnt > 0) {
				ad->selinfo.images_cnt--;
#ifdef _USE_ROTATE_BG
				if (current->item->ext &&
				        !strcasecmp(current->item->ext, GL_JPEG_FILE_EXT) &&
				        ad->selinfo.jpeg_cnt > 0)
					if (_gl_exif_check_img_type(current->item->file_url)) {
						ad->selinfo.jpeg_cnt--;
					}
#endif
			}

			ad->selinfo.elist = eina_list_remove(ad->selinfo.elist,
			                                     current);
			if (burstshot_id == NULL) {
				break;
			} else {
				current->checked = gitem->checked;
				b_removed = true;
			}
		} else if (b_removed) {
			/* Already removed and not more items, quit */
			break;
		}
		current = NULL;
	}
	return 0;
}

/* Just remove one media from selected list, it just for selected view */
int _gl_data_selected_list_remove_one(void *data, gl_item *gitem)
{
	GL_CHECK_VAL(gitem, -1);
	GL_CHECK_VAL(gitem->item, -1);
	GL_CHECK_VAL(gitem->item->uuid, -1);
	GL_CHECK_VAL(data, -1);
	gl_appdata *ad = (gl_appdata *)data;

	/* Update selected images count */
	if (gitem->item->type == MEDIA_CONTENT_TYPE_IMAGE &&
	        ad->selinfo.images_cnt > 0) {
		ad->selinfo.images_cnt--;
#ifdef _USE_ROTATE_BG
		if (gitem->item->ext &&
		        !strcasecmp(gitem->item->ext, GL_JPEG_FILE_EXT) &&
		        ad->selinfo.jpeg_cnt > 0) {
			if (_gl_exif_check_img_type(gitem->item->file_url)) {
				ad->selinfo.jpeg_cnt--;
			}
		}
#endif
	}
	if (gitem->item->storage_type != GL_STORE_T_PHONE) {
		ad->selinfo.disable_hide_cnt--;
	}
	ad->selinfo.elist = eina_list_remove(ad->selinfo.elist,
	                                     gitem);
	return 0;
}

int _gl_data_selected_list_finalize(void *data)
{
	GL_CHECK_VAL(data, -1);
	gl_appdata *ad = (gl_appdata *)data;

	if (ad->selinfo.elist == NULL) {
		return -1;
	}

	gl_item *gitem = NULL;
	int i;
	int count;
	if (ad->gridinfo.medias_elist) {
		count = eina_list_count(ad->gridinfo.medias_elist);
		for (i = 0; i < count ; i++) {
			gitem = eina_list_nth(ad->gridinfo.medias_elist, i);
			if (gitem) {
				gitem->checked = false;
			}
		}
	}
	EINA_LIST_FREE(ad->selinfo.elist, gitem) {
		if (gitem) {
			gitem->checked = false;
		}
	}

	ad->selinfo.elist = NULL;
	ad->selinfo.images_cnt = 0;
	ad->selinfo.disable_hide_cnt = 0;
#ifdef _USE_ROTATE_BG
	ad->selinfo.jpeg_cnt = 0;
#endif
	gl_dbg("Selected list freed");
	return 0;
}

int _gl_data_clear_selected_info(void *data)
{
	GL_CHECK_VAL(data, -1);
	gl_appdata *ad = (gl_appdata *)data;

	if (ad->selinfo.elist == NULL) {
		return -1;
	}

	eina_list_free(ad->selinfo.elist);
	gl_dbg("eina free selected list");

	ad->selinfo.elist = NULL;
	ad->selinfo.images_cnt = 0;
	ad->selinfo.disable_hide_cnt = 0;
#ifdef _USE_ROTATE_BG
	ad->selinfo.jpeg_cnt = 0;
#endif
	return 0;
}

int _gl_data_copy_selected_info_save_current_selected(void *data, Eina_List **elist)
{
	GL_CHECK_VAL(data, -1);
	gl_appdata *ad = (gl_appdata *)data;

	if (ad->selinfo.elist == NULL) {
		return -1;
	}

	_gl_data_util_filter_selected_str_ids(&(ad->selinfo.elist), elist);
	gl_item *gitem = NULL;
	gl_item *gitem1 = NULL;
	Eina_List *tmp;
	int count = eina_list_count(ad->gridinfo.medias_elist);
	int i;
	for (i = 0; i < count ; i++) {
		gitem1 = eina_list_nth(ad->gridinfo.medias_elist, i);
		EINA_LIST_FOREACH(ad->selinfo.elist, tmp, gitem) {
			if (gitem && gitem->item && gitem->item->uuid && gitem1 && gitem1->item && gitem1->item->uuid) {
				if (!strcmp(gitem->item->uuid, gitem1->item->uuid)) {
					ad->selinfo.elist = eina_list_remove(ad->selinfo.elist, gitem);
				}
			}

		}
	}

	return 0;
}

int _gl_data_selected_list_count(void *data)
{
	GL_CHECK_VAL(data, -1);
	gl_appdata *ad = (gl_appdata *)data;

	return eina_list_count(ad->selinfo.elist);
}

bool _gl_data_is_root_path(const char *path)
{
	if (path == NULL) {
		return false;
	}

	char *phone_root_path = _gl_get_root_directory_path(STORAGE_TYPE_INTERNAL);
	char *mmc_root_path = _gl_get_root_directory_path(STORAGE_TYPE_EXTERNAL);

	if ((phone_root_path && !g_strcmp0(phone_root_path, path)) ||
	        (mmc_root_path && !g_strcmp0(mmc_root_path, path))) {
		gl_sdbg("Root path: %s", path);
		GL_FREEIF(phone_root_path);
		GL_FREEIF(mmc_root_path);
		return true;
	}
	GL_FREEIF(phone_root_path);
	GL_FREEIF(mmc_root_path);
	return false;
}

bool _gl_data_check_root_type(const char *path, const char *root)
{
	if (path == NULL || root == NULL) {
		return false;
	}

	if (!strncmp(root, path, strlen(root))) {
		gl_sdbg("Root path: %s", path);
		return true;
	}

	return false;
}

bool _gl_data_is_albums_selected_empty(void *data)
{
	GL_CHECK_VAL(data, 1);
	gl_appdata *ad = (gl_appdata *)data;
	gl_cluster *album = NULL;

	Eina_List *l = NULL;
	EINA_LIST_FOREACH(ad->albuminfo.sel_elist, l, album) {
		if (album && album->cluster && album->cluster->count) {
			return false;
		}
		album = NULL;
	}

	return true;
}

/**
 * If 'All albums' selected for share, only append files contained in 'All albums';
 * If both local albums and web albums selected, 'Share' item should be disabled.
 * path_str -> local albums, path_str2 -> web albums
 */
int _gl_data_get_albums_selected_files(void *data, void *get_cb, void *cb_data)
{
	GL_CHECK_VAL(data, -1);
	gl_appdata *ad = (gl_appdata *)data;
	GL_CHECK_VAL(ad->albuminfo.sel_elist, -1);
	gl_cluster *album = NULL;
	Eina_List *l = NULL;
	int err = -1;
	Eina_List *elist = NULL;
	gl_filter_s filter;
	int count = 0;

	int (*_get_cb)(void * cb_data, gl_media_s * item) = NULL;
	if (get_cb) {
		_get_cb = get_cb;
	}

	memset(&filter, 0x00, sizeof(gl_filter_s));
	_gl_data_util_get_cond(filter.cond);
	filter.sort_type = MEDIA_CONTENT_ORDER_DESC;
	g_strlcpy(filter.sort_keyword, GL_CONDITION_ORDER,
	          KEYWORD_LENGTH);
	filter.collate_type = MEDIA_CONTENT_COLLATE_NOCASE;
	filter.offset = GL_GET_ALL_RECORDS;
	filter.count = GL_GET_ALL_RECORDS;
	filter.with_meta = false;

	EINA_LIST_FOREACH(ad->albuminfo.sel_elist, l, album) {
		if (album == NULL || album->cluster == NULL ||
		        album->cluster->uuid == NULL) {
			continue;
		}
		if (!strcmp(GL_ALBUM_ALL_ID, album->cluster->uuid))
			err = _gl_local_data_get_all_albums_media_list(&filter,
			        &elist);
		else
			err = _gl_local_data_get_album_media_list(&filter,
			        album->cluster->uuid,
			        &elist);
		if ((err == 0) && (elist != NULL)) {
			gl_media_s *item = NULL;
			EINA_LIST_FREE(elist, item) {
				if (item == NULL) {
					gl_dbgE("Invalid item!");
					continue;
				}
				if (_get_cb) {
					_get_cb(cb_data, item);
				}
				count++;
				item = NULL;
			}
		}
	}
	return count;
}

int _gl_data_get_albums_selected_cnt(void *data)
{
	GL_CHECK_VAL(data, -1);
	gl_appdata *ad = (gl_appdata *)data;

	return eina_list_count(ad->albuminfo.sel_elist);
}

int _gl_data_albums_selected_list_remove(void *data, gl_cluster *item)
{
	GL_CHECK_VAL(item, -1);
	GL_CHECK_VAL(item->cluster, -1);
	GL_CHECK_VAL(item->cluster->uuid, -1);
	GL_CHECK_VAL(data, -1);
	gl_appdata *ad = (gl_appdata *)data;
	gl_cluster *current = NULL;
	Eina_List *l = NULL;

	if (GL_STORE_T_PHONE != item->cluster->type ||
	        _gl_data_is_camera_album(item->cluster) ||
	        _gl_data_is_default_album(GL_STR_ALBUM_DOWNLOADS, item->cluster)) {
		--ad->selinfo.disable_hide_cnt;
	}

	Eina_List *sel_list = ad->albuminfo.sel_elist;
	EINA_LIST_FOREACH(sel_list, l, current) {
		if (current == NULL || current->cluster == NULL ||
		        current->cluster->uuid == NULL) {
			gl_dbgE("Invalid gcluster!");
			continue;
		}

		if (!g_strcmp0(current->cluster->uuid, item->cluster->uuid)) {
			int img_cnt = 0;
			int sel_cnt = 0;
			int web_cnt = 0;

			_gl_data_util_check_album_selected_files(item,
			        &img_cnt,
			        &sel_cnt,
			        &web_cnt);

			ad->selinfo.images_cnt = ad->selinfo.images_cnt - img_cnt;
			ad->selinfo.sel_cnt = ad->selinfo.sel_cnt - sel_cnt;

			ad->albuminfo.sel_elist = eina_list_remove(ad->albuminfo.sel_elist,
			                          current);
			break;
		}
		current = NULL;
	}

	return 0;
}

int _gl_data_finalize_albums_selected_list(void *data)
{
	GL_CHECK_VAL(data, -1);
	gl_appdata *ad = (gl_appdata *)data;
	gl_cluster *item = NULL;

	Eina_List *sel_list = ad->albuminfo.sel_elist;
	EINA_LIST_FREE(sel_list, item) {
		if (item) {
			item->checked = false;
		}
	}

	ad->selinfo.images_cnt = 0;
	ad->selinfo.disable_hide_cnt = 0;
	ad->selinfo.sel_cnt = 0;
	ad->albuminfo.sel_elist = NULL;
	return 0;
}

int _gl_data_init(void *data)
{
	GL_CHECK_VAL(data, -1);
	int err = _gl_local_data_connect();
	if (err != 0) {
		gl_dbgE("Connect to media-content DB failed!");
	}
	return err;
}

int _gl_data_finalize(void *data)
{
	GL_CHECK_VAL(data, -1);
	int err = _gl_local_data_disconnect();
	if (err != 0) {
		gl_dbgE("Disconnect with media-content DB failed!");
	}
	return err;
}

int _gl_data_delete_media(void *data, gl_media_s *media_item)
{
	GL_CHECK_VAL(media_item, -1);
	return media_info_delete_from_db(media_item->uuid);
}

/* Creates a thumbnail image for given the media, asynchronously */
int _gl_data_create_thumb(gl_media_s *item,
                          media_thumbnail_completed_cb callback,
                          void *user_data)
{
	GL_CHECK_VAL(item, -1);
	GL_CHECK_VAL(item->media_h, -1);
	int ret = -1;
	gl_sdbg("File[%s]", item->file_url);

	ret = media_info_create_thumbnail(item->media_h, callback, user_data);
	if (ret != MEDIA_CONTENT_ERROR_NONE) {
		gl_dbgE("Failed to create thumbnail[%d]!", ret);
		return -1;
	}
	item->b_create_thumb = true;
	return 0;
}

int _gl_data_cancel_thumb(gl_media_s *item)
{
	GL_CHECK_VAL(item, -1);
	GL_CHECK_VAL(item->media_h, -1);

	media_info_cancel_thumbnail(item->media_h);
	item->b_create_thumb = false;
	return 0;
}

bool _gl_data_is_album_exists(void *data, gl_cluster *album)
{
	GL_CHECK_FALSE(data);
	GL_CHECK_FALSE(album);
	GL_CHECK_FALSE(album->cluster);
	GL_CHECK_FALSE(album->cluster->path);
	return _gl_local_data_is_album_exists(album->cluster->path);
}

int _gl_data_save_selected_str_ids(void *data, Eina_List **elist)
{
	GL_CHECK_VAL(data, -1);
	gl_appdata *ad = (gl_appdata *)data;
	/* Get ID list of selected items */
	if (ad->selinfo.elist == NULL) {
		return -1;
	}
	_gl_data_util_get_selected_str_ids(&(ad->selinfo.elist), elist);
	/* Reset old flags */
	ad->selinfo.images_cnt = 0;
	ad->selinfo.disable_hide_cnt = 0;
#ifdef _USE_ROTATE_BG
	ad->selinfo.jpeg_cnt = 0;
#endif
	return 0;
}

int _gl_data_restore_selected(Eina_List *sel_ids, gl_item *gitem)
{
	GL_CHECK_VAL(gitem, -1);
	GL_CHECK_VAL(gitem->item, -1);
	if (sel_ids) {
		bool b_selected = false;
		b_selected = _gl_data_check_selected_id(sel_ids,
		                                        gitem->item->uuid);
		if (b_selected) {
			gl_dbg("b_selected");
			b_selected = false;
			/* Set checkbox state */
			gitem->checked = true;
			gitem->album->elist = eina_list_append(gitem->album->elist, gitem);
		}
	}
	return 0;
}

int _gl_data_restore_update_selected(Eina_List *sel_ids, gl_item *gitem)
{
	GL_CHECK_VAL(gitem, -1);
	GL_CHECK_VAL(gitem->item, -1);

	if (sel_ids) {
		bool b_selected = false;
		b_selected = _gl_data_check_selected_id(sel_ids,
		                                        gitem->item->uuid);
		if (b_selected) {
			gl_dbg("b_selected");
			b_selected = false;
			/* Set checkbox state */
			gitem->checked = true;
			gitem->album->elist = eina_list_append(gitem->album->elist, gitem);
		}
	}
	return 0;
}

bool _gl_data_check_selected_id(Eina_List *sel_id_list, const char *id)
{
	GL_CHECK_FALSE(sel_id_list);
	GL_CHECK_FALSE(id);
	Eina_List *tmp_elist = NULL;
	gl_item *sit = NULL;
	if (eina_list_count(sel_id_list) == 0) {
		gl_dbgE("sel_id_list is empty!");
		return false;
	}
	EINA_LIST_FOREACH(sel_id_list, tmp_elist, sit) {
		if (sit == NULL || sit->item == NULL || sit->item->uuid == NULL) {
			gl_dbgE("Invalid p_id!");
			sit = NULL;
			continue;
		}
		if (g_strcmp0(id, sit->item->uuid)) {
			sit = NULL;
			continue;
		}
		return true;
	}
	return false;
}


