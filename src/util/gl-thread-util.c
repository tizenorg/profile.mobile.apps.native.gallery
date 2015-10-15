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

#include <string.h>
#include <glib.h>
#include <linux/unistd.h>
#include <pthread.h>
#include "gl-thread-util.h"
#include "gl-ui-util.h"
#include "gl-util.h"
#include "gl-debug.h"
#include "gl-data.h"
#include "gl-popup.h"
#include "gl-progressbar.h"
#include "gl-strings.h"
#include "gl-notify.h"
#include "gl-albums.h"
#include "gl-thumbs.h"

#define GALLERY_MAGIC_MAIN_CONTEXT	(0x1983cdaf)
#define GALLERY_MAGIC_DETAIL_LIST_ITEM	(0x1977abcd)
#define GALLERY_MAGIC_PIPE_DATA		(0x0716ffcc)

#define GALLERY_MAGIC                 unsigned int  __magic
#define GALLERY_MAGIC_SET(d, m)       (d)->__magic = (m)
#define GALLERY_MAGIC_CHECK(d, m)     ((d) && ((d)->__magic == (m)))

#define GL_THREAD_PBAR_TIMER_INC 0.5

typedef struct
{
	GALLERY_MAGIC;
	int finished_cnt;
	int state;		/* 0: operation is over; 1: operation is in process */
	int popup_op;
} gl_thread_pipe_data;

#define GL_TID syscall(__NR_gettid)

static int _gl_thread_operate_medias(void *data)
{
	GL_CHECK_VAL(data, -1);
	gl_appdata *ad = (gl_appdata *)data;
	int ret = -1;

	int (*op_cb) (void *data);
	op_cb = ad->pbarinfo.op_cb;
	ad->pbarinfo.op_cb = NULL;
	gl_dbg("Operation: %p", op_cb);
	if (op_cb)
		ret = op_cb(data);
	if (ret < 0) {
		gl_thread_set_cancel_state(ad, GL_PB_CANCEL_ERROR);
		gl_dbgE("Operation failed!");
		return -1;
	}

	return 0;
}

static int _gl_thread_update_view(void *data)
{
	GL_CHECK_VAL(data, -1);
	gl_appdata *ad = (gl_appdata *)data;

	int (*update_cb) (void *data);
	update_cb = ad->pbarinfo.update_cb;
	ad->pbarinfo.update_cb = NULL;
	ad->pbarinfo.del_item_cb = NULL;
	ad->pbarinfo.total_cnt = 0;
	gl_dbg("Update: %p", update_cb);
	if (update_cb)
		update_cb(data);
	return 0;
}

static void *_gl_thread_data_thread(void *data)
{
	GL_CHECK_NULL(data);
	gl_appdata *ad = (gl_appdata *)data;
	int cancel_flag = 0;

	gl_dbg("@@@@@@@@@@ :::: Child thread ID = %d :::: @@@@@@@@@@", GL_TID);

	gl_dbgW("Waiting for start signal ===");
	/* do not need to wait now......to show progressbar immediately
	gl_thread_wait_next_signal(ad);
	*/
	gl_dbgW("Received start signal +++");

	/* Media movement/deleting operation/add tag/remove tag */
	_gl_thread_operate_medias(data);
	/*  send finish signal  */
	gl_thread_pipe_data pipe_data;
	memset(&pipe_data, 0x00, sizeof(gl_thread_pipe_data));
	GALLERY_MAGIC_SET(&pipe_data, GALLERY_MAGIC_PIPE_DATA);
	/* Set over state */
	pipe_data.state = 0;
	gl_thread_get_cancel_state(ad, &cancel_flag);

	ecore_pipe_write(ad->pbarinfo.sync_pipe, &pipe_data, sizeof(gl_thread_pipe_data));

	gl_dbg("@@@@@@@@@@ :::: Child thread done :::: @@@@@@@@@@");

	return (void *)0;
}

static Eina_Bool _gl_thread_del_pbar_idler_cb(void *data)
{
	gl_dbg("Delete progressbar...");
	GL_CHECK_CANCEL(data);
	gl_appdata *ad = (gl_appdata *)data;
	/* Reset pb_cancel */
	gl_thread_set_cancel_state(ad, GL_PB_CANCEL_NONE);
	/* Destroy lock */
	gl_thread_destroy_lock(ad);
	/* Operation done, destroy progressbar */
	gl_pb_del_pbar(ad);

	GL_IF_DEL_TIMER(ad->pbarinfo.del_pbar_timer);
	return ECORE_CALLBACK_CANCEL;
}

