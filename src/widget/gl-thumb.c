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
#include "gl-thumb.h"
#include "gl-util.h"
#include "gl-pinchzoom.h"
#include "gl-strings.h"
#include "gl-icons.h"
#ifdef _USE_ROTATE_BG
#include "gl-rotate-bg.h"
#include "gl-exif.h"
#endif
#include "gl-gesture.h"

/* Size of thumbnails in gridview */
#define GL_GRID_W 180
#define GL_GRID_H 133

#define GL_GRID_W_ZOOM_03 240
#define GL_GRID_H_ZOOM_03 178

#define GL_GRID_W_ZOOM_02 360
#define GL_GRID_H_ZOOM_02 267

#define GL_GRID_5_PER_ROW 5
#define GL_GRID_8_PER_ROW 8
#define GL_GRID_4_PER_ROW 4
#define GL_GRID_3_PER_ROW 3
#define GL_GRID_2_PER_ROW 2

#define GL_GRID_6_PER_ROW 6
#define GL_GRID_7_PER_ROW 7
#define GL_GRID_10_PER_ROW 10

#define GL_GRID_L_W GL_GRID_W_ZOOM_02
#define GL_GRID_L_H GL_GRID_H_ZOOM_02

#define GL_GRID_ITEM_L_CNT 6
#define GL_GRID_ITEM_ZOOM_02_CNT 8
#define GL_GRID_ITEM_ZOOM_03_CNT 18

#define GL_PHOTOFRAME_VIDEO "video"
#define GL_PHOTOFRAME_VIDEO2 "video2"
#define GL_PHOTOFRAME_VIDEO3 "video3"
#define GL_PHOTOFRAME_DEFAULT "layout"
#define GL_PHOTOFRAME_DEFAULT2 "layout2"
#define GL_PHOTOFRAME_DEFAULT3 "layout3"
#define GL_PHOTOFRAME_BURSTSHOT "burstshot"
#define GL_PHOTOFRAME_BURSTSHOT1 "burstshot1"
#define GL_PHOTOFRAME_BURSTSHOT2 "burstshot2"

