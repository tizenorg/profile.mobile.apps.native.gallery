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

#ifndef _GL_CTXPOPUP_H_
#define _GL_CTXPOPUP_H_

typedef int (*ctx_append_cb) (void *data, Evas_Object *parent);

Elm_Object_Item *_gl_ctxpopup_append(Evas_Object *obj, char *label_id,
				     Evas_Smart_Cb func, const void *data);
Elm_Object_Item *_gl_ctxpopup_append_with_icon(Evas_Object *obj, char *label,
				     char *icon_path, Evas_Smart_Cb func, const void *data);
int _gl_ctxpopup_create(void *data, Evas_Object *but, ctx_append_cb append_cb);
int _gl_ctxpopup_del(void *data);

#endif

