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
#include <Eet.h>
#include <app.h>
#include <stdlib.h>
#include <dlog.h>
#include <media_content.h>
#include "gl_widget_debug.h"
#include "gl-data-type.h"
#include "gl_widget.h"

#define NUMBER_OF_ITERATION 4
#define IMAGES_THRESHOLD 5
#define SWALLOWS_COUNT 7
#define TIMER_INTERVAL 5
#define WIDGET_HEIGHT 500
#define WIDGET_WIDTH 500
#define EDJE_FILE "edje/gallerywidget.edj"
#define GL_WIDGET_ARGV_IV_VIEW_BY		"View By"
#define GL_WIDGET_ARGV_IV_VIEW_BY_FOLER	"By Folder"
#define GL_WIDGET_ARGV_IV_PATH "Path"
#define GL_WIDGET_UG_PKG_IV			"image-viewer-efl"

#define GL_STR_ID_JAN dgettext("sys_string", "IDS_COM_BODY_JANUARY")
#define GL_STR_ID_FEB dgettext("sys_string", "IDS_COM_BODY_FEBRUARY")
#define GL_STR_ID_MAR dgettext("sys_string", "IDS_COM_BODY_MARCH")
#define GL_STR_ID_APR dgettext("sys_string", "IDS_COM_BODY_APRIL")
#define GL_STR_ID_MAY dgettext("sys_string", "IDS_COM_BODY_MAY")
#define GL_STR_ID_JUN dgettext("sys_string", "IDS_COM_BODY_JUNE")
#define GL_STR_ID_JUL dgettext("sys_string", "IDS_COM_BODY_JULY")
#define GL_STR_ID_AUG dgettext("sys_string", "IDS_COM_BODY_AUGUST")
#define GL_STR_ID_SEP dgettext("sys_string", "IDS_COM_BODY_SEPTEMBER")
#define GL_STR_ID_OCT dgettext("sys_string", "IDS_COM_BODY_OCTOBER")
#define GL_STR_ID_NOV dgettext("sys_string", "IDS_COM_BODY_NOVEMBER")
#define GL_STR_ID_DEC dgettext("sys_string", "IDS_COM_BODY_DECEMBER")

int images_in_loops[] = {3, 2, 1, 4}; //count of images in each iteration of animation
int swallows_in_loops[5][10] = {{1, 1, 0, 0, 0, 1 ,0}, {0, 0, 0, 0, 1, 1, 0}, {0, 0, 0, 0, 0, 0, 1}, {1, 1, 1, 1, 0, 0, 0}}; //arrangement of swallows in each iteration of animation
int loop_count = 0; //counter to the iteration number;

static void gl_widget_on_no_image_cb(void *data,Evas_Object *obj,
	const char *emission, const char *source);
static Eina_Bool gl_widget_timer_cb(void *data);

void gl_widget_win_del_cb(void *data, Evas *evas, Evas_Object *obj,
	void *event_info)
{
	ecore_timer_del(data);
}

void gl_widget_key_down_cb(void *data, Evas *evas, Evas_Object *obj,
	void *event_info)
{
	elm_exit();
}

void gl_widget_app_get_resource(const char *edj_file_in, char *edj_path_out,
	int edj_path_max)
{
	char *res_path = app_get_resource_path();
	if (res_path) {
		snprintf(edj_path_out, edj_path_max, "%s%s", res_path,
			edj_file_in);
		free(res_path);
	}
}

