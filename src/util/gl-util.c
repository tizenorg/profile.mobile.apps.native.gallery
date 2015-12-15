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

#include <dlfcn.h>
#ifdef _USE_SVI
#include <svi.h>
#endif
#include <media_content.h>
#include <system_settings.h>
#include <storage.h>
#include "gl-debug.h"
#include "gallery.h"
#include "gl-albums.h"
#include "gl-thumbs.h"
#include "gl-util.h"
#include "gl-ui-util.h"
#include "gl-controlbar.h"
#include "gl-thread-util.h"
#include "gl-popup.h"
#include "gl-ctxpopup.h"
#include "gl-progressbar.h"
#include "gl-ext-ug-load.h"
#include "gl-ext-exec.h"
#include "gl-pinchzoom.h"
#include "gl-albums-sel.h"
#include "gl-thumbs-sel.h"
#include "gl-thumbs-edit.h"
#include "gl-notify.h"
#include "gl-editfield.h"
#include "gl-strings.h"
#include "gl-icons.h"
#include "gl-data-type.h"
#include "gl-fs.h"
#include "gl-db-update.h"
#ifdef _USE_ROTATE_BG
#include "gl-exif.h"
#endif
#include "gl-timeline.h"
#include "gl-file-util.h"
#include "gl-thumbs.h"
#include "gl-data.h"

#define GL_FILE_COPY_BUF_MAX 16384
/* Time for displaying progressbar UI compeletely */
#define GL_TIMER_INTERVAL_PBAR_SHOWED 0.5
#define GL_TIMER_INTERVAL_VIBRATION 0.5
#ifdef _USE_ROTATE_BG
#define GL_ROTATE_DELAY 0.25
#endif
#define GL_SO_PATH_IMAGEVIEWER "/usr/ug/lib/libug-image-viewer-efl.so"

/* Use timer to show progressbar totally before write pipe */
static Eina_Bool _gl_start_thread_op_timer_cb(void *data)
{
	gl_dbg("Emit START signal...");
	GL_CHECK_CANCEL(data);
	gl_appdata *ad = (gl_appdata *)data;
	/* Emit signal to notice child thread to handle next item. */
	gl_thread_emit_next_signal(ad);
	GL_IF_DEL_TIMER(ad->pbarinfo.start_thread_timer);
	return ECORE_CALLBACK_CANCEL;
}

/**
* Use thread to move/copy/delete/add tag to/remove tag from medias
* for displaying progrssabar UI
*/
int _gl_use_thread_operate_medias(void *data, char *pbar_title, int all_cnt,
                                  int op_type)
{
	gl_dbg("all_cnt: %d, op_type: %d.", all_cnt, op_type);
	GL_CHECK_VAL(data, -1);
	gl_appdata *ad = (gl_appdata *)data;

	/* Initialize thread mutex lock */
	gl_thread_init_lock(ad);
	/* Make progressbar */
	gl_pb_make_thread_pbar_wheel(ad, ad->maininfo.win, pbar_title);
	/* Set pb_cancel, indicates thread operation is beginning */
	gl_thread_set_cancel_state(ad, GL_PB_CANCEL_NORMAL);
	/* Initialize progressbar */
	gl_pb_refresh_thread_pbar(ad, 0, all_cnt);
	/* Use timer to emit next signal to show progressbarbar totally */
	GL_IF_DEL_TIMER(ad->pbarinfo.start_thread_timer);
	Ecore_Timer *timer = NULL;
	timer = ecore_timer_add(GL_TIMER_INTERVAL_PBAR_SHOWED,
	                        _gl_start_thread_op_timer_cb, ad);
	ad->pbarinfo.start_thread_timer = timer;
	/* Set media operation type */
	ad->maininfo.medias_op_type = op_type;
	/* Generate child thread */
	gl_thread_gen_data_thread(ad);
	return 0;
}

bool __gl_util_get_supported_storages_callback(int storageId, storage_type_e type, storage_state_e state, const char *path, void *data)
{
	GL_CHECK_FALSE(data);

	gl_appdata *ad = (gl_appdata *)data;
	if (type == STORAGE_TYPE_EXTERNAL) {
		ad->maininfo.externalStorageId = storageId;
		return false;
	}
	return true;
}

static Eina_Bool __gl_db_update_noti_idler_cb(void *data)
{
	GL_CHECK_FALSE(data);
	gl_appdata *ad = (gl_appdata *)data;
	gl_dbgW("Storage state changed!");
	int error_code = -1;

	error_code = storage_foreach_device_supported(__gl_util_get_supported_storages_callback, ad);
	if (error_code == STORAGE_ERROR_NONE) {
		storage_state_e state;
		error_code = storage_get_state(ad->maininfo.externalStorageId, &state);
		if (error_code == STORAGE_ERROR_NONE) {
			if (state == STORAGE_STATE_MOUNTED) {
				gl_dbg("state[%d] : STORAGE_STATE_MOUNTED", state);
				if (ad->maininfo.mmc_state == GL_MMC_STATE_REMOVED) {
					ad->maininfo.mmc_state = GL_MMC_STATE_ADDING_MOVING;
				} else {
					ad->maininfo.mmc_state = GL_MMC_STATE_ADDED;
				}

				gl_update_view(ad, GL_UPDATE_VIEW_MMC_ADDED);
				//evas_object_smart_callback_call(ad->maininfo.naviframe,
				//				"gallery,db,data,updated", data);
				/* Remove ctxpopup */
				_gl_ctxpopup_del(ad);
			} else if (state == STORAGE_STATE_REMOVED ||
			           state == STORAGE_STATE_UNMOUNTABLE) {
				gl_dbg("state[%d] : STORAGE_STATE_REMOVED", state);
				if (ad->maininfo.mmc_state == GL_MMC_STATE_ADDED_MOVING) {
					/*
					 * Set pb_cancel status,
					 * indicates MMC removed while moving or deleting.
					 */
					gl_thread_set_cancel_state(ad, GL_PB_CANCEL_MMC);
					/* MMC moved while moving or deleting */
					ad->maininfo.mmc_state = GL_MMC_STATE_REMOVED_MOVING;
				} else if (ad->maininfo.mmc_state == GL_MMC_STATE_REMOVED_MOVING) {
					gl_dbgW("View updated on GL_MMC_STATE_ADDED_MOVING");
					GL_IF_DEL_IDLER(ad->maininfo.mmc_idler);
					return ECORE_CALLBACK_CANCEL;
				} else {
					/* MMC is extracted */
					ad->maininfo.mmc_state = GL_MMC_STATE_REMOVED;
				}
				gl_update_view(ad, GL_UPDATE_VIEW_MMC_REMOVED);
				//evas_object_smart_callback_call(ad->maininfo.naviframe,
				//				"gallery,db,data,updated", data);
				/* Remove ctxpopup */
				_gl_ctxpopup_del(ad);
			}
		}
	} else {
		gl_dbgE("Error when check MMC Status");
	}
	GL_IF_DEL_IDLER(ad->maininfo.mmc_idler);
	return ECORE_CALLBACK_CANCEL;
}

void _gl_db_update_sdcard_info(int storage_id, storage_state_e state, void *user_data)
{

	GL_CHECK(user_data);
	gl_appdata *ad = (gl_appdata *)user_data;
	GL_IF_DEL_IDLER(ad->maininfo.mmc_idler);
	gl_dbg("mmc_idler: %p", ad->maininfo.mmc_idler);
	ad->maininfo.mmc_idler = ecore_idler_add(__gl_db_update_noti_idler_cb,
	                         user_data);
}

static inline int __gl_set_sound_status(void *data, int status)
{
	GL_CHECK_VAL(data, -1);
	gl_appdata *ad = (gl_appdata *)data;
	ad->maininfo.sound_status = status;
	return 0;
}

static int __gl_get_sound_status(void *data)
{
	GL_CHECK_VAL(data, -1);

	bool check = false;
	int retcode = system_settings_get_value_bool(SYSTEM_SETTINGS_KEY_SOUND_SILENT_MODE, &check);
	if (retcode != SYSTEM_SETTINGS_ERROR_NONE) {
		gl_dbgE("Failed to get sound status!");
		return -1;
	}

	gl_dbg("sound: %d", check);
	__gl_set_sound_status(data, (check == true) ? 0 : 1);
	return 0;
}

static void __gl_change_sound_status_cb(system_settings_key_e key, void *data)
{
	__gl_get_sound_status(data);
}

static int __gl_reg_svi_noti(void *data)
{
	GL_CHECK_VAL(data, -1);
	int retcode = system_settings_set_changed_cb(SYSTEM_SETTINGS_KEY_SOUND_SILENT_MODE,
	              __gl_change_sound_status_cb, data);
	if (retcode != SYSTEM_SETTINGS_ERROR_NONE) {
		gl_dbgE("Failed to register the sound status callback");
		return -1;
	}
	return 0;
}

static int __gl_dereg_svi_noti(void)
{
	int retcode = system_settings_unset_changed_cb(SYSTEM_SETTINGS_KEY_SOUND_SILENT_MODE);
	if (retcode != SYSTEM_SETTINGS_ERROR_NONE) {
		gl_dbgE("Failed to un-register the sound status callback");
		return -1;
	}
	return 0;
}

/* Check if there is any MMC file contained in select_medias_elist */
static bool _gl_check_mmc_file_selected(void *data)
{
	gl_dbg("");
	GL_CHECK_FALSE(data);
	gl_appdata *ad = (gl_appdata *)data;
	gl_item *gitem = NULL;
	int i = 0;
	int on_mmc = -1;
	int cnt = _gl_data_selected_list_count(ad);
	for (i = 0; i < cnt; i++) {
		gitem = NULL;
		on_mmc = -1;

		gitem = _gl_data_selected_list_get_nth(ad, i);
		/* File on MMC is selected */
		if (gitem && gitem->item && gitem->item->file_url) {
			char *mmc_root_path = _gl_get_root_directory_path(STORAGE_TYPE_EXTERNAL);
			GL_CHECK_FALSE(mmc_root_path);
			on_mmc = strncmp(mmc_root_path,
			                 gitem->item->file_url,
			                 strlen(mmc_root_path));
			GL_GFREEIF(mmc_root_path);
			if (on_mmc == 0) {
				return true;
			}
		}
	}
	return false;
}

/* Copy thumbnail and file */
static Eina_Bool __gl_file_cp(void *data, const char *src, const char *dst)
{
	GL_CHECK_FALSE(src);
	GL_CHECK_FALSE(dst);
	/* Copy file first */
	if (_gl_fs_copy(data, src, dst)) {
		if (_gl_local_data_add_media(dst, NULL) < 0) {
			gl_dbgE("Copy media thumbnail failed!");
			return EINA_FALSE;
		}
	} else {
		gl_dbgE("Copy file failed!");
		return EINA_FALSE;
	}
	return EINA_TRUE;
}

/* Move thumbnail and file */
static Eina_Bool __gl_file_mv(void *data, gl_item *gitem, const char *src, const char *dst, int mc_mode)
{
	GL_CHECK_FALSE(src);
	GL_CHECK_FALSE(dst);
	GL_CHECK_FALSE(gitem);
	GL_CHECK_FALSE(data);
	/* Move file first */
	if (_gl_fs_move(data, src, dst)) {
		if (_gl_local_data_move_media(gitem->item, dst) < 0) {
			gl_dbgE("Move media thumbnail failed!");
			return EINA_FALSE;
		}
	} else {
		gl_dbgE("Move file failed!");
		return EINA_FALSE;
	}

	return EINA_TRUE;
}

