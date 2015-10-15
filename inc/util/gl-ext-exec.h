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

#ifndef _GL_EXT_EXEC_H_
#define _GL_EXT_EXEC_H_

typedef enum
{
	GL_LAUNCH_FAIL,
	GL_LAUNCH_SUCCESS
} gl_launch_status;

typedef enum
{
	GL_APP_NONE = -1,
	GL_APP_VIDEOPLAYER,
	GL_APP_EMOLINK,		/* connectivity. */
} gl_ext_app_type;

int gl_ext_exec(void *data, gl_ext_app_type type);
#ifdef GL_EXTENDED_FEATURES
int _gl_ext_load_camera(void *data);
int _gl_ext_load_ie(void *data, void *get_path_cb);
int _gl_ext_load(const char *uri, const char *operation, const char *pkg, const char *pkg_id, const char *mime);
int _gl_ext_launch_image_editor(const char *path_string);
int _gl_ext_launch_share(void *data, int total_cnt, void *get_path_cb);
int _gl_ext_is_larger_than_share_max(int total_cnt);
int _gl_ext_launch_share_with_files(void *data, int total_cnt, char **files);
#ifndef _USE_APP_SLIDESHOW
int _gl_ext_launch_gallery_ug(void *data);
#endif
#endif

#endif /* _GL_EXT_EXEC_H_ */