static void _gl_thread_pipe_cb(void *data, void *buffer, unsigned int nbyte)
{
	gl_dbg(":::::::::: Main thread ID = %d ::::::::::", GL_TID);
	GL_CHECK(data);
	GL_CHECK(buffer);
	gl_appdata *ad = (gl_appdata *)data;
	gl_thread_pipe_data *p_pipe_data = (gl_thread_pipe_data *) buffer;
	gl_dbg("Pipe state is %d", p_pipe_data->state);

	if (!GALLERY_MAGIC_CHECK(p_pipe_data, GALLERY_MAGIC_PIPE_DATA)) {
		gl_dbgE("##### :: Check p_pipe_data Magic failed :: #####");
		return;
	}

	int cancel_flag = false;
	gl_thread_get_cancel_state(ad, &cancel_flag);

	if (p_pipe_data->popup_op)
	{
		if (p_pipe_data->state)
		{
			/* Check cancel_flag */
			if (cancel_flag != GL_PB_CANCEL_NORMAL) {
				gl_dbgE("Failed to kill thread, try again!");
				gl_thread_emit_next_signal(ad);
				return;
			}
		}
	}

	/* Update progressbar state */
	if (p_pipe_data->state) {
		/*int (*del_item_cb) (void *data, int index);
		del_item_cb = ad->pbarinfo.del_item_cb;
		gl_sdbg("Delete callback: %p", del_item_cb);
		if (del_item_cb)
			del_item_cb(data, p_pipe_data->finished_cnt-1);*/

		/* Check cancel_flag */
		if (cancel_flag != GL_PB_CANCEL_NORMAL) {
			gl_dbgE("Failed to kill thread, try again!");
			gl_thread_emit_next_signal(ad);
			return;
		}

		/* 1. Moving/deleting is in porcess */
		gl_pb_refresh_thread_pbar(ad, p_pipe_data->finished_cnt,
					  ad->pbarinfo.total_cnt);
		gl_dbg("@@@ finished/all = %d/%d, updating progressbar @@@",
		       p_pipe_data->finished_cnt, ad->pbarinfo.total_cnt);
		/* Emit signal to notice child thread handle next media */
		gl_dbg("Emit next signal...");
		gl_thread_emit_next_signal(ad);
	} else {
		/* 2. Moving/deleting is over, show finished count */
		gl_dbg("@@@ finished: %d, updating progressbar @@@",
		       ad->pbarinfo.finished_cnt);
		gl_pb_refresh_thread_pbar(ad, ad->pbarinfo.finished_cnt,
					  ad->pbarinfo.finished_cnt);
		ad->pbarinfo.finished_cnt = 0;
		gl_dbg("@@@@@@@ :::: Pipe close && Update view :::: @@@@@@@");
		/* Delete timer for some error case, that timer called after idler callback */
		GL_IF_DEL_TIMER(ad->pbarinfo.start_thread_timer);

		int cancel_flag = false;
		gl_thread_get_cancel_state(ad, &cancel_flag);
		if (cancel_flag == GL_PB_CANCEL_RESET) {
			/* Set medias_op_type none to stop refreshing view*/
			ad->maininfo.medias_op_type = GL_MEDIA_OP_NONE;
			gl_dbgW("Cancel error case, set reset state!");
		} else if (cancel_flag == GL_PB_CANCEL_ERROR) {
			gl_dbgW("Operation error!");
			_gl_notify_create_notiinfo(GL_STR_OPERATION_FAILED);
		}

		if (cancel_flag != GL_PB_CANCEL_BUTTON) {
			/* Use timer to delete progressbar to refresh status totally */
			GL_IF_DEL_TIMER(ad->pbarinfo.del_pbar_timer);
			Ecore_Timer *timer = NULL;
			timer = ecore_timer_add(0.5f, _gl_thread_del_pbar_idler_cb,
						ad);
			ad->pbarinfo.del_pbar_timer = timer;
		}

		gl_dbg("MMC state: %d, OP type: %d.", ad->maininfo.mmc_state,
		       ad->maininfo.medias_op_type);
		/* Operate medias related to MMC while moving medias */
		if (ad->maininfo.mmc_state == GL_MMC_STATE_REMOVED_MOVING) {
		    ad->maininfo.mmc_state = GL_MMC_STATE_REMOVED;

			/**
			* 1, Places: Add tag;
			* 2, Tags: Add tag/remove tag;
			* 3, Albums: Move/Delete/Add tag.
			*
			* Case 1: Source folder/tag/marker does exist, update view.
			* Case 2: Source folder is MMC, and it vanished.
			*/
			gl_cluster *cur_album = _gl_albums_get_current(data);
			if (cur_album && cur_album->cluster &&
			    cur_album->cluster->type == GL_STORE_T_MMC) {
				gl_dbgW("MMC removed, change to albums view!");
				gl_pop_to_ctrlbar_ly(ad);
			} else {
				_gl_thread_update_view(ad);
			}
		} else {
			/* Operated files on MMC, reset MMC state */
			if (ad->maininfo.mmc_state == GL_MMC_STATE_ADDED_MOVING) {
				ad->maininfo.mmc_state = GL_MMC_STATE_ADDED;
			} else if (ad->maininfo.mmc_state == GL_MMC_STATE_ADDING_MOVING) {
				ad->maininfo.mmc_state = GL_MMC_STATE_ADDED;
			}
			/* Refresh view */
			_gl_thread_update_view(ad);
		}

		/* Free Ecore_Pipe object created */
		GL_IF_DEL_PIPE(ad->pbarinfo.sync_pipe);
		/* Set medias_op_type none to stop refreshing view*/
		ad->maininfo.medias_op_type = GL_MEDIA_OP_NONE;
	}
}

