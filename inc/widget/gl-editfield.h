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

#ifndef _GL_EDITFIELD_H_
#define _GL_EDITFIELD_H_

#include "gallery.h"

#ifdef __cplusplus
extern "C"
{
#endif

Evas_Object *_gl_editfield_create(void *data, Evas_Object *parent,
				  char *default_label);
Evas_Object *_gl_editfield_create_genlist(void *data, Evas_Object *parent,
					   Elm_Object_Item *it, char *label);
Evas_Object *_gl_editfield_create_entry(void *data, Evas_Object *layout, char *text);
int _gl_editfield_hide_imf(void *data);
int _gl_editfield_show_imf(void *data);
int _gl_editfield_destroy_imf(void *data);
int _gl_editfield_change_navi_title(void *data, int r);
Evas_Object *_gl_editfield_get_entry(void *data);
int __gl_editfield_set_entry(void *data, Evas_Object *layout,
				    Evas_Object *entry, char *default_label);

#ifdef __cplusplus
}
#endif

#endif // end of _GL_EDITFIELD_H_

