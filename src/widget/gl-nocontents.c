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

#include "gl-nocontents.h"
#include "gl-ui-util.h"
#include "gl-util.h"
#include "gl-icons.h"
#include "gl-strings.h"
#include "gl-debug.h"

/**
 * Create Nocontents.
 */
Evas_Object *_gl_nocontents_create(Evas_Object *parent)
{
	GL_CHECK_NULL(parent);
	Evas_Object *noc_lay = NULL;
	/* Full view nocontents */
	noc_lay = elm_layout_add(parent);
	GL_CHECK_NULL(noc_lay);
	elm_layout_theme_set(noc_lay, "layout", "nocontents", "text");
	Evas_Object *icon = elm_image_add(noc_lay);
	GL_ICON_SET_FILE(icon, GL_ICON_NOCONGENTS_VIDEOS);
	elm_object_part_content_set(noc_lay, "nocontents.image", icon);

	_gl_ui_set_translate_part_str(noc_lay, "elm.text", _gl_str(GL_STR_ID_NO_ITEMS));
	_gl_ui_set_translate_part_str(noc_lay, "elm.help.text", _gl_str(GL_STR_ID_NO_ITEMS_SECOND));
	elm_layout_signal_emit(noc_lay, "text,disabled", "");
	elm_layout_signal_emit(noc_lay, "align.center", "elm");
	return noc_lay;
}

/**
 * Update label of Nocontents.
 */
bool _gl_nocontents_update_label(Evas_Object *noc, const char *new_label)
{
	GL_CHECK_VAL(new_label, -1);
	GL_CHECK_VAL(noc, -1);
	const char *label = NULL;

	label = elm_object_part_text_get(noc, GL_NOCONTENTS_TEXT);
	GL_CHECK_VAL(label, -1);
	gl_dbg("Nocontents label: %s", label);
	/* Update label if they're different */
	if (g_strcmp0(label, GL_STR_NO_ALBUMS)) {
		gl_dbgW("Update nocontents label!");
		elm_object_part_text_set(noc, GL_NOCONTENTS_TEXT, new_label);
		return true;
	}
	return false;
}

