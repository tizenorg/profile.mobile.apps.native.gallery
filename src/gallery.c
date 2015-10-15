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
#include <Evas.h>
#include <signal.h>
#include "gl-main.h"
#include "gl-debug.h"
#include "gl-ui-util.h"
#include "gl-lang.h"
#include "gl-util.h"
#include "gl-data.h"
#include "gl-ext-ug-load.h"
#include "gl-ext-exec.h"
#include "gl-popup.h"
#include "gl-entry.h"
#include "gl-db-update.h"
#include "gl-albums.h"

/* *
 * The basic policy of Tizen applications for OOM case is "silent exit" as
 * Android. Kernel will send SIGTERM signal (standard unix signal) to
 * background application when OOM happens.
 * Application should register SIGTERM handler and silently free every
 * critical resource and exit.
*/

static bool _gallery_create(void *data);
static void __gallery_sigterm_handler(int signum)
{
	/* do something for signal handling */
	gl_dbgW(">>>>>>>>>>>>>>>>> SIGTERM >>>>>>>>>>>>>>> Gallery ");
	elm_exit();
}

static void _gallery_lang_changed_cb(app_event_info_h event_info, void *data)
{
	GL_CHECK(data);
	gl_appdata *ad = (gl_appdata *)data;
	evas_object_smart_callback_call(ad->maininfo.naviframe,
					"gallery,language,changed", data);
	/* Change text in APP */
	_gl_lang_update(data);
}

static void _gallery_resume(void *data)
{
	gl_dbgW("==Resume==");

	GL_CHECK(data);
	gl_appdata *ad = (gl_appdata *)data;

	evas_object_data_set(ad->maininfo.naviframe,
		"gl_obj_key_gallery_paused",
		(void *)0);
	if (ad->maininfo.b_paused) {
		evas_object_smart_callback_call(ad->maininfo.naviframe,
						"gallery,view,resumed", data);
		ad->maininfo.b_paused = false;
	}

	if (ad->uginfo.b_ug_launched) {
		elm_object_tree_focus_allow_set(ad->maininfo.layout, EINA_TRUE);
		elm_object_focus_set(ad->maininfo.layout, EINA_TRUE);
		ad->uginfo.b_ug_launched = false;
	}

	//ug_resume();
	int view_mode = gl_get_view_mode(ad);
	if (ad->maininfo.lang_changed && (view_mode == GL_VIEW_ALBUMS || view_mode == GL_VIEW_ALBUMS_EDIT)) {
		gl_albums_update_view(data);
		ad->maininfo.lang_changed = false;
	}
	/* Video palyer was close when pause Gallery */
	ad->uginfo.b_app_called = false;
	_gl_entry_resume_view(ad);
}

static int _gallery_close_app(void *data)
{
	GL_CHECK_VAL(data, -1);
	gl_appdata *ad = (gl_appdata *)data;
	/* Clear UI */
	_gl_main_clear_view(ad);
	/* Destroy Progressbar timer */
	GL_IF_DEL_TIMER(ad->pbarinfo.pbar_timer);
	/* Free Ecore_Pipe object created */
	GL_IF_DEL_PIPE(ad->pbarinfo.sync_pipe);
	GL_IF_DEL_TIMER(ad->pbarinfo.start_thread_timer);
	GL_IF_DEL_TIMER(ad->pbarinfo.del_pbar_timer);
	GL_IF_DEL_TIMER(ad->ctrlinfo.avoid_multi_tap);
	GL_IF_DEL_JOB(ad->pbarinfo.del_pbar_job);
	/* Destroy timer for removing popup */
	GL_IF_DEL_TIMER(ad->popupinfo.del_timer);
	/* Remove down event handler */
	GL_IF_DEL_EVENT_HANDLER(ad->maininfo.keydown_handler);
	/*Remove font type change event handler*/
	GL_IF_DEL_EVENT_HANDLER(ad->maininfo.font_handler);
	/*Destroy idler for update mmc status chenged callback*/
	GL_IF_DEL_IDLER(ad->maininfo.mmc_idler);
	GL_IF_DEL_TIMER(ad->maininfo.reg_idler);
	/* deregister notify */
	_gl_dereg_storage_state_change_notify(data);
#ifdef _USE_SVI
	/* Finallize svi */
	_gl_fini_svi(data);
#endif
	/* dlopen imageviewer lib */
	_gl_dlclose_imageviewer(data);
	/* Remove db monitor */
	_gl_db_update_finalize(data);
	/* disconnet with libmedia-info */
	_gl_data_finalize(ad);

	gl_dbgW("==Cleaning done==");
	return 0;
}

