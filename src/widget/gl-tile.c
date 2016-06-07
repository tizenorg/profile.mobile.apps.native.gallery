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
#include "gl-tile.h"
#include "gl-util.h"
#include "gl-button.h"
#include "gl-controlbar.h"
#include "gallery.h"
#include "gl-icons.h"
#ifdef _USE_ROTATE_BG
#include "gl-rotate-bg.h"
#endif
#include "gl-gesture.h"
#include "gl-strings.h"
#include "gl-timeline.h"

/* Width and height of album item (4+331, 4+331) */
#define GL_TILE_ITEM_WIDTH 360
#define GL_TILE_ITEM_HEIGHT 354
#define GL_TILE_ITEM_L_W 426
#define GL_TILE_ITEM_L_H 426
#define GL_TILE_2_PER_ROW 2
#define GL_TILE_3_PER_ROW 3
#define GL_TILE_4_PER_ROW 4
#define GL_TILE_ITEM_CNT 6
#define GL_TILE_ITEM_CNT2 12
#define GL_TILE_ITEM_L_CNT 3
/* Items count of each column */
#define GL_TILE_ITEM_C_CNT 3
#define GL_TILE_ALIGN_LEFT 0.0f
#define GL_TILE_ALIGN_CON1 0.25f
#define GL_TILE_ALIGN_CON2 0.75f
#define GL_TILE_ALIGN_MID 0.5f
#define GL_TILE_ALIGN_BOT 1.0f
/* Size of album icon */
#define GL_TILE_W (GL_TILE_ITEM_WIDTH-8)
#define GL_TILE_H (GL_TILE_ITEM_HEIGHT-8)

#define GL_CHECKBOX_STYLE_ALBUM "gallery/album"
#define GL_CHECKBOX_STYLE_ALBUM_GRID "gallery/album/grid"

