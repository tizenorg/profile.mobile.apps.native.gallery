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
#include "gl-ctxpopup.h"
#include "gl-ui-util.h"
#include "gl-util.h"
#include "gl-debug.h"
#include "gl-strings.h"
#include "gl-button.h"
#include "gl-controlbar.h"
#include "gl-thumbs.h"
#include "gl-albums.h"

#define GL_CTXPOPUP_OBJ_DATA_KEY "gl_ctxpopup_data_key"
#define GL_CTXPOPUP_OBJ_MORE_BTN_KEY "gl_ctxpopup_more_btn_key"
#define GL_CTXPOPUP_OBJ_ROTATE_KEY "gl_ctxpopup_rotate_key"
#define GL_CTXPOPUP_STYLE_MORE "more/default"

typedef struct _gl_ctx_data_t gl_ctx_data_s;
struct _gl_ctx_data_t {
	int items_count;
};

static void __gl_ctxpopup_move_for_hwkey(void *data, Evas_Object *ctxpopup)
{
	GL_CHECK(ctxpopup);
	GL_CHECK(data);
	gl_appdata *ad = (gl_appdata *)data;
	int win_w = 0;
	int win_h = 0;
	int move_x = 0;
	int move_y = 0;

	elm_win_screen_size_get(ad->maininfo.win, NULL, NULL, &win_w, &win_h);
	int changed_ang = elm_win_rotation_get(ad->maininfo.win);
	gl_dbg("New angle: %d", changed_ang);

	switch (changed_ang) {
	case APP_DEVICE_ORIENTATION_0:
	case APP_DEVICE_ORIENTATION_180:
		move_x = win_w / 2;
		move_y = win_h;
		break;
	case APP_DEVICE_ORIENTATION_90:
		move_x = win_h / 2;
		move_y = win_w;
		break;
	case APP_DEVICE_ORIENTATION_270:
		move_x = win_h / 2;
		move_y = win_w;
		break;
	}

	gl_dbg("move_x: %d, move_y: %d", move_x, move_y);
	evas_object_move(ctxpopup, move_x, move_y);
}

static bool __gl_ctxpopup_check_btn_pos(void *data, Evas_Object *btn, int x,
                                        int y, int w, int h)
{
	GL_CHECK_FALSE(data);
	GL_CHECK_FALSE(btn);
	gl_appdata *ad = (gl_appdata *)data;
	int win_w = 0, win_h = 0;

	elm_win_screen_size_get(ad->maininfo.win, NULL, NULL, &win_w, &win_h);

	if (((x > 0) && (x < win_w)) && ((y > 0) && (y < win_h)) &&
	        ((w > 0) && (h > 0))) {
		/* button's position is inside the main window, and button's size is not zero */
		return true;
	}

	return false;
}

static char *__gl_ctxpopup_get_icon(char *label_id)
{
	GL_CHECK_NULL(label_id);
	char *path = NULL;

	if (!g_strcmp0(GL_STR_ID_EDIT_IMAGE, label_id)) {
		path = GL_ICON_EDIT_IMAGE;
	} else if (!g_strcmp0(GL_STR_ID_DELETE, label_id)) {
		path = GL_ICON_DELETE;
	} else if (!g_strcmp0(GL_STR_ID_MOVE, label_id)) {
		path = GL_ICON_MOVE;
	} else if (!g_strcmp0(GL_STR_ID_COPY, label_id)) {
		path = GL_ICON_COPY;
	} else if (!g_strcmp0(GL_STR_ID_ROTATE_LEFT, label_id)) {
		path = GL_ICON_ROTATE_LEFT;
	} else if (!g_strcmp0(GL_STR_ID_ROTATE_RIGHT, label_id)) {
		path = GL_ICON_ROTATE_RIGHT;
	}
#ifdef SUPPORT_SLIDESHOW
	else if (!g_strcmp0(GL_STR_ID_SLIDESHOW, label_id)) {
		path = GL_ICON_SLIDESHOW;
	}
#endif
	else if (!g_strcmp0(GL_STR_ID_NEW_ALBUM, label_id)) {
		path = GL_ICON_NEW_ALBUM;
	} else if (!g_strcmp0(GL_STR_ID_SHARE, label_id)) {
		path = GL_ICON_SHARE;
	} else if (!g_strcmp0(GL_STR_ID_EDIT, label_id)) {
		path = GL_ICON_EDIT;
	} else if (!g_strcmp0(GL_STR_ID_RENAME, label_id)) {
		path = GL_ICON_RENAME;
	} else if (!g_strcmp0(GL_STR_ID_SETTINGS, label_id)) {
		path = GL_ICON_SETTINGS;
	}
	return path;
}

