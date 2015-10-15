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

#ifndef _GL_DB_UPDATE_H_
#define _GL_DB_UPDATE_H_

int _gl_db_update_add_timer(void *data);
bool _gl_db_update_lock_once(void *data, bool status);
bool _gl_db_update_lock_always(void *data, bool status);
bool _gl_db_update_set_count(void *data, int count);
int _gl_db_update_get_info(void *data,
			   media_content_db_update_item_type_e *update_item,
			   media_content_db_update_type_e *update_type,
			   GList **uuids);
bool _gl_db_update_reg_cb(void *data);
bool _gl_db_update_init(void *data);
bool _gl_db_update_finalize(void *data);

#endif

