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
#include "gl-fs.h"
#include "gl-entry.h"
#include "gl-albums-rename.h"
#include "gl-albums.h"
#include "gl-ui-util.h"
#include "gl-util.h"
#include "gl-controlbar.h"
#include "gl-editfield.h"
#include "gl-button.h"
#include "gl-popup.h"
#include "gl-strings.h"
#include "gl-fs.h"
#include "gl-db-update.h"
#include "gl-file-util.h"

static void __gl_albums_rename_trans_finished(void *data)
{
	GL_CHECK(data);
	gl_appdata *ad = (gl_appdata *)data;
	int view_mode = gl_get_view_mode(ad);
	gl_dbgW("view_mode: %d", view_mode);

	/* Clear previous view after animation finished */
	if (view_mode == GL_VIEW_ALBUMS_RENAME)
		elm_gengrid_clear(ad->albuminfo.view);
}

static int __gl_albums_rename_pop_view(void *data)
{
	GL_CHECK_VAL(data, -1);
	gl_appdata *ad = (gl_appdata *)data;
	GL_CHECK_VAL(ad->maininfo.naviframe, -1);
	gl_dbg("");

	ad->albuminfo.selected = NULL;
	/* Update album view */
	gl_set_view_mode(data, GL_VIEW_ALBUMS);
	_gl_albums_change_mode(data, false);
	return 0;
}

/* dont call elm_naviframe_item_pop in this callback */
static Eina_Bool __gl_albums_rename_pop_cb(void *data, Elm_Object_Item *it)

{
	/* If the back button is clicked or H/W Back key is pressed,
	this callback will be called right before the
	current view pop. User may implement application logic here. */

	/* To proceed the popping, please return the EINA_TRUE or not,
	popping will be canceled.
	If it needs to delete the current view without any transition effect,
	then call the elm_object_item_del() here and then return the EINA_FALSE */

	gl_dbg("");
	GL_CHECK_VAL(data, -1);
	gl_appdata *ad = (gl_appdata *)data;
	ad->albuminfo.selected = NULL;
	/* Update edit view */
	gl_set_view_mode(data, GL_VIEW_ALBUMS_EDIT);
	gl_albums_update_items(data);
	return EINA_TRUE;
}

/*It is used by rename in longpressed event popup, after rename, should pop to album view, not album edit view*/
static Eina_Bool __gl_albums_rename_pop_to_album_cb(void *data, Elm_Object_Item *it)

{
	gl_dbg("");
	GL_CHECK_VAL(data, -1);
	gl_appdata *ad = (gl_appdata *)data;
	ad->albuminfo.selected = NULL;
	/* Update edit view */
	gl_set_view_mode(data, GL_VIEW_ALBUMS);
	gl_albums_update_items(data);
	return EINA_TRUE;
}

