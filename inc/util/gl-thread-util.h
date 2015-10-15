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

#ifndef _GL_THREAD_UTIL_H_
#define _GL_THREAD_UTIL_H_

#ifdef __cplusplus
extern "C"
{
#endif

typedef enum
{
	GL_PB_CANCEL_NONE, 		/* Initial state */
	GL_PB_CANCEL_NORMAL, 	/* Moving or deleting is in process */
	GL_PB_CANCEL_BUTTON, 	/* Cancel button tapped while moving or deleting */
	GL_PB_CANCEL_MMC, 		/* MMC removed, src album or dest album vanished while moving or deleting */
	GL_PB_CANCEL_ERROR, 	/* Error happened while moving or deleting */
	GL_PB_CANCEL_RESET,	/* Reset gallery while moving or deleting */
} gl_pb_cancel_mode;

int gl_thread_emit_next_signal(void *data);
int gl_thread_wait_next_signal(void *data);
int gl_thread_set_cancel_state(void *data, int val);
int gl_thread_get_cancel_state(void *data, int *val);
void gl_thread_write_pipe(void *data, int finished_cnt, int popup_op);
int gl_thread_gen_data_thread(void *data);
int gl_thread_destroy_lock(void *data);
int gl_thread_init_lock(void *data);
int _gl_thread_set_op_cb(void *data, void *op_cb);
int _gl_thread_set_update_cb(void *data, void *update_cb);
int _gl_thread_set_del_item_cb(void *data, void *del_item_cb);
int _gl_thread_set_total_cnt(void *data, int total_cnt);

#ifdef __cplusplus
}
#endif

#endif	/* _GL_THREAD_UTIL_H_ */