static void __gl_ctxpopup_hide_cb(void *data, Evas_Object *obj, void *ei)
{
	GL_CHECK(obj);
	GL_CHECK(data);
	gl_appdata *ad = (gl_appdata *)data;

	bool ct_rotate = (bool)evas_object_data_get(obj,
	                 GL_CTXPOPUP_OBJ_ROTATE_KEY);

	if (!ct_rotate) {
		gl_dbg("ctxpopup is dismissed");
		evas_object_del(obj);
		ad->maininfo.ctxpopup = NULL;
	} else {
		gl_dbg("ctxpopup is not dismissed");
		/* when "dismissed" cb is called next time,
		  * ctxpopup should be dismissed if device is not rotated. */
		evas_object_data_set(obj, GL_CTXPOPUP_OBJ_ROTATE_KEY,
		                     (void *)false);
		/* If ctxpopup is not dismissed, then it must be shown again.
		  * Otherwise "dismissed" cb will be called one more time. */
		evas_object_show(ad->maininfo.ctxpopup);
	}
}

/* Basic winset: ctxpopup only activated from more button */
static int __gl_ctxpopup_show(void *data, Evas_Object *btn, Evas_Object *ctxpopup)
{
	GL_CHECK_VAL(ctxpopup, -1);
	GL_CHECK_VAL(btn, -1);
	GL_CHECK_VAL(data, -1);
	gl_appdata *ad = (gl_appdata *)data;
	int x_position = 0;
	int y_position = 0;
	int w = 0;
	int h = 0;

	elm_ctxpopup_direction_priority_set(ctxpopup, ELM_CTXPOPUP_DIRECTION_UP,
	                                    ELM_CTXPOPUP_DIRECTION_LEFT,
	                                    ELM_CTXPOPUP_DIRECTION_RIGHT,
	                                    ELM_CTXPOPUP_DIRECTION_DOWN);
	/* Only for more button */
	evas_object_geometry_get(btn, &x_position, &y_position, &w, &h);
	if (__gl_ctxpopup_check_btn_pos(data, btn, x_position, y_position, w, h) == true) {
		/* the more button is inside the main window */
		evas_object_move(ctxpopup, x_position + w / 2, y_position + h / 2);
		gl_dbg("(%d, %d) - (%d, %d)", x_position, y_position, w, h);
	} else {
		/* the more button is not inside the main windown,
		     possibly because the target has HW more/back keys. */
		__gl_ctxpopup_move_for_hwkey(data, ctxpopup);
	}

	evas_object_show(ctxpopup);
	ad->maininfo.ctxpopup = ctxpopup;
	return 0;
}

static void __gl_ctxpopup_parent_resize_cb(void *data, Evas *e,
        Evas_Object *obj, void *ei)
{
	gl_dbg("");
	GL_CHECK(data);
	evas_object_data_set((Evas_Object *)data, GL_CTXPOPUP_OBJ_ROTATE_KEY,
	                     (void *)true);
}

