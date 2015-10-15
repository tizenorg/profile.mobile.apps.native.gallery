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

#ifndef _GL_THUMB_EDIT_H_
#define _GL_THUMB_EDIT_H_

#include <Elementary.h>
#include "gallery.h"

int _gl_thumbs_edit_add_btns(void *data, Evas_Object *parent,
			     Elm_Object_Item *nf_it);
int _gl_thumbs_edit_create_view(void *data);
int _gl_thumbs_edit_update_view(void *data);
int _gl_thumbs_edit_pop_view(void *data);
int _gl_thumbs_edit_update_lang(void *data);
int _gl_thumbs_edit_disable_btns(void *data, bool b_disable);
int _gl_thumbs_edit_get_path(void *data, char **files);

#endif

