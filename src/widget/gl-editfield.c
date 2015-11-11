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

#include "gl-editfield.h"
#include "gl-ui-util.h"
#include "gl-util.h"
#include "gl-popup.h"
#include "gl-data.h"
#include "gl-strings.h"
#include "gl-debug.h"
#include "gl-notify.h"

/* Maximum number of entry string, 50 Chars of album name (UI) */
#define GL_ENTRY_STR_CHAR_MAX 50

/* The maximun length reached callback */
static void __gl_editfield_maxlen_reached_cb(void *data, Evas_Object *obj,
        void *ei)
{
	gl_dbg("Entry maximum length reached, show notification.");
	GL_CHECK(data);
	/* Show popup */
	_gl_notify_create_notiinfo(_gl_str(GL_STR_ID_MAX_NO_CHARS_REACHED));
}

/*
* Callback registered to signal 'changed' of entry.
* It would get the entry string and check whether it's different
* with original string or not, if so, enable 'Done' button, other disable it.
*/
static void __gl_editfield_space_check_cb(void *data, Evas_Object *obj, void *ei)
{
	GL_CHECK(obj);
	GL_CHECK(data);
	gl_appdata *ad = (gl_appdata *)data;

	GL_CHECK(ad->entryinfo.done_btn);
	const char *entry_str = NULL;

	/* Get entry string. */
	entry_str = elm_entry_entry_get(obj);
	GL_CHECK(entry_str);
	/**
	* Changes entry string to utf-8 encoding format,
	* other some special characters cannot be showed correctly.
	*/
	char *entry_utf8 = elm_entry_markup_to_utf8(entry_str);
	GL_CHECK(entry_utf8);
	/**
	*  Removes leading and trailing whitespace
	*  to avoid showing popup when new name is only composed by space
	*  or avoid showing popup in the case when after trip leading and trailing space,
	*  new name is same as existing name
	*/
	g_strstrip((gchar *)entry_utf8);
	GL_CHECK(entry_utf8);
	gl_sdbg("New entry string without space: [%s]", entry_utf8);

	bool b_disabled = false;
	/**
	* If entry string is empty, disable 'Done' button,
	* including create album/tag and rename album/tag.
	*/
	if (!strlen(entry_utf8)) {
		gl_dbg("Entry string is empty!");
		b_disabled = true;
	} else {
		int (*valiate_cb)(void * data, char * text);
		valiate_cb = evas_object_data_get(obj, "gl_entry_validate_cb_key");
		gl_dbgW("valiate_cb[%p]", valiate_cb);
		if (valiate_cb && valiate_cb(data, entry_utf8) < 0) {
			entry_utf8[strlen(entry_utf8) - 1] = '\0';
			elm_entry_entry_set(obj, entry_utf8);
			elm_entry_cursor_end_set(obj);
			if (!strlen(entry_utf8)) {
				b_disabled = true;
			}
		}
	}

	gl_dbg("En/Disable Done button[%d]", b_disabled);
	elm_object_disabled_set(ad->entryinfo.done_btn, b_disabled);

	/* Free string got from elm_entry_markup_to_utf8() */
	GL_FREEIF(entry_utf8);
}

/*
*   make a new album
*/
static void __gl_editfield_enter_cb(void *data, Evas_Object *obj, void *ei)
{
	gl_dbg("");
	GL_CHECK(obj);
	GL_CHECK(data);
	/* Unfocus to hide cursor */
	_gl_editfield_hide_imf(data);
	int (*process_cb)(void * data, bool b_enter);
	process_cb = evas_object_data_get(obj, "gl_entry_process_cb_key");
	gl_dbgW("process_cb[%p]", process_cb);
	if (process_cb) {
		process_cb(data, true);
	} else {
		gl_dbgW("Invalid process_cb");
	}
}

