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

#ifndef GL_WIDGET_H
#define GL_WIDGET_H

typedef struct widget_data {
	Evas_Object *win;
	Evas_Object *layout;
	Evas_Object *bg;
	Evas_Object *conformant;
	char *content;
	Evas_Object *parent;
	Ecore_Timer *timer;
	bool is_edit;
	bool showAlbumDetails;
	bool showDateDetails;
	bool is_ug_launched;
	char **selected_images;
	int selected_count;
	int images_count;
} _widget_data;
#define TIMER_INTERVAL 5

int gl_widget_create(_widget_data *data, int w, int h);
Eina_Bool gl_widget_timer_cb(void *data);

#endif// GL_WIDGET_H
/* End of a file */