/*******************************************************
** Prototype     	: gl_thread_emit_next_signal
** Description  	: Emit signal to notice child thread handle next media.
** Input        	: void *data
** Output       	: None
** Return Value 	:
** Calls        	:
** Called By    	:
**
**  History        	:
**  1.Date         	: 2011/06/10
**    Author       	: Samsung
**    Modification : Created function
**
*********************************************************/
int gl_thread_emit_next_signal(void *data)
{
	GL_CHECK_VAL(data, -1);
	gl_appdata *ad = (gl_appdata *)data;

	pthread_mutex_lock(&(ad->pbarinfo.refresh_lock));
	gl_dbg("refresh_flag: %d.", ad->pbarinfo.refresh_flag);
	if (ad->pbarinfo.refresh_flag == 0)
	{
		ad->pbarinfo.refresh_flag = 1;
		pthread_cond_signal(&(ad->pbarinfo.refresh_cond));
	}
	pthread_mutex_unlock(&(ad->pbarinfo.refresh_lock));

	return 0;
}

/*******************************************************
** Prototype     	: gl_thread_wait_next_signal
** Description  	: Wait start signal to handle next media.
** Input        	: void *data
** Output       	: None
** Return Value 	:
** Calls        	:
** Called By    	:
**
**  History        	:
**  1.Date         	: 2011/06/10
**    Author       	: Samsung
**    Modification : Created function
**
*********************************************************/
int gl_thread_wait_next_signal(void *data)
{
	GL_CHECK_VAL(data, -1);
	gl_appdata *ad = (gl_appdata *)data;

	pthread_mutex_lock(&(ad->pbarinfo.refresh_lock));
	gl_dbg("refresh_flag: %d.", ad->pbarinfo.refresh_flag);
	while (ad->pbarinfo.refresh_flag == 0)
	{
		gl_dbg("Thread waiting...");
		pthread_cond_wait(&(ad->pbarinfo.refresh_cond), &(ad->pbarinfo.refresh_lock));
	}
	ad->pbarinfo.refresh_flag = 0;
	pthread_mutex_unlock(&(ad->pbarinfo.refresh_lock));

	return 0;
}

/*******************************************************
** Prototype     	: gl_thread_set_cancel_state
** Description  	: Set the value of cancel flag.
** Input        	: void *data
** Input        	: int val
** Output       	: None
** Return Value 	:
** Calls        	:
** Called By    	:
**
**  History        	:
**  1.Date         	: 2011/06/10
**    Author       	: Samsung
**    Modification : Created function
**
*********************************************************/
int
gl_thread_set_cancel_state(void *data, int val)
{
	GL_CHECK_VAL(data, -1);
	gl_appdata *ad = (gl_appdata *)data;

	pthread_mutex_lock(&(ad->pbarinfo.pbar_lock));
	ad->pbarinfo.pbar_cancel = val;
	pthread_mutex_unlock(&(ad->pbarinfo.pbar_lock));
	sync();

	return 0;
}