static void __gl_ctxpopup_items_update_cb(void *data, Evas_Object *obj, void *ei)
{
	gl_dbg("");
	GL_CHECK(data);
	int (*update_cb)(void * data, Evas_Object * parent);
	update_cb = evas_object_data_get((Evas_Object *)data,
	                                 "gl_ctxpopup_update_items_cb");
	gl_sdbg("callback: %p", update_cb);
	if (update_cb) {
		update_cb(ei, (Evas_Object *)data);
	}
}


static void __gl_ctxpopup_rotate_cb(void *data, Evas_Object *obj, void *ei)
{
	gl_dbg("");
	GL_CHECK(data);
	gl_appdata *ad = (gl_appdata *)data;

	Evas_Object *more_btn = NULL;
	more_btn = (Evas_Object *)evas_object_data_get(ad->maininfo.ctxpopup,
	           GL_CTXPOPUP_OBJ_MORE_BTN_KEY);
	GL_CHECK(more_btn);
	__gl_ctxpopup_show(data, more_btn, ad->maininfo.ctxpopup);
}

static void __gl_ctxpopup_del_cb(void *data, Evas *e, Evas_Object *obj, void *ei)
{
	GL_CHECK(data);
	GL_CHECK(obj);
	Evas_Object *ctxpopup = obj;
	gl_appdata *ad = (gl_appdata *)data;
	GL_CHECK(ad->maininfo.naviframe);

	evas_object_data_del(ctxpopup, GL_CTXPOPUP_OBJ_MORE_BTN_KEY);
	evas_object_data_del(ctxpopup, GL_CTXPOPUP_OBJ_ROTATE_KEY);
	evas_object_smart_callback_del(ctxpopup, "dismissed",
	                               __gl_ctxpopup_hide_cb);
	evas_object_event_callback_del(ctxpopup, EVAS_CALLBACK_DEL,
	                               __gl_ctxpopup_del_cb);
	evas_object_event_callback_del(ad->maininfo.naviframe,
	                               EVAS_CALLBACK_RESIZE,
	                               __gl_ctxpopup_parent_resize_cb);
	evas_object_smart_callback_del(ad->maininfo.naviframe,
	                               "ctxpopup,items,update",
	                               __gl_ctxpopup_items_update_cb);
	evas_object_smart_callback_del(elm_object_top_widget_get(ctxpopup),
	                               "rotation,changed",
	                               __gl_ctxpopup_rotate_cb);
	void *ctx_data = evas_object_data_get(ctxpopup, GL_CTXPOPUP_OBJ_DATA_KEY);
	GL_FREEIF(ctx_data);
	gl_dbg("done");
}

static int __gl_ctxpopup_add_callbacks(void *data, Evas_Object *ctxpopup)
{
	GL_CHECK_VAL(data, -1);
	GL_CHECK_VAL(ctxpopup, -1);
	gl_appdata *ad = (gl_appdata *)data;
	GL_CHECK_VAL(ad->maininfo.naviframe, -1);

	evas_object_smart_callback_add(ctxpopup, "dismissed",
	                               __gl_ctxpopup_hide_cb, data);
	evas_object_event_callback_add(ctxpopup, EVAS_CALLBACK_DEL,
	                               __gl_ctxpopup_del_cb, data);
	evas_object_event_callback_add(ad->maininfo.naviframe,
	                               EVAS_CALLBACK_RESIZE,
	                               __gl_ctxpopup_parent_resize_cb,
	                               ctxpopup);
	evas_object_smart_callback_add(ad->maininfo.naviframe,
	                               "ctxpopup,items,update",
	                               __gl_ctxpopup_items_update_cb, ctxpopup);
	evas_object_smart_callback_add(elm_object_top_widget_get(ctxpopup),
	                               "rotation,changed",
	                               __gl_ctxpopup_rotate_cb, data);

	gl_dbg("done");
	return 0;
}

