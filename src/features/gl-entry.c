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

#include "gl-entry.h"
#include "gl-ui-util.h"
#include "gl-util.h"
#include "gl-button.h"
#include "gl-editfield.h"
#include "gl-controlbar.h"
#include "gl-debug.h"
#include "gl-strings.h"
#include "gl-fs.h"
#include "gl-popup.h"
#include "gl-notify.h"

typedef struct _gl_entry_t gl_entry_s;
struct _gl_entry_t {
	Evas_Object *layout;
	Elm_Object_Item *nf_it;
	Elm_Object_Item *pre_nf_it;
	Evas_Object *done_btn;

	void *ad;
};

#if 0
/* Free data after layout deleted */
static void __gl_entry_layout_del_cb(void *data, Evas *e, Evas_Object *obj,
                                     void *ei)
{
	gl_dbg("Delete entry layout ---");
	GL_FREEIF(data);
	gl_dbg("Delete entry layout +++");
}

static Eina_Bool __gl_entry_pop_cb(void *data, Elm_Object_Item *it)
{
	GL_CHECK_FALSE(data);
	gl_entry_s *entry_d = (gl_entry_s *)data;
	GL_CHECK_FALSE(entry_d->ad);
	gl_dbg("");

	/* Call cancel callback */
	Evas_Object *entry = _gl_editfield_get_entry(entry_d->ad);
	GL_CHECK_FALSE(entry);
	Eina_Bool(*pop_cb)(void * data, Elm_Object_Item * it);
	pop_cb = evas_object_data_get(entry, "gl_entry_pop_cb_key");
	/* Clear editfield variables */
	_gl_editfield_destroy_imf(entry_d->ad);
	if (pop_cb) {
		return pop_cb(entry_d->ad, it);
	} else {
		return EINA_TRUE;
	}
}
#endif

static void __gl_entry_done_cb(void *data, Evas_Object *obj, void *ei)
{
	GL_CHECK(data);
	gl_entry_s *entry_d = (gl_entry_s *)data;
	GL_CHECK(entry_d->ad);
	gl_dbg("");
	/* Hide editfield */
	_gl_editfield_hide_imf(entry_d->ad);
	gl_appdata *ad = (gl_appdata *)entry_d->ad;

	Evas_Object *entry = _gl_editfield_get_entry(entry_d->ad);
	GL_CHECK(entry);

	int (*process_cb)(void * data, bool b_enter);
	process_cb = evas_object_data_get(entry, "gl_entry_process_cb_key");
	GL_CHECK(process_cb);
	/* Delete callback when it is clicked to prevent it is called for many times */
	if (process_cb(entry_d->ad, false) == 0)
		evas_object_smart_callback_del(obj, "clicked",
		                               __gl_entry_done_cb);
	/* Clear nf_it to cancel hide title in landscape mode */
	ad->entryinfo.nf_it = NULL;
	GL_FREEIF(ad->albuminfo.temp_album_name);
}

static void __gl_entry_cancel_cb(void *data, Evas_Object *obj, void *ei)
{
	GL_CHECK(obj);
	GL_CHECK(data);
	gl_dbg("");
	/* Hide editfield */
	_gl_editfield_hide_imf(data);
	gl_appdata *ad = (gl_appdata *)data;
	GL_IF_DEL_OBJ(ad->popupinfo.popup);
	GL_FREEIF(ad->albuminfo.temp_album_name);
}

#if 0
static void __gl_entry_trans_finished_cb(void *data, Evas_Object *obj, void *ei)
{
	evas_object_smart_callback_del(obj, GL_TRANS_FINISHED,
	                               __gl_entry_trans_finished_cb);
	GL_CHECK(data);
	gl_dbg("");

	Evas_Object *entry = _gl_editfield_get_entry(data);
	GL_CHECK(entry);

	int (*transit_cb)(void * data);
	transit_cb = evas_object_data_get(entry, "gl_entry_transit_cb_key");
	if (transit_cb) {
		transit_cb(data);
	}

}
#endif

