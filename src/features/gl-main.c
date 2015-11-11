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
#include "gl-main.h"
#include "gl-controlbar.h"
#include "gl-ui-util.h"
#include "gl-util.h"
#include "gl-albums.h"
#include "gl-ctxpopup.h"
#include "gl-editfield.h"
#include "gl-notify.h"
#include "gl-thread-util.h"
#include "gl-popup.h"
#include "gl-strings.h"
#include "gl-thumbs.h"
#include "gl-button.h"
#include "gl-tile.h"
#include "gl-db-update.h"
#include "gl-timeline.h"

#define GL_BG_COLOR_DEFAULT 0
#define GL_BG_COLOR_A 255

static void __gl_main_win_del_cb(void *data, Evas_Object *obj, void *ei)
{
	gl_dbgW("_gl_main_win_del_cb!!");
	GL_CHECK(data);

	elm_exit();
}

/**
 * Perform an UI update according to the given rotation angle.
 * Do not call elm_win_rotation_set / elm_win_rotation_with_resize_set.
 * ecore_evas has already called
 * elm_win_rotation_set / elm_win_rotation_with_resize_set function.
 *
 * Please set NULL to app_device_orientation_cb member of app_event_callback_s structure.
 * And don't use app_device_orientation_cb callback which is supported by capi
*/
static void __gl_main_win_rot_changed_cb(void *data, Evas_Object *obj,
        void *event)
{
	GL_CHECK(data);
	GL_CHECK(obj);
	gl_appdata *ad = (gl_appdata *)data;
	GL_CHECK(ad->maininfo.win);

	/* changed_ang value is 0 90 180 270 */
	int changed_ang = elm_win_rotation_get(obj);
	gl_dbg("New angle: %d, old angle: %d", changed_ang,
	       ad->maininfo.rotate_mode);
	if (changed_ang == ad->maininfo.rotate_mode) {
		return;
	} else {
		ad->maininfo.rotate_mode = changed_ang;
	}
#if 0
	enum ug_event evt = UG_EVENT_NONE;
	/* Send event to UG */
	switch (changed_ang) {
	case APP_DEVICE_ORIENTATION_0:
		evt = UG_EVENT_ROTATE_PORTRAIT;
		break;
	case APP_DEVICE_ORIENTATION_90:
		evt = UG_EVENT_ROTATE_LANDSCAPE_UPSIDEDOWN;
		break;
	case APP_DEVICE_ORIENTATION_180:
		evt = UG_EVENT_ROTATE_PORTRAIT_UPSIDEDOWN;
		break;
	case APP_DEVICE_ORIENTATION_270:
		evt = UG_EVENT_ROTATE_LANDSCAPE;
		break;
	}
	ug_send_event(evt);
#endif
	int view_mode = gl_get_view_mode(ad);
	_gl_ui_rotate_view(data, view_mode);
	_gl_timeline_rotate_view(data);
	/*(landscape && SIP showed) -> hide title bar
	_gl_editfield_change_navi_title(data, ad->maininfo.rotate_mode);*/
	/*(landscape && rename mode) -> hide toolbar */
	if (ad->entryinfo.b_hide_toolbar) {
		_gl_ui_set_toolbar_state(data, ad->entryinfo.b_hide_toolbar);
	}
}

/* Parent is main window */
static Evas_Object *__gl_main_create_bg(Evas_Object *parent)
{
	GL_PROFILE_IN;
	GL_CHECK_NULL(parent);
	Evas_Object *bg = NULL;

	bg = evas_object_rectangle_add(evas_object_evas_get(parent));
	evas_object_color_set(bg, GL_BG_COLOR_DEFAULT, GL_BG_COLOR_DEFAULT,
	                      GL_BG_COLOR_DEFAULT, GL_BG_COLOR_A);
	evas_object_size_hint_weight_set(bg, EVAS_HINT_EXPAND,
	                                 EVAS_HINT_EXPAND);
	elm_win_resize_object_add(parent, bg);
	evas_object_show(bg);

	GL_PROFILE_OUT;
	return bg;
}