Elm_Object_Item *_gl_ctxpopup_append(Evas_Object *obj, char *label_id,
                                     Evas_Smart_Cb func, const void *data)
{
	Evas_Object *ic = NULL;
	char *path = __gl_ctxpopup_get_icon(label_id);

	if (path) {
		ic = elm_image_add(obj);
		GL_CHECK_NULL(ic);
		GL_ICON_SET_FILE(ic, path);
	}

	gl_ctx_data_s *ctx_data = NULL;
	ctx_data = (gl_ctx_data_s *)evas_object_data_get(obj,
	           GL_CTXPOPUP_OBJ_DATA_KEY);
	if (ctx_data) {
		ctx_data->items_count++;
	}
	return elm_ctxpopup_item_append(obj, _gl_str(label_id), NULL, func, data);
}

Elm_Object_Item *_gl_ctxpopup_append_with_icon(Evas_Object *obj, char *label,
					char *icon_path, Evas_Smart_Cb func, const void *data)
{
	Evas_Object *ic = NULL;

	if (icon_path) {
		ic = elm_image_add(obj);
		GL_CHECK_NULL(ic);
		GL_ICON_SET_FILE(ic, icon_path);
	}

	gl_ctx_data_s *ctx_data = NULL;
	ctx_data = (gl_ctx_data_s *)evas_object_data_get(obj,
	           GL_CTXPOPUP_OBJ_DATA_KEY);
	if (ctx_data) {
		ctx_data->items_count++;
	}
	return elm_ctxpopup_item_append(obj, label, ic, func, data);
}

int _gl_ctxpopup_create(void *data, Evas_Object *but, ctx_append_cb append_cb)
{
	gl_dbg("");
	GL_CHECK_VAL(append_cb, -1);
	GL_CHECK_VAL(but, -1);
	GL_CHECK_VAL(data, -1);
	gl_appdata *ad = (gl_appdata *)data;
	GL_CHECK_VAL(ad->maininfo.naviframe, -1);

	_gl_ctxpopup_del(data);

	gl_ctx_data_s *ctx_data = (gl_ctx_data_s *)calloc(1, sizeof(gl_ctx_data_s));
	GL_CHECK_VAL(ctx_data, -1);

	Evas_Object *ctxpopup = elm_ctxpopup_add(ad->maininfo.naviframe);
	eext_object_event_callback_add(ctxpopup, EEXT_CALLBACK_BACK, eext_ctxpopup_back_cb, NULL);
	eext_object_event_callback_add(ctxpopup, EEXT_CALLBACK_MORE, eext_ctxpopup_back_cb, NULL);
//	elm_object_style_set(ctxpopup, GL_CTXPOPUP_STYLE_MORE);
	/* more btn is needed to decide ctxpopup's position. */
	evas_object_data_set(ctxpopup, GL_CTXPOPUP_OBJ_MORE_BTN_KEY, but);
	evas_object_data_set(ctxpopup, GL_CTXPOPUP_OBJ_ROTATE_KEY,
	                     (void *)false);
	evas_object_data_set(ctxpopup, GL_CTXPOPUP_OBJ_DATA_KEY,
	                     (void *)ctx_data);

	/* Add callbacks for ctxpopup */
	__gl_ctxpopup_add_callbacks(data, ctxpopup);

	/* Append items */
	append_cb(data, ctxpopup);

	/* check count*/
	if (ctx_data->items_count == 0) {
		gl_dbgW("Remove empty ctxpopup");
		evas_object_del(ctxpopup);
		return -1;
	}

	return __gl_ctxpopup_show(data, but, ctxpopup);
}

int _gl_ctxpopup_del(void *data)
{
	GL_CHECK_VAL(data, -1);
	gl_appdata *ad = (gl_appdata *)data;
	if (ad->maininfo.ctxpopup) {
		evas_object_del(ad->maininfo.ctxpopup);
		ad->maininfo.ctxpopup = NULL;
		gl_dbg("Ctxpopup removed");
	}
	return 0;
}

