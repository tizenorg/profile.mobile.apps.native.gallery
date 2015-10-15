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
#include "gl-pinchzoom.h"
#include "gl-gesture.h"
#include "gl-thumbs.h"
#include "gl-util.h"
#include "gl-debug.h"
#include "gl-thumbs-sel.h"

static Eina_Bool __gl_pinch_zoom_out_cb(Evas_Object *gesture, void *data)
{
	GL_CHECK_CANCEL(data);
	gl_appdata *ad = (gl_appdata *)data;
	int view_m = gl_get_view_mode(ad);
	gl_dbg("view_m: %d", view_m);

	if (ad->pinchinfo.zoom_level > GL_ZOOM_DEFAULT) {
		/* Skip in wrong mode */
		if (view_m != GL_VIEW_THUMBS && view_m != GL_VIEW_THUMBS_EDIT &&
		    view_m != GL_VIEW_THUMBS_SELECT)
			goto PINCH_ZOOM_OUT_OVER;

		if (view_m == GL_VIEW_THUMBS && !_gl_thumbs_get_view(ad, NULL))
			goto PINCH_ZOOM_OUT_OVER;

#ifdef _USE_SCROL_HORIZONRAL
		if (ad->maininfo.rotate_mode == APP_DEVICE_ORIENTATION_270 ||
		    ad->maininfo.rotate_mode == APP_DEVICE_ORIENTATION_90) {
			/* Keep level if pinching zoom in in landscape mode */
			goto PINCH_ZOOM_OUT_OVER;
		}
#endif
		ad->pinchinfo.zoom_level--;
		if (_gl_thumbs_update_size(data, NULL) < 0)
			ad->pinchinfo.zoom_level = GL_ZOOM_DEFAULT;
		goto PINCH_ZOOM_OUT_OVER;
	}

 PINCH_ZOOM_OUT_OVER:

	gl_dbgW("Pinch zoom out: %d", ad->pinchinfo.zoom_level);
	return ECORE_CALLBACK_CANCEL;
}

static Eina_Bool __gl_pinch_zoom_in_cb(Evas_Object *gesture, void *data)
{
	GL_CHECK_CANCEL(data);
	gl_appdata *ad = (gl_appdata *)data;
	int view_m = gl_get_view_mode(ad);
	gl_dbg("view_m: %d", view_m);

	if ((ad->pinchinfo.zoom_level >= GL_ZOOM_DEFAULT) &&
	    (ad->pinchinfo.zoom_level < (GL_ZOOM_MAX - 1))) {
		/* Skip in wrong mode */
		if (view_m != GL_VIEW_THUMBS && view_m != GL_VIEW_THUMBS_EDIT &&
		    view_m != GL_VIEW_THUMBS_SELECT)
			goto PINCH_ZOOM_IN_OVER;

		if (view_m == GL_VIEW_THUMBS && !_gl_thumbs_get_view(ad, NULL))
			goto PINCH_ZOOM_IN_OVER;

#ifdef _USE_SCROL_HORIZONRAL
		if (ad->maininfo.rotate_mode == APP_DEVICE_ORIENTATION_270 ||
		    ad->maininfo.rotate_mode == APP_DEVICE_ORIENTATION_90) {
			/* Keep level if pinching zoom in in landscape mode */
			goto PINCH_ZOOM_IN_OVER;
		}
#endif
		ad->pinchinfo.zoom_level++;
		if (_gl_thumbs_update_size(data, NULL) < 0)
			ad->pinchinfo.zoom_level = GL_ZOOM_IN_TWO;
		goto PINCH_ZOOM_IN_OVER;
	}
 PINCH_ZOOM_IN_OVER:

	gl_dbgW("Pinch zoom in: %d", ad->pinchinfo.zoom_level);
	return ECORE_CALLBACK_CANCEL;
}

int _gl_pinch_add_event(void *data, Evas_Object *layout)
{
	GL_CHECK_VAL(layout, -1);
	GL_CHECK_VAL(data, -1);
	gl_appdata *ad = (gl_appdata *)data;

	gl_dbg("Zoom level: %d", ad->pinchinfo.zoom_level);
	/* Set initialize level */
	if (ad->pinchinfo.zoom_level != GL_ZOOM_DEFAULT &&
	    ad->pinchinfo.zoom_level != GL_ZOOM_IN_ONE &&
	    ad->pinchinfo.zoom_level != GL_ZOOM_IN_TWO) {
		ad->pinchinfo.zoom_level = GL_ZOOM_IN_ONE;
		gl_dbg("Zoom level reset: GL_ZOOM_IN_ONE");
	}

	Evas_Object *gesture = _gl_gesture_add(data, layout);
	GL_CHECK_VAL(gesture, -1);
	_gl_gesture_set_zoom_in_cb(gesture, __gl_pinch_zoom_in_cb, data);
	_gl_gesture_set_zoom_out_cb(gesture, __gl_pinch_zoom_out_cb, data);
	elm_object_part_content_set(layout, "gesture", gesture);
	int mode = gl_get_view_mode(data);
	if (mode == GL_VIEW_THUMBS_SELECT)
		ad->pinchinfo.gesture_sel = gesture;
	else
		ad->pinchinfo.gesture = gesture;
	return 0;
}

int _gl_pinch_reset_flag(void *data)
{
	GL_CHECK_VAL(data, -1);
	gl_appdata *ad = (gl_appdata *)data;

	gl_dbg("Zoom level: %d", ad->pinchinfo.zoom_level);
	/* Set initialize level */
	if (ad->pinchinfo.zoom_level != GL_ZOOM_DEFAULT &&
	    ad->pinchinfo.zoom_level != GL_ZOOM_IN_ONE &&
	    ad->pinchinfo.zoom_level != GL_ZOOM_IN_TWO) {
		ad->pinchinfo.zoom_level = GL_ZOOM_IN_ONE;
		gl_dbg("Zoom level reset: GL_ZOOM_IN_ONE");
	}
	int mode = gl_get_view_mode(data);
	Evas_Object *gesture = NULL;
	if (mode == GL_VIEW_THUMBS_SELECT)
		gesture = ad->pinchinfo.gesture_sel;
	else
		gesture = ad->pinchinfo.gesture;
	if (gesture)
		_gl_gesture_reset_flags(gesture);
	return 0;
}