/* Parent is main window */
static Evas_Object *__gl_main_create_conform(Evas_Object *parent)
{
	GL_PROFILE_IN;
	GL_CHECK_NULL(parent);
	Evas_Object *conform = NULL;
	conform = elm_conformant_add(parent);
	evas_object_size_hint_weight_set(conform, EVAS_HINT_EXPAND,
	                                 EVAS_HINT_EXPAND);
	evas_object_size_hint_align_set(conform, EVAS_HINT_FILL, EVAS_HINT_FILL);
	elm_win_resize_object_add(parent, conform);
	evas_object_show(conform);
	GL_PROFILE_OUT;
	return conform;
}

static int __gl_main_create_ctrl_ly(gl_appdata *ad, Evas_Object *parent)
{
	GL_PROFILE_IN;
	GL_CHECK_VAL(parent, -1);
	GL_CHECK_VAL(ad, -1);

	Evas_Object *layout = _gl_ctrl_add_layout(parent);
	GL_CHECK_VAL(layout, -1);
	ad->ctrlinfo.ctrlbar_view_ly = layout;

	/* Set view layout to control layout */
	elm_object_part_content_set(parent, "elm.swallow.content", layout);
	GL_PROFILE_OUT;
	return 0;
}

static Evas_Object *__gl_main_create_ly(Evas_Object *parent)
{
	GL_PROFILE_IN;
	GL_CHECK_NULL(parent);
	Evas_Object *layout = NULL;

	layout = elm_layout_add(parent);
	GL_CHECK_NULL(layout);
	/* Apply the layout style */
	elm_layout_theme_set(layout, "layout", "application", "default");
	evas_object_size_hint_weight_set(layout, EVAS_HINT_EXPAND,
	                                 EVAS_HINT_EXPAND);
	evas_object_show(layout);
	GL_PROFILE_OUT;
	return layout;
}

static Evas_Object *__gl_main_create_naviframe(Evas_Object *parent)
{
	GL_PROFILE_IN;
	GL_CHECK_NULL(parent);
	Evas_Object *nf = NULL;
	nf = elm_naviframe_add(parent);
	/* Disable Naviframe Back Button Auto creation function */
	elm_naviframe_prev_btn_auto_pushed_set(nf, EINA_FALSE);
	/* Pop the most top view if the Naviframe has the BACK event */
	eext_object_event_callback_add(nf, EEXT_CALLBACK_BACK, eext_naviframe_back_cb,
	                               NULL);
	/* Call the more_cb() of the most top view if the Naviframe has the MORE event */
	eext_object_event_callback_add(nf, EEXT_CALLBACK_MORE, eext_naviframe_more_cb,
	                               NULL);
	evas_object_show(nf);
	gl_dbg("naviframe added");
	GL_PROFILE_OUT;
	return nf;
}

static Eina_Bool __gl_main_reg_idler_cb(void *data)
{
	gl_dbg("start");
	GL_CHECK_CANCEL(data);
	gl_appdata *ad = (gl_appdata *)data;

	/* To skip blocking UX as much as possible */
	if (!ad->maininfo.b_reged_idler) {
		ad->maininfo.b_reged_idler = true;
		gl_dbg("next idler");
		return ECORE_CALLBACK_PASS_ON;
	}

	/* Initialize media-content */
	_gl_data_init(ad);
	/* Register db monitor */
	_gl_db_update_reg_cb(data);
#ifdef _USE_SVI
	/* initializ svi */
	_gl_init_svi(data);
#endif
	/* Open imageviewer UG so lib */
	_gl_dlopen_imageviewer(data);

	GL_IF_DEL_TIMER(ad->maininfo.reg_idler);
	gl_dbg("done");
	return ECORE_CALLBACK_CANCEL;
}