void _gl_launch_iv(void *data, Evas_Object *obj, void *event_info)
{
	if (!obj) {
		dlog_print(DLOG_ERROR, LOG_TAG, "Invalid object!!");
		return;
	}

	_widget_data *widget_data = (_widget_data *)data;
	char *file_name = NULL;
	char *group_name = NULL;
	elm_image_file_get (obj, &file_name, &group_name);
	if (file_name) {
		int ret;
		app_control_h service = NULL;
		if (!widget_data->is_ug_launched) {
			app_control_create(&service);
			if (!service) {
				dlog_print(DLOG_ERROR, LOG_TAG, "fail to create service");
				return;
			}
			app_control_add_extra_data(service, GL_WIDGET_ARGV_IV_VIEW_BY,
					GL_WIDGET_ARGV_IV_VIEW_BY_FOLER);
			app_control_add_extra_data(service, GL_WIDGET_ARGV_IV_PATH, file_name);
			app_control_set_app_id(service, GL_WIDGET_UG_PKG_IV);
			ret = app_control_send_launch_request(service, NULL, NULL);
			if (ret != APP_CONTROL_ERROR_NONE) {
				dlog_print(DLOG_ERROR, LOG_TAG, "image load failed");
				app_control_destroy(service);
			} else {
				widget_data->is_ug_launched = true;
			}
		}
	} else {
		dlog_print(DLOG_ERROR, LOG_TAG, "image path could not be retrieved");
	}
}

int gl_widget_file_exists(const char *path)
{
	struct stat info = {0,};

	if (stat(path, &info) == 0)
		return 1;
	else
		return 0;
}

static Eina_Bool gl_widget_animator_cb(void *data)
{
	dlog_print(DLOG_ERROR, LOG_TAG, "ENTRY gl_widget_animator_cb");
	_widget_data *widget_data = (_widget_data *)data;
	Evas_Object *layout = (Evas_Object *)widget_data->layout;
	char buffer[50] = {0,};
	int i = 0;

	if (!layout) {
		dlog_print(DLOG_ERROR, LOG_TAG, "Invalid layout!!");
		return EINA_FALSE;
	}

	elm_object_signal_emit(layout, "hideall", "elm");

	for (i = 0 ; i < SWALLOWS_COUNT; i++) {
		snprintf(buffer, sizeof(buffer), "image%d", i);
		Evas_Object *content = elm_object_part_content_get(layout, buffer);
		elm_object_part_content_unset(layout, buffer);
		evas_object_hide(content);

		if (swallows_in_loops[loop_count % NUMBER_OF_ITERATION][i] == 1) {
			widget_data->images_count = widget_data->images_count % widget_data->selected_count;
			Evas_Object *image_object = elm_image_add(layout);
			dlog_print(DLOG_ERROR, LOG_TAG, "gl_widget_animator_cb Image is %s",
				widget_data->selected_images[widget_data->images_count]);

			if (!gl_widget_file_exists(widget_data->selected_images[widget_data->images_count])) {
				int index = 0;

				index = widget_data->images_count;
				if (index != widget_data->selected_count-1) {
					strcpy(widget_data->selected_images[index], widget_data->selected_images[widget_data->selected_count-1]);
				} else {
					widget_data->images_count = 0;
				}
				free(widget_data->selected_images[widget_data->selected_count-1]);
				widget_data->selected_count--;
			}

			if (!elm_image_file_set(image_object, widget_data->selected_images[widget_data->images_count], NULL)) {
				dlog_print(DLOG_ERROR, LOG_TAG, "File Set Failed");
			}

			elm_image_aspect_fixed_set(image_object,EINA_FALSE);
			snprintf(buffer, sizeof(buffer), "image%d", i);
			elm_object_part_content_set(layout, buffer, image_object);
			snprintf(buffer, sizeof(buffer), "show%d", i);
			elm_object_signal_emit(layout, buffer, "elm");
			widget_data->images_count++;
			evas_object_smart_callback_add(image_object,
				"clicked", _gl_launch_iv, widget_data);
		}
	}
	return EINA_TRUE;
}