int gl_get_entry_text(Evas_Object * entry, char *entry_text, int len_max)
{
	gl_dbg("");
	GL_CHECK_VAL(entry_text, -1);
	GL_CHECK_VAL(entry, -1);

	memset(entry_text, 0x00, len_max);

	char *entry_str = (char *)elm_entry_entry_get(entry);
	if (entry_str) {
		if (strlen(entry_str) == 0) {
			gl_dbg("Entry string is empty!");
		} else {
			char *entry_utf8 = elm_entry_markup_to_utf8(entry_str);	// changes user input to utf-8 encoding format
			if (entry_utf8 == NULL) {
				gl_dbgE("Make entry string to UTF8 failed!");
				return -1;
			} else if (strlen(entry_utf8) == 0) {
				gl_dbg("Entry text is empty!");
			} else {
				g_strlcpy(entry_text, entry_utf8, len_max);
				entry_text[len_max - 1] = '\0';
			}

			GL_FREE(entry_utf8);
		}
	} else {
		gl_dbgE("Get entry string failed!");
		return -1;
	}

	return 0;
}

/**
* Change utf8 string to markup, then set it as entry text.
* To fix special character "&" issue,  to convert "&" to "&amp;". f.e.
* Myfiles->rename folder to AB&C->Gallery/Rename ->displayed as "AB"
*/
int gl_set_entry_text(Evas_Object *entry, char *entry_text)
{
	GL_CHECK_VAL(entry, -1);
	char *entry_makeup = NULL;
	gl_dbg("Entry UTF8 text: %s.", entry_text);

	if (entry_text) {
		entry_makeup = elm_entry_utf8_to_markup(entry_text);
		if (entry_makeup == NULL) {
			gl_dbgE("Make utf8 string to makeup failed!");
			return -1;
		}
		gl_dbg("Entry makeup text: %s.", entry_makeup);
	}
	/* Empty string set if entry_makeup is NULL */
	elm_entry_entry_set(entry, entry_makeup);

	GL_FREEIF(entry_makeup);
	return 0;
}

/**
* Check album name is valid and remove unuseful characters
* 	1) not only includes space;
*	2) contains invalid character;
*	3) it's empty.
*
* @param b_new
*	true: add a new album ; false: rename a album.
* @param b_enter
*	true: Enter key pressed ; false: Button Done clicked.
*
*/
int _gl_get_valid_album_name(void *data, char *album_name, bool b_new,
                             bool b_enter)
{
	GL_CHECK_VAL(album_name, -1);
	GL_CHECK_VAL(data, -1);
	gl_appdata *ad = (gl_appdata *)data;
	int mode = 0;
	gl_sdbg("Input album name: %s", album_name);
	char *popup_desc;
	popup_desc = (char *)calloc(GL_POPUP_STRING_MAX, sizeof(char));
	if (popup_desc == NULL) {
		gl_dbg("memory allocation fail");
		return -1;
	}
	/* Check album name length */
	if (strlen(album_name) == 0) {
		gl_dbgW("Inserted text is empty!");
		if (b_enter) {
			gl_dbg("Enter key pressed.");
			goto INVALID_ALBUM_NAME;
		}

		if (b_new) {
			mode = GL_POPUP_ALBUM_NEW_EMPTY;
		} else {
			mode = GL_POPUP_ALBUM_RENAME_EMPTY;
		}
		snprintf(popup_desc, GL_POPUP_STRING_MAX, "%s<br>%s",
		         GL_STR_ENTRY_IS_EMPTY, GL_STR_RETRY_Q);
		gl_popup_create_popup(ad, mode, popup_desc);
		goto INVALID_ALBUM_NAME;
	}
	gl_sdbg("Inserted album name: %s, length: %d", album_name, strlen(album_name));

	/* Removes leading and trailing whitespace */
	g_strstrip((gchar *)album_name);
	if (strlen(album_name) == 0) {
		gl_dbgW("Album name includes only space!");
		if (b_enter) {
			gl_dbg("Enter key pressed.");
			goto INVALID_ALBUM_NAME;
		}

		if (b_new) {
			mode = GL_POPUP_ALBUM_NEW_EMPTY;
		} else {
			mode = GL_POPUP_ALBUM_RENAME_EMPTY;
		}
		snprintf(popup_desc, GL_POPUP_STRING_MAX, "%s<br>%s",
		         GL_STR_INVALID_INPUT_PARAMETER, GL_STR_RETRY_Q);
		gl_popup_create_popup(ad, mode, popup_desc);
		goto INVALID_ALBUM_NAME;
	}

	/* Check if there is any invalid character in string */
	if (_gl_fs_validate_name(album_name) == false) {
		gl_dbgW("Album name includes invalid character!");
		if (b_new) {
			mode = GL_POPUP_ALBUM_NEW_INVALID;
		} else {
			mode = GL_POPUP_ALBUM_RENAME_INVALID;
		}
		snprintf(popup_desc, GL_POPUP_STRING_MAX, "%s<br>%s",
		         GL_STR_INVALID_INPUT_PARAMETER, GL_STR_RETRY_Q);
		gl_popup_create_popup(ad, mode, popup_desc);
		goto INVALID_ALBUM_NAME;
	}

	GL_FREE(popup_desc);

	return 0;

INVALID_ALBUM_NAME:
	GL_FREE(popup_desc);
	return -1;
}

int _gl_validate_album_name(void *data, char *album_name)
{
	GL_CHECK_VAL(album_name, -1);
	GL_CHECK_VAL(data, -1);
	gl_sdbg("Input album name: %s", album_name);

	/* Check album name length */
	if (strlen(album_name) == 0) {
		gl_dbgW("Inserted text is empty!");
		_gl_popup_create_local(data, GL_POPUP_NOBUT, GL_STR_ID_ENTRY_IS_EMPTY);
		goto INVALID_ALBUM_NAME;
	}

	/* Removes leading and trailing whitespace */
	g_strstrip((gchar *)album_name);
	gl_sdbg("Inserted album name: %s, length: %d", album_name, strlen(album_name));

	if (strlen(album_name) == 0) {
		gl_dbgW("Album name includes only space!");
		_gl_popup_create_local(data, GL_POPUP_NOBUT, GL_STR_ID_INVALID_INPUT_PARAMETER);
		goto INVALID_ALBUM_NAME;
	}

	/* Check if there is any invalid character in string */
	if (_gl_fs_validate_name(album_name) == false) {
		gl_dbgW("Album name includes invalid character!");
		_gl_popup_create_local(data, GL_POPUP_NOBUT, GL_STR_ID_INVALID_INPUT_PARAMETER);
		goto INVALID_ALBUM_NAME;
	}

	return 0;

INVALID_ALBUM_NAME:

	return -1;
}

int _gl_validate_input_character(void *data, char *album_name)
{
	GL_CHECK_VAL(album_name, -1);
	GL_CHECK_VAL(data, -1);
	gl_sdbg("Input album name: %s", album_name);

	/* Check if there is any invalid character in string */
	if (_gl_fs_validate_character(album_name[strlen(album_name) - 1]) == false) {
		gl_dbgW("Album name includes invalid character!");
		_gl_popup_create_local(data, GL_POPUP_NOBUT, GL_STR_ID_INVALID_INPUT_PARAMETER);
		goto INVALID_ALBUM_NAME;
	}

	return 0;

INVALID_ALBUM_NAME:

	return -1;
}

/* *
* In case of system folder, the displayed name should be translated into system language
*
* @param album
*    check album display name for getting proper translation
*
* @return
*    the translated album display name
*/
char *_gl_get_i18n_album_name(gl_album_s *cluster)
{
	GL_CHECK_NULL(cluster);
	char *i18n_name = NULL;

	if (_gl_data_is_camera_album(cluster)) {
		/* system folder name: Camera */
		i18n_name = GL_STR_ID_CAMERA;
	} else if (_gl_data_is_default_album(GL_STR_ALBUM_DOWNLOADS, cluster)) {
		/* system folder name: Downloads */
		i18n_name = GL_STR_ID_DOWNLOADS;
	} else if (_gl_data_is_screenshot_album(GL_STR_ALBUM_SCREENSHOTS, cluster)) {
		/* system folder name: Screenshots */
		i18n_name = GL_STR_ID_SCREENSHOTS;
	} else if (cluster->type == GL_STORE_T_ALL) {
		GL_FREEIF(cluster->display_name);
		cluster->display_name = strdup(GL_STR_ID_ALL_ALBUMS);
		i18n_name = cluster->display_name;
	} else if (cluster->type == GL_STORE_T_PHONE ||
	           cluster->type == GL_STORE_T_MMC) {
		if (_gl_data_is_root_path(cluster->path)) {
			/* check root case */
			i18n_name = GL_STR_ID_NO_NAME;
		} else {
			/* if the folder is not a system folder, return itself */
			i18n_name = cluster->display_name;
		}
	} else {
		/* if the folder is not a system folder, return itself */
		i18n_name = cluster->display_name;
	}

	if (i18n_name == NULL || strlen(i18n_name) <= 0) {
		i18n_name = GL_STR_ID_NO_NAME;
	}

	return i18n_name;
}

int gl_get_selected_files_path_str(void *data, gchar sep_c, char **path_str,
                                   int *sel_cnt)
{
	GL_CHECK_VAL(path_str, -1);
	GL_CHECK_VAL(data, -1);
	gl_appdata *ad = (gl_appdata *)data;
	GL_CHECK_VAL(ad->selinfo.elist, -1);
	GString *selected_path_list = g_string_new(NULL);
	gl_item *current = NULL;
	int selected_item_cnt = 0;
	Eina_List *l = NULL;

	EINA_LIST_FOREACH(ad->selinfo.elist, l, current) {
		if (current == NULL || current->item == NULL) {
			if (selected_path_list) {
				g_string_free(selected_path_list, true);
				selected_path_list = NULL;
			}
			return -1;
		}

		if (strstr(selected_path_list->str, current->item->file_url)) {
			gl_dbgW("Already appended!");
		} else {
			selected_item_cnt++;
			g_string_append(selected_path_list,
			                current->item->file_url);
			g_string_append_c(selected_path_list, sep_c);
		}

	}
	gl_dbg("Selected items count: %d.", selected_item_cnt);
	if (sel_cnt) {
		*sel_cnt = selected_item_cnt;
	}
	int len = strlen(selected_path_list->str);
	g_string_truncate(selected_path_list, len - 1);
	/**
	* Frees the memory allocated for the GString.
	* If free_segment is true it also frees the character data.
	* If it's false, the caller gains ownership of the buffer
	* and must free it after use with g_free().
	*/
	*path_str = g_string_free(selected_path_list, false);
	GL_CHECK_VAL(*path_str, -1);
	gl_dbg("Total string:\n\n\t>>@@:> %s <:@@<<\n", *path_str);
	return 0;
}

bool gl_make_new_album(const char *name)
{
	gl_dbg("");
	char path[GL_DIR_PATH_LEN_MAX] = { 0, };

	char *default_images_path = _gl_get_directory_path(STORAGE_DIRECTORY_IMAGES);
	GL_CHECK_FALSE(default_images_path);

	snprintf(path, GL_DIR_PATH_LEN_MAX, "%s/%s", default_images_path, name);
	path[strlen(path)] = '\0';
	gl_sdbg("Making %s directory", path);

	_gl_fs_mkdir(default_images_path);
	bool isSuccessful = _gl_fs_mkdir(path);
	GL_FREE(default_images_path);
	return isSuccessful;
}