/**
 *  Use naviframe api to push albums rename view to stack.
 *  @param obj is the content to be pushed.
 */
static int __gl_entry_push_view(void *data, Evas_Object *parent,
                                Evas_Object *obj, char *title,
                                gl_entry_s *entry_d)
{
	gl_dbg("");
	GL_CHECK_VAL(parent, -1);
	GL_CHECK_VAL(data, -1);
	GL_CHECK_VAL(entry_d, -1);
	gl_appdata *ad = (gl_appdata *)data;
	Evas_Object *left_btn = NULL;
	Evas_Object *right_btn = NULL;

	right_btn = _gl_but_create_image_but(parent, NULL, GL_STR_ID_SAVE,
	                                     NULL, __gl_entry_done_cb, entry_d, EINA_FALSE);
	GL_CHECK_VAL(right_btn, -1);
	ad->entryinfo.done_btn = right_btn;
	entry_d->done_btn = right_btn;

	/* Cancel */
	left_btn = _gl_but_create_image_but(parent, NULL, GL_STR_ID_CANCEL,
	                                    NULL, __gl_entry_cancel_cb, data, EINA_FALSE);
	GL_CHECK_VAL(left_btn, -1);

	elm_object_part_content_set(parent, "button1", left_btn);
	elm_object_part_content_set(parent, "button2", right_btn);

	/* Add callbacks to show/hide title toolbar */
	/* _gl_ui_add_conform_cbs(data); */
	/* hide title toolbar */
	/* _gl_editfield_change_navi_title(data, ad->maininfo.rotate_mode); */
	return 0;
}

void _gl_popup_create_folder_imf_changed_cb(void *data, Evas_Object *obj, void *event_info)
{
	GL_CHECK(data);
	gl_appdata *ad = (gl_appdata *)data;
	const char *entry_data = NULL;
	char *name = NULL;
	char new_str[GL_ALBUM_NAME_LEN_MAX] = {0,};

	entry_data = elm_entry_entry_get(ad->entryinfo.entry);
	name = elm_entry_markup_to_utf8(entry_data);
	GL_CHECK(name);

	if (!_gl_fs_validate_name(name)) {
		strncpy(new_str, name, GL_ALBUM_NAME_LEN_MAX - 1);
		int position = elm_entry_cursor_pos_get(ad->entryinfo.entry);
		elm_entry_entry_set(ad->entryinfo.entry, elm_entry_utf8_to_markup(ad->albuminfo.temp_album_name));
		elm_entry_cursor_begin_set(ad->entryinfo.entry);
		elm_entry_cursor_pos_set(ad->entryinfo.entry, position - 1);
		_gl_notify_create_notiinfo(_gl_str(GL_STR_ID_INVALID_INPUT_PARAMETER));
		GL_FREEIF(name);
		return;
	}
	GL_FREEIF(ad->albuminfo.temp_album_name);
	ad->albuminfo.temp_album_name = strdup(name);
	free(name);
}

static void _showFinishedCb(void *data, Evas_Object *obj, void *event_info)
{
	gl_appdata *ad = (gl_appdata *)data;
	_gl_editfield_show_imf(ad);
	evas_object_smart_callback_del(ad->popupinfo.popup, "show,finished",
	                               _showFinishedCb);
}

