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

#ifdef _USE_ROTATE_BG

#include "gl-rotate-bg.h"
#include "gl-exif.h"
#include "gl-debug.h"
#include "gl-icons.h"

#define GL_ROTATE_BG_DATA_KEY "gl_bg_data"
#define GL_ROTATE_BG_GROUP "gl_bg_layout"

typedef struct _gl_bg {
	Evas_Object *base;
	Evas_Object *img;
	char *file;
	bool b_preload;
	void *user_data;
} gl_bg;

static int __gl_rotate_bg_image_rotate_180(Evas_Object *obj)
{
	GL_CHECK_VAL(obj, -1);
	unsigned int *data = NULL;
	unsigned int *data2 = NULL;
	unsigned int *to = NULL;
	unsigned int *from = NULL;
	int x = 0;
	int hw = 0;
	int iw = 0;
	int ih = 0;

	evas_object_image_size_get(obj, &iw, &ih);
	int size = iw * ih * sizeof(unsigned int);

	/* EINA_FALSE for reading */
	data = evas_object_image_data_get(obj, EINA_FALSE);
	/* memcpy */
	data2 = calloc(1, size);
	GL_CHECK_VAL(data2, -1);
	memcpy(data2, data, size);

	data = evas_object_image_data_get(obj, EINA_TRUE);

	hw = iw * ih;
	x = hw;
	to = data;
	from = data2 + hw - 1;
	for (; --x >= 0;) {
		*to = *from;
		to++;
		from--;
	}


	GL_FREE(data2);

	evas_object_image_data_set(obj, data);
	evas_object_image_data_update_add(obj, 0, 0, iw, ih);
	return 0;
}

static int __gl_rotate_bg_image_rotate_90(Evas_Object *obj)
{
	GL_CHECK_VAL(obj, -1);
	unsigned int *data = NULL;
	unsigned int *data2 = NULL;
	unsigned int *to = NULL;
	unsigned int *from = NULL;
	int x = 0;
	int y = 0;
	int w = 0;
	int hw = 0;
	int iw = 0;
	int ih = 0;

	evas_object_image_size_get(obj, &iw, &ih);
	int size = iw * ih * sizeof(unsigned int);

	/* EINA_FALSE for reading */
	data = evas_object_image_data_get(obj, EINA_FALSE);
	/* memcpy */
	data2 = calloc(1, size);
	GL_CHECK_VAL(data2, -1);
	memcpy(data2, data, size);

	/* set width, height */
	w = ih;
	ih = iw;
	iw = w;
	hw = w * ih;

	/* set width, height to image obj */
	evas_object_image_size_set(obj, iw, ih);
	data = evas_object_image_data_get(obj, EINA_TRUE);
	to = data + w - 1;
	hw = -hw - 1;
	from = data2;

	for (x = iw; --x >= 0;) {
		for (y = ih; --y >= 0;) {
			*to = *from;
			from++;
			to += w;
		}

		to += hw;
	}

	GL_FREE(data2);

	evas_object_image_data_set(obj, data);
	evas_object_image_data_update_add(obj, 0, 0, iw, ih);
	return 0;
}

static int __gl_rotate_bg_image_rotate_270(Evas_Object *obj)
{
	GL_CHECK_VAL(obj, -1);
	unsigned int *data = NULL;
	unsigned int *data2 = NULL;
	unsigned int *to = NULL;
	unsigned int *from = NULL;
	int x = 0;
	int y = 0;
	int w = 0;
	int hw = 0;
	int iw = 0;
	int ih = 0;

	evas_object_image_size_get(obj, &iw, &ih);
	int size = iw * ih * sizeof(unsigned int);

	/* EINA_FALSE for reading */
	data = evas_object_image_data_get(obj, EINA_FALSE);
	/* memcpy */
	data2 = calloc(1, size);
	GL_CHECK_VAL(data2, -1);
	memcpy(data2, data, size);

	/* set width, height */
	w = ih;
	ih = iw;
	iw = w;
	hw = w * ih;

	/* set width, height to image obj */
	evas_object_image_size_set(obj, iw, ih);
	data = evas_object_image_data_get(obj, EINA_TRUE);

	to = data + hw - w;
	w = -w;
	hw = hw + 1;
	from = data2;

	for (x = iw; --x >= 0;) {
		for (y = ih; --y >= 0;) {
			*to = *from;
			from++;
			to += w;
		}

		to += hw;
	}

	GL_FREE(data2);

	evas_object_image_data_set(obj, data);
	evas_object_image_data_update_add(obj, 0, 0, iw, ih);
	return 0;
}

