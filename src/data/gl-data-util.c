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
#include "gl-data-util.h"
#include "gl-controlbar.h"
#include "gallery.h"
#include "gl-debug.h"
#include "gl-util.h"
#include "gl-strings.h"
#include "gl-data-type.h"

bool _gl_data_util_get_file_dir_name(const char *file_path, char *filename,
                                     char *dir_name, char *dir_path)
{
	GL_CHECK_FALSE(file_path);
	gint i = 0;
	gint count = 0;
	for (i = strlen(file_path); i >= 0; i--) {
		if (file_path[i] != '\0') {
			count++;
		}
		if (file_path[i] == '/') {
			if (filename != NULL) {
				memcpy(filename, &file_path[i + 1], --count);
				*(filename + count) = '\0';
				gl_sdbg("File Name = %s", filename);
			}
			if (dir_path != NULL) {
				memcpy(dir_path, &file_path[0], i);
				*(dir_path + i) = '\0';
				gl_sdbg("Directory Name = %s", dir_path);
			}
			if (dir_name != NULL) {
				count = 0;
				for (--i; i >= 0; i--) {
					count++;
					if (file_path[i] == '/') {
						memcpy(dir_name, &file_path[i + 1], --count);
						*(dir_name + count) = '\0';
						gl_sdbg("Directory Name = %s", dir_name);
						count = 0;
						return true;
					}
				}
			}
			return true;
		}
	}

	return false;
}

/*
*  create a gl_item
*/
gl_item *_gl_data_util_calloc_gitem(void)
{
	gl_item *gitem = (gl_item *)calloc(1, sizeof(gl_item));
	GL_CHECK_NULL(gitem);
	return gitem;
}

int _gl_data_util_free_gitem(gl_item *gitem)
{
	GL_CHECK_VAL(gitem, -1);
	if (gitem->item) {
		_gl_data_type_free_glitem((void **) & (gitem->item));
		gitem->item = NULL;
	}

	GL_FREEIF(gitem->thumb_data);
	GL_FREE(gitem);
	return 0;
}

gl_cluster *_gl_data_util_calloc_gcluster(void)
{
	gl_cluster *gcluster = (gl_cluster *)calloc(1, sizeof(gl_cluster));
	GL_CHECK_NULL(gcluster);
	return gcluster;
}

int _gl_data_util_free_gcluster(gl_cluster *gcluster)
{
	GL_CHECK_VAL(gcluster, -1);
	if (gcluster->cover) {
		_gl_data_util_free_gitem(gcluster->cover);
		gcluster->cover = NULL;
	}
	if (gcluster->cluster) {
		_gl_data_type_free_glitem((void **) & (gcluster->cluster));
		gcluster->cluster = NULL;
	}
	GL_FREE(gcluster->album_data);
	GL_FREE(gcluster);
	return 0;
}

gl_cluster *_gl_data_util_new_gcluster_all(void *data, int count)
{
	gl_dbg("");
	GL_CHECK_NULL(data);
	gl_cluster *gcluster = NULL;
	gl_album_s *mcluster = NULL;

	gcluster = _gl_data_util_calloc_gcluster();
	GL_CHECK_NULL(gcluster);

	_gl_data_type_new_album(&mcluster);
	if (mcluster == NULL) {
		GL_FREE(gcluster);
		return NULL;
	}

	mcluster->uuid = strdup(GL_ALBUM_ALL_ID);
	mcluster->count = count;
	mcluster->type = GL_STORE_T_ALL;
	mcluster->display_name = strdup(GL_STR_ID_ALL_ALBUMS);
	gcluster->ad = data;
	gcluster->cluster = mcluster;
	return gcluster;
}

gl_cluster *_gl_data_util_new_gcluster_fav(void *data, int count)
{
	gl_dbg("");
	GL_CHECK_NULL(data);
	gl_cluster *gcluster = NULL;
	gl_album_s *mcluster = NULL;

	gcluster = _gl_data_util_calloc_gcluster();
	GL_CHECK_NULL(gcluster);

	_gl_data_type_new_album(&mcluster);
	if (mcluster == NULL) {
		GL_FREE(gcluster);
		return NULL;
	}

	mcluster->uuid = strdup(GL_ALBUM_FAVOURITE_ID);
	mcluster->count = count;
	mcluster->type = GL_STORE_T_FAV;
	mcluster->display_name = strdup(GL_STR_ID_FAVOURITE_ALBUMS);
	gcluster->ad = data;
	gcluster->cluster = mcluster;

	return gcluster;
}

gl_cluster_list *_gl_data_util_calloc_cluster_list(void)
{
	gl_cluster_list *clus_list = (gl_cluster_list *)calloc(1,
	                             sizeof(gl_cluster_list));
	GL_CHECK_NULL(clus_list);
	return clus_list;
}