static Eina_Bool gl_widget_animator_for_one(void *data)
{
	_widget_data *widget_data = (_widget_data *)data;
	Evas_Object *layout = (Evas_Object *)widget_data->layout;

	if (!layout) {
		dlog_print(DLOG_ERROR, LOG_TAG, "Invalid layout!!");
		return EINA_FALSE;
	}

	if (!gl_widget_file_exists(widget_data->selected_images[0])){
		if (widget_data->timer) {
			ecore_timer_del(widget_data->timer);
			widget_data->timer = NULL;
		}

		free(widget_data->selected_images[widget_data->selected_count-1]);
		widget_data->selected_count--;
		Evas_Object *btn = elm_object_part_content_unset(
				widget_data->layout,
				"Edit_button");
		if (btn) {
			evas_object_hide(btn);
		}
		elm_object_signal_emit(widget_data->layout, "hideall", "elm");
		elm_object_signal_emit(widget_data->layout, "hide_album_date_info", "elm");

		elm_object_domain_translatable_part_text_set(widget_data->layout, "TitleText", "gallery",
				"IDS_GALLERY_OPT_GALLERY_ABB");
		elm_object_domain_translatable_part_text_set(widget_data->layout, "HelpText", "gallery",
				"IDS_HS_NPBODY_TAP_HERE_TO_ADD_IMAGES");

		elm_object_signal_callback_add(widget_data->layout, "mouse,clicked,1", "bg", gl_widget_on_no_image_cb, widget_data);
	}

	return EINA_TRUE;
}

static Eina_Bool gl_widget_animator_cb_for_less_than_five_images(void *data)
{
	_widget_data *widget_data = (_widget_data *)data;
	Evas_Object *layout = (Evas_Object *)widget_data->layout;

	if (!layout) {
		dlog_print(DLOG_ERROR, LOG_TAG, "Invalid layout!!");
		return EINA_FALSE;
	}

	elm_object_signal_emit(layout, "hideall", "elm");
	Evas_Object *content = elm_object_part_content_get(layout, "image6");
	elm_object_part_content_unset(layout, "image6");
	evas_object_hide(content);

	Evas_Object *image_object = elm_image_add(layout);
	dlog_print(DLOG_ERROR, LOG_TAG, "selected images: %s",
			widget_data->selected_images[widget_data->images_count]);

	if (!gl_widget_file_exists(widget_data->selected_images[widget_data->images_count])) {
		int index = 0;

		index = widget_data->images_count;
		if (index != widget_data->selected_count-1) {
			strcpy(widget_data->selected_images[index], widget_data->selected_images[widget_data->selected_count-1]);
		} else {
			widget_data->images_count = 0;
		}
		free(widget_data->selected_images[widget_data->selected_count-1]);
		widget_data->selected_count--;
	}

	elm_image_file_set(image_object, widget_data->selected_images[widget_data->images_count], NULL);
	elm_image_aspect_fixed_set(image_object, EINA_FALSE);
	elm_object_part_content_set(layout, "image6", image_object);
	elm_object_signal_emit(layout, "show6", "elm");
	widget_data->images_count++;
	widget_data->images_count = widget_data->images_count % widget_data->selected_count;
	evas_object_smart_callback_add(image_object, "clicked",
		_gl_launch_iv, widget_data);

	return EINA_TRUE;
}

static Eina_Bool gl_widget_timer_cb(void *data)
{
	if (!data) {
		dlog_print(DLOG_ERROR, LOG_TAG, "Invalid userdata!!");
		return EINA_FALSE;
	}

	_widget_data *widget_data = (_widget_data *)data;

	dlog_print(DLOG_ERROR, LOG_TAG, "selected_count[%d]", widget_data->selected_count);
	if (widget_data->selected_count >= IMAGES_THRESHOLD) {
		gl_widget_animator_cb(data);
	} else if (widget_data->selected_count >= 2) {
		gl_widget_animator_cb_for_less_than_five_images(data);
	} else {
		gl_widget_animator_for_one(data);
	}

	loop_count++;
	return EINA_TRUE;
}

static Eina_Bool gl_widget_check_albumInfo(char** patharray, int count)
{
	int refcount = 0;
	if (!patharray || count <= 0) {
		dlog_print(DLOG_ERROR, LOG_TAG, "Invalid data!!");
		return EINA_FALSE;
	}

	char *basepath = NULL;
	char *ref = strrchr(patharray[0], '/');
	unsigned int reflength = ref - patharray[0];

	basepath = (char*)malloc(sizeof(char) * (reflength + 1));
	if (!basepath) {
		dlog_print(DLOG_ERROR, LOG_TAG, "allocation failed!!");
		return EINA_FALSE;
	}

	strncpy(basepath, patharray[0], reflength);
	basepath[reflength] = '\0';

	for (refcount = 1; refcount < count; refcount++) {
		if (strstr(patharray[refcount], basepath) == NULL) {
			free(basepath);
			dlog_print(DLOG_ERROR, LOG_TAG, "different selection!!");
			return EINA_FALSE;
		}
	}

	free(basepath);
	return EINA_TRUE;
}