/* pause gallery and change view to background after tap Back button in main view */
/* Return EINA_FALSE to keep current view unchanged, no pop operation */
/* Always return EINA_FALSE in the first item of naviframe */
static Eina_Bool __gl_main_pop_op(void *data)
{
	GL_CHECK_FALSE(data);
	gl_appdata *ad = (gl_appdata *)data;

	void *pop_cb = evas_object_data_get(ad->maininfo.naviframe,
	                                    GL_NAVIFRAME_POP_CB_KEY);
	if (pop_cb) {
		Eina_Bool(*_pop_cb)(void * data);
		_pop_cb = pop_cb;

		if (_pop_cb(data)) {
			/* Just pop edit view, dont lower app to background */
			return EINA_FALSE;
		}
	}

	GL_CHECK_FALSE(ad->maininfo.win);
	gl_dbgW("Lower Gallery to background!");
	elm_win_lower(ad->maininfo.win);
	return EINA_FALSE;
}

static Eina_Bool __gl_main_pop_cb(void *data, Elm_Object_Item *it)
{
	GL_CHECK_FALSE(data);
	return __gl_main_pop_op(data);
}

/**
 *  Use naviframe api to push albums view to stack.
 *  @param obj is the content to be pushed.
 */
static int __gl_main_push_view(void *data, Evas_Object *parent, Evas_Object *obj)
{
	GL_PROFILE_IN;
	GL_CHECK_VAL(obj, -1);
	GL_CHECK_VAL(parent, -1);
	GL_CHECK_VAL(data, -1);
	gl_appdata *ad = (gl_appdata *)data;
	Elm_Object_Item *nf_it = NULL;
	Evas_Object *prev_btn = NULL;

	/**
	*  add End button in navigation var first view in galery
	*   with style "naviframe/prev_btn/gallery"
	*  so after tap this button,gallery is paused
	*  but change view to background
	*
	* Need to created button and set it to naviframe, cause this is the first
	* naviframe item. Otherwise, prev_btn will not be created.
	*/
	GL_CHECK_VAL(ad->maininfo.win, -1);
	prev_btn = _gl_but_create_but(parent, NULL, NULL,
	                              GL_BUTTON_STYLE_NAVI_PRE, NULL, NULL);
	GL_CHECK_VAL(prev_btn, -1);
	/* Push view to stack */
	nf_it = elm_naviframe_item_push(parent, NULL, prev_btn, NULL, obj,
	                                NULL);
	Evas_Object *unset = elm_object_item_part_content_unset(nf_it, GL_NAVIFRAME_PREV_BTN);
	evas_object_hide(unset);
	ad->ctrlinfo.nf_it = nf_it;
	elm_naviframe_item_pop_cb_set(nf_it, __gl_main_pop_cb, data);

	GL_PROFILE_OUT;
	return 0;
}

/* Add albums view and append nothing */
static int __gl_main_add_view(void *data, Evas_Object *parent)
{
	GL_PROFILE_IN;
	GL_CHECK_VAL(parent, -1);
	GL_CHECK_VAL(data, -1);
	gl_appdata *ad = (gl_appdata *)data;

	/* Add gesture for parent */
	Evas_Object *gesture = _gl_tile_add_gesture(data, parent);
	if (gesture == NULL) {
		gl_dbgE("Failed to create gesture!");
	} else {
		elm_object_part_content_set(parent, "gesture", gesture);
	}

	__gl_main_push_view(data, ad->maininfo.naviframe,
	                    ad->ctrlinfo.ctrlbar_ly);

	GL_PROFILE_OUT;
	return 0;
}