static Evas_Object *__gl_tile_add_icon_bg(Evas_Object *obj, bg_file_set_cb func,
					void *data)
{
	GL_CHECK_NULL(func);
	GL_CHECK_NULL(obj);
	Evas_Object *bg = NULL;

#ifdef _USE_ROTATE_BG
	bg = _gl_rotate_bg_add(obj, true);
#else
	bg = elm_bg_add(obj);
#endif
	GL_CHECK_NULL(bg);

	if (data) {
		double scale = elm_config_scale_get();
		int wid = 0;
		int hei = 0;
		wid = (int)(GL_TILE_W * scale);
		hei = (int)(GL_TILE_H * scale);
#ifdef _USE_ROTATE_BG
		_gl_rotate_bg_add_image(bg, wid, hei);
#else
		elm_bg_load_size_set(bg, wid, hei);
#endif

		func(bg, data);
	}

	evas_object_size_hint_weight_set(bg, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	evas_object_size_hint_align_set(bg, 0.5, 0.5);
	evas_object_show(bg);

	return bg;
}

static Eina_Bool __gl_tile_zoom_out_cb(Evas_Object *gesture, void *data)
{
	GL_CHECK_FALSE(data);
	Evas_Object *parent = NULL;
	Evas_Object *view = NULL;
	gl_appdata *ad = (gl_appdata *)data;
	int view_m = gl_get_view_mode(data);
	gl_dbg("view_m: %d", view_m);

	if (view_m == GL_VIEW_TIMELINE) {
		if ((ad->pinchinfo.zoom_level > GL_ZOOM_DEFAULT)) {
			/* Skip in wrong mode */
			ad->pinchinfo.zoom_level--;
			gl_dbg("Update time view size");
			_gl_timeline_update_grid_size(data);
		} else {
			gl_dbg("Already at max zoom level : %d", ad->pinchinfo.zoom_level);
		}
		return false;
	}

	parent = _gl_gesture_get_parent(gesture);
	GL_CHECK_FALSE(parent);
	view = elm_object_part_content_get(parent, "elm.swallow.view");
	GL_CHECK_CANCEL(view);

	/* Not a valid gengrid */
	if (elm_gengrid_items_count(view) <= 0) {
		return false;
	}

	int level = (int)evas_object_data_get(view, GL_GESTURE_KEY_PINCH_LEVEL);
	if (GL_ZOOM_NONE == level) {
		return false;
	}
	if (level >= GL_ZOOM_DEFAULT) {
		level--;
		evas_object_data_set(view, GL_GESTURE_KEY_PINCH_LEVEL,
		                     (void *)level);
	}
	gl_dbg("level: %d", level);
	/* Update item size */
	_gl_tile_update_item_size(data, view, true);
	return false;
}

static Eina_Bool __gl_tile_zoom_in_cb(Evas_Object *gesture, void *data)
{
	GL_CHECK_FALSE(data);
	Evas_Object *parent = NULL;
	Evas_Object *view = NULL;
	gl_appdata *ad = (gl_appdata *)data;
	int view_m = gl_get_view_mode(data);
	gl_dbg("view_m: %d", view_m);

	if (view_m == GL_VIEW_TIMELINE) {
		if ((ad->pinchinfo.zoom_level >= GL_ZOOM_DEFAULT) &&
		        (ad->pinchinfo.zoom_level < (GL_ZOOM_MAX - 1))) {
			/* Skip in wrong mode */
			ad->pinchinfo.zoom_level++;
			gl_dbg("Update time view size");
			_gl_timeline_update_grid_size(data);
		} else {
			gl_dbg("Already at max zoom level : %d", ad->pinchinfo.zoom_level);
		}
		return false;
	}

	parent = _gl_gesture_get_parent(gesture);
	GL_CHECK_CANCEL(parent);
	view = elm_object_part_content_get(parent, "elm.swallow.view");
	GL_CHECK_FALSE(view);

	/* Not a valid gengrid */
	if (elm_gengrid_items_count(view) <= 0) {
		return false;
	}

	int level = (int)evas_object_data_get(view, GL_GESTURE_KEY_PINCH_LEVEL);
	if (GL_ZOOM_DEFAULT == level) {
		return false;
	}
	if (level < GL_ZOOM_DEFAULT) {
		level++;
		evas_object_data_set(view, GL_GESTURE_KEY_PINCH_LEVEL,
		                     (void *)level);
	}
	gl_dbg("level: %d", level);
	/* Update item size */
	_gl_tile_update_item_size(data, view, true);
	return false;
}

static void __gl_title_grid_del_cb(void *data, Evas *e, Evas_Object *obj,
                                   void *ei)
{
	gl_dbg("Delete grid ---");
	if (obj) {
		evas_object_data_del(obj, GL_GESTURE_KEY_PINCH_LEVEL);
	}
	gl_dbg("Delete grid +++");
}

Evas_Object *_gl_tile_show_part_icon(Evas_Object *obj, const char *part,
                                     bg_file_set_cb func, void *data)
{
	GL_CHECK_NULL(part);
	GL_CHECK_NULL(strlen(part));
	GL_CHECK_NULL(obj);

	if (!g_strcmp0(part, GL_TILE_ICON)) {
		Evas_Object *bg = NULL;
		bg = __gl_tile_add_icon_bg(obj, func, data);
		return bg;
	}
	return NULL;
}

Evas_Object *_gl_tile_show_part_dim(Evas_Object *obj)
{
	GL_CHECK_NULL(obj);

	Evas *evas = evas_object_evas_get(obj);
	GL_CHECK_NULL(evas);
	Evas_Object *bg = NULL;
	bg = evas_object_rectangle_add(evas);
	GL_CHECK_NULL(bg);

	evas_object_color_set(bg, 220, 218, 211, 153);
	evas_object_size_hint_weight_set(bg, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	evas_object_size_hint_align_set(bg, EVAS_HINT_FILL, EVAS_HINT_FILL);
	evas_object_show(bg);
	return bg;
}

Evas_Object *_gl_tile_show_part_checkbox_grid(Evas_Object *obj, bool checked,
			Evas_Smart_Cb func,
			const void *data)
{
	GL_CHECK_NULL(func);
	GL_CHECK_NULL(obj);
	Evas_Object *ck = NULL;

	ck = elm_check_add(obj);
	GL_CHECK_NULL(ck);
	elm_object_style_set(ck, GL_CHECKBOX_STYLE_ALBUM_GRID);
	evas_object_repeat_events_set(ck, EINA_TRUE);
	elm_check_state_set(ck, checked);
	evas_object_smart_callback_add(ck, "changed", func, data);
	evas_object_show(ck);
	return ck;
}

Evas_Object *_gl_tile_show_part_checkbox(Evas_Object *obj, bool checked,
				Evas_Smart_Cb func, const void *data)
{
	GL_CHECK_NULL(func);
	GL_CHECK_NULL(obj);
	Evas_Object *ck = NULL;

	ck = elm_check_add(obj);
	GL_CHECK_NULL(ck);
	evas_object_propagate_events_set(ck, EINA_FALSE);
	elm_check_state_set(ck, checked);
	evas_object_smart_callback_add(ck, "changed", func, data);
	evas_object_show(ck);
	return ck;
}

Evas_Object *_gl_tile_show_part_rename(Evas_Object *obj, Evas_Smart_Cb func,
                                       const void *data)
{
	GL_CHECK_NULL(func);
	GL_CHECK_NULL(obj);
	Evas_Object *btn = NULL;
	btn = _gl_but_create_but(obj, NULL, NULL, GL_BUTTON_STYLE_RENAME, func,
	                         data);
	GL_CHECK_NULL(btn);
	evas_object_propagate_events_set(btn, EINA_FALSE);
	return btn;
}

Evas_Object *_gl_tile_show_part_type_icon(Evas_Object *obj, int sns_type)
{
	GL_CHECK_NULL(obj);
	Evas_Object *icon = elm_icon_add(obj);
	GL_CHECK_NULL(icon);

	switch (sns_type) {
	case GL_TILE_TYPE_CAMERA:
		GL_ICON_SET_FILE(icon, GL_ICON_ALBUM_CAMERA);
		break;
	case GL_TILE_TYPE_DOWNLOAD:
		GL_ICON_SET_FILE(icon, GL_ICON_ALBUM_DOWNLOAD);
		break;
	case GL_TILE_TYPE_FOLDER:
		GL_ICON_SET_FILE(icon, GL_ICON_ALBUM_FOLDER);
		break;
	case GL_TILE_TYPE_MMC_STORAGE:
		GL_ICON_SET_FILE(icon, GL_ICON_ALBUM_MMC);
		break;
	default:
		gl_dbgE("Unknow sns_type[%d]!", sns_type);
	}

	evas_object_show(icon);
	return icon;
}

int _gl_tile_get_mtime(time_t *mtime1, time_t *mtime2, char *buf, int max_len)
{
	char *date1 = NULL;
	char *date2 = NULL;
	char *date3 = NULL;

	date1 = (char *)calloc(1, GL_DATE_INFO_LEN_MAX);
	GL_CHECK_VAL(date1, -1);

	date2 = (char *)calloc(1, GL_DATE_INFO_LEN_MAX);
	if (date2 == NULL) {
		goto GL_TILE_ERROR;
	}

	date3 = (char *)calloc(1, GL_DATE_INFO_LEN_MAX);
	if (date3 == NULL) {
		goto GL_TILE_ERROR;
	}

	struct tm t1;
	memset(&t1, 0x00, sizeof(struct tm));
	localtime_r(mtime1, &t1);
	strftime(date1, GL_DATE_INFO_LEN_MAX, "%Y.%m.%d", &t1);
	strftime(date3, GL_DATE_INFO_LEN_MAX, "%Y.%m", &t1);

	struct tm t2;
	memset(&t2, 0x00, sizeof(struct tm));
	localtime_r(mtime2, &t2);
	strftime(date2, GL_DATE_INFO_LEN_MAX, "%Y.%m.%d", &t2);

	if (!g_strcmp0(date1, date2)) {
		g_strlcpy(buf, date1, max_len);
	} else {
		strftime(date2, GL_DATE_INFO_LEN_MAX, "%Y.%m", &t2);
		snprintf(buf, max_len, "%s - %s", date2, date3);
	}

	GL_FREEIF(date1);
	GL_FREEIF(date2);
	GL_FREEIF(date3);
	return 0;

GL_TILE_ERROR:

	GL_FREEIF(date1);
	GL_FREEIF(date2);
	GL_FREEIF(date3);
	return -1;
}

Evas_Object *_gl_tile_add_gengrid(Evas_Object *parent)
{
	GL_CHECK_NULL(parent);
	Evas_Object *grid = elm_gengrid_add(parent);
	GL_CHECK_NULL(grid);

#ifdef _USE_CUSTOMIZED_GENGRID_STYLE
	elm_object_style_set(grid, GL_GENGRID_STYLE_GALLERY);
#endif
	/* horizontal scrolling */
#ifdef _USE_SCROL_HORIZONRAL
	elm_gengrid_align_set(grid, GL_TILE_ALIGN_MID, GL_TILE_ALIGN_MID);
	elm_gengrid_horizontal_set(grid, EINA_TRUE);
	elm_scroller_bounce_set(grid, EINA_TRUE, EINA_FALSE);
#else
	elm_gengrid_align_set(grid, GL_TILE_ALIGN_MID, 0.0);
	elm_gengrid_horizontal_set(grid, EINA_FALSE);
	elm_scroller_bounce_set(grid, EINA_FALSE, EINA_TRUE);
#endif
	elm_scroller_policy_set(grid, ELM_SCROLLER_POLICY_OFF,
	                        ELM_SCROLLER_POLICY_AUTO);

	elm_gengrid_multi_select_set(grid, EINA_TRUE);
	evas_object_size_hint_weight_set(grid, EVAS_HINT_EXPAND,
	                                 EVAS_HINT_EXPAND);
	evas_object_data_set(grid, GL_GESTURE_KEY_PINCH_LEVEL,
	                     (void *)GL_ZOOM_DEFAULT);
	evas_object_event_callback_add(grid, EVAS_CALLBACK_DEL,
	                               __gl_title_grid_del_cb, grid);
	return grid;
}

#ifdef _USE_SCROL_HORIZONRAL
/* Change icon size and gengrid alignment */
int _gl_tile_update_item_size(void *data, Evas_Object *grid, bool b_update)
{
	GL_CHECK_VAL(grid, -1);
	GL_CHECK_VAL(data, -1);
	gl_appdata *ad = (gl_appdata *)data;
	int rotate_mode = ad->maininfo.rotate_mode;
	int w = 0;
	int h = 0;
	int c = 0;
	int c2 = 0;
	int w_l = 0;
	int h_l = 0;
	int c_l = 0;
	int win_w = 0;
	int win_h = 0;
	double scale = _gl_get_win_factor(ad->maininfo.win, &win_w, &win_h);
	gl_dbg("rotate_mode: %d, b_update: %d", rotate_mode, b_update);

	w = (int)(win_w / GL_TILE_2_PER_ROW);
	h = (int)(GL_TILE_ITEM_HEIGHT * scale);
	w_l = (int)(win_h / GL_TILE_3_PER_ROW);
	h_l = (int)(GL_TILE_ITEM_L_H * scale);
	c = GL_TILE_ITEM_CNT;
	c_l = GL_TILE_ITEM_L_CNT;
	c2 = GL_TILE_ITEM_C_CNT;

	/* Change size in pinch zoom out mode */
	int level = (int)evas_object_data_get(grid, GL_GESTURE_KEY_PINCH_LEVEL);
	if (level < GL_ZOOM_DEFAULT) {
		gl_dbg("Use smaller size");
		w = (int)(win_w / GL_TILE_3_PER_ROW);
		h *= 0.75f;
		c = GL_TILE_ITEM_CNT2;
		c2 = GL_TILE_ITEM_C_CNT + 1;
	}
	gl_dbg("P %dx%d L %dx%d C %dx%d", w, h, w_l, h_l, c, c_l);

	double align_x = 0.0f;
	double align_y = 0.0f;
	unsigned int count = 0;
	count = elm_gengrid_items_count(grid);
	elm_gengrid_align_get(grid, &align_x, &align_y);
	gl_dbg("count: %d, align_x: %f, align_y: %f", count, align_x, align_y);

	if ((rotate_mode == APP_DEVICE_ORIENTATION_270) ||
	        (rotate_mode == APP_DEVICE_ORIENTATION_90)) {
		/* >=3, (0.0, 0.5);  <3, (0.5, 0.5) */
		if (count >= c_l && (align_x > GL_TILE_ALIGN_CON1 ||
		                     align_y > GL_TILE_ALIGN_CON2))
			elm_gengrid_align_set(grid, GL_TILE_ALIGN_LEFT,
			                      GL_TILE_ALIGN_MID);
		else if (count < c_l && (align_x < GL_TILE_ALIGN_CON1 ||
		                         align_y > GL_TILE_ALIGN_CON2))
			elm_gengrid_align_set(grid, GL_TILE_ALIGN_MID,
			                      GL_TILE_ALIGN_MID);

		elm_gengrid_item_size_set(grid, w_l, h_l);
	} else {
		/* >=6, (0.0, 1.0);  <3, (0.5, 0.5);  <6, (0.5, 1.0) */
		if (count >= c && (align_x > GL_TILE_ALIGN_CON1 ||
		                   align_y < GL_TILE_ALIGN_CON2)) {
			elm_gengrid_align_set(grid, GL_TILE_ALIGN_LEFT,
			                      GL_TILE_ALIGN_MID);
		} else if (count < c) {
			if (count < c2 &&
			        (align_x < GL_TILE_ALIGN_CON1 ||
			         align_y > GL_TILE_ALIGN_CON2))
				elm_gengrid_align_set(grid, GL_TILE_ALIGN_MID,
				                      GL_TILE_ALIGN_MID);
			else if (count >= c2 &&
			         (align_x < GL_TILE_ALIGN_CON1 ||
			          align_y < GL_TILE_ALIGN_CON2))
				elm_gengrid_align_set(grid, GL_TILE_ALIGN_MID,
				                      GL_TILE_ALIGN_MID);
		}
		elm_gengrid_item_size_set(grid, w, h);
	}

	if (b_update) {
		elm_gengrid_realized_items_update(grid);
	}
	return 0;
}
#else
/* Change icon size and gengrid alignment */
int _gl_tile_update_item_size(void *data, Evas_Object *grid, bool b_update)
{
	GL_CHECK_VAL(grid, -1);
	GL_CHECK_VAL(data, -1);
	gl_appdata *ad = (gl_appdata *)data;
	int rotate_mode = ad->maininfo.rotate_mode;
	int w = 0;
	int h = 0;
	int c = 0;
	int w_l = 0;
	int h_l = 0;
	int c_l = 0;
	int win_w = 0;
	int win_h = 0;
	double scale = _gl_get_win_factor(ad->maininfo.win, &win_w, &win_h);
	gl_dbg("rotate_mode: %d, b_update: %d", rotate_mode, b_update);

	w = (int)(win_w / GL_TILE_2_PER_ROW);
	h = (int)(298 * scale);
	w_l = (int)(win_h / GL_TILE_3_PER_ROW);
	h_l = w_l;
	c = GL_TILE_ITEM_CNT;
	c_l = GL_TILE_ITEM_L_CNT;

	/* Change size in pinch zoom out mode */
	int level = (int)evas_object_data_get(grid, GL_GESTURE_KEY_PINCH_LEVEL);
	if (level < GL_ZOOM_DEFAULT) {
		gl_dbg("Use smaller size");
		w = (int)(win_w / GL_TILE_3_PER_ROW);
		w_l = (int)(win_h / GL_TILE_4_PER_ROW);
		h = (int)(200 * scale);
		c = GL_TILE_ITEM_CNT2;
	}
	h = w + win_w / 9;
	h_l = w_l + win_h / 9;
	gl_dbg("P %dx%d L %dx%d C %dx%d", w, h, w_l, h_l, c, c_l);

	double align_x = 0.0f;
	double align_y = 0.0f;
	unsigned int count = 0;
	count = elm_gengrid_items_count(grid);
	elm_gengrid_align_get(grid, &align_x, &align_y);
	gl_dbg("count: %d, align_x: %f, align_y: %f", count, align_x, align_y);

	if ((rotate_mode == APP_DEVICE_ORIENTATION_270) ||
	        (rotate_mode == APP_DEVICE_ORIENTATION_90)) {
		/* >=3, (0.5, 0.0);  <3, (0.5, 0.5) */
		if (count >= c_l && align_y > GL_TILE_ALIGN_CON1) {
			elm_gengrid_align_set(grid, GL_TILE_ALIGN_MID, 0.0);
		} else if (count < c_l && align_y < GL_TILE_ALIGN_CON1) {
			elm_gengrid_align_set(grid, GL_TILE_ALIGN_MID, 0.0);
		}

		elm_gengrid_item_size_set(grid, w_l, h_l);
	} else {
		/* >=6, (0.5, 0.0);  <3, (0.5, 0.5);  <6, (0.5, 1.0) */
		if (count >= c && (align_x > GL_TILE_ALIGN_CON1 ||
		                   align_y < GL_TILE_ALIGN_CON2)) {
			elm_gengrid_align_set(grid, GL_TILE_ALIGN_MID, 0.0);
		} else if (count < c && align_y < GL_TILE_ALIGN_CON1) {
			elm_gengrid_align_set(grid, GL_TILE_ALIGN_MID, 0.0);
		}
		elm_gengrid_item_size_set(grid, w, h);
	}

	if (b_update) {
		elm_gengrid_realized_items_update(grid);
	}
	return 0;
}
#endif

Evas_Object *_gl_tile_add_gesture(void *data, Evas_Object *parent)
{
	GL_CHECK_NULL(parent);
	GL_CHECK_NULL(data);
	Evas_Object *gesture = _gl_gesture_add(data, parent);
	GL_CHECK_NULL(gesture);
	_gl_gesture_set_zoom_in_cb(gesture, __gl_tile_zoom_in_cb, data);
	_gl_gesture_set_zoom_out_cb(gesture, __gl_tile_zoom_out_cb, data);
	return gesture;
}

