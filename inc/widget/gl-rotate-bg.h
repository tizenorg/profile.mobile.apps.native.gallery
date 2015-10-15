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

#ifndef _GL_ROTATE_BG_H_
#define _GL_ROTATE_BG_H_

#include "gallery.h"

Evas_Object *_gl_rotate_bg_add(Evas_Object *parent, bool b_preload);
int _gl_rotate_bg_set_data(Evas_Object *bg, void *data);
int _gl_rotate_bg_get_data(Evas_Object *bg, void **data);
int _gl_rotate_bg_set_file(Evas_Object *bg, const char *file, int w, int h);
int _gl_rotate_bg_get_file(Evas_Object *bg, char **file);
int _gl_rotate_bg_rotate_image(Evas_Object *bg, unsigned int orient);
int _gl_rotate_bg_add_image(Evas_Object *bg, int w, int h);
int _gl_rotate_bg_set_image_file(Evas_Object *bg, const char *file);

#endif

#endif