int __gl_editfield_set_entry(void *data, Evas_Object *layout,
                             Evas_Object *entry, char *default_label)
{
	GL_CHECK_VAL(data, -1);
	gl_appdata *ad = (gl_appdata *)data;
	gl_dbg("");

	GL_FREEIF(ad->entryinfo.limit_filter);
	Elm_Entry_Filter_Limit_Size *limit_filter = (Elm_Entry_Filter_Limit_Size *)calloc(1, sizeof(Elm_Entry_Filter_Limit_Size));
	GL_CHECK_VAL(limit_filter, -1);
	ad->entryinfo.limit_filter = limit_filter;
	/* Album name, the length is limited to 50 Chars*/
	limit_filter->max_char_count = GL_ENTRY_STR_CHAR_MAX;
	limit_filter->max_byte_count = 0;

	elm_entry_markup_filter_append(entry, elm_entry_filter_limit_size,
	                               limit_filter);
	elm_entry_input_panel_return_key_type_set(entry,
	        ELM_INPUT_PANEL_RETURN_KEY_TYPE_DONE);
	elm_entry_autocapital_type_set(entry, ELM_AUTOCAPITAL_TYPE_SENTENCE);

	if (gl_set_entry_text(entry, default_label) != 0) {
		gl_dbgE("gl_set_entry_text failed!");
		return -1;
	}
	elm_entry_cursor_end_set(entry);
	evas_object_smart_callback_add(entry, "maxlength,reached",
	                               __gl_editfield_maxlen_reached_cb, ad);
	evas_object_smart_callback_add(entry, "activated",
	                               __gl_editfield_enter_cb, ad);
	/* Add space check callback */
	evas_object_smart_callback_add(entry, "changed",
	                               __gl_editfield_space_check_cb, ad);
	evas_object_smart_callback_add(entry, "preedit,changed",
	                               __gl_editfield_space_check_cb, ad);
	return 0;
}

Evas_Object *_gl_editfield_create(void *data, Evas_Object *parent,
                                  char *default_label)
{
	GL_CHECK_NULL(data);
	gl_appdata *ad = (gl_appdata *)data;
	GL_CHECK_NULL(parent);
	Evas_Object *layout = NULL;
	Evas_Object *entry_layout = NULL;
	Evas_Object *entry = NULL;

	layout = elm_layout_add(parent);
	GL_CHECK_NULL(layout);
	elm_layout_theme_set(layout, "layout", "application", "searchbar_base");
	elm_object_signal_emit(layout, "elm,state,show,searchbar", "elm");

	entry_layout = elm_layout_add(layout);
	if (entry_layout == NULL) {
		evas_object_del(layout);
		return NULL;
	}
	/* Customized theme */
	elm_layout_theme_set(entry_layout, "layout", "searchbar", "default");
	elm_object_part_content_set(layout, "searchbar", entry_layout);

	entry = elm_entry_add(entry_layout);
	if (entry == NULL) {
		evas_object_del(entry_layout);
		return NULL;
	}

	elm_entry_single_line_set(entry, EINA_TRUE);
	elm_entry_scrollable_set(entry, EINA_TRUE);
	elm_object_part_content_set(entry_layout, "elm.swallow.content", entry);
	elm_entry_cnp_mode_set(entry, ELM_CNP_MODE_PLAINTEXT);
	elm_entry_input_panel_layout_set(entry, ELM_INPUT_PANEL_LAYOUT_NORMAL);

	ad->entryinfo.entry = entry;
	ad->entryinfo.context = elm_entry_imf_context_get(ad->entryinfo.entry);
	ad->entryinfo.editfield = entry_layout;
	/* Set callback */
	__gl_editfield_set_entry(ad, entry_layout, entry, default_label);
	evas_object_show(entry_layout);

	return layout;
}