int _gl_entry_create_view(void *data, char *name, char *title)
{
	GL_CHECK_VAL(data, -1);
	gl_appdata *ad = (gl_appdata *)data;
	Evas_Object *entry_ly = NULL;
	gl_dbg("");

	gl_entry_s *entry_d = NULL;
	entry_d = (gl_entry_s *)calloc(1, sizeof(gl_entry_s));
	GL_CHECK_VAL(entry_d, -1);

	if (ad->popupinfo.popup) {
		GL_IF_DEL_OBJ(ad->popupinfo.popup);
		ad->popupinfo.popup = NULL;
	}

	Evas_Object *popup = NULL;
	popup = elm_popup_add(ad->maininfo.layout);
	ad->popupinfo.popup = popup;
	evas_object_size_hint_weight_set(popup, EVAS_HINT_EXPAND,
	                                 EVAS_HINT_EXPAND);

	_gl_ui_set_translate_part_str(popup, "title,text", title);

	entry_ly = elm_layout_add(popup);
	char *path = _gl_get_edje_path();
	GL_CHECK_VAL(path, -1);
	elm_layout_file_set(entry_ly, path, "popup_input_text");
	free(path);

	Evas_Object *entry = elm_entry_add(entry_ly);
	if (entry == NULL) {
		GL_IF_DEL_OBJ(entry_ly);
		GL_IF_DEL_OBJ(popup);
		if (entry_d) {
			free(entry_d);
		}
		return -1;
	}

	elm_entry_single_line_set(entry, EINA_TRUE);
	elm_entry_scrollable_set(entry, EINA_TRUE);
	elm_object_part_content_set(entry_ly, "elm.swallow.content", entry);
	elm_entry_cnp_mode_set(entry, ELM_CNP_MODE_PLAINTEXT);
	elm_entry_input_panel_layout_set(entry, ELM_INPUT_PANEL_LAYOUT_NORMAL);
	elm_entry_input_panel_layout_variation_set(entry, ELM_INPUT_PANEL_LAYOUT_NORMAL_VARIATION_FILENAME);

	ad->entryinfo.entry = entry;
	ad->entryinfo.context = elm_entry_imf_context_get(ad->entryinfo.entry);
	ad->entryinfo.editfield = entry_ly;
	evas_object_smart_callback_add(entry, "changed", (Evas_Smart_Cb)_gl_popup_create_folder_imf_changed_cb, ad);
	/* Set callback */
	char *new_name = NULL;
	char *full_path = NULL;
	char *default_images_path = _gl_get_directory_path(STORAGE_DIRECTORY_IMAGES);
	GL_CHECK_VAL(default_images_path, -1);
	full_path = (char *)_gl_fs_get_unique_new_album_name(default_images_path, GL_STR_ID_ALBUM_DEFAULT, &new_name);
	__gl_editfield_set_entry(ad, entry_ly, entry, new_name);
	elm_entry_select_all(entry);
	GL_FREEIF(new_name);
	GL_FREEIF(full_path);
	elm_object_content_set(popup, entry_ly);
	evas_object_show(popup);

	entry_d->ad = ad;

	__gl_entry_push_view(ad, popup, NULL, title,
	                     entry_d);

	/* Register delete callback */
	eext_object_event_callback_add(popup, EEXT_CALLBACK_BACK,
	                               __gl_entry_cancel_cb, entry_d->ad);
	evas_object_smart_callback_add(popup, "show,finished",
	                               _showFinishedCb, entry_d->ad);
	ad->entryinfo.b_hide_sip = true;
	return 0;

}

int _gl_entry_resume_view(void *data)
{
	GL_CHECK_VAL(data, -1);
	gl_appdata *ad = (gl_appdata *)data;
	Elm_Object_Item *top_nf_it = NULL;

	if (!ad->entryinfo.nf_it) {
		return -1;
	}

	top_nf_it = elm_naviframe_top_item_get(ad->maininfo.naviframe);
	if (ad->entryinfo.nf_it != top_nf_it) {
		gl_dbg("Hide Entry cursor and IMF");
		_gl_editfield_hide_imf(ad);
	} else if (ad->entryinfo.editfield) {
		/* Show cursor and IMF if not popup showed */
		if (ad->popupinfo.popup) {
			gl_dbg("Hide Entry cursor and IMF");
			_gl_editfield_hide_imf(ad);
		} else {
			gl_dbg("Show Entry cursor and IMF");
			_gl_editfield_show_imf(ad);
		}
	}
	return 0;
}