/* Clear eina_list got from DB */
int _gl_data_util_clear_gtype_item_list(Eina_List **elist)
{
	void *current = NULL;

	if (elist && *elist) {
		gl_dbg("Clear gtype items list");
		EINA_LIST_FREE(*elist, current) {
			if (current) {
				_gl_data_type_free_glitem((void **)&current);
				current = NULL;
			}
		}
		*elist = NULL;
	}

	return 0;
}

int _gl_data_util_clear_item_list(Eina_List **elist)
{
	GL_CHECK_VAL(elist, -1);
	gl_item *current = NULL;
	if (*elist) {
		gl_dbg("Clear elist");
		EINA_LIST_FREE(*elist, current) {
			_gl_data_util_free_gitem(current);
			current = NULL;
		}
		*elist = NULL;
	}
	return 0;

}

int _gl_data_util_clear_cluster_list(Eina_List **elist)
{
	GL_CHECK_VAL(elist, -1);
	gl_cluster *current = NULL;
	if (*elist) {
		EINA_LIST_FREE(*elist, current) {
			_gl_data_util_free_gcluster(current);
			current = NULL;
		}
		*elist = NULL;
	}
	return 0;
}

int _gl_data_util_get_selected_cluster_id_list(void *data,
        Eina_List **sel_id_list)
{
	GL_CHECK_VAL(sel_id_list, -1);
	GL_CHECK_VAL(data, -1);
	gl_appdata *ad = (gl_appdata *)data;
	GL_CHECK_VAL(ad->albuminfo.sel_elist, -1);
	gl_cluster *gcluster = NULL;
	char *item_id = NULL;

	/* Save ID of selected clusters */
	EINA_LIST_FREE(ad->albuminfo.sel_elist, gcluster) {
		if (gcluster && gcluster->cluster && gcluster->cluster->uuid) {
			item_id = strdup(gcluster->cluster->uuid);
			*sel_id_list = eina_list_append(*sel_id_list,
			                                (void *)item_id);
		}
	}
	return 0;
}

int _gl_data_util_get_selected_str_ids(Eina_List **sel_list, Eina_List **elist)
{
	GL_CHECK_VAL(elist, -1);
	GL_CHECK_VAL(sel_list, -1);
	gl_item *gitem = NULL;
	char *str_id = NULL;

	/* Save ID of selected items */
	EINA_LIST_FREE(*sel_list, gitem) {
		if (gitem && gitem->item && gitem->item->uuid) {
			str_id = strdup(gitem->item->uuid);
			*elist = eina_list_append(*elist, (void *)str_id);
		}
	}
	*sel_list = NULL;
	return 0;
}

int _gl_data_util_filter_selected_str_ids(Eina_List **sel_list, Eina_List **elist)
{
	GL_CHECK_VAL(elist, -1);
	GL_CHECK_VAL(sel_list, -1);
	gl_item *gitem = NULL;
	Eina_List *tmp;
	char *str_id = NULL;

	/* Save ID of selected items */
	EINA_LIST_FOREACH(*sel_list, tmp, gitem) {
		if (gitem && gitem->item && gitem->item->uuid) {
			str_id = strdup(gitem->item->uuid);
			*elist = eina_list_append(*elist, (void *)str_id);
		}
	}
	return 0;
}

/* Check ID is in the list or not */
bool _gl_data_util_check_selected_str_id(Eina_List **sel_id_list, const char *id)
{
	GL_CHECK_FALSE(id);
	GL_CHECK_FALSE(sel_id_list);
	Eina_List *tmp_elist = NULL;
	void *p_id = NULL;

	if (eina_list_count(*sel_id_list) == 0) {
		gl_dbgE("sel_id_list is empty!");
		return false;
	}

	EINA_LIST_FOREACH(*sel_id_list, tmp_elist, p_id) {
		if (p_id == NULL) {
			gl_dbgE("Invalid p_id!");
			continue;
		}
		/* Get next one if they wasn't equal */
		if (g_strcmp0(id, (char *)p_id)) {
			p_id = NULL;
			continue;
		}
		gl_sdbg("Found: %s", id);
		*sel_id_list = eina_list_remove(*sel_id_list, p_id);
		GL_FREE(p_id);
		return true;
	}
	return false;
}

/* Free list of selected IDs */
int _gl_data_util_free_selected_str_list(Eina_List **sel_id_list)
{
	GL_CHECK_VAL(sel_id_list, -1);
	if (*sel_id_list == NULL) {
		gl_dbg("sel_id_list is empty!");
		return -1;
	}

	void *p_id = NULL;
	EINA_LIST_FREE(*sel_id_list, p_id) {
		if (p_id == NULL) {
			gl_dbgE("Invalid p_id!");
			continue;
		}
		GL_FREE(p_id);
	}
	*sel_id_list = NULL;
	return 0;
}