char *_gl_widget_get_file_date(const char *filename)
{
	struct stat statbuf = {0};
	struct tm tmtime;

	if (!filename)
		return NULL;

	if (stat(filename, &statbuf) == -1)
		return NULL;

	time_t temptime = statbuf.st_mtime;
	memset(&tmtime, 0x00, sizeof(struct tm));
	localtime_r(&temptime, &tmtime);

	struct tm ct;
	time_t ctime = 0;
	memset(&ct, 0x00, sizeof(struct tm));
	time(&ctime);
	localtime_r(&ctime, &ct);

	char *month[12] = { GL_STR_ID_JAN, GL_STR_ID_FEB, GL_STR_ID_MAR, GL_STR_ID_APR, GL_STR_ID_MAY, GL_STR_ID_JUN,
			GL_STR_ID_JUL, GL_STR_ID_AUG, GL_STR_ID_SEP, GL_STR_ID_OCT, GL_STR_ID_NOV, GL_STR_ID_DEC};

	char * str = g_strdup_printf("%s%02d, %s %d", "",
			tmtime.tm_mday, month[tmtime.tm_mon], 1900 + tmtime.tm_year);
	return str;
}

static Eina_Bool gl_widget_check_dateInfo(char** patharray, int count)
{
	int refcount = 0;
	if (!patharray || count <= 0) {
		dlog_print(DLOG_ERROR, LOG_TAG, "Invalid data!!");
		return EINA_FALSE;
	}

	char * date = NULL;

	char * next_date = NULL;
	date = _gl_widget_get_file_date(patharray[0]);
	if (!date) {
		dlog_print(DLOG_ERROR, LOG_TAG, "failed to extract date!!");
		return EINA_FALSE;
	}

	for (refcount = 1; refcount < count; refcount++) {
		next_date = _gl_widget_get_file_date(patharray[refcount]);
		if (!next_date) {
			dlog_print(DLOG_ERROR, LOG_TAG, "failed to extract date!!");
			free(date);
			return EINA_FALSE;
		}
		if (strcmp(date, next_date)) {
			free(date);
			free(next_date);
			return EINA_FALSE;
		}
		free(next_date);
	}

	return EINA_TRUE;
}

static char *gl_widget_extract_album_path(char* pathInfo)
{
	if (!pathInfo) {
		dlog_print(DLOG_ERROR, LOG_TAG, "Invalid path!!");
		return NULL;
	}

	char* albumname = NULL;
	char *albumpath = NULL;
	char *rootpath = NULL;
	char *refptr = NULL;
	Eina_Bool useralbum = EINA_FALSE;

	refptr = strrchr(pathInfo, '/');
	if (!refptr) {
		return NULL;
	}

	unsigned int albumpathlength = refptr - pathInfo;
	albumpath = (char *)malloc((albumpathlength + 1) * sizeof(char));
	if (!albumpath) {
		return NULL;
	}

	strncpy(albumpath, pathInfo, albumpathlength);
	albumpath[albumpathlength] = '\0';

	refptr = strrchr(albumpath, '/');
	if (!refptr) {
		free(albumpath);
		return NULL;
	}

	return albumpath;
}