static Evas_Object *__gl_main_create_win(void *data, const char *name)
{
	GL_PROFILE_IN;
	GL_CHECK_NULL(name);
	GL_CHECK_NULL(data);
	Evas_Object *win = NULL;

	win = elm_win_add(NULL, name, ELM_WIN_BASIC);
	GL_CHECK_NULL(win);

	/* Temporarily disable vsync
	 * In Redwood8974 binary, Gallery lockup happens in view transit animation.
	*/
	/**
	 * Register a list of rotation angles that your application supports
	 *
	 * The window manager is going to decide rotation of application windows
	 * by referring the list of rotation angles.
	 * In this means if your application sets 0, 90 and 270 degrees to be the list of
	 * supported rotation angles, the window manager will not rotate your
	 * application window if the device is rotated 180 degrees
	 *
	 *  --- APP_DEVICE_ORIENTATION_180 ---
	 * it changes to 3 directions only which is same with Galaxy S3/Note 2.
	 * When reverse portrait (180), rotation shoudn't happen.
	 */
	if (elm_win_wm_rotation_supported_get(win)) {
		const int rots[3] = { APP_DEVICE_ORIENTATION_0,
		                      APP_DEVICE_ORIENTATION_90,
		                      APP_DEVICE_ORIENTATION_270
		                    };
		elm_win_wm_rotation_available_rotations_set(win, rots, 3);
	}
	/* pass '-1' value to this API then it will unset preferred rotation angle */
	elm_win_wm_rotation_preferred_rotation_set(win, -1);

	elm_win_autodel_set(win, EINA_TRUE);
	elm_win_title_set(win, name);
	evas_object_smart_callback_add(win, "delete,request",
	                               __gl_main_win_del_cb, data);
	evas_object_smart_callback_add(win, "wm,rotation,changed",
	                               __gl_main_win_rot_changed_cb, data);
	evas_object_show(win);
	GL_PROFILE_OUT;
	return win;
}

/* Register service in idler for reducing launching time */
int _gl_main_add_reg_idler(void *data)
{
	GL_CHECK_VAL(data, -1);
	gl_appdata *ad = (gl_appdata *)data;
	if (!ad->maininfo.b_reged_idler && !ad->maininfo.reg_idler) {
		gl_dbgW("Register idler!");
		ad->maininfo.reg_idler = ecore_timer_add(0.6f, __gl_main_reg_idler_cb,
		                         data);
	}
	return 0;
}

/**
 * <layout>
 * Elm_win
 *    |-> Elm_bg
 *    |-> Elm_conformant
 *        |-> Elm_layout(main_layout)
 *            |-> Elm_naviframe(naviframe)
 *                |-> Elm_layout(ctrlbar_ly)
 *                     |-> Elm_layout(ctrlbar_view_ly)
 *                          |-> Elm_gengrid(albums_view/tags_view/places_view/...)
*/
int _gl_main_create_view(gl_appdata *ad)
{
	GL_PROFILE_IN;
	GL_CHECK_VAL(ad, -1);

	elm_theme_extension_add(NULL, GL_EDJ_FILE);
	/* Set initial rotate angle */
	ad->maininfo.rotate_mode = APP_DEVICE_ORIENTATION_0;
	gl_dbg("Initial rotation mode: %d", ad->maininfo.rotate_mode);
	/* create window */
	ad->maininfo.win = __gl_main_create_win(ad, PACKAGE);
	GL_CHECK_FALSE(ad->maininfo.win);
	/* Background. */
	ad->maininfo.bg = __gl_main_create_bg(ad->maininfo.win);
	GL_CHECK_VAL(ad->maininfo.bg, -1);

	/*
	 * Set Opacity before Conformant window create
	 * When create Conformant Window, it will read currect Opacity and auto send it.
	 * if Opacity is not set first and set conformant direclty,  Opacity value will return
	 * 0(Unknown) once, then 3(Transparent)
	*/
	elm_win_indicator_mode_set(ad->maininfo.win, ELM_WIN_INDICATOR_SHOW);
	elm_win_indicator_opacity_set(ad->maininfo.win,
	                              ELM_WIN_INDICATOR_TRANSPARENT);
	/* Conformant. */
	Evas_Object *conform = __gl_main_create_conform(ad->maininfo.win);
	GL_CHECK_VAL(conform, -1);
	/* Set if this window is an illume conformant window */
	elm_win_conformant_set(ad->maininfo.win, EINA_TRUE);
	/* Modify to start content from 0,0 */
	elm_object_signal_emit(conform, "elm,state,indicator,overlap", "");
	evas_object_data_set(conform, "overlap", (void *)EINA_TRUE);
	/* Base Layout */
	ad->maininfo.layout = __gl_main_create_ly(conform);
	GL_CHECK_VAL(ad->maininfo.layout, -1);
	/* Set base layout to conformant */
	elm_object_content_set(conform, ad->maininfo.layout);
	/* Create Naviframe */
	ad->maininfo.naviframe = __gl_main_create_naviframe(ad->maininfo.layout);
	GL_CHECK_VAL(ad->maininfo.naviframe, -1);
	/* Set Naviframe to main layout */
	elm_object_part_content_set(ad->maininfo.layout, "elm.swallow.content",
	                            ad->maininfo.naviframe);
	/* Save conform pointer to naviframe object */
	evas_object_data_set(ad->maininfo.naviframe, GL_NAVIFRAME_OBJ_DATA_KEY,
	                     conform);
	/* init value */
	evas_object_data_set(ad->maininfo.naviframe, "gl_obj_key_gallery_paused", (void *)0);

	/* Create control layout */
	ad->ctrlinfo.ctrlbar_ly = _gl_ctrl_add_main_layout(ad->maininfo.naviframe);
	GL_CHECK_VAL(ad->ctrlinfo.ctrlbar_ly, -1);
	/* Create view layout */
	__gl_main_create_ctrl_ly(ad, ad->ctrlinfo.ctrlbar_ly);
	/**
	 * Add gengrid(empty albums view) and ctrlbar_ly layout to naviframe,
	 * albums view will be updated in idler callback
	 */
	__gl_main_add_view(ad, ad->ctrlinfo.ctrlbar_view_ly);
	/**
	 * Re-entrance checking of _gl_ctrl_show_view()
	 * Init tab_mode as -1 to enter albums tab correctly
	 */
	_gl_ctrl_set_tab_mode(ad, GL_CTRL_TAB_INIT);
	gl_dbg("done");
	GL_PROFILE_OUT;
	return 0;
}