/* Add editfield to genlist item */
Evas_Object *_gl_editfield_create_genlist(void *data, Evas_Object *parent,
        Elm_Object_Item *it, char *label)
{
	GL_CHECK_NULL(label);
	GL_CHECK_NULL(parent);
	GL_CHECK_NULL(data);
	gl_appdata *ad = (gl_appdata *)data;
	gl_dbg("");
	Evas_Object *entry = NULL;

	_gl_editfield_destroy_imf(ad);
	entry = elm_entry_add(parent);
	if (entry == NULL) {
		gl_dbgE("elm_entry_add failed!");
		return NULL;
	}

	elm_entry_single_line_set(entry, EINA_TRUE);
	elm_entry_scrollable_set(entry, EINA_TRUE);
	GL_FREEIF(ad->entryinfo.limit_filter);
	Elm_Entry_Filter_Limit_Size *limit_filter = (Elm_Entry_Filter_Limit_Size *)calloc(1, sizeof(Elm_Entry_Filter_Limit_Size));
	GL_CHECK_NULL(limit_filter);
	ad->entryinfo.limit_filter = limit_filter;
	/* Tag name, the length is limited to 255 Chars */
	limit_filter->max_char_count = GL_ENTRY_STR_CHAR_MAX;
	limit_filter->max_byte_count = 0;
	elm_entry_cnp_mode_set(entry, ELM_CNP_MODE_PLAINTEXT);
	elm_entry_input_panel_return_key_type_set(entry,
	        ELM_INPUT_PANEL_RETURN_KEY_TYPE_DONE);
	elm_entry_autocapital_type_set(entry, ELM_AUTOCAPITAL_TYPE_SENTENCE);
	elm_entry_markup_filter_append(entry, elm_entry_filter_limit_size,
	                               limit_filter);
	evas_object_smart_callback_add(entry, "maxlength,reached",
	                               __gl_editfield_maxlen_reached_cb, ad);
	evas_object_smart_callback_add(entry, "activated",
	                               __gl_editfield_enter_cb, ad);
	/**
	 * Do not unset rename mode on unfocused event
	 *otherwise clipboard does not work properly
	 */
	ad->entryinfo.entry = entry;
	ad->entryinfo.context = elm_entry_imf_context_get(entry);

	_gl_editfield_show_imf(data);
	gl_set_entry_text(entry, label);
	elm_entry_cursor_end_set(entry);

	if (!ad->entryinfo.b_hide_toolbar) {
		ad->entryinfo.b_hide_toolbar = true;
		_gl_ui_set_toolbar_state(data, ad->entryinfo.b_hide_toolbar);
	}
	return entry;
}

Evas_Object *_gl_editfield_create_entry(void *data, Evas_Object *layout, char *text)
{
	GL_CHECK_NULL(data);
	gl_appdata *ad = (gl_appdata *)data;
	static Elm_Entry_Filter_Limit_Size limit_filter_data;

	Evas_Object *entry = elm_entry_add(layout);
	GL_CHECK_NULL(entry);

	elm_entry_single_line_set(entry, EINA_TRUE);
	elm_entry_scrollable_set(entry, EINA_TRUE);

	limit_filter_data.max_char_count = GL_ENTRY_STR_CHAR_MAX;
	if (text) {
		elm_entry_entry_set(entry, text);
	}
	elm_entry_cursor_end_set(entry);

	elm_entry_markup_filter_append(entry, elm_entry_filter_limit_size,
	                               &limit_filter_data);

	evas_object_smart_callback_add(entry, "maxlength,reached",
	                               __gl_editfield_maxlen_reached_cb, data);
	evas_object_smart_callback_add(entry, "changed",
	                               __gl_editfield_space_check_cb, data);
	evas_object_smart_callback_add(entry, "preedit,changed",
	                               __gl_editfield_space_check_cb, data);
	evas_object_smart_callback_add(entry, "activated",
	                               __gl_editfield_enter_cb, data);
	elm_entry_cnp_mode_set(entry, ELM_CNP_MODE_PLAINTEXT);
	elm_entry_input_panel_return_key_type_set(entry,
	        ELM_INPUT_PANEL_RETURN_KEY_TYPE_DONE);
	elm_entry_autocapital_type_set(entry, ELM_AUTOCAPITAL_TYPE_SENTENCE);
	ad->entryinfo.entry = entry;
	ad->entryinfo.context = elm_entry_imf_context_get(entry);

	return entry;
}