static Eina_Bool gl_widget_check_default_album(char* pathInfo)
{
	if (!pathInfo) {
		dlog_print(DLOG_ERROR, LOG_TAG, "Invalid path!!");
		return EINA_FALSE;
	}
	char *path = gl_widget_extract_album_path(pathInfo);
	if (!path) {
		dlog_print(DLOG_ERROR, LOG_TAG, "Failed to extract path!!");
		return EINA_FALSE;
	}

	if (!strcmp("/opt/usr/media/Pictures/", path) ||
		!strcmp("/opt/usr/media/Images/", path) ||
		!strcmp("/opt/usr/media/DCIM/Camera", path) ||
		!strcmp("/opt/usr/media/Downloads/", path) ||
		!strcmp("/opt/storage/sdcard/DCIM/", path)) {
		free(path);
		return EINA_TRUE;
	}

	free(path);

	return EINA_FALSE;
}

static char *gl_widget_extract_album_info(char* pathInfo)
{
	if (!pathInfo) {
		dlog_print(DLOG_ERROR, LOG_TAG, "Invalid path!!");
		return NULL;
	}

	char* albumname = NULL;
	char *albumpath = NULL;
	char *rootpath = NULL;
	char *refptr = NULL;
	Eina_Bool useralbum = EINA_FALSE;

	refptr = strrchr(pathInfo, '/');
	if (!refptr) {
		return NULL;
	}

	unsigned int albumpathlength = refptr - pathInfo;
	albumpath = (char *)malloc((albumpathlength + 1) * sizeof(char));
	if (!albumpath) {
		return NULL;
	}

	strncpy(albumpath, pathInfo, albumpathlength);
	albumpath[albumpathlength] = '\0';

	refptr = strrchr(albumpath, '/');
	if (!refptr) {
		free(albumpath);
		return NULL;
	}

	unsigned int albumnamelength = strlen(albumpath) - (refptr - albumpath);
	albumname = (char *)malloc((albumnamelength + 1) * sizeof(char));
	if (!albumname) {
		free(albumpath);
		return NULL;
	}

	strncpy(albumname, refptr + 1, albumnamelength);
	albumname[albumnamelength] = '\0';

	free(albumpath);

	return albumname;
}

static char *gl_widget_extract_date_info(char** patharray, int count)
{
	int refcount = 0;
	if (!patharray || count <= 0) {
		dlog_print(DLOG_ERROR, LOG_TAG, "Invalid data!!");
		return NULL;
	}
	char * date = _gl_widget_get_file_date(patharray[0]);
	return date;
}

void _gl_widget_show_album_date_info(int arrayLength, char** pathArray,
		_widget_data* widget_data, Evas_Object* layout) {

	Eina_Bool isSameAlbum = gl_widget_check_albumInfo(pathArray, arrayLength);
	Eina_Bool isDefaultAlbum = false;
	if (isSameAlbum) {
		isDefaultAlbum = gl_widget_check_default_album(pathArray[0]);
	}
	widget_data->showAlbumDetails = isSameAlbum && !isDefaultAlbum;
	widget_data->showDateDetails = gl_widget_check_dateInfo(pathArray,
			arrayLength);
	if (widget_data->showAlbumDetails) {
		char* albuminfo = gl_widget_extract_album_info(pathArray[0]);
		if (albuminfo) {
			dlog_print(DLOG_ERROR, LOG_TAG, "showAlbumDetails!!");
			elm_object_part_text_set(layout, "AlbumInfo", albuminfo);
			elm_object_signal_emit(layout, "showAlbumInfo", "elm");
		}
	} else {
		elm_object_part_text_set(layout, "AlbumInfo", "");
		elm_object_signal_emit(layout, "hideAlbumInfo", "elm");
	}
	if (widget_data->showDateDetails) {
		char* dateinfo = gl_widget_extract_date_info(pathArray, arrayLength);
		if (dateinfo) {
			dlog_print(DLOG_ERROR, LOG_TAG, "showDateDetails!!");
			elm_object_part_text_set(layout, "DateInfo", dateinfo);
			elm_object_signal_emit(layout, "showDateInfo", "elm");
		}
	} else {
		elm_object_part_text_set(layout, "DateInfo", "");
		elm_object_signal_emit(layout, "hideDateInfo", "elm");
	}
}

