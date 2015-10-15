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

#ifndef _GL_MIAN_H_
#define _GL_MIAN_H_

#include "gallery.h"

#ifdef __cplusplus
extern "C"
{
#endif

int _gl_main_add_reg_idler(void *data);
int _gl_main_create_view(gl_appdata *ad);
int _gl_main_clear_view(gl_appdata *ad);
Eina_Bool _gl_main_update_view(void *data);
int _gl_main_reset_view(void *data);

#ifdef __cplusplus
}
#endif

#endif // end of _GL_MIAN_H_