/* Used for loal medias, thumbnail was moved to new dest first, then rename file */
int _gl_move_media_thumb(void *data, gl_item *gitem, char *new_dir_name,
                         bool is_full_path, int *popup_op, int mc_mode)
{
	gl_dbg("Move/Copy[%d] media item START>>>", mc_mode);
	char new_path[GL_FILE_PATH_LEN_MAX] = { 0, };
	char ext[GL_FILE_EXT_LEN_MAX] = { 0, };
	char new_path_noext[GL_FILE_PATH_LEN_MAX] = { 0, };

	if (gitem == NULL || gitem->item == NULL || popup_op == NULL) {
		gl_dbgE("Invalid parameters!");
		return -1;
	}

	*popup_op = GL_POPUP_OP_NONE;

	if (is_full_path) {
		if (!gitem->item->file_url || !strlen(gitem->item->file_url) ||
		        !gitem->item->display_name) {
			gl_dbgE("Invalid file!");
			return -1;
		}
		snprintf(new_path, sizeof(new_path), "%s/%s", new_dir_name,
		         (char *)(gitem->item->display_name));
		gl_sdbg("New path : %s", new_path);

		if (mc_mode == GL_MC_MOVE && !g_strcmp0(new_path, gitem->item->file_url)) {
			*popup_op = GL_POPUP_OP_SAME_ALBUM;
			gl_dbg("File already in destination");
			return 0;
		} else if (gl_file_exists(new_path)) { /* return true if file exists, rename new file */
			_gl_fs_get_path_without_ext(new_path, ext, new_path_noext);
			char *final_path = NULL;
			final_path = _gl_fs_get_unique_full_path(new_path_noext, ext);
			if (final_path == NULL) {
				return -1;
			}
			gl_dbg("Created unique path: %s", final_path);
			memset(new_path, 0x00, GL_FILE_PATH_LEN_MAX);
			g_strlcpy(new_path, final_path, GL_FILE_PATH_LEN_MAX);
			GL_FREE(final_path);
			*popup_op = GL_POPUP_OP_DUPLICATED_NAME;
		}
	} else {
		char *default_images_path = _gl_get_directory_path(STORAGE_DIRECTORY_IMAGES);
		GL_CHECK_VAL(default_images_path, -1);
		snprintf(new_path, GL_FILE_PATH_LEN_MAX, "%s/%s/%s",
				default_images_path, new_dir_name,
		         (char *)(gitem->item->display_name));
		GL_FREE(default_images_path);
	}

	new_path[strlen(new_path)] = '\0';

	Eina_Bool ret = EINA_FALSE;
	switch (mc_mode) {
	case GL_MC_MOVE:
		/* stop using "rename" when moving from mmc to phone for correct db update */
		ret = __gl_file_mv(data, gitem, gitem->item->file_url, new_path, mc_mode);
		break;
	case GL_MC_COPY:
		ret = __gl_file_cp(gitem->ad, gitem->item->file_url, new_path);
		break;
	default:
		gl_dbgE("Wrong mode!");
	}

	gl_dbg("Move/Copy media item OVER<<<");
	if (!ret) {
		return -1;
	} else {
		return 0;
	}
}

/* Used for loal medias, thumbnail was moved to new dest first, then rename file */
int _gl_move_media_thumb_by_id(void *data, const char *uuid, char *new_dir_name,
                               int *popup_op, int mc_mode)
{
	gl_dbg("Move/Copy[%d] media item START>>>", mc_mode);
	char new_path[GL_FILE_PATH_LEN_MAX] = { 0, };
	char ext[GL_FILE_EXT_LEN_MAX] = { 0, };
	char new_path_noext[GL_FILE_PATH_LEN_MAX] = { 0, };

	if (uuid == NULL || popup_op == NULL) {
		gl_dbgE("Invalid parameters!");
		return -1;
	}

	*popup_op = GL_POPUP_OP_NONE;
	media_info_h media_h = NULL;
	int ret = -1;
	char *path = NULL;
	char *name = NULL;

	ret = media_info_get_media_from_db(uuid, &media_h);
	if (ret != MEDIA_CONTENT_ERROR_NONE) {
		gl_dbgE("Get media failed[%d]!", ret);
		goto GL_DONE;
	}
	ret = media_info_get_display_name(media_h, &name);
	if (ret != MEDIA_CONTENT_ERROR_NONE) {
		gl_dbgE("Get storage type failed!");
		goto GL_DONE;
	}
	ret = media_info_get_file_path(media_h, &path);
	if (ret != MEDIA_CONTENT_ERROR_NONE) {
		gl_dbgE("Get media file path failed!");
		goto GL_DONE;
	}

	snprintf(new_path, sizeof(new_path), "%s/%s", new_dir_name, name);
	gl_sdbg("New path : %s", new_path);

	/* Don't move, in 'All albums' */
	if (!g_strcmp0(new_path, path)) {
		*popup_op = GL_POPUP_OP_SAME_ALBUM;
		gl_dbg("File already in destination");
		ret = 0;
		goto GL_DONE;
	} else if (gl_file_exists(new_path)) { /* return true if file exists, rename new file */
		_gl_fs_get_path_without_ext(new_path, ext, new_path_noext);
		char *final_path = NULL;
		final_path = _gl_fs_get_unique_full_path(new_path_noext, ext);
		if (final_path == NULL) {
			goto GL_DONE;
		}
		gl_dbg("Created unique path: %s", final_path);
		memset(new_path, 0x00, GL_FILE_PATH_LEN_MAX);
		g_strlcpy(new_path, final_path, GL_FILE_PATH_LEN_MAX);
		GL_FREE(final_path);
		*popup_op = GL_POPUP_OP_DUPLICATED_NAME;
	}

	new_path[strlen(new_path)] = '\0';

	switch (mc_mode) {
	case GL_MC_MOVE:
		if (!_gl_fs_move(data, path, new_path)) {
			ret = -1;
			goto GL_DONE;
		}
		ret = media_info_move_to_db(media_h, new_path);
		break;
	case GL_MC_COPY:
		if (!__gl_file_cp(data, path, new_path)) {
			ret = -1;
			goto GL_DONE;
		}
		break;
	default:
		gl_dbgE("Wrong mode!");
	}

GL_DONE:
	GL_FREEIF(name);
	GL_FREEIF(path);
	if (media_h) {
		media_info_destroy(media_h);
	}

	gl_dbg("Move/Copy media item OVER<<<");
	if (ret != MEDIA_CONTENT_ERROR_NONE) {
		return -1;
	} else {
		return 0;
	}
}

/* Used for move file to new dest */
int _gl_move_media(gl_item *gitem, char *new_dir_name, bool is_full_path)
{
	gl_dbg("Move media START>>>");
	char new_path[GL_FILE_PATH_LEN_MAX] = { 0, };
	char ext[GL_FILE_EXT_LEN_MAX] = { 0, };
	char new_path_noext[GL_FILE_PATH_LEN_MAX] = { 0, };
	if (!gitem || !gitem->item) {
		gl_dbgE("Invalid gitem!");
		return -1;
	}

	if (is_full_path) {
		if (!gitem->item->file_url || !strlen(gitem->item->file_url) ||
		        !gitem->item->display_name) {
			gl_dbgE("file url or name is invalid!");
			return -1;
		}
		snprintf(new_path, sizeof(new_path), "%s/%s",
		         new_dir_name, (char *)(gitem->item->display_name));
		gl_dbg("New path : %s", new_path);

		if (!g_strcmp0(new_path, gitem->item->file_url)) {
			gl_dbgW("File exists!");
			return 0;
		} else if (gl_file_exists(new_path)) {
			_gl_fs_get_path_without_ext(new_path, ext, new_path_noext);
			char *final_path = NULL;
			final_path = _gl_fs_get_unique_full_path(new_path_noext, ext);
			if (final_path == NULL) {
				return -1;
			}
			gl_dbg("Created unique path: %s", final_path);
			memset(new_path, 0x00, GL_FILE_PATH_LEN_MAX);
			g_strlcpy(new_path, final_path, GL_FILE_PATH_LEN_MAX);
			GL_FREE(final_path);
		}
	} else {
		char *default_images_path = _gl_get_directory_path(STORAGE_DIRECTORY_IMAGES);
		GL_CHECK_VAL(default_images_path, -1);
		snprintf(new_path, GL_FILE_PATH_LEN_MAX, "%s/%s/%s",
				default_images_path, new_dir_name,
		         (char *)(gitem->item->display_name));
		GL_FREE(default_images_path);
	}

	new_path[strlen(new_path)] = '\0';

	if (!_gl_fs_move(gitem->ad, gitem->item->file_url, new_path)) {
		gl_dbg("Move media OVER<<<");
		return -1;
	} else {
		if (_gl_local_data_add_media(new_path, NULL) < 0) {
			gl_dbgE("Move media thumbnail failed!");
			return EINA_FALSE;
		}
		gl_dbg("Move media OVER<<<");
		return 0;
	}
}

/**
* Check if there is any media in Gallery.
*
* False returned if none medias exist, other return True.
*/
bool gl_check_gallery_empty(void *data)
{
	GL_CHECK_FALSE(data);
	gl_appdata *ad = (gl_appdata *)data;

	if (ad->albuminfo.elist == NULL) {
		gl_dbgE("ad->albuminfo.elist is empty!");
		return true;
	}

	Eina_List *clist = ad->albuminfo.elist->clist;
	if (clist == NULL) {
		gl_dbgW("Albums list is invalid!");
		return true;
	}

	int len = eina_list_count(clist);
	if (len == 0) {
		gl_dbgW("Albums list is empty!");
		return true;
	}
	return false;
}

bool gl_is_image_valid(void *data, char *filepath)
{
	GL_CHECK_FALSE(data);
	gl_appdata *ad = (gl_appdata *)data;
	GL_CHECK_FALSE(filepath);

	Evas_Object *image = NULL;
	int width = 0;
	int height = 0;
	Evas *evas = NULL;

	evas = evas_object_evas_get(ad->maininfo.win);
	GL_CHECK_FALSE(evas);

	image = evas_object_image_add(evas);
	GL_CHECK_FALSE(image);

	evas_object_image_filled_set(image, 0);
	evas_object_image_load_scale_down_set(image, 0);
	evas_object_image_file_set(image, filepath, NULL);
	evas_object_image_size_get(image, &width, &height);
	evas_object_del(image);
	image = NULL;

	if (width <= 0 || height <= 0) {
		gl_sdbg("Cannot load file : %s", filepath);
		return false;
	}

	return true;
}

/*
* Check MMC state(Inserted/Removed) for Move/Delete/Add tag/Remove.
*/
int gl_check_mmc_state(void *data, char *dest_folder)
{
	GL_CHECK_VAL(data, -1);
	gl_appdata *ad = (gl_appdata *)data;

	/* MMC hasn't been inserted */
	if (ad->maininfo.mmc_state == GL_MMC_STATE_REMOVED) {
		return 0;
	}

	int on_mmc = -1;
	char *mmc_root_path = _gl_get_root_directory_path(STORAGE_TYPE_EXTERNAL);
	GL_CHECK_VAL(mmc_root_path, -1);
	/**
	* 1, Places: Add tag;
	* 2, Tags: Add tag/remove tag;
	* 3, Albums: Move/Delete/Add tag.
	*/
	/* Move files to MMC album */
	if (dest_folder) {
		on_mmc = strncmp(mmc_root_path, dest_folder,
		                 strlen(mmc_root_path));
		if (on_mmc == 0) {
			goto ON_MMC;
		}
	}

	gl_cluster *cur_album = _gl_albums_get_current(data);
	char src_folder_path[GL_DIR_PATH_LEN_MAX] = { 0, };
	GL_CHECK_VAL(cur_album, -1);
	GL_CHECK_VAL(cur_album->cluster, -1);
	/* Move files from MMC album */
	GL_CHECK_VAL(cur_album->cluster->uuid, -1);
	g_strlcpy(src_folder_path, cur_album->cluster->path,
	          GL_DIR_PATH_LEN_MAX);
	on_mmc = strncmp(mmc_root_path, src_folder_path,
	                 strlen(mmc_root_path));
	/* Check MMC files selected in album [All albums] */
	if (on_mmc == 0) {
		goto ON_MMC;
	} else if (cur_album->cluster->type == GL_STORE_T_ALL) {
		gl_dbg("In album [All albums].");
		if (_gl_check_mmc_file_selected(ad)) {
			goto ON_MMC;
		}
	}
	GL_GFREEIF(mmc_root_path);
	return 0;

ON_MMC:
	gl_dbgW("Operate medias on MMC!");
	ad->maininfo.mmc_state = GL_MMC_STATE_ADDED_MOVING;
	GL_GFREEIF(mmc_root_path);
	return 0;
}

