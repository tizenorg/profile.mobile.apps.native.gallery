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

#ifndef _GL_PROGRESSBAR_H_
#define _GL_PROGRESSBAR_H_

/* Font style for show label of popup */
#define GL_FONT_STYLE_POP_S "<color=#eec900><font=Tizen:style=Medium text_class=tizen><font_size=30>"
#define GL_FONT_STYLE_POP_E "</font_size></font></color>"

int gl_pb_add_pbar_timer(void *data);
int gl_pb_make_thread_pbar_wheel(void *data, Evas_Object * parent, char *title);
int gl_pb_refresh_thread_pbar(void *data, int cur_cnt, int total_cnt);
int gl_pb_del_pbar(void *data);
Evas_Object *gl_pb_make_pbar(void *data, Evas_Object * parent, char *state);
Evas_Object *_gl_pb_make_place_pbar(Evas_Object *parent);

#endif	/* _GL_PROGRESSBAR_H_ */

