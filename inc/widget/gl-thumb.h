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

#ifndef _GL_THUMB_H_
#define _GL_THUMB_H_

#include "gl-data.h"

#define GL_GENGRID_ITEM_STYLE_THUMBNAIL "gallery/thumbnail_view"
#define GL_GENGRID_ITEM_STYLE_DATE "gallery/date_view"
#define GL_THUMB_ICON "elm.swallow.icon"
#define GL_THUMB_CHECKBOX "elm.swallow.check"
#define GL_THUMB_MODE "elm.swallow.mode"
#define GL_THUMB_YEAR "year"
#define GL_THUMB_MONTH "month"
#define GL_CHECKBOX_STYLE_THUMB "gallery/thumbs"

typedef enum _gl_cm_mode_t {
	GL_CM_MODE_NORMAL		= 0,
	GL_CM_MODE_PANORAMA		= 1,
	GL_CM_MODE_SOUND		= 2,
	GL_CM_MODE_ANIMATE		= 3,
	GL_CM_MODE_BEST			= 4,
	GL_CM_MODE_FB			= 5,
	GL_CM_MODE_VOICE_REC	= 10,
	GL_CM_MODE_BURSTSHOT	= 11, /* burstshot mode is not defined media content db */
	GL_CM_MODE_MAX,
} gl_cm_mode_e;

Evas_Object *_gl_thumb_show_image(Evas_Object *obj, char *path, int orient,
				  int w, int h, int zoom_level);
Evas_Object *_gl_thumb_show_burstshot(Evas_Object *obj, char *path1,
				      char *path2, char *path3,
				      unsigned int orient, int w, int h,
				      int zoom_level);
Evas_Object *_gl_thumb_show_video(Evas_Object *obj, char *path,
				  unsigned int v_dur, int w, int h,
				  int zoom_level);
Evas_Object *_gl_thumb_show_checkbox(Evas_Object *obj, bool checked,
				     Evas_Smart_Cb func, const void *data);
Evas_Object *_gl_thumb_show_mode(Evas_Object *obj, gl_cm_mode_e mode);
Evas_Object *_gl_thumb_add_gengrid(Evas_Object *parent);
int _gl_thumb_update_gengrid(Evas_Object *view);
int _gl_thumb_set_size(void *data, Evas_Object *view, int *size_w, int *size_h);
int _gl_thumb_split_set_size(void *data, Evas_Object *view);
bool _gl_thumb_insert_date(void *data, Evas_Object *parent);

#endif