static int __gl_reg_db_noti(void *data)
{
	GL_CHECK_VAL(data, -1);
	gl_appdata *ad = (gl_appdata *)data;
	int error_code = -1;

	error_code = storage_foreach_device_supported(__gl_util_get_supported_storages_callback, ad);
	/* Save the init status of MMC */
	if (error_code == STORAGE_ERROR_NONE) {
		storage_state_e state;
		error_code = storage_get_state(ad->maininfo.externalStorageId, &state);
		if (error_code == STORAGE_ERROR_NONE) {
			if (state == STORAGE_STATE_MOUNTED) {
				gl_dbg("###### :::::: MMC loaded! :::::: ######");
				ad->maininfo.mmc_state = GL_MMC_STATE_ADDED;
			} else if (state == STORAGE_STATE_REMOVED ||
			           state == STORAGE_STATE_UNMOUNTABLE) {
				gl_dbg("###### :::::: MMC removed! :::::: ######");
				ad->maininfo.mmc_state = GL_MMC_STATE_REMOVED;
			}
		}
	}

	gl_dbg("Enable the noti handle for DB update status!");
	/* Set DB update status callback */
	error_code = storage_set_state_changed_cb(ad->maininfo.externalStorageId, _gl_db_update_sdcard_info, ad);
	if (error_code != STORAGE_ERROR_NONE) {
		gl_dbgE("storage_set_state_changed_cb failed!");
	}
	return 0;
}

static int __gl_dereg_db_noti(void *data)
{
	GL_CHECK_VAL(data, -1);
	gl_appdata *ad = (gl_appdata *)data;
	int error_code = -1;

	gl_dbg("Disable the noti handle for DB update status!");
	error_code = storage_unset_state_changed_cb(ad->maininfo.externalStorageId, _gl_db_update_sdcard_info);
	if (error_code != STORAGE_ERROR_NONE) {
		gl_dbgE("storage_unset_state_changed_cb failed!");
	}
	return 0;
}

/*
* Get view mode of app
*/
int gl_get_view_mode(void *data)
{
	GL_CHECK_VAL(data, GL_VIEW_NONE);
	gl_appdata *ad = (gl_appdata *)data;
	return ad->maininfo.view_mode;
}

/*
* Set view mode of app
*/
int gl_set_view_mode(void *data, int mode)
{
	GL_CHECK_VAL(data, -1);
	gl_appdata *ad = (gl_appdata *)data;
	ad->maininfo.view_mode = mode;
	return 0;
}

int gl_del_invalid_widgets(void *data, int invalid_m)
{
	GL_CHECK_VAL(data, -1);
	gl_appdata *ad = (gl_appdata *)data;
	gl_dbg("");
	if (ad->popupinfo.popup) {
		/* Remove popup */
		evas_object_del(ad->popupinfo.popup);
		ad->popupinfo.popup = NULL;
	} else if (ad->uginfo.ug) {
		/* Destroy UG */
		gl_dbg("Destroy UG!");
		if (ad->uginfo.ug) {
			app_control_destroy(ad->uginfo.ug);
			ad->uginfo.ug = NULL;
			ad->uginfo.ug_type = -1;
		}
	}

	switch (invalid_m) {
	case GL_INVALID_NEW_ENTRY:
	case GL_INVALID_NEW_ENTRY_NOC:
		if (ad->entryinfo.mode == GL_ENTRY_NEW_ALBUM) {
			gl_dbg("Destroy New album/tag view!");
			if (invalid_m == GL_INVALID_NEW_ENTRY) {
				gl_dbg("Pop to nf_it_edit");
				elm_naviframe_item_pop_to(ad->gridinfo.nf_it);
			}
		}
		break;
	default:
		ad->entryinfo.mode = GL_ENTRY_NONE;
		break;
	}
	return 0;
}

/* 'Delete medias' is available in tab Albums and Places */
static int __gl_del_medias_op(void *data)
{
	GL_CHECK_VAL(data, -1);
	gl_appdata *ad = (gl_appdata *)data;
	/* Get all selected medias count */
	int cnt = _gl_data_selected_list_count(ad);
	gl_item *gitem = NULL;
	int i = 0;
	int popup_op = GL_POPUP_OP_NONE;

	/* Removed media from selected_media_elist */
	Eina_List *media_elist = NULL;
	for (i = 0; i < cnt; i++) {
		gitem = eina_list_nth(ad->selinfo.elist, i);
		if (gitem != NULL && gitem->item != NULL) {
			if (_gl_data_delete_file(gitem->item->file_url, gitem->item->uuid, false) < 0) {
				gl_dbgE("Failed to delete file!");
			}
			_gl_thumbs_get_list(data, &media_elist);
			media_elist = eina_list_remove(media_elist, gitem);
			_gl_thumbs_set_list(data, media_elist);
			gl_dbgE("elm_item: %p", gitem->elm_item);
			gitem->b_deleted = true;
		} else {
			gl_dbgE("Invalid item!");
		}

		gl_dbg("Write pipe, make progressbar updated!");
		gl_thread_write_pipe(ad, i + 1, popup_op);
		gitem = NULL;
	}
	return 0;
}

/* Delete UI item */
/*
static int __gl_del_gengrid_item(void *data, int index)
{
	GL_CHECK_VAL(data, -1);
	gl_appdata *ad = (gl_appdata *)data;
	gl_item *gitem = NULL;
	gl_dbg("elist cnt: %d", eina_list_count(ad->selinfo.elist));
	// Get selected media
	gitem = eina_list_nth(ad->selinfo.elist, index);
	if (gitem) {
		gl_dbgE("elm_item: %p", gitem->elm_item);
		if (gitem->elm_item) {
			elm_object_item_del(gitem->elm_item);
			gitem->elm_item = NULL;
		}
	}
	return 0;
}
*/
int _gl_update_operation_view(void *data, const char *noti_str)
{
	GL_CHECK_VAL(data, -1);
	gl_appdata *ad = (gl_appdata *)data;
	int cancel_flag = false;
	gl_thread_get_cancel_state(ad, &cancel_flag);
	gl_dbgW("======================");

	if (GL_VIEW_THUMBS == gl_get_view_mode(data)) {
		elm_naviframe_item_pop_to(ad->gridinfo.nf_it);
		_gl_thumbs_update_items(data);
	} else if (!ad->albuminfo.b_new_album && !_gl_thumbs_check_zero(data)) {
		/* Deleting process done, change to normal view */
		_gl_thumbs_edit_pop_view(ad);
	} else {
		gl_dbgW("Empty album, change to Albums view!");
		Elm_Object_Item *nf_it = elm_naviframe_top_item_get(ad->maininfo.naviframe);
		if (nf_it == ad->gridinfo.nf_it) {
			/* pop_cb will be launched */
			gl_set_view_mode(data, GL_VIEW_THUMBS);
		}
		elm_naviframe_item_pop_to(ad->ctrlinfo.nf_it);
		ad->albuminfo.b_new_album = false;
		/* Refine me */
		if (_gl_ctrl_get_tab_mode(ad) == GL_CTRL_TAB_TIMELINE) {
			gl_set_view_mode(data, GL_VIEW_TIMELINE);
			_gl_timeline_update_view(data);
		} else if (_gl_ctrl_get_tab_mode(ad) == GL_CTRL_TAB_ALBUMS) {
			gl_set_view_mode(data, GL_VIEW_ALBUMS);
			gl_albums_update_view(data);
		}

		/* Add notification */
		_gl_notify_create_notiinfo(noti_str);
	}
	/* Add notification */
	if (noti_str && cancel_flag != GL_PB_CANCEL_ERROR) {
		_gl_notify_create_notiinfo(noti_str);
	}
	return 0;
}

/* Update view after deleting process done */
static int __gl_update_del_medias_view(void *data)
{
	GL_CHECK_VAL(data, -1);
	gl_appdata *ad = (gl_appdata *)data;

	/* Clear selected list */
	_gl_data_selected_list_finalize(data);
	Eina_List *media_elist = NULL;
	_gl_data_update_item_list(ad, NULL, &media_elist);
	_gl_thumbs_clear_and_set_list(ad, media_elist);

	if (_gl_thumbs_check_zero(data)) {
		gl_sdbg("is_zero");
		_gl_update_albums_list(ad);
	} else {
		/*update view soon after removing over*/
		_gl_thumbs_update_view(ad);
//		_gl_thumbs_update_sequence(ad);
	}
	_gl_update_operation_view(ad, GL_STR_DELETED);

	_gl_db_update_lock_once(data, false);
	return 0;
}

char *_gl_delete_folder(char *path)
{
	GL_CHECK_NULL(path);
	char *del_path = strdup(path);
	if (NULL == del_path) {
		return NULL;
	}
	char *sub_path = del_path;
	while (gl_file_dir_is_empty(del_path) == 1) {
		gl_dbg("this dir is empty, rm it!");
		if (gl_file_rmdir(del_path) == EINA_FALSE) {
			gl_dbgE("file_rmdir failed!");
			GL_FREE(del_path);
			return NULL;
		}
		sub_path = strrchr(del_path, '/');
		if (sub_path) {
			*sub_path = '\0';
		}
	}
	GL_FREE(del_path);
	return path;
}

int gl_del_medias(void *data)
{
	gl_dbg("");
	GL_CHECK_VAL(data, -1);
	gl_appdata *ad = (gl_appdata *)data;
	/* Get all selected medias count */
	int cnt = _gl_data_selected_list_count(ad);
	if (cnt != 0) {
		_gl_db_update_lock_always(data, true);

		/* Check MMC state for cancel operation */
		gl_check_mmc_state(ad, NULL);
		gl_dbg("MMC state: %d.", ad->maininfo.mmc_state);
		_gl_set_file_op_cbs(data, __gl_del_medias_op, NULL,
		                    __gl_update_del_medias_view, cnt);
		_gl_use_thread_operate_medias(ad, GL_STR_ID_DELETING, cnt,
		                              GL_MEDIA_OP_DELETE);
	}
	return 0;
}