/* Check in selected list of web album id */
bool _gl_data_util_check_selected_int_id(Eina_List **sel_id_list, int id)
{
	GL_CHECK_FALSE(sel_id_list);
	Eina_List *tmp_elist = NULL;
	void *p_id = NULL;

	if (eina_list_count(*sel_id_list) == 0) {
		gl_dbgE("sel_id_list is empty!");
		return false;
	}

	EINA_LIST_FOREACH(*sel_id_list, tmp_elist, p_id) {
		if (p_id == NULL) {
			gl_dbgE("Invalid p_id!");
			continue;
		}

		if (id != (int)p_id) {
			p_id = NULL;
			continue;
		}

		*sel_id_list = eina_list_remove(*sel_id_list, p_id);
		return true;
	}
	return false;
}

int _gl_data_util_check_album_selected_files(gl_cluster *album,
        int *img_cnt, int *sel_cnt,
        int *web_cnt)
{
	GL_CHECK_VAL(album, -1);
	GL_CHECK_VAL(album->cluster, -1);
	GL_CHECK_VAL(album->cluster->uuid, -1);
	int _sel_cnt = 0;
	int _img_cnt = 0;
	int _web_cnt = 0;
	int err = -1;
	Eina_List *itemlist = NULL;
	gl_filter_s filter;

	memset(&filter, 0x00, sizeof(gl_filter_s));
	_gl_data_util_get_cond(filter.cond);
	filter.sort_type = MEDIA_CONTENT_ORDER_DESC;
	filter.collate_type = MEDIA_CONTENT_COLLATE_NOCASE;
	g_strlcpy(filter.sort_keyword, GL_CONDITION_ORDER, KEYWORD_LENGTH);
	filter.offset = GL_GET_ALL_RECORDS;
	filter.count = GL_GET_ALL_RECORDS;
	filter.with_meta = false;

	if (album->cluster->type == GL_STORE_T_ALL) {
		gl_dbg("All albums");
		err = _gl_local_data_get_all_albums_media_list(&filter,
		        &itemlist);
	} else if (album->cluster->type == GL_STORE_T_PHONE ||
	           album->cluster->type == GL_STORE_T_MMC) {
		gl_dbg("Local album");
		err = _gl_local_data_get_album_media_list(&filter,
		        album->cluster->uuid,
		        &itemlist);
	} else {
		gl_dbgE("Wrong cluster type!");
		return -1;
	}

	if ((err == 0) && (itemlist != NULL)) {
		gl_media_s *item = NULL;
		EINA_LIST_FREE(itemlist, item) {
			if (item == NULL || item->uuid == NULL) {
				gl_dbgE("Invalid item!");
				continue;
			}

			_sel_cnt++;
			if (album->cluster->type == GL_STORE_T_PHONE ||
			        album->cluster->type == GL_STORE_T_ALL) {
				if (item->type == MEDIA_CONTENT_TYPE_IMAGE) {
					_img_cnt++;
				}
			} else {
				/*  image for web */
				_web_cnt++;
			}
			item = NULL;
		}
	}
	gl_dbg("Selected items count: %d, image count: %d, web count: %d",
	       _sel_cnt, _img_cnt, _web_cnt);
	if (sel_cnt) {
		*sel_cnt = _sel_cnt;
	}
	if (img_cnt) {
		*img_cnt = _img_cnt;
	}
	if (web_cnt) {
		*web_cnt = _web_cnt;
	}
	return 0;
}

/* Set viewby_m as -1 or GL_VIEW_BY_NUM to get cond view default viewby mode
 * @cond is allocated by caller, maybe it's an array
 */
int _gl_data_util_get_cond(char *cond)
{
	GL_CHECK_VAL(cond, -1);
	g_strlcpy(cond, GL_CONDITION_IMAGE_VIDEO, CONDITION_LENGTH);
	return 0;
}

int _gl_data_util_get_fav_cond(char *cond)
{
	GL_CHECK_VAL(cond, -1);
	g_strlcpy(cond, "((MEDIA_TYPE=0 OR MEDIA_TYPE=1) AND (MEDIA_FAVOURITE>0)"
	          " AND (MEDIA_STORAGE_TYPE=0 OR MEDIA_STORAGE_TYPE=1 "
	          "OR MEDIA_STORAGE_TYPE=101 OR MEDIA_STORAGE_TYPE=121))", CONDITION_LENGTH);
	return 0;
}

