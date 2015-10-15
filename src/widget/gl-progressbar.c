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

#include "gallery.h"
#include "gl-progressbar.h"
#include "gl-ui-util.h"
#include "gl-util.h"
#include "gl-debug.h"
#include "gl-albums.h"
#include "gl-thumbs.h"
#include "gl-controlbar.h"
#include "gl-thread-util.h"
#include "gl-strings.h"
#include "gl-button.h"

#define GL_ALBUMS_PBAR_BG_COLOR_VAL 48
#define GL_WEB_DOWNLOAD_INVALID_ID 0//-2
#define GL_PBAR_STATUS_LEN_MAX 128
#define GL_PBAR_WIDTH_MIN_INC (576*elm_config_scale_get()) //(144x4)

#define GL_PROGRESSBAR_STYLE_GALLERY_PROGRESS "gallery/list_progress"
#define GL_PROGRESSBAR_STYLE_PROGRESS "list_progress"
#define GL_PROGRESSBAR_STYLE_WHEEL "wheel"
#define GL_LABEL_STYLE_DEFAULT GL_CHECKBOX_STYLE_DEFAULT
#define GL_PROGRESSBAR_STYLE_PROCESS "list_process"
/* Font style for show text of downloading web album */
#define GL_FONT_STYLE_ALBUM_S "<color=#000000FF><font=Tizen:style=Medium text_class=tizen><font_size=28>"
#define GL_FONT_STYLE_ALBUM_E "</font_size></font></color>"

static void __gl_pb_cancel_thread_pbar_cb(void *data, Evas_Object *obj, void *ei)
{
	gl_dbg("");
	/* set cancel false value */
	GL_CHECK(data);
	gl_appdata *ad = (gl_appdata *)data;
	ad->maininfo.hide_noti = true;
	gl_thread_set_cancel_state(data, GL_PB_CANCEL_BUTTON);
	/* delete progressbar */
	gl_pb_del_pbar(data);
}

static Eina_Bool __gl_pb_pbar_timer_cb(void *data)
{
	gl_dbg("Timeout, destroy progressbar and timer.");
	gl_pb_del_pbar(data);
	return ECORE_CALLBACK_CANCEL;
}

#if 0
/* We need to perform cancellation, not just delete popup */
static inline void __gl_pb_ea_back_cb(void *data, Evas_Object *obj, void *ei)
{
	gl_dbg("Use customized callback");
	Evas_Object *btn_cancel = NULL;
	btn_cancel = elm_object_part_content_get(obj, "button1");
	GL_CHECK(btn_cancel);
	evas_object_smart_callback_call(btn_cancel, "clicked", NULL);
}
#endif

