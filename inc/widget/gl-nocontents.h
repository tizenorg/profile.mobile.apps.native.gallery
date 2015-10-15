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

#ifndef _GL_NOCONTENTS_H_
#define _GL_NOCONTENTS_H_

#include "gallery.h"

#ifdef __cplusplus
extern "C"
{
#endif

Evas_Object *_gl_nocontents_create(Evas_Object *parent);
bool _gl_nocontents_update_label(Evas_Object *noc, const char *new_label);

#ifdef __cplusplus
}
#endif

#endif // end of _GL_NOCONTENTS_H_
