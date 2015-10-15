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

#include <Elementary.h>
#include <tizen.h>
#include <app.h>
#include <dlog.h>

#include <widget.h>
#include <widget_errno.h>
#include <widget_service.h>
#include <widget_provider_app.h>
#include <widget_app.h>
#include <widget_app_efl.h>

#include <media_content.h>

#include "gl_widget.h"
#include "gl_widget_debug.h"

void *getWidgetData(widget_context_h context)
{
	void *data = NULL;
	widget_app_context_get_tag(context, &data);
	return data;
}

int widget_Create(widget_context_h context, bundle *content, int w, int h, void *data)
{
	Evas_Object *win = NULL;
	int ret = widget_app_get_elm_win(context, &win);
	if (ret != WIDGET_ERROR_NONE) {
		DbgPrint("failed to get window. err = %d", ret);
	}
	elm_config_accel_preference_set("3d");

	struct widget_data *widget_data = NULL;

	widget_data = calloc(1, sizeof(*widget_data));
	if (!widget_data) {
		DbgPrint("failed to create instance");
		return WIDGET_ERROR_OUT_OF_MEMORY;
	}
	bindtextdomain(PACKAGE, "/usr/apps/org.tizen.gallery/res/locale");

	widget_data->win = win;

	evas_object_resize(widget_data->win, w, h);
	evas_object_color_set(widget_data->win, 0, 0, 0, 0);
	evas_object_show(widget_data->win);

	widget_data->is_edit = false;
	widget_data->is_ug_launched = false;
	gl_widget_create(widget_data, w, h);
	DbgPrint("Resized to %dx%d\n", w, h);

	widget_app_context_set_tag(context, widget_data);

	return WIDGET_ERROR_NONE;
}

int widget_Destroy(widget_context_h context, widget_app_destroy_type_e reason, bundle *content, void *data)
{
#if 0
	if (reason != WIDGET_APP_DESTROY_TYPE_PERMANENT) {
		// Save the current status at the bundle object.
	}
#endif
	struct widget_data *widget_data = NULL;

	widget_data = (struct widget_data *)getWidgetData(context);
	if (!widget_data) {
		return WIDGET_ERROR_INVALID_PARAMETER;
	}
	widget_app_context_set_tag(context, NULL);

	evas_object_del(widget_data->win);
	free(widget_data->content);

	if (widget_data->timer) {
		ecore_timer_del(widget_data->timer);
	}
	free(widget_data);

	remove("/opt/usr/media/selectedimages.eet");

	return WIDGET_ERROR_NONE;
}

int widget_Pause(widget_context_h context, void *data)
{
	DbgPrint("widget paused");
	return WIDGET_ERROR_NONE;
}

int widget_Resume(widget_context_h context, void *data)
{
	DbgPrint("widget resumed");
	struct widget_data *widget_data = NULL;
	widget_data = (struct widget_data *)getWidgetData(context);
	if (!widget_data) {
		return WIDGET_ERROR_INVALID_PARAMETER;
	}
	if (widget_data->is_ug_launched) {
		widget_data->is_ug_launched = false;
	}
	return WIDGET_ERROR_NONE;
}

int widget_Resize(widget_context_h context, int w, int h, void *data)
{
	struct widget_data *widget_data = NULL;

	widget_data = (struct widget_data *)getWidgetData(context);
	if (!widget_data) {
		return WIDGET_ERROR_INVALID_PARAMETER;
	}

	evas_object_resize(widget_data->win, w, h);
	evas_object_resize(widget_data->bg, w, h);
	evas_object_resize(widget_data->conformant, w, h);

	DbgPrint("Resized to %dx%d\n", w, h);

	return WIDGET_ERROR_NONE;
}

int widget_Update(widget_context_h context, bundle *content, int force, void *data)
{
	return WIDGET_ERROR_NONE;
}

static void app_language_changed(app_event_info_h event_info, void *data)
{
	DbgPrint("language change triggered");
	char *lang = NULL;
	app_event_get_language(event_info, &lang);
	if (lang) {
		elm_language_set(lang);
		free(lang);
	}
}

static void app_region_changed(app_event_info_h event_info, void *data)
{
	DbgPrint("region change triggered");
}

widget_class_h app_create(void *data)
{
	app_event_handler_h lang_changed_handler;
	app_event_handler_h region_changed_handler;

	widget_app_add_event_handler(&lang_changed_handler, APP_EVENT_LANGUAGE_CHANGED,
		&app_language_changed, data);
	widget_app_add_event_handler(&region_changed_handler, APP_EVENT_REGION_FORMAT_CHANGED,
		&app_region_changed, data);

	widget_instance_lifecycle_callback_s ops;
	ops.create = widget_Create;
	ops.destroy = widget_Destroy;
	ops.pause = widget_Pause;
	ops.resume = widget_Resume;
	ops.resize = widget_Resize;
	ops.update = widget_Update;

	return widget_app_class_create(ops, data);
}

void app_terminate(void *data)
{
	/**
	 * @TODO:
	 */
}

int main(int argc, char *argv[])
{
	int result = WIDGET_ERROR_NONE;

	widget_app_lifecycle_callback_s ops = {0,};
	ops.create = app_create;
	ops.terminate = app_terminate;

	result = widget_app_main(argc, argv, &ops, NULL);
	if (result != WIDGET_ERROR_NONE) {
		DbgPrint("widget_app_main() is failed. err = %d", result);
	}

	return result;
}

/* End of a file */