/*
* @param b_enter
*	True: Enter key on Keyboard pressed, False: Button Done clicked
*/
static int __gl_albums_rename_process(void *data, bool b_enter)
{
	gl_dbg("b_enter: %d", b_enter);
	GL_CHECK_VAL(data, -1);
	gl_appdata *ad = (gl_appdata *)data;
	bool b_root_path = false;
	gl_cluster *album_item = NULL;

	/*If click "Done", will back to album view, pop callback should not be called.*/
	evas_object_data_del(ad->entryinfo.entry, "gl_entry_pop_cb_key");

	if (gl_get_view_mode(ad) != GL_VIEW_ALBUMS_RENAME) {
		gl_dbgE("View mode is wrong!");
		goto RENAME_FAILED;
	}

	album_item = ad->albuminfo.selected;
	if (album_item == NULL || album_item->cluster == NULL ||
	    album_item->cluster->uuid == NULL) {
		gl_dbgE("selected_album is NULL!");
		goto RENAME_FAILED;
	}

	char name[GL_FILE_NAME_LEN_MAX] = { 0, };
	/* Get utf8 format string */
	int ret = -1;
	ret = gl_get_entry_text(ad->entryinfo.entry, name,
				GL_FILE_NAME_LEN_MAX);
	if (ret != 0) {
		gl_dbgE("Get entry text failed!");
		goto RENAME_FAILED;
	}

	/* Get valid name */
	if (_gl_validate_album_name(data, name) != 0) {
		if (b_enter)
			_gl_editfield_hide_imf(ad);
		return -1;
	}

	gl_sdbg("Valid album name: %s", name);
	/* Get src folder path */
	char old_path[GL_DIR_PATH_LEN_MAX] = { 0, };
	if (album_item->cluster->path) {
		g_strlcpy(old_path, album_item->cluster->path,
			  GL_DIR_PATH_LEN_MAX);
		gl_sdbg("Src folder: %s", old_path);
		if (_gl_data_is_root_path(old_path)) {
			/**
			* Root path, couldn't rename root path,
			* make the new dest folder
			* and move all files for root path to it.
			*/
			gl_dbg("Rename [No Name] album's name.");
			b_root_path = true;
		}
	} else {
		gl_dbgE("gl_db_get_folder_fullpath failed(%d)!", ret);
		goto RENAME_FAILED;
	}

	/* Get dest folder path */
	char new_path[GL_DIR_PATH_LEN_MAX] = { 0, };
	char parent[GL_DIR_PATH_LEN_MAX] = { 0, };

	if (b_root_path) {
		snprintf(new_path, GL_DIR_PATH_LEN_MAX, "%s/%s",
			 GL_DEFAULT_PATH_IMAGES, name);
		g_strlcpy(parent, GL_DEFAULT_PATH_IMAGES, GL_DIR_PATH_LEN_MAX);
	} else {
		g_strlcpy(new_path, old_path, GL_DIR_PATH_LEN_MAX);
		g_strlcpy(parent, old_path, GL_DIR_PATH_LEN_MAX);

		int i = 0;
		int length = strlen(old_path);
		for (i = length; i >= 0; i--) {
			if (new_path[i] == '/') {
				gl_dbg("length=%d, i=%d", length, i);
				 /* Path like "/root/abc/" */
				if (i == length - 1)
					continue;
				memcpy(&new_path[i + 1], name, strlen(name));
				new_path[i + 1 + strlen(name)] =  '\0';
				parent[i] =  '\0';
				break;
			}
		}
	}

	gl_sdbg("Dest folder: %s", new_path);
#ifdef _RENAME_ALBUM_SENSITIVE
	if (!strcasecmp(old_path, new_path))
#else
	if (!g_strcmp0(old_path, new_path))
#endif
	{
		gl_dbgW("Same as current name!");
		goto RENAME_FAILED;
	}

	/**
	* If dir is empty, 1 is returned,
	* if it contains at least 1 file, 0 is returned.
	* On failure, -1 is returned.
	*/
#ifdef _RENAME_ALBUM_SENSITIVE
	if (_gl_fs_check_name_case(new_path, name))
#endif
	{
		memset(new_path, 0x00, GL_DIR_PATH_LEN_MAX);
		snprintf(new_path, GL_DIR_PATH_LEN_MAX, "%s/%s", parent, name);
		gl_sdbg("New dir path: %s", new_path);
		int res = gl_file_dir_is_empty(new_path);
		gl_dbg("file_dir_is_empty return value: %d", res);
		if (res == 0) {
			char *popup_desc = NULL;
			popup_desc = (char *)calloc(GL_POPUP_STRING_MAX,
						   sizeof(char));
			if (popup_desc == NULL) {
				gl_dbgE("memory allocation fail!");
				return -1;
			}

			snprintf(popup_desc, GL_POPUP_STRING_MAX, "%s<br>%s",
				GL_STR_SAME_NAME_ALREADY_IN_USE, GL_STR_RETRY_Q);
			popup_desc[strlen(popup_desc)] = '\0';
			gl_popup_create_popup(ad, GL_POPUP_ALBUM_RENAME_DUPLICATE,
					      popup_desc);
			GL_FREE(popup_desc);
			gl_dbgW("New folder already exists!");
			return -1;
		}
	}

	_gl_db_update_lock_once(ad, true);
	if (b_root_path) {
		/**
		* Case 1: Rename virtual album 'No Name'.
		* It couldn't be done by updating album record in DB directly.
		* Cuz it would change the folder path of all folder records,
		* to make them invalid.
		*
		* Create new folder under default images path,
		* Move all medias under root path to new folder.
		*/
		if (gl_make_new_album(name) == false) {
			gl_popup_create_popup(ad, GL_POPUP_NOBUT,
					      GL_STR_UNABLE_tO_RENAME);
			gl_dbgE("Failed to make a new directory!");
			goto RENAME_FAILED;
		}

		/* Move medias from 'No Name' album to new album */
		if (gl_move_root_album(ad, album_item, new_path) != 0) {
			gl_dbg("gl_move_root_album failed!");
			gl_popup_create_popup(ad, GL_POPUP_NOBUT,
					      GL_STR_UNABLE_tO_RENAME);
			goto RENAME_FAILED;
		} else {
			gl_dbg("New album added, update albums list.");
			ad->albuminfo.selected = NULL;
			_gl_data_update_cluster_list(ad);
		}
	} else {
		/**
		* Case 2: Rename normal album. Update album record in DB.
		*/
		media_folder_set_name(album_item->cluster->folder_h, name);
		ret = media_folder_update_to_db(album_item->cluster->folder_h);
		if (ret < 0) {
			gl_dbg("media_folder_set_name failed (%d)!", ret);
			goto RENAME_FAILED;
		}

		/**
		* Change folder name in file system
		* for some special name, DB update success, but file mv failed.
		* So, do this first.
		*/
		if (gl_file_exists(old_path)) {
			if (!gl_file_mv(old_path, new_path))
				gl_dbg("file_mv failed!");
		} else {
			gl_dbgW("Source folder path doesn't exist!");
		}
		/* Update memory */
		GL_FREEIF(album_item->cluster->display_name);
		album_item->cluster->display_name = strdup(name);
		GL_FREEIF(album_item->cluster->path);
		album_item->cluster->path = strdup(new_path);
	}

	elm_naviframe_item_pop(ad->maininfo.naviframe);
	__gl_albums_rename_pop_view(data);
	return 0;

 RENAME_FAILED:

	elm_naviframe_item_pop(ad->maininfo.naviframe);
	__gl_albums_rename_pop_view(data);
	return -1;
}