static void gl_widget_result_cb(app_control_h request, app_control_h reply,
		app_control_result_e result, void *data)
{
	_widget_data *widget_data = (_widget_data *)data;
	if (!widget_data) {
		dlog_print(DLOG_ERROR, LOG_TAG, "Invalid userdata!!");
		return;
	}
	widget_data->is_ug_launched = false;
	Evas_Object *layout = widget_data->layout;
	if (!layout) {
		return;
	}
	char **pathArray = NULL;
	int arrayLength = 0;
	int i = 0;
	int j = 0;
	char buffer[100] = {0,};

	char *dbPath = "/opt/usr/media/selectedimages.eet";
	Eet_File *fp = NULL;

	app_control_get_extra_data_array(reply, APP_CONTROL_DATA_PATH,
		&pathArray, &arrayLength);
	if (arrayLength > 0) {
		dlog_print(DLOG_ERROR, LOG_TAG, "Result Length %d", arrayLength);
		widget_data->selected_images = (char **)malloc(arrayLength * sizeof(char *));
		fp = eet_open(dbPath,EET_FILE_MODE_READ_WRITE);
		if (!fp) {
			dlog_print(DLOG_ERROR, LOG_TAG, "File open failed");
			return;
		}
		snprintf(buffer, sizeof(buffer), "%d", arrayLength);
		eet_write(fp, "imageCnt", buffer, strlen(buffer) + 1, 0);
		if (widget_data->selected_images) {
			for (j = 0 ; j < arrayLength; j++) {
				widget_data->selected_images[j] = strdup(pathArray[j]);
				snprintf(buffer, sizeof(buffer), "image%d", j);
				eet_write(fp, buffer, pathArray[j],
					strlen(pathArray[j]) + 1, 0);
			}
			eet_close(fp);
		} else {
			dlog_print(DLOG_ERROR, LOG_TAG, "Selected Image is NULL");
		}
		if (!widget_data->is_edit) {
			_gl_widget_create_edit_btn(widget_data);
		}
	} else {
		if (!widget_data->is_edit) {
			Evas_Object *btn = elm_object_part_content_unset(
						widget_data->layout,
						"Edit_button");
			if (btn) {
				evas_object_hide(btn);
			}
		}
		return;
	}
	if (widget_data->is_edit) {
		if (widget_data->timer) {
			ecore_timer_del(widget_data->timer);
		}
		widget_data->is_edit = false;
	}

	_gl_widget_show_album_date_info(arrayLength, pathArray, widget_data,
			layout);
	for (i = 0; i < arrayLength; i++) {
		if (pathArray[i]) {
			free (pathArray[i]);
		}
	}
	if (pathArray) {
		free(pathArray);
	}
	widget_data->selected_count = arrayLength;
	if (widget_data->selected_count >= IMAGES_THRESHOLD) {
		gl_widget_animator_cb(widget_data);
	} else {
		gl_widget_animator_cb_for_less_than_five_images(widget_data);
	}

	loop_count++;
	elm_object_signal_callback_del(layout, "mouse,clicked,1",
		"bg", gl_widget_on_no_image_cb);

	if (widget_data->selected_count) {
		widget_data->timer = ecore_timer_loop_add(TIMER_INTERVAL,
			  gl_widget_timer_cb, widget_data);
	}
}

static void gl_widget_on_edit_cb(void *data, Evas_Object *obj, void *event_info)
{
	if (!data) {
		dlog_print(DLOG_ERROR, LOG_TAG, "Invalid userdata!!");
		return;
	}
	_widget_data *widget_data = (_widget_data *)data;
	widget_data->is_edit = true;
	app_control_h service = NULL;
	int ret = -1;
	if (!widget_data->is_ug_launched) {
		ret = app_control_create(&service);
		if (ret != APP_CONTROL_ERROR_NONE) {
			dlog_print(DLOG_ERROR, LOG_TAG, "Failed to create app control!!");
		} else {
			app_control_set_operation(service, APP_CONTROL_OPERATION_PICK);
			app_control_add_extra_data(service,
					"http://tizen.org/appcontrol/data/selection_mode",
					"multiple");
			app_control_set_mime(service, "image/*");
			app_control_add_extra_data(service, "launch-type", "multiple");
			app_control_add_extra_data(service, "file-type", "image");
			app_control_set_app_id(service,"ug-gallery-efl");

			ret = app_control_send_launch_request(service,
					gl_widget_result_cb, (void *)widget_data);
			if (ret != APP_CONTROL_ERROR_NONE) {
				dlog_print(DLOG_ERROR, LOG_TAG,
						"lauching operation pic failed");
				ret = -1;
			} else {
				ret = 0;
				widget_data->is_ug_launched = true;
			}

			app_control_destroy(service);
		}
	}
}