int _gl_main_clear_view(gl_appdata *ad)
{
	GL_CHECK_VAL(ad, -1);
	/* Remove win rotation callback */
	evas_object_smart_callback_del(ad->maininfo.win, "wm,rotation,changed",
	                               __gl_main_win_rot_changed_cb);
	_gl_ctxpopup_del(ad);
	elm_theme_extension_del(NULL, GL_EDJ_FILE);
	return 0;
}

Eina_Bool _gl_main_update_view(void *data)
{
	GL_PROFILE_IN;
	GL_CHECK_CANCEL(data);
	gl_dbg("start");

	gl_appdata *ad = (gl_appdata *)data;
	/* Initialize media-content */
	_gl_data_init(ad);
	gl_dbg("DB initialization:done");
	/* Get cluster list from media-content */
	_gl_data_get_cluster_list(ad);
	GL_CHECK_FALSE(ad->albuminfo.elist);
	gl_dbg("_gl_data_get_cluster_list:done");
	_gl_ctrl_show_view(data, GL_STR_ALBUMS);
	gl_dbg("done");
	GL_PROFILE_OUT;
	return ECORE_CALLBACK_CANCEL;
}

int _gl_main_reset_view(void *data)
{
	GL_CHECK_VAL(data, -1);
	gl_appdata *ad = (gl_appdata *)data;
	GL_CHECK_VAL(ad->maininfo.win, -1);
	int view_mode = gl_get_view_mode(ad);
	gl_dbg("");

	if (view_mode == GL_VIEW_NONE) {
		/* First launch gallery then show shortcut album/tag */
		_gl_data_init(ad);
		gl_dbg("Launch gallery at the first time");
		_gl_data_get_cluster_list(ad);
		GL_CHECK_VAL(ad->albuminfo.elist, -1);
		/* Set view mode */
		gl_set_view_mode(ad, GL_VIEW_ALBUMS);
		/* Select tabbar item contains shortcut item */
		_gl_ctrl_show_view(data, GL_STR_ALBUMS);
		/* Register servier if albums view wasn't created */
		_gl_main_add_reg_idler(data);
	}

	elm_win_activate(ad->maininfo.win);
	return 0;
}

