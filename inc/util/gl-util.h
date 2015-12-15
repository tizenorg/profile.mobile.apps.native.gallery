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

#ifndef _GL_UTIL_H_
#define _GL_UTIL_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "gl-data.h"
#include "gallery.h"
#include <storage.h>

/*	File system related String definition	*/

#define GL_HOME_ALBUM_COVER_NUM 1

#define GL_FILE_EXISTS(path) \
	(path && (1 == gl_file_exists(path)) && (gl_file_size(path) > 0))
#define GL_ICON_SET_FILE(icon, img) \
	elm_image_file_set(icon, GL_IMAGES_EDJ_FILE, img)

#define GL_BG_SET_FILE(icon, img) \
	elm_bg_file_set(icon, GL_IMAGES_EDJ_FILE, img)

#define GL_IF_DEL_TIMER(timer) \
	do { \
		if(timer != NULL) { \
			ecore_timer_del(timer); \
			timer = NULL; \
		} \
	} while (0)

#define GL_IF_DEL_IDLER(idler) \
	do { \
		if(idler != NULL) { \
			ecore_idler_del(idler); \
			idler = NULL; \
		} \
	} while (0)

#define GL_IF_DEL_JOB(job) \
	do { \
		if(job != NULL) { \
			ecore_job_del(job); \
			job = NULL; \
		} \
	} while (0)

#define GL_IF_DEL_ANIMATOR(animator) \
	do { \
		if(animator != NULL) { \
			ecore_animator_del(animator); \
			animator = NULL; \
		} \
	} while (0)

#define GL_IF_DEL_TRANSIT(transit) \
	do { \
		if(transit != NULL) { \
			elm_transit_del(transit); \
			transit = NULL; \
		} \
	} while (0)

#define GL_IF_DEL_PIPE(pipe) \
	do { \
		if(pipe != NULL) { \
			ecore_pipe_del(pipe); \
			pipe = NULL; \
		} \
	} while (0)

#define GL_IF_DEL_EVENT_HANDLER(handler) \
	do { \
		if(handler != NULL) { \
			ecore_event_handler_del(handler); \
			handler = NULL; \
		} \
	} while (0)

#define GL_IF_DEL_OBJ(obj) \
	do { \
		if(obj != NULL) { \
			evas_object_del(obj); \
			obj = NULL; \
		} \
	} while (0)

#define GL_IF_FREE_ELIST(elist) \
	do { \
		if(elist != NULL) { \
			eina_list_free(elist); \
			elist = NULL; \
		} \
	} while (0)


typedef enum
{
	GL_MMC_STATE_NONE,
	GL_MMC_STATE_ADDED,
	GL_MMC_STATE_ADDED_MOVING,	/* Move medias to MMC or from MMC */
	GL_MMC_STATE_ADDING_MOVING,	/* Insert MMC while operate medias */
	GL_MMC_STATE_REMOVED,
	GL_MMC_STATE_REMOVED_MOVING,	/* Remove MMC while move medias to MMC or from MMC */
} gl_mmc_state_mode;

typedef enum
{
	GL_UPDATE_VIEW_NONE,
	GL_UPDATE_VIEW_NORMAL,
	GL_UPDATE_VIEW_MMC_REMOVED, /* Update view when MMC removed */
	GL_UPDATE_VIEW_MMC_ADDED,   /* Update view when MMC inserted */
	GL_UPDATE_VIEW_INOTIFY,     /* Data changed, update view from inotify request */
} gl_update_view_mode;

typedef enum
{
	GL_MEDIA_OP_NONE,
	GL_MEDIA_OP_MOVE,
	GL_MEDIA_OP_COPY,
	GL_MEDIA_OP_DELETE,
	/*Delete albums*/
	GL_MEDIA_OP_DELETE_ALBUM,
#ifdef _USE_ROTATE_BG
	GL_MEDIA_OP_ROTATING_LEFT,
	GL_MEDIA_OP_ROTATING_RIGHT,
#endif
	GL_MEDIA_OP_MOVE_TIMELINE,
	GL_MEDIA_OP_COPY_TIMELINE,
	GL_MEDIA_OP_DELETE_TIMELINE,
#ifdef _USE_ROTATE_BG
	GL_MEDIA_OP_ROTATING_LEFT_TIMELINE,
	GL_MEDIA_OP_ROTATING_RIGHT_TIMELINE,
#endif
} gl_media_op_mode;

typedef enum
{
	GL_POPUP_OP_NONE,
	GL_POPUP_OP_SAME_ALBUM,
	GL_POPUP_OP_DUPLICATED_NAME,
	GL_POPUP_OP_PROTECTED_FILE,
} gl_popup_op_mode;