void _gl_widget_create_edit_btn(_widget_data* widget_data) {

	if (widget_data) {
		Evas_Object* boxTop = elm_box_add(widget_data->layout);
		evas_object_size_hint_weight_set(boxTop, EVAS_HINT_EXPAND,
				EVAS_HINT_EXPAND);
		evas_object_size_hint_align_set(boxTop, EVAS_HINT_FILL, EVAS_HINT_FILL);
		elm_box_horizontal_set(boxTop, EINA_TRUE);
		elm_box_homogeneous_set(boxTop, EINA_FALSE);
		Evas_Object* button = elm_button_add(boxTop);
		elm_object_style_set(button, "transparent");
		Evas_Object* layoutButton = elm_layout_add(button);
		evas_object_size_hint_weight_set(layoutButton, EVAS_HINT_EXPAND,
				EVAS_HINT_EXPAND);
		evas_object_size_hint_align_set(layoutButton, EVAS_HINT_FILL,
				EVAS_HINT_FILL);
		elm_layout_file_set(layoutButton,
				"/usr/apps/org.tizen.gallery/res/edje/gallerywidget.edj",
				"today_button");
		elm_object_domain_translatable_part_text_set(layoutButton, "elm.text",
				"gallery", "IDS_QP_ACBUTTON_EDIT_ABB");
		evas_object_show(layoutButton);
		elm_object_content_set(button, layoutButton);
		elm_box_pack_end(boxTop, button);
		evas_object_show(boxTop);
		evas_object_show(button);
		elm_object_part_content_set(widget_data->layout, "Edit_button", boxTop);
		evas_object_smart_callback_add(button, "clicked", gl_widget_on_edit_cb,
				widget_data);
	}
}

static int gl_widget_launch_gallery_ug(_widget_data *widget_data)
{
	app_control_h service = NULL;
	int ret = -1;

	if (!widget_data->is_ug_launched) {
		ret = app_control_create(&service);
		if (ret != APP_CONTROL_ERROR_NONE) {
			dlog_print(DLOG_ERROR, LOG_TAG, "Failed to create app control!!");
		} else {
			app_control_set_operation(service, APP_CONTROL_OPERATION_PICK);
			app_control_add_extra_data(service,
					"http://tizen.org/appcontrol/data/selection_mode",
					"multiple");
			app_control_set_mime(service, "image/*");
			app_control_add_extra_data(service, "launch-type", "multiple");
			app_control_add_extra_data(service, "file-type", "image");
			app_control_set_app_id(service,"ug-gallery-efl");

			ret = app_control_send_launch_request(service,
					gl_widget_result_cb,
					(void *)widget_data);
			if (ret != APP_CONTROL_ERROR_NONE) {
				dlog_print(DLOG_ERROR, LOG_TAG, "lauching operation pic failed");
				ret = -1;
			} else {
				ret = 0;
				widget_data->is_ug_launched = true;
			}

			app_control_destroy(service);
		}
	}
	return ret;
}

static void gl_widget_on_no_image_cb(void *data, Evas_Object *obj,
	const char *emission, const char *source)
{
	_widget_data *widget_data = (_widget_data *)data;
	if (!widget_data) {
		dlog_print(DLOG_ERROR, LOG_TAG, "Invalid userdata!!");
		return;
	}

	Evas_Object *layout = widget_data->layout;
	if (!layout) {
		dlog_print(DLOG_ERROR, LOG_TAG, "Invalid layout!!");
		return;
	}

	gl_widget_launch_gallery_ug(widget_data);
}

