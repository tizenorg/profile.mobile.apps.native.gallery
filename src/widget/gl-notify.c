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
#include <time.h>
#include <sys/time.h>
#include <notification.h>
#include <bundle.h>
#include "gl-notify.h"
#include "gl-thumbs.h"
#include "gl-thumbs-sel.h"
#include "gl-pinchzoom.h"
#include "gl-controlbar.h"
#include "gl-thread-util.h"
#include "gl-ui-util.h"
#include "gl-util.h"
#include "gl-strings.h"
#include "gl-debug.h"
#include "gl-ext-exec.h"

typedef struct _gl_noti_t gl_noti_s;
struct _gl_noti_t {
	notification_h handle;
	int priv_id;
};

int _gl_notify_create_notiinfo(const char *text)
{
	GL_CHECK_VAL(text, -1);
	int ret = notification_status_message_post(text);
	if (ret != 0)
		gl_sdbgE("status_message_post()... [0x%x]!", ret);
	return ret;
}

/**
* Check select-all widget state.
*/
int _gl_notify_check_selall(void *data, Elm_Object_Item *nf_it, int all_cnt,
			    int selected_cnt)
{
	GL_CHECK_VAL(data, -1);
	GL_CHECK_VAL(nf_it, -1);
	gl_appdata *ad = (gl_appdata *)data;
	int view_mode = gl_get_view_mode(ad);

	gl_dbg("selected_cnt/all_cnt = %d/%d", selected_cnt, all_cnt);
	if (selected_cnt > all_cnt) {
		gl_dbgE("selected_cnt > all_cnt!");
		return -1;
	} else if (selected_cnt == all_cnt) {
		ad->selinfo.ck_state = EINA_TRUE;
	} else {
		ad->selinfo.ck_state = EINA_FALSE;
	}

	if (ad->selinfo.selectall_ck) {
		elm_check_state_set(ad->selinfo.selectall_ck, ad->selinfo.ck_state);
	}
	if (selected_cnt > 0) {
		/* Enable/Disable control bar buttons */
		_gl_ctrl_enable_btns(ad, nf_it);
		_gl_thumbs_disable_slideshow(ad, false);
	} else if (view_mode == GL_VIEW_THUMBS_SELECT) {
		/* Disable control bar buttons */
		_gl_thumbs_sel_disable_btns(data, true);
	} else {
		/* Disable control bar buttons */
		_gl_ui_disable_menu(nf_it, true);
		_gl_ctrl_disable_items(nf_it, true);
		_gl_thumbs_disable_slideshow(ad, true);
	}

	return 0;
}

int _gl_notify_destroy(void *noti)
{
	GL_CHECK_VAL(noti, -1);
	gl_noti_s *noti_d = noti;

	if (noti_d->handle) {
		int err = 0;
		err = notification_delete(noti_d->handle);
		if (err != NOTIFICATION_ERROR_NONE) {
			gl_dbgE("notification_delete().. [0x%x]", err);
		}

		err = notification_free(noti_d->handle);
		if (err != NOTIFICATION_ERROR_NONE) {
			gl_dbgE("notification_free().. [0x%x]", err);
		}
	}
	noti_d->handle = NULL;
	GL_GFREE(noti_d);
	return 0;
}

int _gl_notify_update_size(void *noti, unsigned long long total)
{
	GL_CHECK_VAL(noti, -1);
	gl_noti_s *noti_d = noti;
	GL_CHECK_VAL(noti_d->handle, -1);
	gl_dbg("%lld", total);

	int err = 0;
	err = notification_set_size(noti_d->handle, (double)total);
	if (err != NOTIFICATION_ERROR_NONE) {
		gl_dbgE("notification_update_size().. [0x%x]", err);
		return -1;
	}
	return 0;
}

int _gl_notify_update_progress(void *noti, unsigned long long total,
			       unsigned long long byte)
{
	GL_CHECK_VAL(noti, -1);
	gl_noti_s *noti_d = noti;
	GL_CHECK_VAL(noti_d->handle, -1);
	gl_dbg("%lld/%lld", byte, total);

	int err = 0;
	double progress = (double)byte / total;
	err = notification_set_progress(noti_d->handle, progress);
	if (err != NOTIFICATION_ERROR_NONE) {
		gl_dbgE("notification_update_progress().. [0x%x]", err);
		return -1;
	}
	return 0;
}