/* check its orientation */
int __gl_rotate_bg_rotate_image(Evas_Object *obj, unsigned int orient)
{
	switch (orient) {
	case GL_ORIENTATION_ROT_90:
		__gl_rotate_bg_image_rotate_90(obj);
		break;
	case GL_ORIENTATION_ROT_180:
		__gl_rotate_bg_image_rotate_180(obj);
		break;
	case GL_ORIENTATION_ROT_270:
		__gl_rotate_bg_image_rotate_270(obj);
		break;
	default:
		break;
	}

	return 0;
}

static void __gl_rotate_bg_delete_cb(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
	if (data) {
		gl_bg *bg_data = (gl_bg *)data;
		GL_FREEIF(bg_data->file);
		GL_FREE(data);
	}
}

static void __gl_rotate_bg_custom_resize(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
	GL_CHECK(data);
	gl_bg *bg_data = (gl_bg *)data;
	Evas_Coord bx = 0;
	Evas_Coord by = 0;
	Evas_Coord bw = 0;
	Evas_Coord bh = 0;
	Evas_Coord iw = 0;
	Evas_Coord ih = 0;
	Evas_Coord fx = 0;
	Evas_Coord fy = 0;
	Evas_Coord fw = 0;
	Evas_Coord fh = 0;
	Evas_Coord nx = 0;
	Evas_Coord ny = 0;
	Evas_Coord nw = 0;
	Evas_Coord nh = 0;

	if ((!bg_data->img) || (!bg_data->base)) {
		gl_dbgE("Invalid object!");
		return;
	}
	/* grab image size */
	evas_object_image_size_get(bg_data->img, &iw, &ih);
	if ((iw < 1) || (ih < 1)) {
		gl_dbgE("(iw < 1) || (ih < 1)!");
		return;
	}

	/* grab base object dimensions */
	evas_object_geometry_get(bg_data->base, &bx, &by, &bw, &bh);

	/* set some defaults */
	nx = bx;
	ny = by;
	nw = bw;
	nh = bh;


	fw = bw;
	fh = ((ih * fw) / iw);
	if (fh < bh) {
		fh = bh;
		fw = ((iw * fh) / ih);
	}
	fx = ((bw - fw) / 2);
	fy = ((bh - fh) / 2);

	evas_object_move(bg_data->img, nx, ny);
	evas_object_resize(bg_data->img, nw, nh);
	evas_object_image_fill_set(bg_data->img, fx, fy, fw, fh);
}