/* 'Delete medias' is available in Dali view */
int _gl_del_media_by_id(void *data, const char *uuid)
{
	GL_CHECK_VAL(uuid, -1);
	GL_CHECK_VAL(data, -1);
	media_info_h media_h = NULL;
	media_content_storage_e storage_type = 0;
	int ret = -1;

	ret = media_info_get_media_from_db(uuid, &media_h);
	if (ret != MEDIA_CONTENT_ERROR_NONE || media_h == NULL) {
		gl_dbgE("Get media failed[%d]!", ret);
		goto DEL_MEDIA_BY_ID_FAILED;
	}
	ret = media_info_get_storage_type(media_h, &storage_type);
	if (ret != MEDIA_CONTENT_ERROR_NONE) {
		gl_dbgE("Get storage type failed!");
		goto DEL_MEDIA_BY_ID_FAILED;
	}
	char *path = NULL;
	ret = media_info_get_file_path(media_h, &path);
	if (ret != MEDIA_CONTENT_ERROR_NONE) {
		gl_dbgE("Get media file path failed!");
		GL_FREEIF(path);
		goto DEL_MEDIA_BY_ID_FAILED;
	}
	if (!gl_file_unlink(path)) {
		char *error_msg = calloc(1, GL_ARRAY_LEN_MAX);
		if (error_msg != NULL) {
			strerror_r(errno, error_msg, GL_ARRAY_LEN_MAX);
			gl_dbgE("file_unlink failed[%s]!", error_msg);
			GL_FREE(error_msg);
		}
		GL_FREEIF(path);
		goto DEL_MEDIA_BY_ID_FAILED;
	}

	/* Delete record from DB then delete media from file system */
	ret = media_info_delete_from_db(uuid);
	if (ret != MEDIA_CONTENT_ERROR_NONE) {
		gl_dbgE("media_info_delete_from_db failed[%d]!", ret);
		goto DEL_MEDIA_BY_ID_FAILED;
	}

	GL_FREEIF(path);

	if (media_h) {
		media_info_destroy(media_h);
		media_h = NULL;
	}
	return 0;

DEL_MEDIA_BY_ID_FAILED:

	GL_FREEIF(path);

	if (media_h) {
		media_info_destroy(media_h);
		media_h = NULL;
	}
	return -1;
}

int gl_remove_album(void *data, gl_cluster *album_item, bool is_hide)
{
	GL_CHECK_VAL(data, -1);

	if (album_item == NULL || album_item->cluster == NULL ||
	        album_item->cluster->uuid == NULL) {
		gl_dbgE("Invalid album!");
		return -1;
	}

	gl_sdbg("Remove album: %s, id=%s", album_item->cluster->display_name,
	        album_item->cluster->uuid);
	/* remove album from db*/
	if (_gl_local_data_delete_album(album_item->cluster,
	                                NULL, data, is_hide) != 0) {
		gl_dbgE("delete album failed!");
		//return -1;
	}

	_gl_delete_folder(album_item->cluster->path);
	return 0;
}

static int __gl_update_del_albums_view(void *data)
{
	GL_CHECK_VAL(data, -1);
	_gl_update_albums_list(data);
	/* update the albums view */
	_gl_albums_change_mode(data, false);
	_gl_db_update_lock_always(data, false);
	return 0;
}

/* delete albums */
static int __gl_del_albums_op(void *data)
{
	GL_CHECK_VAL(data, -1);
	gl_appdata *ad = (gl_appdata *)data;
	GL_CHECK_VAL(ad->albuminfo.sel_elist, -1);
	gl_cluster *album = NULL;
	int cnt = _gl_data_get_albums_selected_cnt(ad);
	int i = 0;
	int ret = -1;

	if (cnt == 0) {
		gl_dbgE("Empty list!");
		return -1;
	}
	gl_dbg("cnt= %d", cnt);
	for (i = 0; i < cnt; i++) {
		album = eina_list_nth(ad->albuminfo.sel_elist, i);

		if (album && album->cluster && album->cluster->uuid) {
			if ((ret = gl_remove_album(ad, album, false)) < 0) {
				gl_dbgE("remove album failed[%d]!", ret);
			}
		}
		gl_thread_write_pipe(ad, i + 1, GL_POPUP_OP_NONE);
	}
	return 0;
}

int _gl_del_albums(void *data)
{
	gl_dbg("");
	GL_CHECK_VAL(data, -1);
	gl_appdata *ad = (gl_appdata *)data;
	/* Get all selected albums count */
	int cnt = _gl_data_get_albums_selected_cnt(ad);
	if (cnt != 0) {
		_gl_db_update_lock_always(data, true);
		/* Check MMC state for cancel operation */
		gl_check_mmc_state(ad, NULL);
		gl_dbg("MMC state: %d.", ad->maininfo.mmc_state);
		_gl_set_file_op_cbs(data, __gl_del_albums_op, NULL,
		                    __gl_update_del_albums_view, cnt);
		_gl_use_thread_operate_medias(ad, GL_STR_ID_DELETING, cnt,
		                              GL_MEDIA_OP_DELETE_ALBUM);
	}

	return 0;
}

int _gl_data_update_sel_item_list(void *data, Eina_List *p_elist)
{
	GL_CHECK_VAL(data, -1);
	gl_appdata *ad = (gl_appdata *)data;
	gl_item *gitem = NULL;
	int count = eina_list_count(p_elist);
	int i;
	gitem = eina_list_nth(p_elist, 0);
	if (gitem) {
		gitem->album->elist = NULL;
	}
	for (i = 0; i < count ; i++) {
		gitem = eina_list_nth(p_elist, i);
		_gl_data_restore_update_selected(ad->selinfo.elist, gitem);
	}
	return 0;
}

#ifdef _USE_ROTATE_BG
/* b_path == true, Use saved folder path to check cluster uuid
 * b_select == true, Update albums in select view for tag adding
 */
int _gl_refresh_albums_list(void *data, bool b_path, bool b_select)
{
	GL_CHECK_VAL(data, -1);
	gl_appdata *ad = (gl_appdata *)data;
	gl_cluster *cur_album = _gl_albums_get_current(data);
	char *uuid = NULL;
	bool b_get_cur_album = false;

	if (ad->albuminfo.view) {
		elm_gengrid_clear(ad->albuminfo.view);
	}
	if (ad->albuminfo.select_view) {
		elm_gengrid_clear(ad->albuminfo.select_view);
	}
	/* Save cluster ID to set new cur_album from new albums list */
	if (cur_album && cur_album->cluster && cur_album->cluster->uuid) {
		uuid = strdup(cur_album->cluster->uuid);
		b_get_cur_album = true;
		if (ad->gridinfo.view &&
		        ad->gridinfo.view != ad->gridinfo.nocontents) {
			elm_gengrid_clear(ad->gridinfo.view);
		}
		if (ad->gridinfo.select_view &&
		        ad->gridinfo.select_view != ad->gridinfo.nocontents) {
			elm_gengrid_clear(ad->gridinfo.select_view);
		}
	} else {
		if (_gl_ctrl_get_tab_mode(ad) == GL_CTRL_TAB_ALBUMS) {
			gl_dbgW("Current album is unavailable!");
		} else if (!_gl_thumbs_check_zero(data)) {
			/* Update data if items list exists */
			Eina_List *sel_id_list = NULL;
			Eina_List *media_elist = NULL;
			int view_m = gl_get_view_mode(ad);
			if (view_m == GL_VIEW_THUMBS_EDIT ||
			        view_m == GL_VIEW_THUMBS_SELECT) {
				gl_dbg("Edit view.");
				/* Get ID list of selected items */
				_gl_data_save_selected_str_ids(data, &sel_id_list);
			}
			_gl_data_clear_selected_info(ad);
			_gl_data_update_item_list(ad, &sel_id_list, &media_elist);
			_gl_thumbs_clear_and_set_list(ad, media_elist);
		}
	}

	_gl_data_get_cluster_list_edit_mode(ad);
	/* need to reset current album */
	_gl_albums_set_current(data, NULL);
	cur_album = NULL;

	if (b_get_cur_album) {
		if (b_path) {
			gl_dbg("Path: %s", ad->albuminfo.dest_folder);
			if (strlen(ad->albuminfo.dest_folder) > 0 &&
			        GL_FILE_EXISTS(ad->albuminfo.dest_folder)) {
				gl_dbgW("Cluster record updated!");
				_gl_data_get_cluster_by_path(ad,
				                             ad->albuminfo.dest_folder,
				                             &cur_album);
			} else {
				gl_dbgE("Invalid folder path!");
				_gl_data_get_cluster_by_id(ad, uuid, &cur_album);
			}
		} else {
			_gl_data_get_cluster_by_id(ad, uuid, &cur_album);
		}

		if (cur_album && cur_album->cluster) {
			_gl_albums_set_current(data, cur_album);
			gl_sdbg("Reset current album: %s",
			        cur_album->cluster->display_name);
			/* Get ID list of selected items */
			int view_m = gl_get_view_mode(ad);
			Eina_List *sel_id_list = NULL;
			Eina_List *media_elist = NULL;
			if (view_m == GL_VIEW_THUMBS_EDIT ||
			        view_m == GL_VIEW_THUMBS_SELECT) {
				_gl_data_copy_selected_info_save_current_selected(ad, &sel_id_list);
			}
			_gl_data_update_item_list(ad, &sel_id_list, &media_elist);
			_gl_data_update_sel_item_list(ad, media_elist);
			_gl_thumbs_clear_and_set_list(ad, media_elist);
		} else {
			/* Clear selected items list if current album doesn't exist */
			_gl_data_clear_selected_info(ad);
			/* Clear items list if current album doesn't exist */
			_gl_thumbs_clear_and_set_list(ad, NULL);
		}
	}

	GL_FREEIF(uuid);
	return 0;
}

#endif

int _gl_update_albums_data(void *data)
{
	GL_CHECK_VAL(data, -1);
	gl_appdata *ad = (gl_appdata *)data;
	gl_cluster *cur_album = _gl_albums_get_current(data);
	char *uuid = NULL;

	if (cur_album && cur_album->cluster && cur_album->cluster->uuid) {
		uuid = strdup(cur_album->cluster->uuid);
	}

	_gl_data_get_cluster_list(ad);
	/* need to reset current album */
	_gl_albums_set_current(data, NULL);
	cur_album = NULL;

	if (uuid) {
		_gl_data_get_cluster_by_id(ad, uuid, &cur_album);
		if (cur_album) {
			_gl_albums_set_current(data, cur_album);
		}
		GL_FREE(uuid);
	}

	return 0;
}

int _gl_update_albums_list(void *data)
{
#ifdef _USE_ROTATE_BG
	return _gl_refresh_albums_list(data, false, false);
#else
	GL_CHECK_VAL(data, -1);
	gl_appdata *ad = (gl_appdata *)data;
	gl_cluster *cur_album = _gl_albums_get_current(data);
	char cluster_id[GL_MTYPE_ITEN_ID_LEN_MAX] = { 0, };
	bool b_get_cur_album = false;

	/* Save cluster ID to set new current_album from new albums list */
	if (cur_album && cur_album->cluster && cur_album->cluster->uuid) {
		g_strlcpy(cluster_id, cur_album->cluster->uuid,
		          GL_MTYPE_ITEN_ID_LEN_MAX);
		b_get_cur_album = true;
	} else {
		gl_dbg("None album selected, current album is unavailable.");
	}

	_gl_data_get_cluster_list(ad);
	/* need to reset current album */
	_gl_albums_set_current(data, NULL);
	cur_album = NULL;

	if (b_get_cur_album) {
		_gl_data_get_cluster_by_id(ad, cluster_id, &cur_album);
		if (cur_album && cur_album->cluster) {
			_gl_albums_set_current(data, cur_album);
			gl_sdbg("Reset current album: %s",
			        cur_album->cluster->display_name);
			Eina_List *sel_id_list = NULL;
			Eina_List *media_elist = NULL;
			int view_m = gl_get_view_mode(ad);
			if (view_m == GL_VIEW_THUMBS_EDIT ||
			        view_m == GL_VIEW_THUMBS_SELECT) {
				gl_dbg("Edit view.");
				/* Get ID list of selected items */
				_gl_data_save_selected_str_ids(data, &sel_id_list);
			}
			_gl_data_clear_selected_info(ad);
			_gl_data_update_item_list(ad, &sel_id_list, &media_elist);
			_gl_thumbs_clear_and_set_list(ad, media_elist);
			return 0;
		} else {
			/* Clear items list if current album doesn't exist */
			if (ad->selinfo.elist) {
				_gl_data_clear_selected_info(ad);
			}
		}
	}

	return 0;
#endif
}