static Evas_Object *__gl_thumb_add_image(Evas_Object *parent, char *path,
					unsigned int orient, int w, int h)
{
	GL_CHECK_NULL(parent);

#ifdef _USE_ROTATE_BG
	Evas_Object *bg = _gl_rotate_bg_add(parent, true);
#else
	Evas_Object *bg = elm_bg_add(parent);
#endif
	if (bg == NULL) {
		return NULL;
	}

#ifdef _USE_ROTATE_BG
	_gl_rotate_bg_set_file(bg, path, w, h);
	_gl_rotate_bg_rotate_image(bg, orient);
#else
	elm_bg_file_set(bg, path, NULL);
	elm_bg_load_size_set(bg, w, h);
	evas_object_size_hint_max_set(bg, w, h);
	evas_object_size_hint_aspect_set(bg, EVAS_ASPECT_CONTROL_VERTICAL, 1, 1);
	evas_object_size_hint_weight_set(bg, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	evas_object_size_hint_align_set(bg, EVAS_HINT_FILL, EVAS_HINT_FILL);
#endif
	return bg;
}

Evas_Object *_gl_thumb_show_image(Evas_Object *obj, char *path, int orient,
                                  int w, int h, int zoom_level)
{
	GL_CHECK_NULL(obj);

	Evas_Object *layout = elm_layout_add(obj);
	GL_CHECK_NULL(layout);

	Evas_Object *bg = __gl_thumb_add_image(layout, path, orient, w, h);
	if (bg == NULL) {
		evas_object_del(layout);
		return NULL;
	}

	char *bs_style = GL_PHOTOFRAME_DEFAULT;
	if (zoom_level == GL_ZOOM_IN_ONE) {
		bs_style = GL_PHOTOFRAME_DEFAULT2;
	} else if (zoom_level == GL_ZOOM_IN_TWO) {
		bs_style = GL_PHOTOFRAME_DEFAULT3;
	}
	elm_layout_theme_set(layout, GL_CLASS_GENGRID, GL_GRP_PHOTOFRAME,
	                     bs_style);
	elm_object_part_content_set(layout, "elm.swallow.icon", bg);

	evas_object_show(layout);

	return layout;
}

Evas_Object *_gl_thumb_show_burstshot(Evas_Object *obj, char *path1,
                                      char *path2, char *path3,
                                      unsigned int orient, int w, int h,
                                      int zoom_level)
{
	GL_CHECK_NULL(obj);
	Evas_Object *bg = NULL;
	char *part = NULL;
	char *path = NULL;
	int i = 0;

	Evas_Object *layout = elm_layout_add(obj);
	GL_CHECK_NULL(layout);
	const char *bs_style = GL_PHOTOFRAME_BURSTSHOT;
	if (zoom_level == GL_ZOOM_IN_ONE) {
		bs_style = GL_PHOTOFRAME_BURSTSHOT1;
	} else if (zoom_level == GL_ZOOM_IN_TWO) {
		bs_style = GL_PHOTOFRAME_BURSTSHOT2;
	}
	elm_layout_theme_set(layout, GL_CLASS_GENGRID, GL_GRP_PHOTOFRAME,
	                     bs_style);

	/* Add bg for burstshot mode */
	for (i = 0; i < 3; i++) {
		switch (i) {
		case 0:
			part = "elm.swallow.icon1";
			path = path1;
			break;
		case 1:
			part = "elm.swallow.icon2";
			path = path2;
			break;
		case 2:
			part = "elm.swallow.icon3";
			path = path3;
			break;
		}

		bg = __gl_thumb_add_image(layout, path, orient, w, h);
		if (bg == NULL) {
			evas_object_del(layout);
			return NULL;
		}
		elm_object_part_content_set(layout, part, bg);
	}
	evas_object_show(layout);
	return layout;
}

Evas_Object *_gl_thumb_show_video(Evas_Object *obj, char *path,
                                  unsigned int v_dur, int w, int h,
                                  int zoom_level)
{
	GL_CHECK_NULL(obj);

	Evas_Object *layout = elm_layout_add(obj);
	GL_CHECK_NULL(layout);

#ifdef _USE_ROTATE_BG
	Evas_Object *bg = _gl_rotate_bg_add(layout, true);
#else
	Evas_Object *bg = elm_bg_add(layout);
#endif
	if (bg == NULL) {
		evas_object_del(layout);
		return NULL;
	}

#ifdef _USE_ROTATE_BG
	_gl_rotate_bg_set_file(bg, path, w, h);
	_gl_rotate_bg_rotate_image(bg, GL_ORIENTATION_ROT_0);
#else
	elm_bg_file_set(bg, path, NULL);
	elm_bg_load_size_set(bg, w, h);
	evas_object_size_hint_max_set(bg, w, h);
	evas_object_size_hint_aspect_set(bg, EVAS_ASPECT_CONTROL_VERTICAL, 1, 1);
	evas_object_size_hint_weight_set(bg, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	evas_object_size_hint_align_set(bg, EVAS_HINT_FILL, EVAS_HINT_FILL);
#endif

	char *bs_style = GL_PHOTOFRAME_VIDEO;
	if (zoom_level == GL_ZOOM_IN_ONE) {
		bs_style = GL_PHOTOFRAME_VIDEO2;
	} else if (zoom_level == GL_ZOOM_IN_TWO) {
		bs_style = GL_PHOTOFRAME_VIDEO3;
	}
	elm_layout_theme_set(layout, GL_CLASS_GENGRID, GL_GRP_PHOTOFRAME,
	                     bs_style);
	elm_object_part_content_set(layout, "elm.swallow.icon", bg);

	char *dur_str = _gl_get_duration_string(v_dur);
	GL_CHECK_NULL(dur_str);
	elm_object_part_text_set(layout, GL_GRID_TEXT, dur_str);
	GL_FREE(dur_str);

	evas_object_show(layout);
	return layout;
}

Evas_Object *_gl_thumb_show_checkbox(Evas_Object *obj, bool checked,
                                     Evas_Smart_Cb func, const void *data)
{
	GL_CHECK_NULL(func);
	GL_CHECK_NULL(obj);
	Evas_Object *ck = NULL;

	ck = elm_check_add(obj);
	GL_CHECK_NULL(ck);

#ifdef _USE_GRID_CHECK
	elm_object_style_set(ck, "grid");
#else
	elm_object_style_set(ck, GL_CHECKBOX_STYLE_THUMB);
#endif
	evas_object_propagate_events_set(ck, EINA_FALSE);

	elm_check_state_set(ck, checked);

	evas_object_smart_callback_add(ck, "changed", func, data);

	evas_object_show(ck);
	return ck;
}

Evas_Object *_gl_thumb_show_mode(Evas_Object *obj, gl_cm_mode_e mode)
{
	Evas_Object *icon = NULL;
	const char *file = NULL;

	switch (mode) {
	case GL_CM_MODE_PANORAMA:
		file = GL_ICON_PANORAMA;
		break;
	case GL_CM_MODE_BURSTSHOT:
		file = GL_ICON_BURSTSHOT;
		break;
	case GL_CM_MODE_SOUND:
		file = GL_ICON_SOUND_SHOT;
		break;
	case GL_CM_MODE_ANIMATE:
		file = GL_ICON_SOUND_SHOT;
		break;
	case GL_CM_MODE_BEST:
		file = GL_ICON_SOUND_SHOT;
		break;
	default:
		break;
	}

	if (file) {
		GL_CHECK_NULL(obj);
		icon = elm_icon_add(obj);
		GL_CHECK_NULL(icon);
		GL_ICON_SET_FILE(icon, file);
		evas_object_show(icon);
	}
	return icon;
}

Evas_Object *_gl_thumb_add_gengrid(Evas_Object *parent)
{
	GL_CHECK_NULL(parent);
	Evas_Object *grid = elm_gengrid_add(parent);
	GL_CHECK_NULL(grid);

#ifdef _USE_CUSTOMIZED_GENGRID_STYLE
	elm_object_style_set(grid, GL_GENGRID_STYLE_GALLERY);
#endif
#ifdef _USE_SCROL_HORIZONRAL
	elm_gengrid_align_set(grid, 0.5, 0.5);
	elm_gengrid_horizontal_set(grid, EINA_TRUE);
	elm_scroller_bounce_set(grid, EINA_TRUE, EINA_FALSE);
#else
	elm_gengrid_align_set(grid, 0.0, 0.0);
	elm_gengrid_horizontal_set(grid, EINA_FALSE);
	elm_scroller_bounce_set(grid, EINA_FALSE, EINA_TRUE);
#endif
	elm_scroller_policy_set(grid, ELM_SCROLLER_POLICY_OFF,
	                        ELM_SCROLLER_POLICY_AUTO);
	elm_gengrid_multi_select_set(grid, EINA_TRUE);
	evas_object_size_hint_weight_set(grid, EVAS_HINT_EXPAND,
	                                 EVAS_HINT_EXPAND);
	return grid;
}

/* Update all gengrid item, class callback would be called again */
int _gl_thumb_update_gengrid(Evas_Object *view)
{
	GL_CHECK_VAL(view, -1);

	Elm_Object_Item *_first_it = NULL;
	_first_it = elm_gengrid_first_item_get(view);
	while (_first_it) {
		elm_gengrid_item_update(_first_it);
		_first_it = elm_gengrid_item_next_get(_first_it);
	}

	return 0;
}

#ifdef _USE_SCROL_HORIZONRAL
/* Change icon size and gengrid alignment */
int _gl_thumb_set_size(void *data, Evas_Object *view, int *size_w, int *size_h)
{
	GL_CHECK_VAL(view, -1);
	GL_CHECK_VAL(data, -1);
	gl_appdata *ad = (gl_appdata *)data;
	int rotate_mode = ad->maininfo.rotate_mode;
	int _w = 0;
	int _h = 0;
	int _w_l = 0;
	int _h_l = 0;
	int align_c = 0;
	int count = elm_gengrid_items_count(view);
	int win_w = 0;
	int win_h = 0;
	double scale = _gl_get_win_factor(ad->maininfo.win, &win_w, &win_h);
	gl_dbg("rotate_mode: %d, zoom level: %d", rotate_mode,
	       ad->pinchinfo.zoom_level);

	_w_l = (int)(GL_GRID_L_W * scale);
	_h_l = (int)(GL_GRID_L_H * scale);

	if (ad->pinchinfo.zoom_level == GL_ZOOM_IN_TWO) {
		_w = (int)(win_w / GL_GRID_3_PER_ROW);
		_h = (int)(GL_GRID_H_ZOOM_02 * scale);
		align_c = GL_GRID_ITEM_ZOOM_02_CNT;
	} else if (ad->pinchinfo.zoom_level == GL_ZOOM_IN_ONE) {
GL_THUMB_SIZE:
		_w = (int)(win_w / GL_GRID_4_PER_ROW);
		_h = (int)(GL_GRID_H_ZOOM_03 * scale);
		align_c = GL_GRID_ITEM_ZOOM_03_CNT;
	} else if (ad->pinchinfo.zoom_level == GL_ZOOM_DEFAULT) {
		_w = (int)(win_w / GL_GRID_8_PER_ROW);
		_h = (int)(GL_GRID_H * scale);
	} else {
		/* Set size same as ZOOM_IN_TWO except zoom_in_one and default level */
		goto GL_THUMB_SIZE;
	}

	if ((rotate_mode == APP_DEVICE_ORIENTATION_270) ||
	        (rotate_mode == APP_DEVICE_ORIENTATION_90)) {
		align_c = GL_GRID_ITEM_L_CNT;
		elm_gengrid_item_size_set(view, _w_l, _h_l);
		if (size_w) {
			*size_w = _w_l - 4;
		}
		if (size_h) {
			*size_h = _h_l - 4;
		}
	} else {
		elm_gengrid_item_size_set(view, _w, _h);
		if (size_w) {
			*size_w = _w - 4;
		}
		if (size_h) {
			*size_h = _h - 4;
		}
	}
	if (count <= align_c) {
		elm_gengrid_align_set(view, 0.5, 0.5);
	} else {
		elm_gengrid_align_set(view, 0.0, 0.5);
	}
	if (size_w && size_h) {
		gl_dbg("P: %dx%d, size_w: %d,size_h: %d", _w, _h, *size_w, *size_h);
	}
	return 0;
}
#else
/* Change icon size and gengrid alignment */
int _gl_thumb_set_size(void *data, Evas_Object *view, int *size_w, int *size_h)
{
	GL_CHECK_VAL(view, -1);
	GL_CHECK_VAL(data, -1);
	gl_appdata *ad = (gl_appdata *)data;
	int rotate_mode = ad->maininfo.rotate_mode;
	int _w = 0;
	int _h = 0;
	int _w_l = 0;
	int _h_l = 0;
	int win_w = 0;
	int win_h = 0;
	double scale = _gl_get_win_factor(ad->maininfo.win, &win_w, &win_h);

	gl_dbg("Scale is : %lf", scale);
	if (gl_get_view_mode(ad) == GL_VIEW_THUMBS_EDIT) {
		ad->pinchinfo.zoom_level = GL_ZOOM_IN_ONE;
	}
	gl_dbg("rotate_mode: %d, zoom level: %d", rotate_mode,
	       ad->pinchinfo.zoom_level);

	if (ad->pinchinfo.zoom_level == GL_ZOOM_IN_TWO) {
		_w = (int)(win_w / GL_GRID_3_PER_ROW);
		_w_l = (int)(win_h / GL_GRID_6_PER_ROW);
	} else if (ad->pinchinfo.zoom_level == GL_ZOOM_IN_ONE) {
GL_THUMB_SIZE:
		_w = (int)(win_w / GL_GRID_4_PER_ROW);
		_w_l = (int)(win_h / GL_GRID_7_PER_ROW);
	} else if (ad->pinchinfo.zoom_level == GL_ZOOM_DEFAULT) {
		_w = (int)(win_w / GL_GRID_8_PER_ROW);
		_w_l = (int)(win_h / GL_GRID_10_PER_ROW);
	} else {
		/* Set size same as ZOOM_IN_TWO except zoom_in_one and default level */
		goto GL_THUMB_SIZE;
	}

	_h_l = _w_l;
	_h = _w;
	if ((rotate_mode == APP_DEVICE_ORIENTATION_270) ||
	        (rotate_mode == APP_DEVICE_ORIENTATION_90)) {
		elm_gengrid_item_size_set(view, _w_l, _h_l);
		if (size_w) {
			*size_w = _w_l - 4;
		}
		if (size_h) {
			*size_h = _h_l - 4;
		}
	} else {
		elm_gengrid_item_size_set(view, _w, _h);
		if (size_w) {
			*size_w = _w - 4;
		}
		if (size_h) {
			*size_h = _h - 4;
		}
	}
	/*if (elm_gengrid_items_count(view) <= align_c)
		elm_gengrid_align_set(view, 0.5, 0.5);
	else
		elm_gengrid_align_set(view, 0.0, 0.0);*/
	if (size_w && size_h) {
		gl_dbg("P: %dx%d, size_w: %d,size_h: %d", _w, _h, *size_w, *size_h);
	}
	return 0;
}
#endif

int _gl_thumb_split_set_size(void *data, Evas_Object *view)
{
	GL_CHECK_VAL(view, -1);
	GL_CHECK_VAL(data, -1);
	gl_appdata *ad = (gl_appdata *)data;
	int rotate_mode = ad->maininfo.rotate_mode;
	int _w = 0;
	int _h = 0;
	int _w_l = 0;
	int _h_l = 0;
	int win_w = 0;
	int win_h = 0;
	double scale = _gl_get_win_factor(ad->maininfo.win, &win_w, &win_h);
	gl_dbg("rotate_mode: %d, zoom level: %d, scale : %f", rotate_mode,
	       ad->pinchinfo.zoom_level, scale);

	_w = (int)((2 * win_w) / (3 * GL_GRID_2_PER_ROW));
	_h = _w;
	_w_l = (int)((win_h - win_w / 3) / GL_GRID_5_PER_ROW);
	_h_l = _w_l;

	if ((rotate_mode == APP_DEVICE_ORIENTATION_270) ||
	        (rotate_mode == APP_DEVICE_ORIENTATION_90)) {
		elm_gengrid_item_size_set(view, _w_l, _h_l);
	} else {
		elm_gengrid_item_size_set(view, _w, _h);
	}

	return 0;
}

/*Preappend date item*/
bool _gl_thumb_insert_date(void *data, Evas_Object *parent)
{
	GL_CHECK_FALSE(data);
	GL_CHECK_FALSE(parent);
	gl_appdata *ad = (gl_appdata *)data;
	bool b_remove = false;

	if (ad->pinchinfo.zoom_level != GL_ZOOM_DEFAULT) {
		b_remove = true;
	}

	gl_item *gitem = NULL;
	gl_item *pre_gitem = NULL;
	gl_item *inserted_gitem = NULL;
	bool b_insert = false;
	Elm_Object_Item *elm_item = NULL;
	Elm_Object_Item *next_elm_item = NULL;
	Elm_Gengrid_Item_Class *dgic = &(ad->gridinfo.date_gic);

	elm_item = elm_gengrid_first_item_get(parent);
	while (elm_item) {
		next_elm_item = elm_gengrid_item_next_get(elm_item);
		gitem = elm_object_item_data_get(elm_item);
		if (gitem == NULL) {
			gl_dbgE("Invalid data item!");
			continue;
		} else if (b_remove) { /* Remove */
			if (elm_gengrid_item_item_class_get(elm_item) == dgic) {
				gl_dbg("Remove date item %p", dgic);
				gitem->preappend = false;
				elm_object_item_del(elm_item);
			}
			goto GL_THUMB_NEXT;
		} else if (pre_gitem == NULL) { /* Insert */
			b_insert = true;
		} else if (gitem->elm_item && !gitem->preappend &&
		           pre_gitem->item) {
			struct tm t1;
			struct tm t2;
			int mth = 0;
			int yth = 0;
			localtime_r((time_t *) & (pre_gitem->item->mtime),
			            &t1);
			mth = t1.tm_mon;
			yth = t1.tm_year;
			localtime_r((time_t *) & (gitem->item->mtime), &t2);
			if (mth != t2.tm_mon || yth != t2.tm_year) {
				b_insert = true;
			}
		}
		if (b_insert && gitem->elm_item && !gitem->preappend) {
			elm_gengrid_item_insert_before(parent, dgic,
			                               gitem, gitem->elm_item,
			                               NULL, NULL);
			gitem->preappend = true;
			gitem->pre_gitem = gitem;
			inserted_gitem = gitem;
		} else if (inserted_gitem) {
			gitem->pre_gitem = inserted_gitem;
		}

		pre_gitem = gitem;
		b_insert = false;

GL_THUMB_NEXT:

		gitem->pre_gitem = NULL;
		elm_item = next_elm_item;
	}

	return true;
}