/* Hide input panel and entry cursor */
int _gl_editfield_hide_imf(void *data)
{
	GL_CHECK_VAL(data, -1);
	gl_appdata *ad = (gl_appdata *)data;
	gl_dbg("");

	/* Unfocus to hide cursor */
	if (ad->entryinfo.entry) {
		elm_object_focus_set(ad->entryinfo.entry, EINA_FALSE);
	}

	return 0;
}

/* Show input panel and entry cursor */
int _gl_editfield_show_imf(void *data)
{
	GL_CHECK_VAL(data, -1);
	gl_appdata *ad = (gl_appdata *)data;
	gl_dbg("");

	if (ad->entryinfo.entry) {
		evas_object_show(ad->entryinfo.entry);
		elm_object_focus_set(ad->entryinfo.entry, EINA_TRUE);
	}

	return 0;
}

int _gl_editfield_destroy_imf(void *data)
{
	GL_CHECK_VAL(data, -1);
	gl_appdata *ad = (gl_appdata *)data;

	if (ad->entryinfo.entry == NULL) {
		return -1;
	}
	/* Remove callbacks */
	_gl_ui_del_conform_cbs(ad->maininfo.naviframe);

	evas_object_smart_callback_del(ad->entryinfo.entry,
	                               "maxlength,reached",
	                               __gl_editfield_maxlen_reached_cb);
	evas_object_smart_callback_del(ad->entryinfo.entry,
	                               "activated", __gl_editfield_enter_cb);

	evas_object_smart_callback_del(ad->entryinfo.entry,
	                               "changed",
	                               __gl_editfield_space_check_cb);
	evas_object_smart_callback_del(ad->entryinfo.entry,
	                               "preedit,changed",
	                               __gl_editfield_space_check_cb);
	ad->entryinfo.mode = GL_ENTRY_NONE;

	GL_IF_DEL_OBJ(ad->entryinfo.popup);
	GL_IF_DEL_OBJ(ad->entryinfo.editfield);
	GL_FREEIF(ad->entryinfo.limit_filter);
	ad->entryinfo.entry = NULL;
	ad->entryinfo.context = NULL;
	ad->entryinfo.done_btn = NULL;
	ad->entryinfo.nf_it = NULL;
	ad->entryinfo.op_cb = NULL;
	GL_FREEIF(ad->albuminfo.temp_album_name);
	return 0;
}

int _gl_editfield_change_navi_title(void *data, int r)
{
	GL_CHECK_VAL(data, -1);
	gl_appdata *ad = (gl_appdata *)data;
	if (ad->entryinfo.nf_it) {
		gl_dbg("entryinfo.nf_it created");
		if (!ad->entryinfo.b_hide_sip) {
			gl_dbg("SIP is hided, no need hide the title");
			return 0;
		}
	} else {
		gl_dbg("entryinfo.nf_it not created");
		return -1;
	}
	GL_CHECK_VAL(ad->maininfo.naviframe, -1);
	Elm_Object_Item *navi_it = elm_naviframe_top_item_get(ad->maininfo.naviframe);
	GL_CHECK_VAL(navi_it, -1);
	switch (r) {
	case APP_DEVICE_ORIENTATION_0:
	case APP_DEVICE_ORIENTATION_180:
		elm_naviframe_item_title_enabled_set(navi_it, EINA_TRUE, EINA_FALSE);
		break;
	case APP_DEVICE_ORIENTATION_90:
	case APP_DEVICE_ORIENTATION_270:
		elm_naviframe_item_title_enabled_set(navi_it, EINA_FALSE, EINA_FALSE);
		break;
	default:
		break;
	}
	return 0;
}

Evas_Object *_gl_editfield_get_entry(void *data)
{
	GL_CHECK_NULL(data);
	gl_appdata *ad = (gl_appdata *)data;
	return ad->entryinfo.entry;
}