static void _gallery_terminate(void *data)
{
	gl_dbgW("==Terminate==");
	GL_CHECK(data);
	_gallery_close_app(data);
}

/*
static void __gl_albums_sel_cb(void *data, int type, void *ei)
{
	GL_CHECK(data);
	gl_cluster *album_item = (gl_cluster *)data;
	GL_CHECK(album_item->cluster);
	GL_CHECK(album_item->ad);
	gl_appdata *ad = (gl_appdata *)album_item->ad;
	int view_mode = gl_get_view_mode(ad);
//	elm_gengrid_item_selected_set(ei, EINA_FALSE);

	gl_dbg("view mode: %d.", view_mode);
	if (view_mode != GL_VIEW_ALBUMS_EDIT) {
		if (album_item->cluster->count == 0) {
			gl_dbgW("Empty album, return!");
			return;
		}
		gl_albums_open_album(album_item);
	}
}
*/

static Eina_Bool __gallery_key_down_cb(void *data, int type, void *event)
{
	if (!data) {
		gl_dbgE("Invalid parameter!");
		return ECORE_CALLBACK_PASS_ON;
	}

	Ecore_Event_Key *key_event = (Ecore_Event_Key *)event;

	if (!key_event) {
		return ECORE_CALLBACK_PASS_ON;
	}
	if (!g_strcmp0(key_event->keyname, "XF86PowerOff")) {
		gl_sdbgW("Power key");
	} else if (!g_strcmp0(key_event->keyname, "XF86Menu")) {
		gl_sdbgW("menu key is pressed");
	}	gl_sdbg("Key down : %s", key_event->keyname);

	return ECORE_CALLBACK_PASS_ON;
}

static void _gallery_low_battery_cb(app_event_info_h event_info, void *data)
{
	gl_dbg("");

	/* ug_send_event(UG_EVENT_LOW_BATTERY); */
	/* 0 : default handler, 1:user handler */
}

static bool _gallery_create(void *data)
{
	GL_PROFILE_IN;
	gl_dbgW("==Create==");
	GL_CHECK_FALSE(data);
	gl_appdata *ad = (gl_appdata *)data;
	ad->maininfo.reentrant = TRUE;
	ad->uginfo.b_ug_launched = false;

	/* regitering sigterm  */
	if (signal(SIGTERM, __gallery_sigterm_handler) == SIG_IGN) {
		gl_dbgW("SIG_IGN");
		signal(SIGTERM, SIG_IGN);
	}
	bindtextdomain(PACKAGE, "/usr/apps/org.tizen.gallery/res/locale");
#ifdef _USE_OPENGL_BACKEND
	elm_config_preferred_engine_set("opengl_x11");
#else
	elm_config_preferred_engine_set("software_x11");
#endif
	/* initialize gallery main view(layout) */
	if (_gl_main_create_view(data) != 0) {
		gl_dbgE("_gl_main_create_view failed");
		return false;
	}

	GL_PROFILE_F_IN("register noti callback");
	/* memory malloc */
	_gl_db_update_init(data);
	/* initialize notify */
	_gl_reg_storage_state_change_notify(data);
	/* Register key down handler */

	ad->maininfo.keydown_handler = ecore_event_handler_add(
		ECORE_EVENT_KEY_DOWN,
		__gallery_key_down_cb,
		data);

	GL_PROFILE_F_OUT("register noti callback");
	GL_PROFILE_OUT;
	return true;
}

