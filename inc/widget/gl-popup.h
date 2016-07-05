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

#ifndef _GL_POPUP_H_
#define _GL_POPUP_H_

#define GL_POPUP_STRING_MAX 256

typedef enum _gl_popup_mode {
	GL_POPUP_NOBUT,
	GL_POPUP_NOBUT_TITLE,
	GL_POPUP_NOBUT_APPEXIT,
	GL_POPUP_ONEBUT,
	GL_POPUP_ALBUM_NEW_EMPTY,
	GL_POPUP_ALBUM_NEW_DUPLICATE,
	GL_POPUP_ALBUM_NEW_INVALID,
	GL_POPUP_ALBUM_RENAME_EMPTY,
	GL_POPUP_ALBUM_RENAME_DUPLICATE,
	GL_POPUP_ALBUM_RENAME_INVALID,
	GL_POPUP_ALBUM_DELETE,
	GL_POPUP_MEDIA_MOVE,
	GL_POPUP_MEDIA_DELETE,
	GL_POPUP_SLIDESHOW,
	GL_POPUP_LONGPRESSED,
	GL_POPUP_ALBUM_MEMORY_FULL
} gl_popup_mode;

typedef enum _gl_popup_del_m_t {
	GL_POPUP_DEL_FILE,
	GL_POPUP_DEL_ALBUM,
	GL_POPUP_DEL_MAX
} gl_popup_del_m_e;

int _gl_popup_create(void *data, const char *title, const char *text,
		     const char *str1, Evas_Smart_Cb cb1, const void *data1,
		     const char *str2, Evas_Smart_Cb cb2, const void *data2,
		     const char *str3, Evas_Smart_Cb cb3, const void *data3);
int _gl_popup_create_del(void *data, int mode, void *sel_cb, void *del_cb,
			 void *cb_data);
int gl_popup_create_popup(void *data, gl_popup_mode mode, char *desc);
Evas_Object *_gl_popup_create_local(void *data, gl_popup_mode mode, char *desc);
int _gl_popup_create_slideshow(void *data, void *op_func);
int _gl_popup_create_move(void *data, void *op_func);
int _gl_popup_create_copy(void *data, void *op_func);
int _gl_popup_create_move_with_append(void *data, void *op_func, void *append_func, const char *title);
int _gl_popup_create_longpressed_album(void *data, void *op_func, char *album_name);
int _gl_popup_create_longpressed_thumb(void *data, void *op_func, char *file_name);
int _gl_popup_create_longpressed_album_with_append(void *data, void *op_func, void *append_func, const char *title);
int _gl_popup_add_buttons(void *data, const char *text, Evas_Smart_Cb cb_func);
int _gl_popup_add_block_callback(void *data);
void _gl_list_pop_create(void *data, Evas_Object *obj, void *ei, char *title, char *first_text, char *second_text, int state_index);
int _gl_popup_create_copy_move(void *data, void *sel_cb, void *cb_data);
void _gl_update_copy_move_popup(void *data);

#endif /* _GL_POPUP_H_ */

