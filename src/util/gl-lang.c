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

#include <system_settings.h>
#include "gallery.h"
#include "gl-lang.h"
#include "gl-strings.h"
#include "gl-ctxpopup.h"
#include "gl-debug.h"
#include "gl-ui-util.h"
#include "gl-util.h"
#include "gl-progressbar.h"
#include "gl-controlbar.h"
#include "gl-notify.h"
#include "gl-albums-sel.h"
#include "gl-timeline.h"
#include "gl-albums-edit.h"
#include "gl-albums-rename.h"
#include "gl-thumbs.h"
#include "gl-thumbs-sel.h"
#include "gl-thumbs-edit.h"
#include "gl-nocontents.h"

static int __gl_lang_update_albums(void *data)
{
	GL_CHECK_VAL(data, -1);
	gl_appdata *ad = (gl_appdata *)data;
	GL_CHECK_VAL(ad->ctrlinfo.nf_it, -1);
	_gl_ui_change_navi_title(ad->ctrlinfo.nf_it, GL_STR_ID_ALBUM, true);
	return 0;
}

int _gl_lang_update(void *data)
{
	char *locale = NULL;
	int retcode = system_settings_get_value_string(SYSTEM_SETTINGS_KEY_LOCALE_LANGUAGE, &locale);

	if (retcode != SYSTEM_SETTINGS_ERROR_NONE) {
		gl_dbgW("failed to get language[%d]", retcode);
	}

	if (locale) {
		elm_language_set(locale);
		GL_FREE(locale);
	}
	GL_CHECK_VAL(data, -1);
	gl_appdata *ad = (gl_appdata *)data;
	ad->maininfo.lang_changed = true;
	int view_mode = gl_get_view_mode(ad);

	/* Remove ctxpopup */
	_gl_ctxpopup_del(data);

	switch (view_mode) {
	case GL_VIEW_ALBUMS:
		__gl_lang_update_albums(data);
		break;
	case GL_VIEW_ALBUMS_EDIT:
		_gl_albums_edit_update_lang(data);
		break;
	case GL_VIEW_ALBUMS_RENAME:
		__gl_lang_update_albums(data);
		_gl_albums_edit_update_lang(data);
		break;
	case GL_VIEW_ALBUMS_SELECT:
		__gl_lang_update_albums(data);
		break;
	case GL_VIEW_TIMELINE:
		_gl_timeline_update_lang(data);
		break;
	case GL_VIEW_THUMBS:
		if (_gl_ctrl_get_tab_mode(ad) == GL_CTRL_TAB_ALBUMS) {
			__gl_lang_update_albums(data);
		}
		_gl_thumbs_update_lang(data);
		break;
	case GL_VIEW_THUMBS_EDIT:
		if (_gl_ctrl_get_tab_mode(ad) == GL_CTRL_TAB_ALBUMS) {
			__gl_lang_update_albums(data);
		}
		_gl_thumbs_edit_update_lang(data);
		break;
	case GL_VIEW_THUMBS_SELECT:
		if (_gl_ctrl_get_tab_mode(ad) == GL_CTRL_TAB_ALBUMS) {
			__gl_lang_update_albums(data);
		}
		_gl_thumbs_update_lang(data);
		_gl_thumbs_sel_update_lang(data);
		break;
	default:
		gl_dbgW("Other view_mode[%d]", view_mode);
	}

	return 0;
}