/*******************************************************
** Prototype     	: gl_thread_get_cancel_state
** Description  	: Get the value of cancel flag.
** Input        	: void *data
** Output       	: int* val
** Return Value 	:
** Calls        	:
** Called By    	:
**
**  History        	:
**  1.Date         	: 2011/06/10
**    Author       	: Samsung
**    Modification : Created function
**
*********************************************************/
int
gl_thread_get_cancel_state(void *data, int *val)
{
	GL_CHECK_VAL(val, -1);
	gl_appdata *ad = (gl_appdata *)data;

	pthread_mutex_lock(&(ad->pbarinfo.pbar_lock));
	*val = ad->pbarinfo.pbar_cancel;
	pthread_mutex_unlock(&(ad->pbarinfo.pbar_lock));

	return 0;
}


/*******************************************************
** Prototype     	: gl_thread_write_pipe
** Description  	: Write date to pipe in order to make progressbar refreshed
** Input        	: void *data
** Input        	: int finished_cnt
** Input        	: int popup_op
** Output       	: None
** Return Value 	:
** Calls        	:
** Called By    	:
**
**  History        	:
**  1.Date         	: 2011/06/10
**    Author       	: Samsung
**    Modification : Created function
**
*********************************************************/
void gl_thread_write_pipe(void *data, int finished_cnt, int popup_op)
{
	GL_CHECK(data);
	gl_appdata *ad = (gl_appdata *)data;
	int cancel_flag = false;
	gl_dbg("Wait next signal...");
	gl_thread_wait_next_signal(ad);
	gl_dbg("Receive next signal...");
	gl_thread_pipe_data pipe_data;
	memset(&pipe_data, 0x00, sizeof(gl_thread_pipe_data));
	GALLERY_MAGIC_SET(&pipe_data, GALLERY_MAGIC_PIPE_DATA);

	pipe_data.state = 1;
	pipe_data.finished_cnt = finished_cnt;
	pipe_data.popup_op = popup_op;

	gl_thread_get_cancel_state(ad, &cancel_flag);

	if (cancel_flag == GL_PB_CANCEL_BUTTON ||
		cancel_flag == GL_PB_CANCEL_MMC ||
		cancel_flag == GL_PB_CANCEL_ERROR ||
		cancel_flag == GL_PB_CANCEL_RESET)
	{
		//send cancel signal through pipe
		pipe_data.finished_cnt = -1;
		/* Set over state */
		pipe_data.state = 0;
		ecore_pipe_write(ad->pbarinfo.sync_pipe, &pipe_data, sizeof(gl_thread_pipe_data));
		//exit the child thread
		if (cancel_flag == GL_PB_CANCEL_BUTTON)
		{
			gl_dbg("Cancel button tapped, child thread exit!");
		}
		else if (cancel_flag == GL_PB_CANCEL_MMC)
		{
			gl_dbg("MMC removed, child thread exit!");
		}
		else if (cancel_flag == GL_PB_CANCEL_ERROR)
		{
			gl_dbg("Error happened, child thread exit!");
		}
		else if (cancel_flag == GL_PB_CANCEL_RESET) {
			gl_dbg("Reset gallery, child thread exit!");
		}

		pthread_exit((void *)1);
	}
	else
	{
		gl_dbg("Writing pipe...");
		ecore_pipe_write(ad->pbarinfo.sync_pipe, &pipe_data, sizeof(gl_thread_pipe_data));
	}
}

/*******************************************************
** Prototype     	: gl_thread_gen_data_thread
** Description  	: Create child thread for moving or deleting medias
** Input        	: void *data
** Output       	: None
** Return Value 	:
** Calls        	:
** Called By    	:
**
**  History        	:
**  1.Date         	: 2011/06/10
**    Author       	: Samsung
**    Modification : Created function
**
*********************************************************/
int
gl_thread_gen_data_thread(void *data)
{
	GL_CHECK_VAL(data, -1);
	gl_appdata *ad = (gl_appdata *)data;
	pthread_t tid;
	pthread_attr_t attr;
	int ret = -1;

	gl_dbg("Creating child thread.");
	//add pipe for update progressbar status
	ad->pbarinfo.sync_pipe = ecore_pipe_add(_gl_thread_pipe_cb, ad);
	//initialize thread attributes
	ret = pthread_attr_init(&attr);
	if (ret == 0)
	{
		ret = pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
		if (ret == 0)
		{
			//create child thread
			ret = pthread_create(&tid, &attr, _gl_thread_data_thread, ad);
			if (ret != 0)
			{
				gl_dbgE("[Error] ##### :: pthread_create failed :: #####");
				pthread_attr_destroy(&attr);
				return -1;
			}
		}
	}

	gl_dbg("\n\n[Done] @@@@@@@@@@ :::: pthread_create successfull :::: @@@@@@@@@@\n");
	pthread_attr_destroy(&attr);

	return 0;
}

