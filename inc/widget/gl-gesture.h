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

#ifndef _GL_GESTURE_H_
#define _GL_GESTURE_H_

#include <Elementary.h>
#include "gallery.h"

#define GL_GESTURE_KEY_PINCH_LEVEL "pinch_level"

typedef enum _gl_zoom_mode {
	GL_ZOOM_NONE,
	GL_ZOOM_DEFAULT,
	GL_ZOOM_IN_ONE,
	GL_ZOOM_IN_TWO,
	GL_ZOOM_MAX,
} gl_zoom_mode;

typedef Eina_Bool (*gl_gesture_cb) (Evas_Object *gesture, void *data);

Evas_Object *_gl_gesture_add(void *data, Evas_Object *parent);
int _gl_gesture_set_zoom_in_cb(Evas_Object *gesture, gl_gesture_cb cb,
			       void *data);
int _gl_gesture_set_zoom_out_cb(Evas_Object *gesture, gl_gesture_cb cb,
				void *data);
int _gl_gesture_reset_flags(Evas_Object *gesture);
Evas_Object *_gl_gesture_get_parent(Evas_Object *gesture);

#endif /* _GL_GESTURE_H_ */