typedef enum
{
	GL_SHARE_NONE,
	GL_SHARE_IMAGE_ONE,		/* One image selection */
	GL_SHARE_IMAGE_ONE_JPEG,		/* One jpeg image selection */
	GL_SHARE_IMAGE_MULTI,	/* Multiple images selection */
	GL_SHARE_IMAGE_MULTI_JPEG,	/* Multiple jpeg images selection */
	GL_SHARE_IMAGE_VIDEO,	/* Image(s) and Video(s) selection */
	GL_SHARE_VIDEO_ONE,		/* One video selection */
	GL_SHARE_VIDEO_MULTI,	/* Multiple videos selection */
} gl_share_mode;

typedef enum
{
	GL_INVALID_NONE,
	GL_INVALID_NEW_ENTRY,
	GL_INVALID_NEW_ENTRY_NOC,	/* Show nocontents view */
} gl_invalid_mode;

enum _gl_mc_mode {
	GL_MC_NONE,
	GL_MC_MOVE,
	GL_MC_COPY,
};
typedef enum _gl_mc_mode gl_mc_mode_e;

/* Get images of album */
struct _gl_get_album_images_path_t {
	int index;
	char **files;
};
typedef struct _gl_get_album_images_path_t gl_get_album_images_path_s;

int gl_get_view_mode(void *data);
int gl_set_view_mode(void *data, int mode);
int _gl_use_thread_operate_medias(void *data, char *pbar_title, int all_cnt,
				  int op_type);
int _gl_move_media_thumb(void *data, gl_item *gitem, char *new_dir_name,
			 bool is_full_path, int *popup_op, int mc_mode);
int _gl_move_media_thumb_by_id(void *data, const char *uuid, char *new_dir_name,
			       int *popup_op, int mc_mode);
int _gl_move_media(gl_item *gitem, char *new_dir_name, bool is_full_path);
int gl_move_copy_to_album(void *data);
int gl_move_root_album(void* data, gl_cluster* cur_album, char* dest_path);
int gl_del_medias(void *data);
int _gl_del_media_by_id(void *data, const char *uuid);
int gl_check_mmc_state(void *data, char *dest_folder);
int _gl_del_albums(void *data);
int _gl_refresh_albums_list(void *data, bool b_path, bool b_select);
int _gl_update_albums_data(void *data);
int _gl_update_albums_list(void *data);
void gl_set_mmc_notifd(int fd);
Eina_Bool gl_update_view(void *data, int mode);
int gl_get_selected_files_path_str(void *data, gchar sep_c, char **path_str,
				   int *sel_cnt);
bool gl_make_new_album(const char *name);
bool gl_check_gallery_empty(void* data);
bool gl_is_image_valid(void *data, char *filepath);
#ifdef _USE_ROTATE_BG
int _gl_delay(double sec);
int _gl_rotate_images(void *data, bool b_left);
int _gl_rotate_image_by_id(void *data, const char *uuid, bool b_left);
#endif
int gl_get_entry_text(Evas_Object * entry, char *entry_text, int len_max);
int gl_set_entry_text(Evas_Object *entry, char *entry_text);
int _gl_get_valid_album_name(void *data, char* album_name, bool b_new,
			     bool b_enter);
int _gl_validate_album_name(void *data, char* album_name);
int _gl_validate_input_character(void *data, char* album_name);
char* _gl_get_i18n_album_name(gl_album_s *cluster);
int gl_get_share_mode(void *data);
int gl_del_invalid_widgets(void *data, int invalid_m);
int gl_pop_to_ctrlbar_ly(void *data);
int gl_play_vibration(void *data);
time_t gl_util_get_current_systemtime(void);
char *_gl_get_duration_string(unsigned int v_dur);
gl_icon_type_e _gl_get_icon_type(gl_item *git);
char *_gl_str(char *str_id);
bool _gl_is_str_id(const char *str_id);
double _gl_get_win_factor(Evas_Object *win, int *width, int *height);
int _gl_get_font_size(void);
#ifdef _USE_SVI
int _gl_init_svi(void *data);
int _gl_fini_svi(void *data);
int _gl_play_sound(void *data);
int _gl_play_vibration(void *data);
#endif
int _gl_reg_storage_state_change_notify(void *data);
int _gl_dereg_storage_state_change_notify(void *data);
int _gl_set_file_op_cbs(void *data, void *op_cb, void *del_item_cb,
			void *update_cb, int total_cnt);
int _gl_append_album_images_path(void *data, gl_media_s *item);
int _gl_get_album_images_path(void *data, char **files, bool b_hide);
char *_gl_delete_folder(char *path);
int gl_remove_album(void *data, gl_cluster *album_item, bool is_hide);
int _gl_free_selected_list(void *data);
int _gl_dlopen_imageviewer(void *data);
int _gl_dlclose_imageviewer(void *data);
char *_gl_get_edje_path(void);
char * _gl_get_directory_path(int storage_directory_type);
char * _gl_get_root_directory_path(int storage_type);

#ifdef __cplusplus
}
#endif

#endif /* _GL_UTIL_H_ */