static void _gallery_pause(void *data)
{
	gl_dbgW("==Pause==");

	GL_CHECK(data);
	gl_appdata *ad = (gl_appdata *)data;
	ad->maininfo.reentrant = TRUE;

	evas_object_data_set(ad->maininfo.naviframe,
		"gl_obj_key_gallery_paused",
		(void *)1);
	evas_object_smart_callback_call(ad->maininfo.naviframe,
					"gallery,view,paused", data);
	ad->maininfo.b_paused = true;

	//ug_pause();
}


/* analysis parameters */
static int __gallery_parse_param(void *data, app_control_h service)
{
	GL_CHECK_VAL(service, -1);
	GL_CHECK_VAL(data, -1);
	gl_dbg("");

	char *value = NULL;
	app_control_get_extra_data(service,
		"http://tizen.org/appcontrol/data/multiwindow", &value);
	if (value) {
		if (!strcmp(value, "on")) {
			gl_dbg("multiwindow value = %s", value);
		}
		free(value);
	}

	return -1;
}

static void _gallery_reset(app_control_h service, void *data)
{
	GL_PROFILE_IN;
	gl_dbgW("==Reset==");
	GL_CHECK(data);
	gl_appdata *ad = (gl_appdata *)data;
	GL_CHECK(ad->maininfo.win);

	if (ad->maininfo.reentrant) {
		ad->maininfo.reentrant = FALSE;
	} else {
		gl_dbgW("Gallery reentrant is not allowed!");
		return;
	}

	if (ad->maininfo.b_paused) {
		evas_object_smart_callback_call(ad->maininfo.naviframe,
						"gallery,view,resumed", data);
		ad->maininfo.b_paused = false;
	}

	/* Parse parameters */
	if (__gallery_parse_param(data, service) != 0) {
		int view_m = gl_get_view_mode(ad);
		gl_dbg("view_m: %d", view_m);

		/* Launch Gallery at the first time */
		if (view_m == GL_VIEW_NONE) {
			_gl_main_update_view(ad);
		} else {
			/* Step: Settings>gengeral>storage>pictures,videos */
			gl_dbgW("Checkme: Need to Update view????");
			/* gl_update_view(ad, GL_UPDATE_VIEW_NORMAL); */
		}
		elm_win_activate(ad->maininfo.win);
		GL_PROFILE_OUT;
		return;
	}

	_gl_main_reset_view(data);
	GL_PROFILE_OUT;
}

EXPORT_API int main(int argc, char *argv[])
{
	gl_dbgW("==Gallery==");
	gl_appdata gl_ad;
	int ret = APP_ERROR_NONE;

	ui_app_lifecycle_callback_s event_callback;

	app_event_handler_h hLowBatteryHandle;
	app_event_handler_h hLanguageChangedHandle;

	event_callback.create = _gallery_create;
	event_callback.terminate = _gallery_terminate;
	event_callback.pause = _gallery_pause;
	event_callback.resume = _gallery_resume;
	event_callback.app_control = _gallery_reset;

	ret = ui_app_add_event_handler(&hLowBatteryHandle,
		APP_EVENT_LOW_BATTERY, _gallery_low_battery_cb, (void *)&gl_ad);
	if (ret != APP_ERROR_NONE) {
		gl_dbgE("failed to add LOW_BATTERY event_handler: [%d]", ret);
		return -1;
	}

	ret = ui_app_add_event_handler(&hLanguageChangedHandle,
		APP_EVENT_LANGUAGE_CHANGED, _gallery_lang_changed_cb,
		(void *)&gl_ad);
	if (ret != APP_ERROR_NONE) {
		gl_dbgE("failed to add LANGUAGE_CHANGED event_handler: [%d]",
			ret);
		return -1;
	}
	/* Enable OpenGL */
	/* 2013.06.20
	 * Use elm_config_preferred_engine_set to enable OpenGL as the backend
	 * of EFL. */
	/*setenv("ELM_ENGINE", "gl", 1);*/

	memset(&gl_ad, 0x00, sizeof(gl_appdata));

	ret = ui_app_main(argc, argv, &event_callback, &gl_ad);

	gl_dbgW("==Gallery gone==");
	return ret;
}