/**
* Move files under root album [/opt/media or /opt/storage/sdcard] to a newly created album.
* Used in 'Rename' album case.
*/
int gl_move_root_album(void *data, gl_cluster *cur_album, char *dest_path)
{
	GL_CHECK_VAL(data, -1);
	GL_CHECK_VAL(cur_album, -1);
	GL_CHECK_VAL(cur_album->cluster, -1);
	gl_appdata *ad = (gl_appdata *)data;
	Eina_List *medias_elist = NULL;

	/* Get all medias of current album */
	_gl_data_get_items_album(ad, cur_album, GL_GET_ALL_RECORDS,
	                         GL_GET_ALL_RECORDS, &medias_elist);
	GL_CHECK_VAL(medias_elist, -1);

	Eina_List *l = NULL;
	gl_item *gitem = NULL;
	int popup_op = GL_POPUP_OP_NONE;

	/* Move medias of album to dest folder */
	EINA_LIST_FOREACH(medias_elist, l, gitem) {
		if (gitem && gitem->item) {
			gl_sdbg("Move [%s]", gitem->item->file_url);
			if (_gl_move_media_thumb(data, gitem, dest_path, true, &popup_op, GL_MC_MOVE) != 0) {
				gl_dbgW("Failed to move this item");
			}

			gitem = NULL;
		} else {
			gl_dbgE("Invalid item!");
		}
	}

	/* Free item list */
	_gl_data_util_clear_item_list(&(medias_elist));
	return 0;
}

/* 'Move medias' is only available in tab Albums */
static int __gl_move_copy_op(void *data)
{
	GL_CHECK_VAL(data, -1);
	gl_appdata *ad = (gl_appdata *)data;
	int i = 0;
	/* Get selected medias count */
	int cnt = _gl_data_selected_list_count(ad);
	gl_item *gitem = NULL;
	int popup_op = GL_POPUP_OP_NONE;

	for (i = 0; i < cnt; i++) {
		gitem = eina_list_nth(ad->selinfo.elist, i);
		if (gitem && gitem->item) {
			gl_sdbg("Selected [%s]", gitem->item->file_url);

			if (ad->maininfo.medias_op_type == GL_MEDIA_OP_COPY) {
				if (_gl_move_media_thumb(data, gitem, ad->albuminfo.dest_folder, true, &popup_op, GL_MC_COPY) != 0) {
					gl_dbg("Failed to copy this item");
				}
				gl_dbg("File Copied:::::::%d/%d-->try to update progressbar", i, cnt);
			} else if (ad->maininfo.medias_op_type == GL_MEDIA_OP_MOVE) {
				if (_gl_move_media_thumb(data, gitem, ad->albuminfo.dest_folder, true, &popup_op, ad->albuminfo.file_mc_mode) != 0) {
					gl_dbg("Failed to move this item");
				}

				/*_gl_thumbs_get_list(data, &media_elist);
				media_elist = eina_list_remove(media_elist, gitem);
				_gl_thumbs_set_list(data, media_elist);
				gl_dbgE("elm_item: %p", gitem->elm_item);
				gitem->b_deleted = true;*/
				gl_dbg("File Moved:::::::%d/%d-->try to update progressbar", i, cnt);
			} else {
				gl_dbg("Invalid medias_op_type: %d!", ad->maininfo.medias_op_type);
			}
		} else {
			gl_dbg("Invalid item!");
		}

		gl_dbg("Write pipe, make progressbar updated!");
		gl_thread_write_pipe(ad, i + 1, popup_op);

		popup_op = GL_POPUP_OP_NONE;
	}
	return 0;
}

/* Update view after moving process done */
static int __gl_update_move_copy_view(void *data)
{
	GL_CHECK_VAL(data, -1);
	gl_appdata *ad = (gl_appdata *)data;
	int view_mode = gl_get_view_mode(ad);
	gl_dbg("view_mode: %d", view_mode);

	const char *noti_str = NULL;
	if (view_mode == GL_VIEW_THUMBS_SELECT) {
		noti_str = GL_STR_ADDED;
		/* Clear selected list */
		_gl_data_clear_selected_info(data);
		_gl_albums_set_current(data, NULL);
		/* Update albums list and items list */
		_gl_update_albums_data(ad);
	} else if (ad->maininfo.medias_op_type == GL_MEDIA_OP_COPY) {
		noti_str = GL_STR_COPIED;
		/* Clear selected list */
		_gl_data_selected_list_finalize(data);
		/* Update albums list */
		_gl_update_albums_data(ad);
	} else if (ad->maininfo.medias_op_type == GL_MEDIA_OP_MOVE) {
		noti_str = GL_STR_MOVED;
		/* Clear selected list */
		_gl_data_selected_list_finalize(data);
		/* Update albums list and items list */
		if (_gl_thumbs_check_zero(data)) {
			gl_sdbg("is_zero");
			_gl_update_albums_list(ad);
		} else {
			_gl_update_albums_list(ad);
			/*update view soon after removing over*/
			_gl_thumbs_update_view(ad);
//			_gl_thumbs_update_sequence(ad);
		}
	} else {
		gl_dbgE("Wrong mode!");
	}

	if (ad->maininfo.hide_noti) {
		noti_str = NULL;
		_gl_db_update_add_timer(data);
		ad->maininfo.hide_noti = false;
	}

	_gl_update_operation_view(ad, noti_str);

	_gl_db_update_lock_always(data, false);
	return 0;
}

unsigned long long _gl_get_file_storage_space_required(char *path)
{
	struct stat info;
	if (stat(path, &info)) {
		gl_dbgE("Fail to stat item : %s", path);
		return 0;
	}
	unsigned long long size = (unsigned long long)info.st_size;
	return size;
}

void _gl_get_total_space_required(void *data, unsigned long long *total_space)
{
	gl_appdata *ad = (gl_appdata *)data;
	int cnt = _gl_data_selected_list_count(ad);
	gl_item *gitem = NULL;
	int i;
	for (i = 0; i < cnt; i++) {
		gitem = eina_list_nth(ad->selinfo.elist, i);
		if (gitem && gitem->item) {
			gl_sdbg("Selected [%s]", gitem->item->file_url);
			*total_space += _gl_get_file_storage_space_required(gitem->item->file_url);
		}
	}

}

int gl_move_copy_to_album(void *data)
{
	GL_CHECK_VAL(data, -1);
	gl_appdata *ad = (gl_appdata *)data;
	char folder_fullpath[GL_DIR_PATH_LEN_MAX] = { 0, };
	gl_cluster *cur_album = _gl_albums_get_current(data);
	int cnt = _gl_data_selected_list_count(ad);

	_gl_db_update_lock_always(data, true);

	if (ad->albuminfo.file_mc_mode == GL_MC_COPY || ad->albuminfo.file_mc_mode == GL_MC_MOVE) {

		unsigned long long free_size = gl_fs_get_free_space(GL_STORE_T_PHONE);
		gl_dbg("Free space is %lld", free_size);

		if (free_size == 0) {
			gl_dbgW("Low memory.");
			gl_popup_create_popup(ad, GL_POPUP_ALBUM_MEMORY_FULL,
			                      GL_DEVICE_MEMORY_FULL);
			goto GL_FAILED;
		}
		unsigned long long total_space = 0;
		_gl_get_total_space_required(ad, &total_space);
		gl_dbgW("Total space required is : %lld .", total_space);
		if (total_space > free_size) {
			gl_dbgW("Low memory.");
			gl_popup_create_popup(ad, GL_POPUP_ALBUM_MEMORY_FULL,
			                      GL_DEVICE_MEMORY_FULL);
			goto GL_FAILED;
		}

	}

	/**
	 * 'move_album_id == NULL' is new album case,
	 * other move/save to some existed album.
	 */
	if (ad->albuminfo.path == NULL) {
		gl_dbg("---Popup list item: New album---");
		if (gl_make_new_album(ad->albuminfo.new_name) == false) {
			gl_popup_create_popup(ad, GL_POPUP_NOBUT,
			                      GL_STR_SAME_NAME_ALREADY_IN_USE);
			gl_dbgE("Failed to make a new directory!");
			goto GL_FAILED;
		}
		char *default_images_path = _gl_get_directory_path(STORAGE_DIRECTORY_IMAGES);
		GL_CHECK_VAL(default_images_path, -1);
		snprintf(folder_fullpath, GL_DIR_PATH_LEN_MAX, "%s/%s",
				default_images_path, ad->albuminfo.new_name);
		GL_FREE(default_images_path);
	} else {
		g_strlcpy(folder_fullpath, ad->albuminfo.path,
		          GL_DIR_PATH_LEN_MAX);
		GL_FREE(ad->albuminfo.path);
	}

	if (!ad->albuminfo.b_new_album &&
	        (cur_album == NULL || cur_album->cluster == NULL)) {
		gl_dbgE("[Error] Current album is NULL!");
		goto GL_FAILED;
	}

	memset(ad->albuminfo.dest_folder, 0x00, GL_DIR_PATH_LEN_MAX);
	g_strlcpy(ad->albuminfo.dest_folder, folder_fullpath,
	          GL_DIR_PATH_LEN_MAX);
	/* Check MMC state for cancel operation */
	gl_check_mmc_state(ad, folder_fullpath);
	gl_dbg("MMC state: %d.", ad->maininfo.mmc_state);
	if (cnt != 0) {
		if (ad->albuminfo.file_mc_mode == GL_MC_MOVE) {
			_gl_set_file_op_cbs(data, __gl_move_copy_op,
			                    NULL, __gl_update_move_copy_view, cnt);
			_gl_use_thread_operate_medias(ad, GL_STR_ID_MOVING, cnt,
			                              GL_MEDIA_OP_MOVE);
		} else if (ad->albuminfo.file_mc_mode == GL_MC_COPY) {
			_gl_set_file_op_cbs(data, __gl_move_copy_op, NULL,
			                    __gl_update_move_copy_view, cnt);
			_gl_use_thread_operate_medias(ad, GL_STR_ID_COPYING, cnt,
			                              GL_MEDIA_OP_COPY);
		} else {
			gl_dbgE("Wrong mode!");
		}
	}
	return 0;

GL_FAILED:

	_gl_notify_check_selall(ad, ad->gridinfo.nf_it, ad->gridinfo.count,
	                        _gl_data_selected_list_count(ad));
	/* Update the label text */
	_gl_thumbs_update_label_text(ad->gridinfo.nf_it,
	                             _gl_data_selected_list_count(ad), false);
	_gl_db_update_lock_always(data, false);
	return -1;
}

#ifdef _USE_ROTATE_BG

int _gl_delay(double sec)
{
	gl_dbg("Start");
	struct timeval tv;
	unsigned int start_t = 0;
	unsigned int end_t = 0;
	unsigned int delay_t = (unsigned int)(sec * GL_TIME_USEC_PER_SEC);

	gettimeofday(&tv, NULL);
	start_t = tv.tv_sec * GL_TIME_USEC_PER_SEC + tv.tv_usec;

	for (end_t = start_t; end_t - start_t < delay_t;) {
		gettimeofday(&tv, NULL);
		end_t = tv.tv_sec * GL_TIME_USEC_PER_SEC + tv.tv_usec;
	}

	gl_dbg("End");
	return 0;
}

