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

#ifndef _GL_EXT_UG_LOAD_H_
#define _GL_EXT_UG_LOAD_H_

#include "gl-data.h"

typedef enum
{
	GL_UG_FAIL,
	GL_UG_SUCCESS
} gl_ug_load_status;

typedef enum
{
	GL_UG_IMAGEVIEWER,
	GL_UG_GALLERY_SETTING_SLIDESHOW,
	GL_UG_GALLERY_SETTING,
	GL_UG_CNT,
} gl_ext_ug_type;

typedef enum
{
	GL_UG_IV,
	/*Invoke imageviewer to slideshow on local album */
	GL_UG_IV_SLIDESHOW,
	/*Invoke imageviewer to slideshow All items */
	GL_UG_IV_SLIDESHOW_ALL,
#ifdef _USE_APP_SLIDESHOW
	/*Invoke imageviewer to slideshow selected files */
	GL_UG_IV_SLIDESHOW_SELECTED,
#endif
	GL_UG_IV_CNT,
} gl_ext_iv_type;

int gl_ext_load_ug(void *data, gl_ext_ug_type type);
int gl_ext_load_iv_ug(void *data, gl_item *item, gl_ext_iv_type type);
int gl_ext_load_iv_ug_select_mode(void *data, gl_media_s *item, gl_ext_iv_type type);
int gl_ext_load_iv_time_ug_select_mode(void *data, gl_media_s *item, gl_ext_iv_type type, int sort_type);
int _gl_ext_load_iv_timeline(void *data, const char *path, int sequence, int sort_type);

#endif /* _GL_EXT_UG_LOAD_H_ */
