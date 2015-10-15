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
#include "gl-albums-new.h"
#include "gl-entry.h"
#include "gl-thumbs.h"
#include "gl-ui-util.h"
#include "gl-util.h"
#include "gl-data.h"
#include "gl-controlbar.h"
#include "gl-editfield.h"
#include "gl-popup.h"
#include "gl-strings.h"
#include "gl-fs.h"
#include "gl-notify.h"
#include "gl-file-util.h"

static int __gl_albums_new_pop_view(void *data)
{
	GL_CHECK_VAL(data, -1);
	gl_appdata *ad = (gl_appdata *)data;
	gl_dbg("");

	int view_mode = gl_get_view_mode(ad);
	if (ad->albuminfo.b_new_album) {
		ad->albuminfo.b_new_album = false;
	}  else if (view_mode == GL_VIEW_THUMBS_EDIT) {
		_gl_notify_check_selall(ad, ad->gridinfo.nf_it,
					ad->gridinfo.count,
					_gl_data_selected_list_count(ad));
		/* Update the label text */
		_gl_thumbs_update_label_text(ad->gridinfo.nf_it,
					     _gl_data_selected_list_count(ad),
					     false);
	}

	return 0;
}

/* dont call elm_naviframe_item_pop in this callback */
static Eina_Bool __gl_albums_new_pop_cb(void *data, Elm_Object_Item *it)

{
	/* If the back button is clicked or H/W Back key is pressed,
	this callback will be called right before the
	current view pop. User may implement application logic here. */

	/* To proceed the popping, please return the EINA_TRUE or not,
	popping will be canceled.
	If it needs to delete the current view without any transition effect,
	then call the elm_object_item_del() here and then return the EINA_FALSE */

	gl_dbg("");
	__gl_albums_new_pop_view(data);
	return EINA_TRUE;
}

int _gl_albums_new_create_view(void *data, void *op_func)
{
	GL_CHECK_VAL(data, -1);
	gl_appdata *ad = (gl_appdata *)data;
	gl_dbg("");

	ad->entryinfo.mode = GL_ENTRY_NEW_ALBUM;
	ad->entryinfo.op_cb = op_func;
	/* Create entry view */
	if (_gl_entry_create_view(data, NULL, GL_STR_ID_NEW_ALBUM) < 0) {
		ad->entryinfo.mode = GL_ENTRY_NONE;
		ad->entryinfo.op_cb = NULL;
		return -1;
	}
	/* Set callbacks */
	Evas_Object *entry = _gl_editfield_get_entry(data);
	GL_CHECK_VAL(entry, -1);
	evas_object_data_set(entry, "gl_entry_process_cb_key",
			     _gl_albums_new_process);
	evas_object_data_set(entry, "gl_entry_pop_cb_key",
			     __gl_albums_new_pop_cb);
	return 0;
}

/*
* @param b_enter
*	True: Enter key on Keyboard pressed, False: Button Done clicked
*/
int _gl_albums_new_process(void *data, bool b_enter)
{
	gl_dbg("b_enter: %d", b_enter);
	GL_CHECK_VAL(data, -1);
	gl_appdata *ad = (gl_appdata *)data;

	if (ad->entryinfo.editfield == NULL) {
		gl_dbgE("Edit filed is NULL");
		goto NEW_ALBUM_FAILED;
	}

	Evas_Object *entry = ad->entryinfo.entry;
	char name[GL_ALBUM_NAME_LEN_MAX] = { 0, };

	/* Get utf8 format string */
	if (gl_get_entry_text(entry, name, GL_ALBUM_NAME_LEN_MAX) != 0) {
		gl_dbgE("Get entry text failed!");
		goto NEW_ALBUM_FAILED;
	}

	/* Get valid name */
	if (_gl_validate_album_name(data, name) != 0) {
		if (b_enter)
			_gl_editfield_hide_imf(ad);
		return -1;
	}
	gl_sdbg("Valid album name: %s", name);

	/* Check whether the new folder exist */
	char new_path[GL_DIR_PATH_LEN_MAX] = { 0, };

	/* Make dir full new_path of new album */
	snprintf(new_path, GL_DIR_PATH_LEN_MAX, "%s/%s", GL_DEFAULT_PATH_IMAGES,
		 name);
	gl_sdbg("New dir new_path: %s", new_path);
#ifdef _RENAME_ALBUM_SENSITIVE
	if (_gl_fs_check_name_case(new_path, name))
#endif
	{
		memset(new_path, 0x00, GL_DIR_PATH_LEN_MAX);
		snprintf(new_path, GL_DIR_PATH_LEN_MAX, "%s/%s",
			 GL_DEFAULT_PATH_IMAGES, name);
		gl_sdbg("New dir new_path: %s", new_path);
		int res = gl_file_dir_is_empty(new_path);
		/**
		* If dir is empty, 1 is returned,
		* if it contains at least 1 file, 0 is returned.
		* On failure, -1 is returned.
	 	*/
		gl_dbg("file_dir_is_empty return value: %d", res);
		if (res == 0) {
			gl_dbgW("New folder already exists!");
			_gl_popup_create_local(data, GL_POPUP_NOBUT,
					       GL_STR_ID_SAME_NAME_ALREADY_IN_USE);
			return -1;
		}
	}

	/* Save new album name */
	char *new_album = ad->albuminfo.new_name;
	g_strlcpy(new_album, name, GL_ALBUM_NAME_LEN_MAX);
	new_album[GL_ALBUM_NAME_LEN_MAX - 1] = '\0';

	if (ad->albuminfo.view)
		elm_gengrid_clear(ad->albuminfo.view);

	if (ad->entryinfo.op_cb) {
		int (*mc_cb)(void *data);
		/* Get operation funciton */
		mc_cb = ad->entryinfo.op_cb;
		/* Move/Save/Copy files to dest album */
		mc_cb(ad);
	}
	GL_IF_DEL_OBJ(ad->popupinfo.popup);
	return 0;

NEW_ALBUM_FAILED:
	GL_IF_DEL_OBJ(ad->popupinfo.popup);
	return -1;
}