int _gl_albums_rename_create_view(void *data, gl_cluster *album)
{
	GL_CHECK_VAL(album, -1);
	GL_CHECK_VAL(album->cluster, -1);
	GL_CHECK_VAL(album->cluster->display_name, -1);
	GL_CHECK_VAL(data, -1);
	gl_appdata *ad = (gl_appdata *)data;
	gl_dbg("");

	ad->entryinfo.mode = GL_ENTRY_RENAME_ALBUM;

	/* Create entry view */
	if (_gl_entry_create_view(data, album->cluster->display_name, GL_STR_ID_CHNAGE_NAME) < 0) {
		ad->entryinfo.mode = GL_ENTRY_NONE;
		return -1;
	}
	/* Set callbacks */
	Evas_Object *entry = _gl_editfield_get_entry(data);
	GL_CHECK_VAL(entry, -1);
	evas_object_data_set(entry, "gl_entry_process_cb_key",
			     __gl_albums_rename_process);
	if (GL_VIEW_ALBUMS == gl_get_view_mode(data))
		evas_object_data_set(entry, "gl_entry_pop_cb_key",
				     __gl_albums_rename_pop_to_album_cb);
	else
		evas_object_data_set(entry, "gl_entry_pop_cb_key",
				     __gl_albums_rename_pop_cb);

	evas_object_data_set(entry, "gl_entry_transit_cb_key",
			     __gl_albums_rename_trans_finished);
	gl_set_view_mode(data, GL_VIEW_ALBUMS_RENAME);
	return 0;
}

int _gl_albums_rename_update_view(void *data)
{
	GL_CHECK_VAL(data, -1);
	gl_appdata *ad = (gl_appdata *)data;
	gl_dbg("");

	if (gl_check_gallery_empty(data) ||
	    ad->albuminfo.elist->edit_cnt == 0) {
		/* Remove invalid widgets */
		gl_del_invalid_widgets(data, GL_INVALID_NONE);
		gl_dbgW("None editable albums, show albums view!");
		_gl_albums_change_mode(data, false);
		return 0;
	}

	/* Album selected for rename was deleted */
	if (ad->albuminfo.selected == NULL) {
		gl_dbgW("Rename invalid album!");
		/* Remove rename view */
		elm_naviframe_item_pop(ad->maininfo.naviframe);
		return 0;
	}

	/* Cluster list updated, clear view */
	elm_gengrid_clear(ad->albuminfo.view);
	return 0;
}

int _gl_albums_rename_update_lang(void *data)
{
	GL_CHECK_VAL(data, -1);
	gl_appdata *ad = (gl_appdata *)data;
	GL_CHECK_VAL(ad->entryinfo.nf_it, -1);

	if (ad->popupinfo.popup) {
		char *popup_desc = (char *)calloc(GL_POPUP_STRING_MAX,
						 sizeof(char));
		if (popup_desc) {
			snprintf(popup_desc, GL_POPUP_STRING_MAX, "%s<br>%s",
					 GL_STR_SAME_NAME_ALREADY_IN_USE, GL_STR_RETRY_Q);
			popup_desc[strlen(popup_desc)] = '\0';
			gl_popup_create_popup(ad, GL_POPUP_ALBUM_RENAME_DUPLICATE,
								  popup_desc);
			GL_FREE(popup_desc);
		}
	}

	return 0;
}