int _gl_rotate_op(void *data)
{
	GL_CHECK_VAL(data, -1);
	gl_appdata *ad = (gl_appdata *)data;
	/* Get all selected medias count */
	int cnt = _gl_data_selected_list_count(ad);
	gl_item *gitem = NULL;
	int i = 0;
	int popup_op = GL_POPUP_OP_NONE;
	unsigned int orient = GL_ORIENTATION_ROT_ERR;
	unsigned int new_orient = GL_ORIENTATION_ROT_ERR;
	int ret = -1;
	bool b_left = false;

	if (ad->maininfo.medias_op_type == GL_MEDIA_OP_ROTATING_LEFT) {
		b_left = true;
	}

	gl_cluster *current_album = _gl_albums_get_current(data);
	/* Save cluster path to set new current_album from new albums list */
	if (current_album && current_album->cluster &&
	        current_album->cluster->path &&
	        strlen(current_album->cluster->path))
		g_strlcpy(ad->albuminfo.dest_folder,
		          current_album->cluster->path, GL_DIR_PATH_LEN_MAX);
	else {
		gl_dbgE("Invalid folder path!");
	}

	/* Removed media from selected_media_elist */
	for (i = 0; i < cnt; i++) {
		gitem = eina_list_nth(ad->selinfo.elist, i);
		if (gitem != NULL && gitem->item != NULL &&
		        gitem->item->file_url &&
		        gitem->item->type == MEDIA_CONTENT_TYPE_IMAGE &&
		        GL_FILE_EXISTS(gitem->item->file_url)) {
			/* Save orient in file */
			ret = _gl_exif_get_orientation(gitem->item->file_url,
			                               &orient);
			if (ret == 0) {
				new_orient = _gl_exif_get_rotated_orientation(orient,
				             b_left);
				_gl_exif_set_orientation(gitem->item->file_url,
				                         new_orient);
				/* Update thumbnail */
				media_info_refresh_metadata_to_db(gitem->item->uuid);
			}
		} else {
			gl_dbgE("Invalid item!");
		}

		/* Add some delay for last two images to wait for thumb updated */
		if (i > cnt - 2) {
			_gl_delay(GL_ROTATE_DELAY);
		}

		if (i == cnt) {
			gl_dbgW("Last image rotated!");
			/* Add another delay for last thumb */
			_gl_delay(GL_ROTATE_DELAY);
		}
		gl_dbg("Write pipe, make progressbar updated!");
		gl_thread_write_pipe(ad, i + 1, popup_op);
		gitem = NULL;
	}
	return 0;
}

/* Update view after rotation process done */
int _gl_update_rotate_view(void *data)
{
	GL_CHECK_VAL(data, -1);
	gl_appdata *ad = (gl_appdata *)data;

	gl_item *gitem = NULL;
	Eina_List *tmp = NULL;
	EINA_LIST_FOREACH(ad->selinfo.elist, tmp, gitem) {
		if (gitem && gitem->elm_item) {
			gitem->checked = false;
			elm_gengrid_item_update(gitem->elm_item);
		}
	}

	/* Clear selected list */
	_gl_data_selected_list_finalize(data);

	memset(ad->albuminfo.dest_folder, 0x00, GL_DIR_PATH_LEN_MAX);

	_gl_update_operation_view(ad, NULL);
	_gl_db_update_lock_always(data, false);
	return 0;
}

int _gl_rotate_images(void *data, bool b_left)
{
	gl_dbg("");
	GL_CHECK_VAL(data, -1);
	gl_appdata *ad = (gl_appdata *)data;
	/* Get all selected medias count */
	int cnt = _gl_data_selected_list_count(ad);
	/* Check MMC state for cancel operation */
	gl_check_mmc_state(ad, NULL);
	gl_dbg("MMC state: %d.", ad->maininfo.mmc_state);
	/* Rotate left */
	int op_type = GL_MEDIA_OP_ROTATING_LEFT;
	/* Rotate right */
	if (!b_left) {
		op_type = GL_MEDIA_OP_ROTATING_RIGHT;
	}
	_gl_db_update_lock_always(data, true);
	_gl_set_file_op_cbs(data, _gl_rotate_op, NULL, _gl_update_rotate_view,
	                    cnt);
	_gl_use_thread_operate_medias(ad, GL_STR_ID_ROTATING, cnt, op_type);

	return 0;
}

int _gl_rotate_image_by_id(void *data, const char *uuid, bool b_left)
{
	GL_CHECK_VAL(uuid, -1);
	GL_CHECK_VAL(data, -1);
	unsigned int orient = GL_ORIENTATION_ROT_ERR;
	unsigned int new_orient = GL_ORIENTATION_ROT_ERR;
	int ret = -1;
	media_info_h media_h = NULL;
	char *path = NULL;
	media_content_type_e media_type = 0;

	ret = media_info_get_media_from_db(uuid, &media_h);
	if (ret != MEDIA_CONTENT_ERROR_NONE || media_h == NULL) {
		gl_dbgE("Get media failed[%d]!", ret);
		if (media_h) {
			media_info_destroy(media_h);
		}
		return -1;
	}
	ret = media_info_get_media_type(media_h, &media_type);
	if (ret != MEDIA_CONTENT_ERROR_NONE) {
		gl_dbgE("Get storage type failed!");
		media_info_destroy(media_h);
		return -1;
	}
	ret = media_info_get_file_path(media_h, &path);
	if (ret != MEDIA_CONTENT_ERROR_NONE) {
		gl_dbgE("Get media file path failed!");
		media_info_destroy(media_h);
		return -1;
	}
	if (media_type == MEDIA_CONTENT_TYPE_IMAGE && GL_FILE_EXISTS(path)) {
		/* Save orient in file */
		ret = _gl_exif_get_orientation(path, &orient);
		if (ret == 0) {
			new_orient = _gl_exif_get_rotated_orientation(orient,
			             b_left);
			_gl_exif_set_orientation(path, new_orient);
			/* Update thumbnail */
			media_info_refresh_metadata_to_db(uuid);
		}
	}

	GL_FREE(path);
	media_info_destroy(media_h);
	media_h = NULL;
	return 0;
}
#endif

Eina_Bool gl_update_view(void *data, int mode)
{
	GL_CHECK_FALSE(data);
	gl_appdata *ad = (gl_appdata *)data;
	int view_mode = gl_get_view_mode(ad);
	gl_dbg("view_mode: %d", view_mode);
	if (mode == GL_UPDATE_VIEW_NONE) {
		gl_dbg("Don't need to update");
		return EINA_FALSE;
	}

	/* MMC, WEB, INOTIFY, need to update data first */
	if (mode >= GL_UPDATE_VIEW_MMC_REMOVED) {
		/* Albums list should be updated first */
		_gl_update_albums_list(data);
	}

	if (view_mode == GL_VIEW_ALBUMS ||
	        view_mode == GL_VIEW_ALBUMS_EDIT ||
	        view_mode == GL_VIEW_ALBUMS_RENAME) {
		/* Albums list should be updated first */
		gl_albums_update_view(ad);
	} else if (view_mode == GL_VIEW_THUMBS) {
		gl_cluster *cur = _gl_albums_get_current(data);
		/* MMC removed, change to albums view if in mmc album */
		if (mode == GL_UPDATE_VIEW_MMC_REMOVED &&
		        _gl_ctrl_get_tab_mode(ad) == GL_CTRL_TAB_ALBUMS && cur &&
		        cur->cluster && cur->cluster->type == GL_STORE_T_MMC) {
			gl_dbgW("MMC removed, change to albums view!");
			gl_pop_to_ctrlbar_ly(ad);
			return EINA_TRUE;
		}

		_gl_thumbs_update_view(data);
		return EINA_TRUE;
	} else if (view_mode == GL_VIEW_THUMBS_EDIT) {
		/* Check thread operation case */
		if (ad->pbarinfo.sync_pipe) {
			gl_dbgW("Thread operation is in process!");
			return EINA_TRUE;
		}

		gl_cluster *cur = _gl_albums_get_current(data);
		if (mode == GL_UPDATE_VIEW_MMC_REMOVED &&
		        _gl_ctrl_get_tab_mode(ad) == GL_CTRL_TAB_ALBUMS && cur &&
		        cur->cluster && cur->cluster->type == GL_STORE_T_MMC) {
			/* MMC removed, change to albums view if in mmc album */
			gl_dbgW("MMC removed, change to albums view!");
			gl_pop_to_ctrlbar_ly(ad);
			return EINA_TRUE;
		}

		_gl_thumbs_update_view(data);
		return EINA_TRUE;
	} else if (view_mode == GL_VIEW_ALBUMS_SELECT) {
		_gl_albums_sel_update_view(ad);
	} else if (view_mode == GL_VIEW_THUMBS_SELECT) {
		Evas_Object *layout_inner = elm_object_part_content_get(ad->albuminfo.select_layout, "split.view");
		if (!layout_inner) {
			gl_dbgE("Failed to add split album view!");
		} else {
			_create_split_album_view(ad, layout_inner);
			elm_object_part_content_set(ad->albuminfo.select_layout, "split.view", layout_inner);
		}
		gl_cluster* album = _gl_albums_get_current(ad);
		_gl_albums_set_current(ad, album);
		_gl_thumbs_sel_create_view(ad, album);
	} else if (view_mode == GL_VIEW_TIMELINE) {
		_gl_timeline_update_view(data);
	}
	return EINA_TRUE;
}

/**
* Parse medias type and count of selected items,
* and set different type for share items.
*/
int gl_get_share_mode(void *data)
{
	GL_CHECK_VAL(data, -1);
	gl_appdata *ad = (gl_appdata *)data;
	int share_mode = GL_SHARE_NONE;
	int image_cnt = 0;
	int video_cnt = 0;
	int sel_cnt = 0;
	int view_m = gl_get_view_mode(ad);
	if (view_m == GL_VIEW_ALBUMS_EDIT) {
		sel_cnt = ad->selinfo.sel_cnt;
	} else if (view_m == GL_VIEW_ALBUMS) {
		sel_cnt = ad->selinfo.sel_cnt;
	} else {
		GL_CHECK_VAL(ad->selinfo.elist, -1);
		sel_cnt = eina_list_count(ad->selinfo.elist);
	}
	image_cnt = ad->selinfo.images_cnt;
	if (image_cnt > sel_cnt) {
		gl_dbgE("Images count is wrong!");
		return -1;
	}

	video_cnt = sel_cnt - image_cnt;
	gl_dbg("Selected items count: %d, image count: %d, video count: %d.",
	       sel_cnt, image_cnt, video_cnt);

	if (image_cnt && video_cnt) {
		share_mode = GL_SHARE_IMAGE_VIDEO;
	} else if (image_cnt) {
		if (ad->selinfo.jpeg_cnt == sel_cnt) {
			if (image_cnt == 1) {
				share_mode = GL_SHARE_IMAGE_ONE_JPEG;
			} else {
				share_mode = GL_SHARE_IMAGE_MULTI_JPEG;
			}
		} else {
			if (image_cnt == 1) {
				share_mode = GL_SHARE_IMAGE_ONE;
			} else {
				share_mode = GL_SHARE_IMAGE_MULTI;
			}
		}
	} else if (video_cnt) {
		if (video_cnt == 1) {
			share_mode = GL_SHARE_VIDEO_ONE;
		} else {
			share_mode = GL_SHARE_VIDEO_MULTI;
		}
	} else {
		gl_dbgE("Error: no video and image!");
		return -1;
	}
	gl_dbg("share_mode is %d.", share_mode);

	return share_mode;
}