/*******************************************************
** Prototype     	: gl_thread_destroy_lock
** Description  	: Destroy mutex lock.
** Input        	: void *data
** Output       	: None
** Return Value 	:
** Calls        	:
** Called By    	:
**
**  History        	:
**  1.Date         	: 2011/06/10
**    Author       	: Samsung
**    Modification : Created function
**
*********************************************************/
int
gl_thread_destroy_lock(void *data)
{
	GL_CHECK_VAL(data, -1);
	gl_appdata *ad = (gl_appdata *)data;
	gl_dbg("@@@@@@@@@@ :::: Destroy MUTEX :::: @@@@@@@@@@");

	/**
	* The variable below was accessed without holding a guarding lock.
	* In a multithreaded environment, this can lead to a race condition.
	* Add lock to prevent from RC.
	*/
	pthread_mutex_lock(&(ad->pbarinfo.refresh_lock));
	ad->pbarinfo.refresh_flag = 0;
	pthread_mutex_unlock(&(ad->pbarinfo.refresh_lock));

	pthread_cond_destroy(&(ad->pbarinfo.refresh_cond));
	pthread_mutex_destroy(&(ad->pbarinfo.pbar_lock));
	pthread_mutex_destroy(&(ad->pbarinfo.refresh_lock));

	return 0;
}

/*******************************************************
** Prototype     	: gl_thread_init_lock
** Description  	: Initialize mutex lock
** Input        	: void *data
** Output       	: None
** Return Value 	:
** Calls        	:
** Called By    	:
**
**  History        	:
**  1.Date         	: 2011/06/10
**    Author       	: Samsung
**    Modification : Created function
**
*********************************************************/
int gl_thread_init_lock(void *data)
{
	GL_CHECK_VAL(data, -1);
	gl_appdata *ad = (gl_appdata *)data;
	gl_dbg("@@@@@@@@@@ :::: Initialize MUTEX :::: @@@@@@@@@@");

	pthread_mutex_init(&(ad->pbarinfo.pbar_lock), NULL);
	pthread_mutex_init(&(ad->pbarinfo.refresh_lock), NULL);
	pthread_cond_init(&(ad->pbarinfo.refresh_cond), NULL);

	/**
	* The variable below was accessed without holding a guarding lock.
	* In a multithreaded environment, this can lead to a race condition.
	* Add lock to prevent from RC.
	*/
	pthread_mutex_lock(&(ad->pbarinfo.refresh_lock));
	ad->pbarinfo.refresh_flag = 0;
	pthread_mutex_unlock(&(ad->pbarinfo.refresh_lock));

	return 0;
}

int _gl_thread_set_op_cb(void *data, void *op_cb)
{
	GL_CHECK_VAL(data, -1);
	gl_appdata *ad = (gl_appdata *)data;
	ad->pbarinfo.op_cb = op_cb;
	return 0;
}

int _gl_thread_set_update_cb(void *data, void *update_cb)
{
	GL_CHECK_VAL(data, -1);
	gl_appdata *ad = (gl_appdata *)data;
	ad->pbarinfo.update_cb = update_cb;
	return 0;
}

int _gl_thread_set_del_item_cb(void *data, void *del_item_cb)
{
	GL_CHECK_VAL(data, -1);
	gl_appdata *ad = (gl_appdata *)data;
	ad->pbarinfo.del_item_cb = del_item_cb;
	return 0;
}

int _gl_thread_set_total_cnt(void *data, int total_cnt)
{
	GL_CHECK_VAL(data, -1);
	gl_appdata *ad = (gl_appdata *)data;
	ad->pbarinfo.total_cnt = total_cnt;
	return 0;
}