int gl_pb_make_thread_pbar_wheel(void *data, Evas_Object *parent, char *title)
{
	GL_CHECK_VAL(data, -1);
	gl_appdata *ad = (gl_appdata *)data;
	Evas_Object *popup = NULL;
	Evas_Object *progressbar = NULL;

	gl_pb_del_pbar(ad);
	popup = elm_popup_add(parent);

	/*Delete the Popup if the Popup has a BACK event.*/
	eext_object_event_callback_add(popup, EEXT_CALLBACK_BACK,
			__gl_pb_cancel_thread_pbar_cb, data);
	Evas_Object *popupLayout = elm_layout_add(popup);
	elm_layout_file_set(popupLayout, GL_EDJ_FILE, "popup_text_progressbar_view_layout");
	evas_object_size_hint_weight_set(popupLayout, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	evas_object_show(popupLayout);

	progressbar = elm_progressbar_add(popup);
	elm_object_style_set(progressbar, GL_PROGRESSBAR_STYLE_WHEEL);
	evas_object_size_hint_align_set(progressbar, EVAS_HINT_FILL, EVAS_HINT_FILL);
	evas_object_size_hint_weight_set(progressbar, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	elm_progressbar_pulse(progressbar, EINA_TRUE);
	evas_object_show(progressbar);

	if (title && (!strcmp(GL_STR_ID_MOVING, title) || !strcmp(GL_STR_ID_COPYING, title))) {
		elm_object_part_content_set(popupLayout, "progressbar", progressbar);
	} else {
		elm_object_part_content_set(popupLayout, "progressbar_left", progressbar);
		_gl_ui_set_translate_part_str(popupLayout, "elm.text.description", title);
	}
	elm_object_content_set(popup, popupLayout);
	evas_object_show(popup);

	ad->pbarinfo.popup = popup;
	ad->pbarinfo.pbar = progressbar;
	ad->pbarinfo.finished_cnt = 0;


	return 0;
}

int gl_pb_refresh_thread_pbar(void *data, int cur_cnt, int total_cnt)
{
	GL_CHECK_VAL(data, -1);
	char status_info[GL_POPUP_DESC_LEN_MAX] = { 0, };
	double percent = 0.0;
	gl_appdata *ad = (gl_appdata *)data;
	GL_CHECK_VAL(ad->pbarinfo.pbar, -1);
	GL_CHECK_VAL(ad->pbarinfo.status_label, -1);

	snprintf(status_info, sizeof(status_info),
		 GL_FONT_STYLE_POP_S"%d/%d"GL_FONT_STYLE_POP_E, cur_cnt,
		 total_cnt);
	/* Save medias count already operated */
	ad->pbarinfo.finished_cnt = cur_cnt;
	elm_object_text_set(ad->pbarinfo.status_label, status_info);
	evas_object_show(ad->pbarinfo.status_label);

	if (total_cnt != 0) {
		percent = (double)cur_cnt / (double)total_cnt;
		elm_progressbar_value_set(ad->pbarinfo.pbar, percent);
	}

	return 0;
}

/* Use timer to destroy progressbar */
int gl_pb_add_pbar_timer(void *data)
{
	GL_CHECK_VAL(data, -1);
	gl_appdata *ad = (gl_appdata *)data;
	ad->pbarinfo.pbar_timer = ecore_timer_add(1.0f, __gl_pb_pbar_timer_cb,
						  data);
	return 0;
}

int gl_pb_del_pbar(void *data)
{
	GL_CHECK_VAL(data, -1);
	gl_appdata *ad = (gl_appdata *)data;
	gl_dbg("destroy progressbar");

	if (ad->pbarinfo.popup) {
		evas_object_del(ad->pbarinfo.popup);
		ad->pbarinfo.popup = NULL;
		ad->pbarinfo.pbar = NULL;
		ad->pbarinfo.status_label = NULL;
		ad->pbarinfo.finished_cnt = 0;
	}
	GL_IF_DEL_TIMER(ad->pbarinfo.pbar_timer);
	return 0;
}

Evas_Object *gl_pb_make_pbar(void *data, Evas_Object *parent, char *state)
{
	GL_CHECK_NULL(data);
	gl_appdata *ad = (gl_appdata *)data;
	gl_dbg("Make progressbar, state:%s", state);
	char label_str[GL_POPUP_DESC_LEN_MAX] = { 0, };

	if (ad->pbarinfo.popup) {
		if (ad->pbarinfo.pbar_timer) {
			gl_dbg("Del pbar timer in update case!");
			ecore_timer_del(ad->pbarinfo.pbar_timer);
			ad->pbarinfo.pbar_timer = NULL;
		}
		gl_dbg("Progressbar already created, update its label.");
		snprintf(label_str, sizeof(label_str),
			 GL_FONT_STYLE_POP_S"%s"GL_FONT_STYLE_POP_E, state);
		elm_object_text_set(ad->pbarinfo.status_label, label_str);
		return ad->pbarinfo.popup;
	}
	gl_pb_del_pbar(ad);
	Evas_Object *popup = NULL;
	Evas_Object *progressbar = NULL;
	Evas_Object *layout = NULL;
	Evas_Object *label = NULL;

	popup = elm_popup_add(parent);

	/*Delete the Popup if the Popup has a BACK event.*/
	eext_object_event_callback_add(popup, EEXT_CALLBACK_BACK, eext_popup_back_cb,
				     NULL);

	elm_object_style_set(popup, "content_no_vhpad");

	label = elm_label_add(popup);
	elm_object_style_set(label, "popup/default");
	snprintf(label_str, sizeof(label_str),
		 GL_FONT_STYLE_POP_S"%s"GL_FONT_STYLE_POP_E, state);
	elm_object_text_set(label, label_str);
	/**
	* width got from evas_object_geometry_get equals 0
	* Text couldn't be showed, comment 3 lines codes below
	*/
	/*Evas_Coord width = 0;
	evas_object_geometry_get(popup, NULL, NULL, &width, NULL);
	elm_label_wrap_width_set(label, (int)(width * 0.5));*/
	elm_label_line_wrap_set(label, ELM_WRAP_MIXED);
	evas_object_size_hint_align_set(label, EVAS_HINT_FILL, EVAS_HINT_FILL);
	evas_object_size_hint_weight_set(label, EVAS_HINT_EXPAND, 0.0);
	evas_object_show(label);

	layout = elm_layout_add(popup);
	elm_layout_file_set(layout, GL_EDJ_FILE, "popup_processing");
	evas_object_size_hint_weight_set(layout, EVAS_HINT_EXPAND,
					 EVAS_HINT_EXPAND);

	progressbar = elm_progressbar_add(popup);
	elm_object_style_set(progressbar, GL_PROGRESSBAR_STYLE_PROCESS);
	elm_progressbar_horizontal_set(progressbar, EINA_TRUE);
	elm_progressbar_unit_format_set(progressbar, NULL);
	elm_progressbar_pulse(progressbar, EINA_TRUE);
	evas_object_size_hint_align_set(progressbar, EVAS_HINT_FILL,
					EVAS_HINT_FILL);
	evas_object_size_hint_weight_set(progressbar, EVAS_HINT_EXPAND,
					 EVAS_HINT_EXPAND);
	evas_object_show(progressbar);

	elm_object_part_content_set(layout, "elm.swallow.content", progressbar);
	elm_object_part_content_set(layout, "elm.swallow.text", label);

	elm_object_content_set(popup, layout);
	evas_object_show(popup);

	ad->pbarinfo.popup = popup;
	ad->pbarinfo.pbar = progressbar;
	ad->pbarinfo.status_label = label;
	return popup;
}

Evas_Object *_gl_pb_make_place_pbar(Evas_Object *parent)
{
	GL_CHECK_NULL(parent);
	gl_dbg("");

	Evas_Object *progressbar = NULL;
	progressbar = elm_progressbar_add(parent);
	elm_object_style_set(progressbar, GL_PROGRESSBAR_STYLE_PROCESS);
	elm_progressbar_unit_format_set(progressbar, NULL);
	elm_progressbar_pulse(progressbar, EINA_TRUE);
	evas_object_size_hint_align_set(progressbar, EVAS_HINT_FILL,
					EVAS_HINT_FILL);
	evas_object_size_hint_weight_set(progressbar, EVAS_HINT_EXPAND,
					 EVAS_HINT_EXPAND);
	evas_object_show(progressbar);
	return progressbar;
}

