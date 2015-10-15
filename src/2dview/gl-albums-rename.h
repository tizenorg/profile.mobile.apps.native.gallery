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

#ifndef _GL_ALBUMS_RENAME_H_
#define _GL_ALBUMS_RENAME_H_

#include <Elementary.h>
#include "gallery.h"

int _gl_albums_rename_create_view(void *data, gl_cluster *album);
int _gl_albums_rename_update_view(void *data);
int _gl_albums_rename_update_lang(void *data);

#endif