/**
* It's in thumbnails view, video list view, or selectioinfo view.
* Destroy invalid widegets or UGs.
* Pop current invalid view to controlbar layout,
* to show Albums view/Map/Tags view.
*/
int gl_pop_to_ctrlbar_ly(void *data)
{
	GL_CHECK_VAL(data, -1);
	gl_appdata *ad = (gl_appdata *)data;
	gl_dbg("");

	/* Destroy thumbnails/video_list view then pop to ctrlbar_ly */
	_gl_thumbs_destroy_view(data, true);

	if (gl_check_gallery_empty(ad)) {
		/* None albums exist, Change to albums view. */
		gl_dbgW("Empty Gallery, change to Albums view.");
		if (_gl_ctrl_get_tab_mode(ad) != GL_CTRL_TAB_ALBUMS) {
			/* Change to Albums tab. */
			gl_dbg("Not in Albums tab.");
			_gl_ctrl_sel_tab(data, GL_CTRL_TAB_ALBUMS);
			return 0;
		}
		/* None albums, disable edit button/controlbar */
		_gl_albums_check_btns(data);
	} else {
		/* Change to albums view. */
		gl_dbg("Change to Albums view.");
		/* None editable albums, disable edit button */
		_gl_albums_check_btns(data);
	}
	return 0;
}

time_t gl_util_get_current_systemtime(void)
{
	struct timeval now;
	memset(&now, 0x00, sizeof(struct timeval));
	struct tm local_time;
	memset(&local_time, 0x00, sizeof(struct tm));

	/* get current time */
	gettimeofday(&now, NULL);
	localtime_r((time_t *)(&(now.tv_sec)), &local_time);

	return mktime(&local_time);
}

/* Change int to char * of video duration, caller need to free the allocated memory */
char *_gl_get_duration_string(unsigned int v_dur)
{
	char *dur_str = calloc(1, GL_FILE_PATH_LEN_MAX);
	GL_CHECK_NULL(dur_str);
	if (v_dur > 0) {
		int duration = floor(v_dur / GL_TIME_MSEC_PER_SEC);
		int dur_hr = 0;
		int dur_min = 0;
		int dur_sec = 0;
		int tmp = 0;

		if (duration >= GL_TIME_SEC_PER_HOUR) {
			dur_sec = duration % GL_TIME_SEC_PER_MIN;
			tmp = floor(duration / GL_TIME_SEC_PER_MIN);
			dur_min = tmp % GL_TIME_MIN_PER_HOUR;
			dur_hr = floor(tmp / GL_TIME_MIN_PER_HOUR);
		} else if (duration >= GL_TIME_SEC_PER_MIN) {
			dur_hr = 0;
			dur_min = floor(duration / GL_TIME_SEC_PER_MIN);
			dur_sec = duration % GL_TIME_SEC_PER_MIN;
		} else {
			dur_hr = 0;
			dur_min = 0;
			dur_sec = duration % GL_TIME_SEC_PER_MIN;
		}

		snprintf(dur_str, GL_FILE_PATH_LEN_MAX, "%02d:%02d:%02d",
		         dur_hr, dur_min, dur_sec);
	} else {
		snprintf(dur_str, GL_FILE_PATH_LEN_MAX, "00:00:00");
	}
	dur_str[strlen(dur_str)] = '\0';
	return dur_str;
}

gl_icon_type_e _gl_get_icon_type(gl_item *git)
{
	if (git == NULL || git->item == NULL || git->item->file_url == NULL) {
		gl_dbgE("Invalid item :%p", git);
		return GL_ICON_CORRUPTED_FILE;
	}

	return GL_ICON_NORMAL;
}

char *_gl_str(char *str_id)
{
	GL_CHECK_NULL(str_id);
	if (strstr(str_id, "IDS_COM")) {
		return dgettext(GL_STR_DOMAIN_SYS, str_id);
	} else if (strstr(str_id, "IDS_")) {
		return dgettext(GL_STR_DOMAIN_LOCAL, str_id);
	} else {
		return str_id;
	}
}

bool _gl_is_str_id(const char *str_id)
{
	GL_CHECK_NULL(str_id);
	if (strstr(str_id, "IDS_")) {
		return true;
	} else {
		return false;
	}
}

double _gl_get_win_factor(Evas_Object *win, int *width, int *height)
{
	if (win == NULL) {
		gl_dbgE("Invalid window!");
		return 1.0f;
	}

	double factor = 1.0f;
	int win_x = 0;
	int win_y = 0;
	int win_w = 0;
	int win_h = 0;

	elm_win_screen_size_get(win, &win_x, &win_y, &win_w, &win_h);
	gl_dbg("Window size: %dx%d, %dx%d", win_x, win_y, win_w, win_h);
	int win_p_h = win_w > win_h ? win_w : win_h;
	double scale = elm_config_scale_get();
	gl_dbg("scale: %f", scale);

	if (win_p_h < GL_WIN_HEIGHT) {
		factor = (double)(win_p_h - (int)(GL_FIXED_HEIGHT * scale)) / (double)(GL_WIN_HEIGHT - GL_FIXED_HEIGHT);
	} else if (scale > factor) {
		factor = (GL_WIN_HEIGHT - GL_FIXED_HEIGHT * scale) / (GL_WIN_HEIGHT - GL_FIXED_HEIGHT);
	} else if (scale < factor) {
		factor = (GL_WIN_HEIGHT - GL_FIXED_HEIGHT) / (GL_WIN_HEIGHT - GL_FIXED_HEIGHT * scale);
	} else {
		factor = scale;
	}
	gl_dbg("factor: %f", factor);
	if (width) {
		*width = win_w;
	}
	if (height) {
		*height = win_h;
	}
	return factor;
}

/**
 * @brief font size
 *
 * 0 : small
 * 1 : normal
 * 2 : large
 * 3 : huge
 * 4 : giant
 */
int _gl_get_font_size(void)
{
	int mode = -1;
	int retcode = system_settings_get_value_int(SYSTEM_SETTINGS_KEY_FONT_SIZE, &mode);
	gl_dbg("mode: %d", mode);
	if (retcode != SYSTEM_SETTINGS_ERROR_NONE) {
		gl_dbgE("Failed to get font size!");
		return -1;
	}
	return mode;
}

#ifdef _USE_SVI
int _gl_init_svi(void *data)
{
	GL_CHECK_VAL(data, -1);
	gl_appdata *ad = (gl_appdata *)data;

	if (svi_init(&(ad->maininfo.svi_handle)) != 0) {
		gl_dbgE("svi_init failed!");
		return -1;
	}

	return __gl_get_sound_status(data);
}

int _gl_fini_svi(void *data)
{
	GL_CHECK_VAL(data, -1);
	gl_appdata *ad = (gl_appdata *)data;
	if (ad->maininfo.svi_handle) {
		svi_fini(ad->maininfo.svi_handle);
		ad->maininfo.svi_handle = 0;
	}
	return 0;
}

int _gl_play_sound(void *data)
{
	GL_CHECK_VAL(data, -1);
	gl_appdata *ad = (gl_appdata *)data;
	if (ad->maininfo.sound_status)
		svi_play(ad->maininfo.svi_handle, SVI_VIB_NONE,
		         SVI_SND_TOUCH_TOUCH1);
	return 0;
}

int _gl_play_vibration(void *data)
{
	GL_CHECK_VAL(data, -1);
	gl_appdata *ad = (gl_appdata *)data;
	svi_play(ad->maininfo.svi_handle, SVI_VIB_TOUCH_TOUCH, SVI_SND_NONE);
	return 0;
}
#endif

int _gl_reg_storage_state_change_notify(void *data)
{
	__gl_reg_db_noti(data);
	__gl_reg_svi_noti(data);
	return 0;
}

int _gl_dereg_storage_state_change_notify(void *data)
{
	__gl_dereg_db_noti(data);
	__gl_dereg_svi_noti();
	return 0;
}

int _gl_set_file_op_cbs(void *data, void *op_cb, void *del_item_cb,
                        void *update_cb, int total_cnt)
{
	GL_CHECK_VAL(data, -1);
	_gl_thread_set_op_cb(data, op_cb);
	_gl_thread_set_update_cb(data, update_cb);
	_gl_thread_set_del_item_cb(data, del_item_cb);
	_gl_thread_set_total_cnt(data, total_cnt);
	return 0;
}

int _gl_append_album_images_path(void *data, gl_media_s *item)
{
	GL_CHECK_VAL(item, -1);
	GL_CHECK_VAL(item->file_url, -1);
	GL_CHECK_VAL(data, -1);

	gl_get_album_images_path_s *get_d = (gl_get_album_images_path_s *)data;
	GL_CHECK_VAL(get_d->files, -1);

	char **files = get_d->files;
	files[get_d->index++] = g_strdup(item->file_url);
	gl_sdbg("file_url: %s!", files[get_d->index - 1]);
	return 0;
}

int _gl_get_album_images_path(void *data, char **files, bool b_hide)
{
	GL_CHECK_VAL(data, 0);
	GL_CHECK_VAL(files, 0);
	gl_get_album_images_path_s *get_d = g_new0(gl_get_album_images_path_s, 1);
	GL_CHECK_VAL(get_d, 0);
	get_d->files = files;
	int count = _gl_data_get_albums_selected_files(data,
	            _gl_append_album_images_path,
	            (void *)get_d);
	GL_GFREE(get_d);
	return count;
}

int _gl_free_selected_list(void *data)
{
	GL_CHECK_VAL(data, -1);
	int view_mode = gl_get_view_mode(data);
	if (GL_VIEW_ALBUMS == view_mode) {
		_gl_data_finalize_albums_selected_list(data);
	}
	if (GL_VIEW_THUMBS == view_mode) {
		_gl_data_selected_list_finalize(data);
	}
	return 0;
}

int _gl_dlopen_imageviewer(void *data)
{
	GL_CHECK_VAL(data, -1);
	gl_appdata *ad = (gl_appdata *)data;

	if (ad->maininfo.dlopen_iv_handle) {
		gl_dbgE("Already opened imageviewer so lib!");
		return -1;
	}

	ad->maininfo.dlopen_iv_handle = dlopen(GL_SO_PATH_IMAGEVIEWER, RTLD_NOW);
	if (ad->maininfo.dlopen_iv_handle == NULL) {
		gl_sdbgE("Lib %s is not opened, %s!", GL_SO_PATH_IMAGEVIEWER,
		         dlerror());
		return -1;
	}
	gl_sdbg("dlopen %s done", GL_SO_PATH_IMAGEVIEWER);
	return 0;
}

int _gl_dlclose_imageviewer(void *data)
{
	GL_CHECK_VAL(data, -1);
	gl_appdata *ad = (gl_appdata *)data;

	if (ad->maininfo.dlopen_iv_handle) {
		dlclose(ad->maininfo.dlopen_iv_handle);
		ad->maininfo.dlopen_iv_handle = NULL;
		gl_sdbg("dlclose %s done", GL_SO_PATH_IMAGEVIEWER);
		return 0;
	}
	return -1;
}

char *_gl_get_edje_path(void)
{
	char edj_path[1024] = {0};
	char *path = app_get_resource_path();
	if (!path) {
		return NULL;
	}
	snprintf(edj_path, 1024, "%s%s/%s", path , "edje", GL_EDJ_FILE);
	free(path);
	return strdup(edj_path);
}

char * _gl_get_directory_path(int storage_directory_type)
{
	char *path = NULL;
	storage_get_directory(STORAGE_TYPE_INTERNAL, storage_directory_type, &path);
	return path;
}

char * _gl_get_root_directory_path(int storage_type)
{
	char *path = NULL;
	storage_get_root_directory(storage_type, &path);
	return path;
}
