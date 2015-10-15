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

#ifndef _GL_NOTIFY_H_
#define _GL_NOTIFY_H_

#include "gallery.h"

#ifdef __cplusplus
extern "C"
{
#endif

int _gl_notify_create_notiinfo(const char *text);
int _gl_notify_check_selall(void *data, Elm_Object_Item *nf_it, int all_cnt,
			    int selected_cnt);
int _gl_notify_destroy(void *noti);
int _gl_notify_update_size(void *noti, unsigned long long total);
int _gl_notify_update_progress(void *noti, unsigned long long total,
			       unsigned long long byte);

#ifdef __cplusplus
}
#endif

#endif // end of _GL_NOTIFY_H_