Evas_Object *_gl_rotate_bg_add_layout(Evas_Object *parent, const char *file, const char *group)
{
	Evas_Object *eo = NULL;
	int r = 0;

	eo = elm_layout_add(parent);
	if (eo) {
		r = elm_layout_file_set(eo, file, group);
		if (!r) {
			evas_object_del(eo);
			return NULL;
		}

		evas_object_size_hint_weight_set(eo, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
		evas_object_size_hint_align_set(eo, EVAS_HINT_FILL, EVAS_HINT_FILL);
	}

	return eo;
}

Evas_Object *_gl_rotate_bg_add(Evas_Object *parent, bool b_preload)
{
	GL_CHECK_NULL(parent);

	gl_bg *bg_data = (gl_bg *)calloc(1, sizeof(gl_bg));
	GL_CHECK_NULL(bg_data);

	Evas_Object *base = NULL;
	base = _gl_rotate_bg_add_layout(parent, GL_EDJ_FILE,
	                                GL_ROTATE_BG_GROUP);
	if (base == NULL) {
		GL_FREE(bg_data);
		return NULL;
	}

	bg_data->base = base;
	bg_data->b_preload = b_preload;
	if (!b_preload)
		evas_object_event_callback_add(base, EVAS_CALLBACK_RESIZE,
		                               __gl_rotate_bg_custom_resize,
		                               bg_data);
	evas_object_event_callback_add(base, EVAS_CALLBACK_DEL,
	                               __gl_rotate_bg_delete_cb, bg_data);

	evas_object_data_set(base, GL_ROTATE_BG_DATA_KEY, bg_data);
	return base;
}

int _gl_rotate_bg_set_data(Evas_Object *bg, void *data)
{
	GL_CHECK_VAL(bg, -1);

	gl_bg *bg_data = evas_object_data_get(bg, GL_ROTATE_BG_DATA_KEY);
	GL_CHECK_VAL(bg_data, -1);
	bg_data->user_data = data;
	return 0;
}

int _gl_rotate_bg_get_data(Evas_Object *bg, void **data)
{
	GL_CHECK_VAL(data, -1);
	GL_CHECK_VAL(bg, -1);

	gl_bg *bg_data = evas_object_data_get(bg, GL_ROTATE_BG_DATA_KEY);
	GL_CHECK_VAL(bg_data, -1);
	*data = bg_data->user_data;
	return 0;
}

int _gl_rotate_bg_set_file(Evas_Object *bg, const char *file, int w, int h)
{
	GL_CHECK_VAL(file, -1);
	GL_CHECK_VAL(bg, -1);

	gl_bg *bg_data = evas_object_data_get(bg, GL_ROTATE_BG_DATA_KEY);
	GL_CHECK_VAL(bg_data, -1);

	Evas_Object *image_obj = NULL;
	if (!bg_data->b_preload) {
		image_obj = evas_object_image_add(evas_object_evas_get(bg));
		evas_object_repeat_events_set(image_obj, EINA_TRUE);
		bg_data->img = image_obj;
		bg_data->file = strdup(file);
		elm_object_part_content_set(bg, "elm.swallow.image", image_obj);
		evas_object_image_load_size_set(image_obj, w, h);
		evas_object_image_file_set(image_obj, file, NULL);
		evas_object_image_preload(image_obj, EINA_TRUE);
	} else {
		image_obj = elm_image_add(bg);
		evas_object_repeat_events_set(image_obj, EINA_TRUE);
		bg_data->img = image_obj;
		bg_data->file = strdup(file);
		elm_object_part_content_set(bg, "elm.swallow.image", image_obj);
		elm_image_fill_outside_set(image_obj, EINA_TRUE);
		elm_image_file_set(image_obj, file, NULL);
		elm_image_preload_disabled_set(image_obj, EINA_FALSE);
		elm_image_smooth_set(image_obj, EINA_FALSE);
	}
	return 0;
}

int _gl_rotate_bg_get_file(Evas_Object *bg, char **file)
{
	GL_CHECK_VAL(file, -1);
	GL_CHECK_VAL(bg, -1);

	gl_bg *bg_data = evas_object_data_get(bg, GL_ROTATE_BG_DATA_KEY);
	GL_CHECK_VAL(bg_data, -1);
	*file = bg_data->file;
	return 0;
}

int _gl_rotate_bg_rotate_image(Evas_Object *bg, unsigned int orient)
{
	GL_CHECK_VAL(bg, -1);

	gl_bg *bg_data = evas_object_data_get(bg, GL_ROTATE_BG_DATA_KEY);
	GL_CHECK_VAL(bg_data, -1);

	if (bg_data->file && g_strcmp0(bg_data->file, GL_ICON_NO_THUMBNAIL) &&
	        g_strcmp0(bg_data->file, GL_ICON_CONTENTS_BROKEN)) {
		__gl_rotate_bg_rotate_image(bg_data->img, orient);
	} else {
		__gl_rotate_bg_rotate_image(bg_data->img, GL_ORIENTATION_ROT_0);
	}
	if (!bg_data->b_preload) {
		__gl_rotate_bg_custom_resize(bg_data, NULL, NULL, NULL);
	}
	return 0;
}

int _gl_rotate_bg_add_image(Evas_Object *bg, int w, int h)
{
	GL_CHECK_VAL(bg, -1);

	gl_bg *bg_data = evas_object_data_get(bg, GL_ROTATE_BG_DATA_KEY);
	GL_CHECK_VAL(bg_data, -1);

	Evas_Object *image_obj = NULL;
	if (!bg_data->b_preload) {
		image_obj = evas_object_image_add(evas_object_evas_get(bg));
		evas_object_repeat_events_set(image_obj, EINA_TRUE);
		bg_data->img = image_obj;
		elm_object_part_content_set(bg, "elm.swallow.image", image_obj);
		evas_object_image_load_size_set(image_obj, w, h);
	} else {
		image_obj = elm_image_add(bg);
		evas_object_repeat_events_set(image_obj, EINA_TRUE);
		bg_data->img = image_obj;
		elm_object_part_content_set(bg, "elm.swallow.image", image_obj);
	}
	return 0;
}

int _gl_rotate_bg_set_image_file(Evas_Object *bg, const char *file)
{
	GL_CHECK_VAL(bg, -1);

	gl_bg *bg_data = evas_object_data_get(bg, GL_ROTATE_BG_DATA_KEY);
	GL_CHECK_VAL(bg_data, -1);
	GL_CHECK_VAL(bg_data->img, -1);

	if (!bg_data->b_preload) {
		evas_object_image_file_set(bg_data->img, file, NULL);
		evas_object_image_preload(bg_data->img, EINA_FALSE);
		__gl_rotate_bg_custom_resize(bg_data, NULL, NULL, NULL);
	} else {
		elm_image_fill_outside_set(bg_data->img, EINA_TRUE);
		elm_image_file_set(bg_data->img, file, NULL);
		elm_image_preload_disabled_set(bg_data->img, EINA_FALSE);
		elm_image_smooth_set(bg_data->img, EINA_FALSE);
	}

	return 0;
}

#endif