Eina_Bool gl_widget_load_preselected_images(_widget_data *widget_data)
{
	if (!widget_data) {
		dlog_print(DLOG_ERROR, LOG_TAG, "Invalid userdata!!");
		return -1;
	}

	char *dbPath = "/opt/usr/media/selectedimages.eet";
	Eet_File *fp = NULL;

	fp = eet_open(dbPath, EET_FILE_MODE_READ);
	if (fp) {
		int size = 0;
		int i = 0;
		int arrayLength = 0;
		char buffer[100] = {0,};
		char *data = NULL;

		data = eet_read(fp, "imageCnt", &size);
		if (data != NULL) {
			snprintf(buffer, sizeof(buffer), "%s", data);
			arrayLength = atoi(buffer);
			widget_data->selected_count = arrayLength;
			dlog_print(DLOG_ERROR, LOG_TAG, "widget_data->selected_count %d - arrayLength %d", widget_data->selected_count, arrayLength);
			free(data);
		}
		widget_data->selected_images = (char **)malloc(arrayLength * sizeof(char *));
		if (!(widget_data->selected_images)) {
			eet_close(fp);
			return EINA_FALSE;
		}

		for (i = 0 ; i < arrayLength; i++) {
			widget_data->selected_images[i] = (char *)malloc(100 * sizeof(char));
			if (widget_data->selected_images[i]) {
				snprintf(buffer, sizeof(buffer), "image%d", i);
				data = eet_read(fp, buffer, &size);
				if (data) {
					strncpy(widget_data->selected_images[i], data, 100);
					free(data);
				}
			}
		}

		eet_close(fp);
		return EINA_TRUE;
	}
	return EINA_FALSE;
}

int gl_widget_create(_widget_data *widget_data, int w, int h)
{
	if (!widget_data) {
		dlog_print(DLOG_ERROR, LOG_TAG, "Invalid userdata!!");
		return -1;
	}
	char edj_path[PATH_MAX] = {0,};
	Evas_Object *layout = NULL;
	dlog_print(DLOG_ERROR, LOG_TAG, "here0 - %x", widget_data->win);
	layout = elm_layout_add(widget_data->win);
	if (!layout) {
		dlog_print(DLOG_ERROR, LOG_TAG, "Invalid layout!!");
		return -1;
	}

	elm_layout_file_set(layout,
		"/usr/apps/org.tizen.gallery/res/edje/gallerywidget.edj",
		"widget_custom_main");
	evas_object_size_hint_weight_set(layout, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	evas_object_resize(layout, WIDGET_WIDTH,WIDGET_HEIGHT);
	evas_object_show(layout);
	widget_data->layout = layout;

	if (!gl_widget_load_preselected_images(widget_data)) {
		elm_object_domain_translatable_part_text_set(layout, "TitleText", "gallery",
				"IDS_GALLERY_OPT_GALLERY_ABB");
		elm_object_domain_translatable_part_text_set(layout, "HelpText", "gallery",
				"IDS_HS_NPBODY_TAP_HERE_TO_ADD_IMAGES");
		elm_object_signal_callback_add(layout, "mouse,clicked,1", "bg", gl_widget_on_no_image_cb, widget_data);
	} else {
		if (widget_data->selected_count >= IMAGES_THRESHOLD) {
			gl_widget_animator_cb(widget_data);
		} else {
			gl_widget_animator_cb_for_less_than_five_images(widget_data);
		}
		loop_count++;
		if (widget_data->selected_count) {
			widget_data->timer = ecore_timer_loop_add(TIMER_INTERVAL, gl_widget_timer_cb, widget_data);
		}
		_gl_widget_create_edit_btn(widget_data);
		_gl_widget_show_album_date_info(widget_data->selected_count, widget_data->selected_images, widget_data,
				layout);
	}

	evas_object_resize(layout, w, h);
	evas_object_event_callback_add(widget_data->win, EVAS_CALLBACK_KEY_DOWN,
		gl_widget_key_down_cb, NULL);

	return 0;
}
/* End of a file */
