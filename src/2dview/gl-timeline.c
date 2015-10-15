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

#include <stdlib.h>
#include <time.h>
#include "gl-debug.h"
#include "gl-timeline.h"
#include "gl-controlbar.h"
#include "gl-ui-util.h"
#include "gl-util.h"
#include "gl-data.h"
#include "gl-strings.h"
#ifdef _USE_ROTATE_BG
#include "gl-rotate-bg.h"
#include "gl-exif.h"
#endif
#include "gl-thumb.h"
#include "gl-thumbs.h"
#include "gl-main.h"
#include "gl-albums.h"
#include "gl-albums-sel.h"
#include "gl-nocontents.h"
#include "gl-ext-exec.h"
#include "gl-ext-ug-load.h"
#include "gl-ctxpopup.h"
#include "gl-popup.h"
#include "gl-button.h"
#include "gl-ext-exec.h"
#include "gl-notify.h"
#include "gl-db-update.h"
#include "gl-thread-util.h"
#include "gl-fs.h"
#include "gl-file-util.h"
#include "gl-gesture.h"

#define GL_TIMELINE_TITLE_H 46
#define GL_TIMELINE_PAD_H 8
#define GL_TIMELINE_CONTENT_P_H 956
#define GL_TIMELINE_CONTENT_L_H 429
#define GL_TIMELINE_PAGE_SIZE 450

#define GL_TL_PINCH_OUT_2_CNT 15
#define GL_TL_PINCH_OUT_1_CNT 8
#define GL_TL_PINCH_OUT_CNT_STEP 3

/* Signals to hide/show title swallow */
#define GL_TL_SWALLOW_TITLE_SHOW "elm,swallow_title,state,show"
#define GL_TL_SWALLOW_TITLE_HIDE "elm,swallow_title,state,hide"
/* Signals to transit title during updating if date(time) changed */
#define GL_TL_TITLE2_SHOW_FINISHED "elm,action,title2,show,finished"
#define GL_TL_TITLE_HIDE_FINISHED "elm,action,title,hide,finished"
#define GL_TL_TITLE_FADE_OUT "elm,action,title,fade_out"
#define GL_TL_TITLE2_FADE_IN "elm,action,title2,fade_in"
#define GL_TL_TITLE_HIDE_DEFERRED "elm,state,title,hide,deferred"
#define GL_TL_TITLE2_SHOW_DEFERRED "elm,state,title2,show,deferred"
#define GL_TL_TITLE_SHOW "elm,state,title,show"
#define GL_TL_TITLE2_CREATED "elm,state,title2,created"

#define GL_TL_CONTENTS_FORMAT "contents_%d_%d"
#define GL_TL_CHECKBOX_FORMAT "contents_%d_%d_checkbox"
#define GL_TL_CONTENT_FORMAT "content_%d"

#define GL_TL_TILE_DATA_KEY "gl_tile_data"
#define GL_TL_DATA_KEY "gl_tl_data"

#define GL_GRID_4_PER_ROW 4
#define GL_GRID_7_PER_ROW 7

#define GL_GRID_8_PER_ROW 8
#define GL_GRID_3_PER_ROW 3

#define GL_GRID_6_PER_ROW 6
#define GL_GRID_10_PER_ROW 10

#define GL_GRID_H_ZOOM_03 178
#define GL_GRID_H_ZOOM_02 267

#define GL_TL_DEL_TRANSITS(tiles) \
	do { \
		gl_tile_s *tile = NULL; \
		if (tiles) { \
			gl_dbgW("Delete transits of tiles!"); \
			Eina_List *l = NULL; \
			EINA_LIST_FOREACH(tiles, l, tile) { \
				if (tile) { \
					GL_IF_DEL_TRANSIT(tile->trans); \
					tile = NULL; \
				} \
			} \
		} \
	} while (0)

#define GL_TL_DEL_IDLERS(idlers) \
	do { \
		if (idlers) { \
			gl_tlp_s *it_d = NULL; \
			EINA_LIST_FREE(idlers, it_d) { \
				if (it_d) { \
					GL_IF_DEL_IDLER(it_d->update_idler); \
					it_d = NULL; \
				} \
			} \
			idlers = NULL; \
		} \
	} while (0)

/* Slider unload->free pages ==> Delete contents first before release list */
#define GL_TL_CLEAR_PAGES(timeline_d, b_set_zop) \
	do { \
		GL_TL_DEL_TRANSITS(timeline_d->tiles); \
		GL_TL_DEL_IDLERS(timeline_d->idlers); \
		if (b_set_zop) \
			_gl_slider_set_zop(timeline_d->view, GL_SLIDE_ZOP_DEFAULT); \
		_gl_slider_unload(timeline_d->view); \
		__gl_timeline_free_pages(timeline_d); \
	} while (0)

typedef enum _gl_zoom_mode_t {
	GL_TM_ZOOM_NONE = -3,
	GL_TM_ZOOM_OUT_TWO,
	GL_TM_ZOOM_OUT_ONE,
	GL_TM_ZOOM_DEFAULT,
	GL_TM_ZOOM_MAX,
} gl_zoom_mode_e;

typedef enum _gl_trans_op_t {
	GL_TRANS_OP_DEFAULT, /* Initial state */
	GL_TRANS_OP_DONE, /* Transition is done, do some updating */
	GL_TRANS_OP_START, /* Preparation is one, start to transit tiles */
	GL_TRANS_OP_PREPARE, /* Prepare to add transition */
	GL_TRANS_OP_SHOW, /* Tiles already created, just show them with transition */
	GL_TRANS_OP_MAX,
} gl_trans_op_e;

typedef enum _gl_tl_view_t {
	GL_TL_VIEW_NORMAL,
	GL_TL_VIEW_EDIT,
	GL_TL_VIEW_SHARE,
	GL_TL_VIEW_MAX,
} gl_tl_view_e;

typedef struct _gl_tm_t gl_tm_s;
struct _gl_tm_t {
	int tm_year;
	int tm_mon;
	int tm_mday;
};

typedef struct _gl_sel_data_t gl_sel_s;
struct _gl_sel_data_t {
	Eina_List *sel_list; /* List about image selected */
	int jpge_cnt;
	int image_cnt;
};

typedef enum _gl_time_view_display_order {
	TIME_ORDER_ASC = 0,
	TIME_ORDER_DESC
}_gl_time_view_order;

struct _gl_timeline_t {
	gl_appdata *ad;
	Evas_Object *parent;
	Evas_Object *layout;
	Evas_Object *view;
	Evas_Object *nocontents;
	Evas_Object *date_layout;
	Elm_Object_Item *nf_it;
	int count; /* All images count add in TIMELINE */
	time_t last_mtime;
	int w;
	int h;
	bool delete_data;
	gl_mouse_s mouse;
	Eina_List *count_list; /* List about image count of each item in default zoom level */
	gl_tl_view_e view_m; /* View mode: normal/edit/share */
	gl_zoom_mode_e zoom_level;
	Eina_List *list; /* Page data list */
	/* mtime range for title */
	gl_tm_s tm_s;
	gl_tm_s tm_e;
	bool b_created;
	Ecore_Animator *animator;
	bool b_updating_title; /* Flag: updating title, it's not done if TRUE set */
	gl_cluster *album; /* Create 'All albums' album for share/edit operation */
	int tran_op; /* Add effect */
	Eina_List *tiles; /* All tiles of one page */
	Eina_List *idlers; /* For showing original file */
	Eina_List *thumbs; /* Object list for creating thumbnails */
	Eina_List *data_list;	/* Media list */
	gl_sel_s *sel_d;
	bool is_list_reverse;
	_gl_time_view_order time_media_display_order; /* keep track of the media items display order */
	Elm_Object_Item *realized_item;
};

typedef struct _gl_tlp_t gl_tlp_s;
struct _gl_tlp_t {
	int index;
	int start_pos;
	int end_pos;
	int count;
	int w;
	int h;
	Evas_Object *layout;
	Ecore_Idler *update_idler;
	bool b_created;
	gl_timeline_s *timeline_d;
};

typedef struct _gl_tile_t gl_tile_s;
struct _gl_tile_t {
	Evas_Object *bg;
	Evas_Object *parent;
	Elm_Transit *trans;
	int count;
	int total;
	gl_tlp_s *it_d;
	gl_timeline_s *timeline_d;
};

int _get_count_of_items_on_same_date(gl_media_s *item, Eina_List *list, char **text, int start_index);
int _gl_time_get_number_of_items_per_row(void *data);
static void __gl_timeline_page_deleted_cb(void *data, Evas_Object *obj,
					  void *ei);
static int __gl_timeline_change_mode(void *data, int mode);
static Evas_Object *__gl_timeline_create_list_view(gl_timeline_s *timeline_d, bool update);

#ifdef _USE_APP_SLIDESHOW
static int __gl_timeline_edit(void *data);
#endif

static bool __gl_timeline_is_checked(gl_timeline_s *timeline_d, char *uuid)
{
	GL_CHECK_FALSE(uuid);
	GL_CHECK_FALSE(timeline_d);
	GL_CHECK_FALSE(timeline_d->sel_d);

	Eina_List *l = NULL;
	char *item = NULL;
	EINA_LIST_FOREACH(timeline_d->sel_d->sel_list, l, item) {
		if (item && !g_strcmp0(item, uuid))
			return true;
	}
	return false;
}

#if 0
static bool __gl_timeline_clear_sel_cnt(gl_timeline_s *timeline_d)
{
	GL_CHECK_FALSE(timeline_d);
	if (timeline_d->sel_d == NULL)
		return false;

	timeline_d->sel_d->sel_list = NULL;
	timeline_d->sel_d->jpge_cnt = 0;
	timeline_d->sel_d->image_cnt = 0;
	return true;
}
#endif

static bool __gl_timeline_clear_sel_list(gl_timeline_s *timeline_d)
{
	GL_CHECK_FALSE(timeline_d);
	if (timeline_d->sel_d == NULL)
		return false;

	char *item = NULL;
	EINA_LIST_FREE(timeline_d->sel_d->sel_list, item) {
		GL_GFREEIF(item);
	}
	timeline_d->sel_d->sel_list = NULL;
	return false;
}

static int __gl_timeline_get_sel_cnt(gl_timeline_s *timeline_d)
{
	GL_CHECK_VAL(timeline_d, 0);
	GL_CHECK_VAL(timeline_d->sel_d, 0);
	return eina_list_count(timeline_d->sel_d->sel_list);
}

static bool __gl_timeline_sel_append_item(gl_timeline_s *timeline_d, char *uuid)
{
	GL_CHECK_FALSE(uuid);
	GL_CHECK_FALSE(timeline_d);
	GL_CHECK_FALSE(timeline_d->sel_d);

	Eina_List *l = NULL;
	char *item = NULL;
	EINA_LIST_FOREACH(timeline_d->sel_d->sel_list, l, item) {
		if (item && !g_strcmp0(item, uuid)) {
			gl_dbgW("Appended!");
			return false;
		}
	}
	char *tmp = g_strdup(uuid);
	GL_CHECK_FALSE(tmp);
	timeline_d->sel_d->sel_list = eina_list_append(timeline_d->sel_d->sel_list,
						       (void *)tmp);
	return true;
}

static bool __gl_timeline_check_special_file(gl_timeline_s *timeline_d,
					     gl_media_s *item, bool b_append)
{
	GL_CHECK_FALSE(item);
	GL_CHECK_FALSE(timeline_d);
	GL_CHECK_FALSE(timeline_d->sel_d);

	if (b_append) {
		if (item->type == MEDIA_CONTENT_TYPE_IMAGE) {
			timeline_d->sel_d->image_cnt++;
			if (item->ext != NULL &&
				!strcasecmp(item->ext, GL_JPEG_FILE_EXT) &&
			    _gl_exif_check_img_type(item->file_url))
				timeline_d->sel_d->jpge_cnt++;
		}
	} else {
		if (item->type == MEDIA_CONTENT_TYPE_IMAGE) {
			if (timeline_d->sel_d->image_cnt > 0)
				timeline_d->sel_d->image_cnt--;
			else
				gl_dbgW("Image count is 0!");
			if (item->ext != NULL &&
					!strcasecmp(item->ext, GL_JPEG_FILE_EXT) &&
					_gl_exif_check_img_type(item->file_url)) {
				if (timeline_d->sel_d->jpge_cnt > 0)
					timeline_d->sel_d->jpge_cnt--;
				else
					gl_dbgW("JPG count is 0!");
			}
		}
	}
	return true;
}

int __gl_timeline_check_btns_state(gl_timeline_s *timeline_d, int all_cnt,
				   int sel_cnt)
{
	GL_CHECK_VAL(timeline_d, -1);
	GL_CHECK_VAL(timeline_d->ad, -1);
	GL_CHECK_VAL(timeline_d->sel_d, -1);
	gl_appdata *ad = (gl_appdata *)timeline_d->ad;

	gl_dbg("sel_cnt/all_cnt = %d/%d", sel_cnt, all_cnt);
	if (sel_cnt > all_cnt) {
		gl_dbgE("selected_cnt > all_cnt!");
		return -1;
	} else if (sel_cnt == all_cnt) {
		ad->selinfo.ck_state = EINA_TRUE;
	} else {
		ad->selinfo.ck_state = EINA_FALSE;
	}

	if (ad->selinfo.selectall_ck) {
		elm_check_state_set(ad->selinfo.selectall_ck, ad->selinfo.ck_state);
	}

	/* Enable/Disable control bar buttons */
	if (sel_cnt > 0) {
		gl_dbg("view_m: %d", ad->tlinfo->view_m);
		if (ad->tlinfo->view_m == GL_TL_VIEW_EDIT) {
			/* Disable delete/edit/addtag if facebook files selected */
			/* Disable edit/addtag if cloud files selected */
			/* facebook files only for share */
			_gl_ui_disable_menu(timeline_d->nf_it, false);
			_gl_ctrl_disable_items(timeline_d->nf_it, false);
		} else { /* Share mode */
				_gl_ctrl_disable_items(timeline_d->nf_it, false);
		}
	} else {
		/* Disable control bar buttons */
		_gl_ui_disable_menu(timeline_d->nf_it, true);
		_gl_ctrl_disable_items(timeline_d->nf_it, true);
	}

	return 0;
}

static bool __gl_timeline_sel_remove_item(gl_timeline_s *timeline_d, char *uuid)
{
	GL_CHECK_FALSE(uuid);
	GL_CHECK_FALSE(timeline_d);
	GL_CHECK_FALSE(timeline_d->sel_d);

	Eina_List *l = NULL;
	char *item = NULL;
	EINA_LIST_FOREACH(timeline_d->sel_d->sel_list, l, item) {
		if (item && !g_strcmp0(item, uuid)) {
			timeline_d->sel_d->sel_list = eina_list_remove(timeline_d->sel_d->sel_list,
								       (void *)item);
			GL_GFREE(item);
			return true;
		}
	}
	return false;
}

#if 0
static int __gl_timeline_update_sel_list(gl_timeline_s *timeline_d)
{
	GL_CHECK_VAL(timeline_d, -1);
	GL_CHECK_VAL(timeline_d->sel_d, -1);
	int ret = -1;
	Eina_List *new_l = NULL;
	Eina_List *list = NULL;

	ret = _gl_data_get_items(GL_GET_ALL_RECORDS, GL_GET_ALL_RECORDS, &list);
	if (ret != 0 || !list)
		gl_dbgW("Empty!");

	/* Clear old selected count of sepecial files type */
	__gl_timeline_clear_sel_cnt(timeline_d);

	gl_media_s *item = NULL;
	char *tmp = NULL;
	EINA_LIST_FREE(list, item) {
		if (!item || !item->uuid)
			continue;
		if (__gl_timeline_sel_remove_item(timeline_d, item->uuid)) {
			tmp = g_strdup(item->uuid);
			new_l = eina_list_append(new_l, (void *)tmp);
			__gl_timeline_check_special_file(timeline_d, item, true);
		}
		_gl_data_type_free_glitem((void **)(&item));
		item = NULL;
		tmp = NULL;
	}

	__gl_timeline_clear_sel_list(timeline_d);
	return 0;
}

static int __gl_timeline_check_op(gl_tlp_s *it_d, Evas_Object *obj, int index)
{
	GL_CHECK_VAL(obj, -1);
	GL_CHECK_VAL(it_d, -1);
	GL_CHECK_VAL(it_d->timeline_d, -1);
	GL_CHECK_VAL(it_d->timeline_d->ad, -1);
	gl_dbg("index: %d", index);
	gl_dbg("start_pos-%d, end_pos-%d", it_d->start_pos, it_d->end_pos);

#ifdef _USE_SVI
	/* Play touch sound */
	_gl_play_sound(it_d->timeline_d->ad);
#endif

	Eina_List *list = NULL;
	_gl_data_get_items(it_d->start_pos, it_d->end_pos, &list);
	if (list == NULL) {
		gl_dbgW("Empty list!");
		return -1;
	}

	gl_media_s *item = NULL;
	item = eina_list_nth(list, index);

	Eina_Bool checked = elm_check_state_get(obj);
	gl_dbg("checked: %d", checked);
	gl_sdbg("uuid: %s", item->uuid);

	if (checked) {
		__gl_timeline_check_special_file(it_d->timeline_d, item, true);
		__gl_timeline_sel_append_item(it_d->timeline_d, item->uuid);
	} else {
		__gl_timeline_check_special_file(it_d->timeline_d, item, false);
		__gl_timeline_sel_remove_item(it_d->timeline_d, item->uuid);
	}

	/* Update the label text */
	int sel_cnt = __gl_timeline_get_sel_cnt(it_d->timeline_d);
	_gl_ui_update_navi_title_text(it_d->timeline_d->nf_it, sel_cnt, false);

	__gl_timeline_check_btns_state(it_d->timeline_d, it_d->timeline_d->count,
				       sel_cnt);

	_gl_data_type_free_media_list(&list);
	return 0;
}
#endif

static int __gl_timeline_thumb_check_op(gl_timeline_s *it_d, Evas_Object *obj, gl_media_s *item)
{
	GL_CHECK_VAL(obj, -1);
	GL_CHECK_VAL(it_d, -1);
	GL_CHECK_VAL(it_d->ad, -1);
	GL_CHECK_VAL(item, -1);

#ifdef _USE_SVI
	/* Play touch sound */
	_gl_play_sound(it_d->ad);
#endif

	Eina_Bool checked = elm_check_state_get(obj);
	item->check_state = checked;
	if (checked) {
		__gl_timeline_check_special_file(it_d, item, true);
		__gl_timeline_sel_append_item(it_d, item->uuid);
	} else {
		__gl_timeline_check_special_file(it_d, item, false);
		__gl_timeline_sel_remove_item(it_d, item->uuid);
	}

	/* Update the label text */
	int sel_cnt = __gl_timeline_get_sel_cnt(it_d);
	_gl_ui_update_navi_title_text(it_d->nf_it, sel_cnt, false);

	__gl_timeline_check_btns_state(it_d, it_d->count,
				       sel_cnt);

	return 0;
}

static void __gl_timeline_thumbs_check_changed(void *data, Evas_Object *obj, void *ei)
{
	GL_CHECK(obj);
	GL_CHECK(data);
	gl_media_s *gitem = (gl_media_s *)data;
	gl_timeline_s *td = evas_object_data_get(obj, "data");
	GL_CHECK(td);

	__gl_timeline_thumb_check_op(td, obj, gitem);
}

#if 0
static int __gl_timeline_open_file(gl_tlp_s *it_d, int index)
{
	GL_CHECK_VAL(it_d, -1);
	GL_CHECK_VAL(it_d->timeline_d, -1);
	GL_CHECK_VAL(it_d->timeline_d->ad, -1);
	gl_dbg("index: %d", index);
	gl_dbg("start_pos-%d, end_pos-%d", it_d->start_pos, it_d->end_pos);

#ifdef _USE_SVI
	/* Play touch sound */
	_gl_play_sound(it_d->timeline_d->ad);
#endif

	Eina_List *list = NULL;
	_gl_data_get_items(it_d->start_pos, it_d->end_pos, &list);
	if (list == NULL) {
		gl_dbgW("Empty list!");
		return -1;
	}
	gl_media_s *item = NULL;
	item = eina_list_nth(list, index);

	if (item && item->file_url)
		_gl_ext_load_iv_timeline(it_d->timeline_d->ad, item->file_url,
					 (it_d->start_pos + index + 1));
	else
		gl_dbgW("Image wasn't found!");
	_gl_data_type_free_media_list(&list);
	return 0;
}

static void __gl_timeline_mouse_down(void *data, Evas *e, Evas_Object *obj,
				     void *ei)
{
	GL_CHECK(ei);
	GL_CHECK(data);
	gl_tlp_s *it_d = (gl_tlp_s *)data;
	GL_CHECK(it_d->timeline_d);
	Evas_Event_Mouse_Down *ev = (Evas_Event_Mouse_Down *)ei;

	it_d->timeline_d->mouse.x = ev->output.x;
	it_d->timeline_d->mouse.y = ev->output.y;
}

static void __gl_timeline_mouse_up(void *data, Evas *e, Evas_Object *obj,
				   void *ei)
{
	GL_CHECK(ei);
	GL_CHECK(data);
	gl_tlp_s *it_d = (gl_tlp_s *)data;
	GL_CHECK(it_d->timeline_d);
	GL_CHECK(it_d->timeline_d->ad);

	if (!ei || !obj)
		return;

	Evas_Event_Mouse_Up *ev = (Evas_Event_Mouse_Up *)ei;
	if (ev->event_flags & EVAS_EVENT_FLAG_ON_HOLD ||
	    ev->event_flags & EVAS_EVENT_FLAG_ON_SCROLL)
		return;

	if ((abs(it_d->timeline_d->mouse.x - ev->output.x) > GL_MOUSE_RANGE) ||
	    (abs(it_d->timeline_d->mouse.y - ev->output.y) > GL_MOUSE_RANGE))
		return;

	int saved_i = 0;
#ifdef _USE_ROTATE_BG
	_gl_rotate_bg_get_data(obj, (void **)(&saved_i));
#endif
	if (it_d->timeline_d->view_m != GL_TL_VIEW_NORMAL) {
		char *part = NULL;
		part = g_strdup_printf(GL_TL_CHECKBOX_FORMAT, it_d->count,
				       saved_i + 1);
		Evas_Object *ck = NULL;
		ck = elm_object_part_content_get(it_d->layout, part);
		GL_FREEIF(part);
		elm_check_state_set(ck, !elm_check_state_get(ck));
		__gl_timeline_check_op(it_d, ck, saved_i);
	} else {
		__gl_timeline_open_file(it_d, saved_i);
	}
}

static int __gl_timeline_clear_bg_data(Evas_Object *bg)
{
	GL_CHECK_VAL(bg, -1);
	gl_media_s *item = NULL;
	gl_timeline_s *timeline_d = NULL;

	item = (gl_media_s *)evas_object_data_get(bg, GL_TL_TILE_DATA_KEY);
	if (item) {
		gl_dbg("Free media item!");
		_gl_data_type_free_glitem((void **)(&item));
	}
	evas_object_data_del(bg, GL_TL_TILE_DATA_KEY);

	timeline_d = (gl_timeline_s *)evas_object_data_get(bg, GL_TL_DATA_KEY);
	if (timeline_d && timeline_d->thumbs) {
		gl_dbg("Remove object!");
		timeline_d->thumbs = eina_list_remove(timeline_d->thumbs,
						      (void *)bg);
	}
	return 0;
}

static void __gl_timeline_del_bg_cb(void *data, Evas *e, Evas_Object *obj,
				    void *ei)
{
	gl_dbgW("Delete bg ---");
	__gl_timeline_clear_bg_data(obj);
	/* Remove mouse event if bg deleted */
	evas_object_event_callback_del(obj, EVAS_CALLBACK_MOUSE_DOWN,
				       __gl_timeline_mouse_down);
	evas_object_event_callback_del(obj, EVAS_CALLBACK_MOUSE_UP,
				       __gl_timeline_mouse_up);
	gl_dbgW("Delete bg +++");
}

static Evas_Object *__gl_timeline_add_page_bg(Evas_Object *layout,
					      const char *part,
					      unsigned int orient, char *path,
				    	      int w, int h)
{
	GL_CHECK_NULL(layout);
#ifdef _USE_ROTATE_BG
	Evas_Object *bg = _gl_rotate_bg_add(layout, false);
	GL_CHECK_NULL(bg);

	_gl_rotate_bg_set_file(bg, path, w, h);
	_gl_rotate_bg_rotate_image(bg, orient);

	gl_dbg("part-%s", part);
	elm_object_part_content_set(layout, part, bg);
	return bg;
#else
	return NULL;
#endif
}

static void __gl_timeline_check_changed(void *data, Evas_Object *obj, void *ei)
{
	GL_CHECK(data);
	gl_tlp_s *it_d = (gl_tlp_s *)data;

	int idx = (int)evas_object_data_get(obj, "gl_tl_idx_key");
	__gl_timeline_check_op(it_d, obj, idx);
}

static Evas_Object *__gl_timeline_add_ck(Evas_Object *layout,
					const char *part, bool b_checked,
					gl_tlp_s *it_d)
{
	GL_CHECK_NULL(layout);
	Evas_Object *ck = NULL;
	ck = _gl_thumb_show_checkbox(layout, b_checked,
				     __gl_timeline_check_changed, it_d);
	gl_dbg("part-%s", part);
	elm_object_part_content_set(layout, part, ck);
	return ck;
}

static Evas_Object *__gl_timeline_add_page_ly(Evas_Object *parent,
					      gl_tlp_s *it_d)
{
	GL_CHECK_NULL(it_d);
	GL_CHECK_NULL(parent);
	Evas_Object *ly = NULL;

	char *group = NULL;
	if (it_d->h > it_d->w)
		group = g_strdup_printf("gallery/timeline_%d", it_d->count);
	else
		group = g_strdup_printf("gallery/timeline_%dl", it_d->count);

	ly = gl_ui_load_edj(parent, GL_EDJ_FILE, group);
	GL_GFREE(group);
	GL_CHECK_NULL(ly);

	Evas_Object *bg = evas_object_rectangle_add(evas_object_evas_get(ly));
	if (!bg) {
		evas_object_del(ly);
		return NULL;
	}

	evas_object_color_set(bg, 0, 0, 0, 0);
	elm_object_part_content_set(ly, "bg_c", bg);

	evas_object_show(ly);
	return ly;
}
#endif

static bool __gl_timeline_is_yesterday(struct tm *req_tm, struct tm *now_tm)
{
	if (now_tm->tm_yday == 0) { /* It is the first day of this year */
		if (req_tm->tm_year == now_tm->tm_year - 1 &&
		    req_tm->tm_mon == 11 && req_tm->tm_mday == 31)
			return true;
		else
			return false;
	} else {
		if (req_tm->tm_year == now_tm->tm_year &&
		    req_tm->tm_yday == now_tm->tm_yday - 1)
			return true;
		else
			return false;
	}
}

static bool __gl_timeline_is_today(struct tm *req_tm, struct tm *now_tm)
{
	if (req_tm->tm_year == now_tm->tm_year &&
	    req_tm->tm_yday == now_tm->tm_yday)
		return true;
	else
		return false;
}

/* change 'time_t' to 'struct tm' */
static int __gl_timeline_get_tm(time_t mtime1, time_t mtime2, struct tm *t1,
				struct tm *t2)
{
	GL_CHECK_VAL(t1, -1);
	GL_CHECK_VAL(t2, -1);

	memset(t1, 0x00, sizeof(struct tm));
	localtime_r(&mtime1, t1);
	memset(t2, 0x00, sizeof(struct tm));
	localtime_r(&mtime2, t2);
	return 0;
}

#if 0
/* check mtime is changed or not */
static bool __gl_timeline_is_tm_changed(struct tm t1, struct tm t2,
					gl_tm_s *tms, gl_tm_s *tme)
{
	GL_CHECK_FALSE(tms);
	GL_CHECK_FALSE(tme);

	gl_dbgW("%d/%d/%d-%d/%d/%d =? %d/%d/%d-%d/%d/%d!", t1.tm_year, t1.tm_mon,
		t1.tm_mday, t2.tm_year, t2.tm_mon, t2.tm_mday, tms->tm_year,
		tms->tm_mon, tms->tm_mday, tme->tm_year, tme->tm_mon,
		tme->tm_mday);

	if (tms->tm_year == t1.tm_year && tms->tm_mon == t1.tm_mon &&
	    tms->tm_mday == t1.tm_mday && tme->tm_year == t2.tm_year &&
	    tme->tm_mon == t2.tm_mon &&	tme->tm_mday == t2.tm_mday)
		return false;

	tms->tm_year = t1.tm_year;
	tms->tm_mon = t1.tm_mon;
	tms->tm_mday = t1.tm_mday;

	tme->tm_year = t2.tm_year;
	tme->tm_mon = t2.tm_mon;
	tme->tm_mday = t2.tm_mday;
	return true;
}
#endif

/* Caller need to free strings returned */
static int __gl_timeline_get_mtime_str(struct tm t1, struct tm t2, char **str1,
				       char **str2)
{
	GL_CHECK_VAL(str1, -1);
	GL_CHECK_VAL(str2, -1);
	struct tm ct;
	time_t ctime = 0;
	const char *prefix1 = "";
	const char *prefix2 = "";
	/*char *week_day[7] = { GL_STR_SUN, GL_STR_MON, GL_STR_TUE, GL_STR_WED,
			      GL_STR_THU, GL_STR_FRI, GL_STR_SAT};*/
	char *month[12] = { GL_STR_ID_JAN, GL_STR_ID_FEB, GL_STR_ID_MAR, GL_STR_ID_APR, GL_STR_ID_MAY, GL_STR_ID_JUN,
			GL_STR_ID_JUL, GL_STR_ID_AUG, GL_STR_ID_SEP, GL_STR_ID_OCT, GL_STR_ID_NOV, GL_STR_ID_DEC};

	/* Current time */
	memset(&ct, 0x00, sizeof(struct tm));
	time(&ctime);
	localtime_r(&ctime, &ct);

	if (t1.tm_year == t2.tm_year && t1.tm_mon == t2.tm_mon &&
	    t1.tm_mday == t2.tm_mday) {
		/* Same day */
		if (__gl_timeline_is_today(&t1, &ct)) {
			/* Today */
			*str1 = g_strdup_printf("%s%s", prefix1,
						GL_STR_TODAY);
			*str2 = g_strdup_printf("%s%s", prefix2,
								  "");
			return 0;
		}
	} else if (t1.tm_year == t2.tm_year && t1.tm_mon == t2.tm_mon) {
		/* Same month */
		if (__gl_timeline_is_yesterday(&t1, &ct)) {
			/* Yesterday */
			*str1 = g_strdup_printf("%s%s", prefix1,
					GL_STR_YESTERDAY);
			*str2 = g_strdup_printf("%s%s", prefix2,
					"");
			return 0;
		}
		*str2 = g_strdup_printf("%s%d.%02d.%02d ~ %02d.%02d", prefix2,
				GL_DEFAULT_YEAR + t2.tm_year,
				t2.tm_mon + 1, t2.tm_mday,
				t1.tm_mon + 1, t1.tm_mday);
		*str1 = g_strdup_printf("%s%02d %s", prefix2,
				t1.tm_mday, month[t1.tm_mon]);
	} else if (t1.tm_year == t2.tm_year) {
		/* Same year */
		*str2 = g_strdup_printf("%s%d.%02d.%02d ~ %02d.%02d", prefix2,
					GL_DEFAULT_YEAR + t2.tm_year,
					t2.tm_mon + 1, t2.tm_mday,
					t1.tm_mon + 1, t1.tm_mday);
		*str1 = g_strdup_printf("%s %s", prefix2, month[t1.tm_mon]);
	} else {
		*str2 = g_strdup_printf("%s%d.%02d.%02d ~ %d.%02d.%02d",
					prefix2, GL_DEFAULT_YEAR + t2.tm_year,
					t2.tm_mon + 1, t2.tm_mday,
					GL_DEFAULT_YEAR + t1.tm_year,
					t1.tm_mon + 1, t1.tm_mday);
		*str1 = g_strdup_printf("%s%d ", prefix2, GL_DEFAULT_YEAR + t1.tm_year);
	}
	return 0;
}

#if 0
static Evas_Object *__gl_timeline_add_title(Evas_Object *parent, char *text1,
					    char *text2)
{
	gl_dbg("");
	GL_CHECK_NULL(parent);
	Evas_Object *bx = NULL;
	Evas_Object *data1 = NULL;
	Evas_Object *data2 = NULL;
	double scale = elm_config_scale_get();

	if (text2 == NULL)
		goto GL_UI_FAILED;

	bx = elm_box_add(parent);
	GL_CHECK_NULL(bx);
	elm_box_horizontal_set(bx, EINA_TRUE);
	elm_box_homogeneous_set(bx, EINA_FALSE);
	evas_object_size_hint_weight_set(bx, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	evas_object_size_hint_align_set(bx, 0.0, 0.5);
	elm_box_align_set(bx, 0.0, 0.5);

	Evas *evas = evas_object_evas_get(parent);
	if (evas == NULL)
		goto GL_UI_FAILED;
	/* Pad - 11 */
	Evas_Object *rect1 = evas_object_rectangle_add(evas);
	if (rect1 == NULL)
		goto GL_UI_FAILED;
	Evas_Coord pad_w = (Evas_Coord)(11 * scale);
	Evas_Coord pad_h = (Evas_Coord)(GL_TIMELINE_TITLE_H * scale);
	evas_object_resize(rect1, pad_w, pad_h);
	evas_object_size_hint_min_set(rect1, pad_w, pad_h);
	evas_object_size_hint_max_set(rect1, pad_w, pad_h);
	elm_box_pack_end(bx, rect1);

	/* data 1 */
	if (text1) {
		data1 = elm_label_add(parent);
		if (data1 == NULL)
			goto GL_UI_FAILED;
		evas_object_size_hint_weight_set(data1, 0.0, EVAS_HINT_EXPAND);
		evas_object_size_hint_align_set(data1, 0.0, 0.5);
		elm_box_pack_end(bx, data1);
		evas_object_show(data1);
		/* Set text */
		gl_dbg("%s", text1);
		elm_object_text_set(data1, text1);
		GL_GFREE(text1);

		/* Pad - 10 */
		Evas_Object *rect2 = evas_object_rectangle_add(evas);
		if (rect2 == NULL)
			goto GL_UI_FAILED;
		Evas_Coord pad2_w = (Evas_Coord)(10 * scale);
		evas_object_resize(rect2, pad2_w, pad_h);
		evas_object_size_hint_min_set(rect2, pad2_w, pad_h);
		evas_object_size_hint_max_set(rect2, pad2_w, pad_h);
		elm_box_pack_end(bx, rect2);
	}

	/* data 2 */
	data2 = elm_label_add(parent);
	if (data2 == NULL)
		goto GL_UI_FAILED;
	evas_object_size_hint_weight_set(data2, 0.0, EVAS_HINT_EXPAND);
	evas_object_size_hint_align_set(data2, 0.0, 0.5);
	elm_box_pack_end(bx, data2);
	evas_object_show(data2);
	/* Set text */
	gl_dbg("%s", text2);
	elm_object_text_set(data2, text2);
	GL_GFREE(text2);

	/* Pad - 11 */
	Evas_Object *rect3 = evas_object_rectangle_add(evas);
	if (rect3 == NULL)
		goto GL_UI_FAILED;
	evas_object_resize(rect3, pad_w, pad_h);
	evas_object_size_hint_min_set(rect3, pad_w, pad_h);
	evas_object_size_hint_max_set(rect3, pad_w, pad_h);
	elm_box_pack_end(bx, rect3);

	evas_object_show(bx);
	return bx;

 GL_UI_FAILED:

	gl_dbgE("Failed to add title!");
	GL_IF_DEL_OBJ(bx);
	return NULL;
}
#endif

/* Reset title for previous animation if another animation is triggerred */
static int __gl_timeline_reset_title(gl_timeline_s *timeline_d)
{
	GL_CHECK_VAL(timeline_d, -1);

	Evas_Object *title = elm_object_part_content_unset(timeline_d->layout,
							   "title2");
	GL_CHECK_VAL(title, -1);
	gl_dbg("Set new title: %p", title);
	elm_object_part_content_set(timeline_d->layout, "title", title);

	edje_object_signal_emit(_EDJ(timeline_d->layout), GL_TL_TITLE_SHOW,
				"elm");
	edje_object_message_signal_process(_EDJ(timeline_d->layout));
	return 0;
}

/* "elm,state,title,hide"
 */
static void __gl_timeline_on_title_hide_finished(void *data, Evas_Object *obj,
						 const char *emission,
						 const char *source)
{
	GL_CHECK(data);
	gl_timeline_s *timeline_d = (gl_timeline_s *)data;

	if (!timeline_d->b_updating_title) {
		gl_dbgW("Updating done!");
		return;
	}

	Evas_Object *title = elm_object_part_content_unset(timeline_d->layout,
							   "title");
	gl_dbg("Delete old title: %p", title);
	GL_IF_DEL_OBJ(title);
}

/* "elm,state,title2,show"
 */
static void __gl_timeline_on_title2_show_finished(void *data, Evas_Object *obj,
						  const char *emission,
						  const char *source)
{
	GL_CHECK(data);
	gl_timeline_s *timeline_d = (gl_timeline_s *)data;

	if (!timeline_d->b_updating_title) {
		gl_dbgW("Updating done!");
		return;
	}

	gl_dbg("Update title");
	__gl_timeline_reset_title(timeline_d);
}

static int __gl_timeline_add_title_trans_finished_cbs(gl_timeline_s *timeline_d,
						      bool b_add)
{
	GL_CHECK_VAL(timeline_d, -1);
	gl_dbg("b_add: %d", b_add);

	if (b_add) {
		/* Add callback for title updating */
		edje_object_signal_callback_add(_EDJ(timeline_d->layout),
						GL_TL_TITLE2_SHOW_FINISHED, "",
						__gl_timeline_on_title2_show_finished,
						(void *)timeline_d);
		edje_object_signal_callback_add(_EDJ(timeline_d->layout),
						GL_TL_TITLE_HIDE_FINISHED, "",
						__gl_timeline_on_title_hide_finished,
						(void *)timeline_d);
	} else {
		/* Add callback for title updating */
		edje_object_signal_callback_del(_EDJ(timeline_d->layout),
						GL_TL_TITLE2_SHOW_FINISHED, "",
						__gl_timeline_on_title2_show_finished);
		edje_object_signal_callback_del(_EDJ(timeline_d->layout),
						GL_TL_TITLE_HIDE_FINISHED, "",
						__gl_timeline_on_title_hide_finished);
	}
	return 0;
}

#if 0
static Eina_Bool __gl_timeline_title_trans_cb(void *data)
{
	GL_CHECK_CANCEL(data);
	gl_timeline_s *timeline_d = (gl_timeline_s *)data;
	gl_dbg("");

	if (!timeline_d->b_updating_title) {
		gl_dbgW("Updating done!");
		return ECORE_CALLBACK_CANCEL;
	}

	timeline_d->animator = NULL;

	edje_object_signal_emit(_EDJ(timeline_d->layout),
				GL_TL_TITLE_FADE_OUT, "elm");
	//edje_object_message_signal_process(_EDJ(timeline_d->layout));
	edje_object_signal_emit(_EDJ(timeline_d->layout),
				GL_TL_TITLE2_FADE_IN, "elm");
	edje_object_message_signal_process(_EDJ(timeline_d->layout));
	edje_object_signal_emit(_EDJ(timeline_d->layout),
				GL_TL_TITLE_HIDE_DEFERRED, "elm");
	edje_object_message_signal_process(_EDJ(timeline_d->layout));
	edje_object_signal_emit(_EDJ(timeline_d->layout),
				GL_TL_TITLE2_SHOW_DEFERRED, "elm");
	edje_object_message_signal_process(_EDJ(timeline_d->layout));

	gl_dbg("done");
	return ECORE_CALLBACK_CANCEL;
}

static int __gl_timeline_start_title_trans(gl_timeline_s *timeline_d)
{
	GL_CHECK_VAL(timeline_d, -1);
	edje_object_signal_emit(_EDJ(timeline_d->layout), GL_TL_TITLE_SHOW,
				"elm");
	edje_object_signal_emit(_EDJ(timeline_d->layout), GL_TL_TITLE2_CREATED,
				"elm");
	edje_object_message_signal_process(_EDJ(timeline_d->layout));
	timeline_d->animator = ecore_animator_add(__gl_timeline_title_trans_cb,
						  timeline_d);
	return 0;
}

static int __gl_timeline_update_title(gl_timeline_s *timeline_d, gl_tlp_s *it_d,
				      bool b_forced)
{
	GL_CHECK_VAL(it_d, -1);
	GL_CHECK_VAL(timeline_d, -1);
	gl_dbg("index-%d, count-%d", it_d->index, it_d->count);
	gl_dbg("start_pos-%d, end_pos-%d", it_d->start_pos, it_d->end_pos);

	Eina_List *list = NULL;
	_gl_data_get_items(it_d->start_pos, it_d->end_pos, &list);
	if (list == NULL) {
		gl_dbgW("Empty list!");
		return -1;
	}

	Evas_Object *title = NULL;
	gl_media_s *item = NULL;
	int i = 0;
	time_t mtime1 = 0;
	time_t mtime2 = 0;
	int count = eina_list_count(list);

	for (i = 0; i < count; i++) {
		item = eina_list_nth(list, i);
		if (item == NULL) {
			gl_dbgE("Invalid item!");
			continue;
		}

		if (i == 0)
			mtime1 = item->mtime;
		if (i == count - 1)
			mtime2 = item->mtime;

		item = NULL;
	}
	_gl_data_type_free_media_list(&list);

	struct tm t1;
	struct tm t2;
	__gl_timeline_get_tm(mtime1, mtime2, &t1, &t2);
	/* Time changed? */
	if (!b_forced &&
	    !__gl_timeline_is_tm_changed(t1, t2, &(timeline_d->tm_s), &(timeline_d->tm_e))) {
		if (!elm_object_part_content_get(timeline_d->layout, "title")) {
			gl_dbgW("Title is empty, recreate it!");
		} else {
			gl_dbgW("Same mtime, change nothing!");
			return 0;
		}
	}
	/* Get title range -- string */
	char *text1 = NULL;
	char *text2 = NULL;
	__gl_timeline_get_mtime_str(t1, t2, &text1, &text2);
	/* Add title object */
	title = __gl_timeline_add_title(timeline_d->layout, text1, text2);
	GL_CHECK_VAL(title, -1);
	if (!b_forced && /* Dont transit if it's forced to update title */
	    elm_object_part_content_get(timeline_d->layout, "title")) {
		/* Play Animation for title updating */
		gl_dbg("Animate title");
		if (timeline_d->b_updating_title) {
			gl_dbgW("Cancel previous animation!");
			GL_IF_DEL_ANIMATOR(timeline_d->animator);
			/* Delete callback first to skip useless operation */
			__gl_timeline_add_title_trans_finished_cbs(timeline_d,
								   false);
			__gl_timeline_reset_title(timeline_d);
			/* Add callback again */
			__gl_timeline_add_title_trans_finished_cbs(timeline_d,
								   true);
		} else {
			timeline_d->b_updating_title = true;
		}
		elm_object_part_content_set(timeline_d->layout, "title2", title);
		__gl_timeline_start_title_trans(timeline_d);
	} else {
		gl_dbg("Set title");
		elm_object_part_content_set(timeline_d->layout, "title", title);
	}
	return 0;
}

static int __gl_timeline_get_tile_size(int count, int idx, bool b_land, int *w,
				       int *h)
{
	GL_CHECK_VAL(w, -1);
	GL_CHECK_VAL(h, -1);

	if (count == 1) {
		if (b_land) {
			*w = GL_WIN_HEIGHT;
			*h = GL_TIMELINE_CONTENT_L_H;
		} else {
			*w = GL_WIN_WIDTH;
			*h = GL_TIMELINE_CONTENT_P_H;
		}
	} else if (count == 2 || (count == 3 && idx == 0)) {
		if (!b_land) {
			*w = GL_WIN_WIDTH * 2 / 3;
			*h = GL_TIMELINE_CONTENT_P_H / 2;
		} else if (idx == 0) {
			*w = GL_WIN_HEIGHT * 2 / 3;
			*h = GL_TIMELINE_CONTENT_L_H;
		}
	}
	return 0;
}
#endif

/* Free data after page deleted */
static void __gl_timeline_del_page_cb(void *data, Evas *e, Evas_Object *obj,
					void *ei)
{
	gl_dbgW("Delete page ---");
	GL_CHECK(data);
	evas_object_smart_callback_del(obj, "layout,page,deleted",
				       __gl_timeline_page_deleted_cb);
	gl_tlp_s *it_d = (gl_tlp_s *)data;
	GL_IF_DEL_IDLER(it_d->update_idler);
	it_d->b_created = false;
	gl_dbgW("Delete page +++");
}

static void __gl_timeline_page_deleted_cb(void *data, Evas_Object *obj,
						 void *ei)
{
	gl_dbgW("Deleted page ------");
	evas_object_event_callback_del(obj, EVAS_CALLBACK_DEL,
				       __gl_timeline_del_page_cb);
	GL_CHECK(data);
	gl_tlp_s *it_d = (gl_tlp_s *)data;
	GL_IF_DEL_IDLER(it_d->update_idler);
	it_d->b_created = false;
	gl_dbgW("Deleted page ++++++");
}

#if 0
static int __gl_timeline_update_page(gl_tlp_s *it_d)
{
	GL_CHECK_VAL(it_d, -1);
	GL_CHECK_VAL(it_d->layout, -1);
	gl_dbg("index-%d, count-%d", it_d->index, it_d->count);
	gl_dbg("start_pos-%d, end_pos-%d", it_d->start_pos, it_d->end_pos);

	Eina_List *list = NULL;
	_gl_data_get_items(it_d->start_pos, it_d->end_pos, &list);
	if (list == NULL) {
		gl_dbgW("Empty list!");
		return -1;
	}

	bool b_landscape = false;
	if (it_d->w > it_d->h)
		b_landscape = true;

	Evas_Object *layout = it_d->layout;
	Evas_Object *bg = NULL;
	gl_media_s *item = NULL;
	int i = 0;
	char *part = NULL;
	int count = eina_list_count(list);
	char *path = NULL;
	unsigned int orient = 0;
	int w = GL_TIMELINE_PAGE_SIZE;
	int h = GL_TIMELINE_PAGE_SIZE;

	for (i = 0; i < count; i++) {
		item = eina_list_nth(list, i);
		if (item == NULL) {
			gl_dbgE("Invalid item!");
			continue;
		}

/* Checkme: Scrolling doesn't work smoothly if original file was used */
		/* Get orientation for original file */
		if (((i == 0 && count <= 5) ||
		     (i == 1 && count <= 3 && !b_landscape) ||
		     (i == 2 && count == 4 && !b_landscape) ||
		     (i == 1 && (count == 2 || count == 4) && b_landscape)) &&
		    item->type == MEDIA_CONTENT_TYPE_IMAGE &&
		    GL_FILE_EXISTS(item->file_url)) {
			path = item->file_url;
#ifdef _USE_ROTATE_BG
			if (_gl_exif_get_orientation(path, &orient) < 0) {
				path = item->thumb_url;
				return -1;
			}
#endif
			__gl_timeline_get_tile_size(count, i, b_landscape, &w,
						    &h);
			part = g_strdup_printf(GL_TL_CONTENTS_FORMAT,
					       it_d->count, i + 1);
			bg = elm_object_part_content_get(layout, part);
			if (bg) {
				_gl_rotate_bg_set_file(bg, path, w, h);
				_gl_rotate_bg_rotate_image(bg, orient);
			} else {
				gl_dbgE("BG is invalid!");
			}
			GL_GFREEIF(part);
		}
		part = NULL;
		item = NULL;
	}
	_gl_data_type_free_media_list(&list);
	gl_dbg("done");
	return 0;
}

static void __gl_timeline_page_transited_cb(void *data, Evas_Object *obj,
						 void *ei)
{
	gl_dbgW("Transited page ------");
	evas_object_event_callback_del(obj, EVAS_CALLBACK_DEL,
				       __gl_timeline_del_page_cb);
	GL_CHECK(data);
	gl_tlp_s *it_d = (gl_tlp_s *)data;
	GL_IF_DEL_IDLER(it_d->update_idler);
	_gl_slider_set_zop(it_d->timeline_d->view, GL_SLIDE_ZOP_DEFAULT);
	__gl_timeline_update_page((gl_tlp_s *)data);
	gl_dbgW("evas_object_freeze_events_set EINA_FALSE");
	evas_object_freeze_events_set(it_d->timeline_d->view, EINA_FALSE);
	gl_dbgW("Transited page ++++++");
}

static Eina_Bool __gl_timeline_page_update_idler_cb(void *data)
{
	gl_dbgW("start");
	GL_CHECK_CANCEL(data);
	gl_tlp_s *it_d = (gl_tlp_s *)data;
	GL_IF_DEL_IDLER(it_d->update_idler);
	__gl_timeline_update_page((gl_tlp_s *)data);
	gl_dbgW("done");
	return ECORE_CALLBACK_CANCEL;
}

static int __gl_timeline_check_page_idler(gl_tlp_s *it_d)
{
	GL_CHECK_VAL(it_d, -1);
	GL_CHECK_VAL(it_d->timeline_d, -1);

	gl_slide_zop_e zoom = _gl_slider_get_zop(it_d->timeline_d->view);
	gl_dbg("zoom-%d, tran_op-%d", zoom, it_d->timeline_d->tran_op);
	if (it_d->timeline_d->tran_op == GL_TRANS_OP_PREPARE ||
	    it_d->timeline_d->tran_op == GL_TRANS_OP_DONE) {
		gl_dbg("Update checked by transit callback");
	} else if (it_d->timeline_d->tran_op == GL_TRANS_OP_START) {
		gl_dbg("Delay idler callback");
		it_d->timeline_d->idlers = eina_list_append(it_d->timeline_d->idlers,
							    it_d);
	} else if (zoom == GL_SLIDE_ZOP_OUT || zoom == GL_SLIDE_ZOP_IN) {
		gl_dbg("Updated by transit callback");
	} else {
		GL_IF_DEL_IDLER(it_d->update_idler);
		gl_dbg("Add idler callback");
		it_d->update_idler = ecore_idler_add(__gl_timeline_page_update_idler_cb,
						     it_d);
	}
	return 0;
}

static int __gl_timeline_add_page_cbs(gl_tlp_s *it_d, Evas_Object *layout)
{
	GL_CHECK_VAL(layout, -1);
	GL_CHECK_VAL(it_d, -1);

	/* Register delete callback */
	evas_object_event_callback_add(layout, EVAS_CALLBACK_DEL,
				       __gl_timeline_del_page_cb, it_d);
	evas_object_smart_callback_add(layout, "layout,page,deleted",
				       __gl_timeline_page_deleted_cb, it_d);
	/* Register transit callback */
	evas_object_smart_callback_add(layout, "layout,page,transited",
				       __gl_timeline_page_transited_cb, it_d);
	return 0;
}

static void __gl_timeline_transit_op(Elm_Transit_Effect *data,
				     Elm_Transit *transit, double progress)
{
	gl_dbg("");
}

static void __gl_timeline_transit_done(Elm_Transit_Effect *data, Elm_Transit *transit)
{
	GL_CHECK(data);
	gl_tile_s *tile = (gl_tile_s *)data;
	gl_dbgW("%d/%d", tile->count, tile->total);
	if (tile->count != tile->total)
		return;

	gl_dbgW("tran_op: %d", tile->timeline_d->tran_op);
	if (tile->timeline_d->tran_op == GL_TRANS_OP_START) {
		tile->timeline_d->tran_op = GL_TRANS_OP_DONE;
		_gl_slider_update_first_item(tile->timeline_d->view);
	}
	tile->timeline_d->tran_op = GL_TRANS_OP_DEFAULT;
	gl_dbg("idlers: %p", tile->timeline_d->idlers);
	if (tile->timeline_d->idlers) {
		gl_tlp_s *it_d = NULL;
		EINA_LIST_FREE(tile->timeline_d->idlers, it_d) {
			gl_dbg("it_d: %p", it_d);
			it_d->update_idler = ecore_idler_add(__gl_timeline_page_update_idler_cb,
							     it_d);
			it_d = NULL;
		}
		tile->timeline_d->idlers = NULL;
	}
}

static void __gl_timeline_transit_del_cb(void *data, Elm_Transit *transit)
{
	gl_dbg("");
	GL_CHECK(data);
	gl_tile_s *tile = (gl_tile_s *)data;
	if (tile->count == tile->total) {
		gl_dbgW("evas_object_freeze_events_set EINA_FALSE");
		evas_object_freeze_events_set(tile->timeline_d->view, EINA_FALSE);
		GL_IF_FREE_ELIST(tile->timeline_d->tiles);
	}
	/* Free data */
	GL_FREEIF(data);
}

static int __gl_timeline_tile_transit(gl_tile_s *tile)
{
#define GL_TL_TRANS_TIME 0.7
	gl_dbg("");
	GL_CHECK_VAL(tile, -1);

        tile->trans = elm_transit_add();
        elm_transit_object_add(tile->trans, tile->bg);

        elm_transit_tween_mode_set(tile->trans,
				   ELM_TRANSIT_TWEEN_MODE_DECELERATE);

	elm_transit_effect_color_add(tile->trans, 0, 0, 0, 0, 255, 255, 255,
				     255);
	elm_transit_effect_zoom_add(tile->trans, 0.4, 1.0);
	elm_transit_effect_add(tile->trans,__gl_timeline_transit_op,
			       (Elm_Transit_Effect *)tile,
			       __gl_timeline_transit_done);
	elm_transit_del_cb_set(tile->trans, __gl_timeline_transit_del_cb, tile);

	elm_transit_duration_set(tile->trans, GL_TL_TRANS_TIME);
	elm_transit_objects_final_state_keep_set(tile->trans, EINA_FALSE);
	elm_transit_go(tile->trans);
	return 0;
}

static int __gl_timeline_transit_tiles(gl_timeline_s *timeline_d)
{
	gl_dbgW("start!");
	GL_CHECK_VAL(timeline_d, -1);
	GL_CHECK_VAL(timeline_d->tiles, -1);
	gl_tile_s *tile = NULL;
	Eina_List *l = NULL;

	gl_dbgW("evas_object_freeze_events_set EINA_TRUE");
	evas_object_freeze_events_set(timeline_d->view, EINA_TRUE);

	/* Remove previous transits first */
	EINA_LIST_FOREACH(timeline_d->tiles, l, tile) {
		if (tile) {
			GL_IF_DEL_TRANSIT(tile->trans);
			tile = NULL;
		}
	}
	/* Transit each tile */
	EINA_LIST_FOREACH(timeline_d->tiles, l, tile) {
		__gl_timeline_tile_transit(tile);
		tile = NULL;
	}
	gl_dbgW("done!");
	return 0;
}

static int __gl_timeline_add_tile(gl_tlp_s *it_d, Evas_Object *parent,
				  Evas_Object *bg, int count, int total)
{
	GL_CHECK_VAL(it_d, -1);

	gl_tile_s *tile = (gl_tile_s *)calloc(1, sizeof(gl_tile_s));
	GL_CHECK_VAL(tile, -1);
	tile->bg = bg;
	tile->parent = parent;
	tile->it_d = it_d;
	tile->count = count;
	tile->total = total;
	tile->timeline_d = it_d->timeline_d;
	it_d->timeline_d->tiles = eina_list_append(it_d->timeline_d->tiles,
						   tile);
	return 0;
}

static Evas_Object *__gl_timeline_get_page_tiles(gl_tlp_s *it_d)
{
	GL_CHECK_NULL(it_d);
	GL_CHECK_NULL(it_d->timeline_d);
	gl_dbg("index-%d, count-%d", it_d->index, it_d->count);
	gl_dbg("start_pos-%d, end_pos-%d", it_d->start_pos, it_d->end_pos);

	int i = 0;
	int count = it_d->count;
	char *part = NULL;
	Evas_Object *bg = NULL;

	for (i = 0; i < count; i++) {
		/* Add images */
		part = g_strdup_printf(GL_TL_CONTENTS_FORMAT, it_d->count,
				       i + 1);

		bg= elm_object_part_content_get(it_d->layout, part);
		__gl_timeline_add_tile(it_d, NULL, bg, i + 1, count);

		part = NULL;
	}
	return 0;
}

/* Transit tiles without updating images or layout */
static int __gl_timeline_show_tiles_trans(gl_timeline_s *timeline_d)
{
	GL_CHECK_VAL(timeline_d, -1);
	/* Get first image of current page */
	void *sitem = _gl_slider_get_sitem(timeline_d->view);
	GL_CHECK_VAL(sitem, -1);
	gl_tlp_s *it_d = (gl_tlp_s *)_gl_slider_get_sitem_data(sitem);
	GL_CHECK_VAL(it_d, -1);
	/* Show transition */
	if (it_d->timeline_d->tran_op == GL_TRANS_OP_PREPARE) {
		__gl_timeline_get_page_tiles(it_d);
		it_d->timeline_d->tran_op = GL_TRANS_OP_SHOW;
		__gl_timeline_transit_tiles(it_d->timeline_d);
	}
	return 0;
}

/* Only for local medias */
static void __gl_timeline_create_thumb_cb(media_content_error_e error,
					const char *path, void *user_data)
{
	GL_CHECK(user_data);
	Evas_Object *bg = (Evas_Object *)user_data;

	if (error == MEDIA_CONTENT_ERROR_NONE && GL_FILE_EXISTS(path) &&
	    g_strcmp0(path, GL_ICON_DB_DEFAULT_THUMB)) {
		gl_sdbgW("path[%s]!", path);
		_gl_rotate_bg_set_file(bg, path, GL_TIMELINE_PAGE_SIZE,
				       GL_TIMELINE_PAGE_SIZE);
		_gl_rotate_bg_rotate_image(bg, GL_ORIENTATION_ROT_0);
	} else {
		gl_sdbgE("[%d]Invalid path[%s]!", error, path);
	}

	__gl_timeline_clear_bg_data(bg);
}

/* Add thumbs creation */
static int __gl_timeline_create_thumbs(gl_timeline_s *timeline_d)
{
	GL_CHECK_VAL(timeline_d, -1);
	Eina_List *l = NULL;
	Evas_Object *bg = NULL;
	gl_media_s *item = NULL;
	int ret = -1;

	EINA_LIST_FOREACH(timeline_d->thumbs, l, bg) {
		if (bg == NULL)
			continue;
		item = (gl_media_s *)evas_object_data_get(bg,
							  GL_TL_TILE_DATA_KEY);
		if (item && !GL_FILE_EXISTS(item->thumb_url) &&
		    GL_FILE_EXISTS(item->file_url)) {
			gl_dbgW("Add thumb creation!");
			ret = _gl_data_create_thumb(item,
						    __gl_timeline_create_thumb_cb,
						    bg);
			if (ret == 0)
				continue;
			else
				goto GL_TL_FAILED;
		} else {
 GL_TL_FAILED:

			evas_object_data_del(bg, GL_TL_TILE_DATA_KEY);
			timeline_d->thumbs = eina_list_remove(timeline_d->thumbs,
							      bg);
			if (item)
				_gl_data_type_free_glitem((void **)(&item));
		}
	}
	return 0;
}
#endif

/* Cancel thumb creation*/
static int __gl_timeline_cancel_thumbs(gl_timeline_s *timeline_d)
{
	GL_CHECK_VAL(timeline_d, -1);
	Eina_List *l = NULL;
	Evas_Object *bg = NULL;
	gl_media_s *item = NULL;

	EINA_LIST_FOREACH(timeline_d->thumbs, l, bg) {
		if (bg == NULL)
			continue;
		item = (gl_media_s *)evas_object_data_get(bg,
							  GL_TL_TILE_DATA_KEY);
		if (item && item->b_create_thumb) {
			gl_dbgW("Cancel thumb creation!");
			_gl_data_cancel_thumb(item);
		}
	}
	return 0;
}

#if 0
static int __gl_timeline_check_thumb(gl_media_s *item, Evas_Object *bg)
{
	GL_CHECK_VAL(bg, -1);
	GL_CHECK_VAL(item, -1);

	if (item->storage_type != GL_STORE_T_PHONE &&
	    item->storage_type != GL_STORE_T_MMC)
		return -1;

	if (GL_FILE_EXISTS(item->file_url)) {
		int ret = -1;
		ret = _gl_data_create_thumb(item, __gl_timeline_create_thumb_cb,
					    (void *)bg);
		return ret;
	}
	return -1;
}

static int __gl_timeline_check_glitem(gl_timeline_s *timeline_d,
				      gl_media_s *item, Evas_Object *bg,
				      bool b_check_thumb)
{
	/* Check thumb */
	if (b_check_thumb) {
		gl_dbg("Check thumb");
		if (__gl_timeline_check_thumb(item, bg) == 0) {
			evas_object_data_set(bg, GL_TL_TILE_DATA_KEY,
					     (void *)item);
			evas_object_data_set(bg, GL_TL_DATA_KEY,
					     (void *)timeline_d);
			timeline_d->thumbs = eina_list_append(timeline_d->thumbs,
							      (void *)bg);
		} else {
			_gl_data_type_free_glitem((void **)(&item));
		}
	} else {
		_gl_data_type_free_glitem((void **)(&item));
	}
	return 0;
}

static Evas_Object *__gl_timeline_add_page(Evas_Object *parent, gl_tlp_s *it_d)
{
	GL_CHECK_NULL(it_d);
	GL_CHECK_NULL(it_d->timeline_d);
	GL_CHECK_NULL(parent);
	gl_dbg("index-%d, count-%d", it_d->index, it_d->count);
	gl_dbg("start_pos-%d, end_pos-%d", it_d->start_pos, it_d->end_pos);

	Eina_List *list = NULL;
	_gl_data_get_items(it_d->start_pos, it_d->end_pos, &list);
	if (list == NULL) {
		gl_dbgW("Empty list!");
		return NULL;
	}
	int count = eina_list_count(list);

	it_d->w = it_d->timeline_d->w;
	it_d->h = it_d->timeline_d->h;
	bool b_landscape = (it_d->h > it_d->w) ? false : true;
	Evas_Object *layout = NULL;
	layout = __gl_timeline_add_page_ly(parent, it_d);
	GL_CHECK_NULL(layout);

	if (!it_d->b_created) {
		it_d->b_created = true;
		/* Add idler for showing original files for big tile */
		if (count <= 5)
			__gl_timeline_check_page_idler(it_d);
		else
			gl_dbg("count>5, no idler for showing original file");
	}
	it_d->layout = layout;
	__gl_timeline_add_page_cbs(it_d, layout);

	Evas_Object *bg = NULL;
	Evas_Object *ck = NULL;
	gl_media_s *item = NULL;
	int i = 0;
	char *part = NULL;
	char *path = NULL;
	unsigned int orient = 0;
	int w = GL_TIMELINE_PAGE_SIZE;
	int h = GL_TIMELINE_PAGE_SIZE;
	bool b_check_thumb = false;

	for (i = 0; i < count; i++) {
		item = eina_list_nth(list, i);
		if (item == NULL) {
			gl_dbgE("Invalid item!");
			continue;
		}
/* Checkme: Scrolling doesn't work smoothly if original file was used */
		/* Get orientation for original file */
		if (((i == 0 && count <= 5) ||
		     (i == 1 && count <= 3 && !b_landscape) ||
		     (i == 2 && count == 4 && !b_landscape) ||
		     (i == 1 && (count == 2 || count == 4) && b_landscape)) &&
		    item->type == MEDIA_CONTENT_TYPE_IMAGE &&
		    (it_d->timeline_d->tran_op == GL_TRANS_OP_DONE ||
		     !it_d->b_created) && GL_FILE_EXISTS(item->file_url)) {
			path = item->file_url;
#ifdef _USE_ROTATE_BG
			if (_gl_exif_get_orientation(path, &orient) < 0) {
				path = item->thumb_url;
				orient = GL_ORIENTATION_ROT_0;
				if (!GL_FILE_EXISTS(path)) {
					path = GL_ICON_NO_THUMBNAIL;
					b_check_thumb = true;
				}
			}
#endif
			__gl_timeline_get_tile_size(count, i, b_landscape, &w,
						    &h);
		} else {
			path = item->thumb_url;
#ifdef _USE_ROTATE_BG
			orient = GL_ORIENTATION_ROT_0;
#endif
			if (!GL_FILE_EXISTS(path)) {
				path = GL_ICON_NO_THUMBNAIL;
				b_check_thumb = true;
			}
		}

		/* Add images */
		part = g_strdup_printf(GL_TL_CONTENTS_FORMAT, it_d->count,
				       i + 1);
		bg = __gl_timeline_add_page_bg(layout, part, orient, path, w,
					       h);
		evas_object_event_callback_add(bg, EVAS_CALLBACK_DEL,
					       __gl_timeline_del_bg_cb, NULL);
#ifdef _USE_ROTATE_BG
		_gl_rotate_bg_set_data(bg, (void *)i);
#endif
		GL_GFREEIF(part);
		/* Add mouse event for launching Imageviewer */
		evas_object_event_callback_add(bg, EVAS_CALLBACK_MOUSE_DOWN,
					       __gl_timeline_mouse_down, it_d);
		evas_object_event_callback_add(bg, EVAS_CALLBACK_MOUSE_UP,
					       __gl_timeline_mouse_up, it_d);
		/* Add checkbox */
		if (it_d->timeline_d->view_m != GL_TL_VIEW_NORMAL) {
			part = g_strdup_printf(GL_TL_CHECKBOX_FORMAT,
					       it_d->count, i + 1);
			ck = __gl_timeline_add_ck(layout, part,
						  __gl_timeline_is_checked(it_d->timeline_d, item->uuid),
						  it_d);
			evas_object_data_set(ck, "gl_tl_idx_key", (void *)i);
			GL_GFREEIF(part);
		}
		/* Check data */
		__gl_timeline_check_glitem(it_d->timeline_d, item, bg ,
					   b_check_thumb);
		b_check_thumb = false;

		if (it_d->timeline_d->tran_op == GL_TRANS_OP_PREPARE)
			__gl_timeline_add_tile(it_d, parent, bg, i + 1, count);

		item = NULL;
	}
	GL_IF_FREE_ELIST(list);

	if (it_d->timeline_d->tran_op == GL_TRANS_OP_PREPARE) {
		/* Only transit tiles once */
		it_d->timeline_d->tran_op = GL_TRANS_OP_START;
		__gl_timeline_transit_tiles(it_d->timeline_d);
	}
	return layout;
}

static Evas_Object *__gl_timeline_reset_cks(Evas_Object *parent, gl_tlp_s *it_d)
{
	GL_CHECK_NULL(it_d);
	GL_CHECK_NULL(it_d->timeline_d);
	GL_CHECK_NULL(parent);
	gl_dbg("index-%d, count-%d", it_d->index, it_d->count);
	gl_dbg("start_pos-%d, end_pos-%d", it_d->start_pos, it_d->end_pos);

	Eina_List *list = NULL;
	_gl_data_get_items(it_d->start_pos, it_d->end_pos, &list);
	if (list == NULL) {
		gl_dbgW("Empty list!");
		return NULL;
	}
	int count = eina_list_count(list);

	bool b_landscape = (it_d->h > it_d->w) ? false : true;
	Evas_Object *layout = NULL;
	layout = __gl_timeline_add_page_ly(parent, it_d);
	GL_CHECK_NULL(layout);

	it_d->b_created = true;
	it_d->layout = layout;
	__gl_timeline_add_page_cbs(it_d, layout);

	Evas_Object *bg = NULL;
	Evas_Object *ck = NULL;
	gl_media_s *item = NULL;
	int i = 0;
	char *part = NULL;
	char *path = NULL;
	unsigned int orient = 0;
	int w = GL_TIMELINE_PAGE_SIZE;
	int h = GL_TIMELINE_PAGE_SIZE;
	bool b_check_thumb = false;

	for (i = 0; i < count; i++) {
		item = eina_list_nth(list, i);
		if (item == NULL) {
			gl_dbgE("Invalid item!");
			continue;
		}
/* Checkme: Scrolling doesn't work smoothly if original file was used */
		/* Get orientation for original file */
		if (((i == 0 && count <= 5) ||
		     (i == 1 && count <= 3 && !b_landscape) ||
		     (i == 2 && count == 4 && !b_landscape) ||
		     (i == 1 && (count == 2 || count == 4) && b_landscape)) &&
		    item->type == MEDIA_CONTENT_TYPE_IMAGE &&
		    GL_FILE_EXISTS(item->file_url)) {
			path = item->file_url;
#ifdef _USE_ROTATE_BG
			if (_gl_exif_get_orientation(path, &orient) < 0) {
				path = item->thumb_url;
				orient = GL_ORIENTATION_ROT_0;
				if (!GL_FILE_EXISTS(path)) {
					path = GL_ICON_NO_THUMBNAIL;
					b_check_thumb = true;
				}
			}
#endif
			__gl_timeline_get_tile_size(count, i, b_landscape, &w,
						    &h);
		} else {
			path = item->thumb_url;
#ifdef _USE_ROTATE_BG
			orient = GL_ORIENTATION_ROT_0;
#endif
			if (!GL_FILE_EXISTS(path)) {
				path = GL_ICON_NO_THUMBNAIL;
				b_check_thumb = true;
			}
		}

		/* Add images */
		part = g_strdup_printf(GL_TL_CONTENTS_FORMAT, it_d->count,
				       i + 1);
		bg = __gl_timeline_add_page_bg(layout, part, orient, path, w,
					       h);
		evas_object_event_callback_add(bg, EVAS_CALLBACK_DEL,
					       __gl_timeline_del_bg_cb, NULL);
#ifdef _USE_ROTATE_BG
		_gl_rotate_bg_set_data(bg, (void *)i);
#endif
		GL_GFREEIF(part);
		/* Add mouse event for launching Imageviewer */
		evas_object_event_callback_add(bg, EVAS_CALLBACK_MOUSE_DOWN,
					       __gl_timeline_mouse_down, it_d);
		evas_object_event_callback_add(bg, EVAS_CALLBACK_MOUSE_UP,
					       __gl_timeline_mouse_up, it_d);
		/* Add checkbox */
		if (it_d->timeline_d->view_m != GL_TL_VIEW_NORMAL) {
			part = g_strdup_printf(GL_TL_CHECKBOX_FORMAT,
					       it_d->count, i + 1);
			ck = __gl_timeline_add_ck(layout, part,
						  __gl_timeline_is_checked(it_d->timeline_d, item->uuid),
						  it_d);
			evas_object_data_set(ck, "gl_tl_idx_key", (void *)i);
			GL_GFREEIF(part);
		}
		/* Check data */
		__gl_timeline_check_glitem(it_d->timeline_d, item, bg ,
					   b_check_thumb);
		b_check_thumb = false;

		item = NULL;
	}
	GL_IF_FREE_ELIST(list);
	return layout;
}

static bool __gl_timeline_set_cks_state(gl_tlp_s *it_d, bool b_checked)
{
	GL_CHECK_FALSE(it_d);
	GL_CHECK_FALSE(it_d->timeline_d);
	gl_dbg("index-%d, count-%d", it_d->index, it_d->count);
	gl_dbg("start_pos-%d, end_pos-%d", it_d->start_pos, it_d->end_pos);
	gl_dbg("checkbox: %d", b_checked);

	Evas_Object *ck = NULL;
	int i = 0;
	char *part = NULL;
	Eina_Bool checked = EINA_FALSE;

	for (i = 0; i < it_d->count; i++) {
		part = g_strdup_printf(GL_TL_CHECKBOX_FORMAT, it_d->count,
				       i + 1);
		ck = elm_object_part_content_get(it_d->layout, part);
		if (ck) {
			checked = elm_check_state_get(ck);
			if (checked != b_checked)
				elm_check_state_set(ck, b_checked);
		} else {
			gl_dbgW("Invalid checkbox!");
		}
		GL_GFREEIF(part);
	}
	return true;
}

static int __gl_timeline_free_pages(gl_timeline_s *timeline_d)
{
	GL_CHECK_VAL(timeline_d, -1);
	if (timeline_d->list) {
		gl_dbgW("##############  Free pages ###############");
		void *tl_d = NULL;
		EINA_LIST_FREE(timeline_d->list, tl_d) {
			GL_FREEIF(tl_d);
		}
		timeline_d->list = NULL;
	} else {
		gl_dbgW("None pages!");
	}
	return 0;
}

static int __gl_timeline_update_tiles_cnt(int zlevel, int default_cnt)
{
	int tiles_cnt = 0;

	if (zlevel == GL_TM_ZOOM_OUT_TWO) {
		tiles_cnt = GL_TL_PINCH_OUT_2_CNT;
	} else if (zlevel == GL_TM_ZOOM_OUT_ONE) {
		if (default_cnt <= 2)
			tiles_cnt = default_cnt * GL_TL_PINCH_OUT_CNT_STEP;
		else
			tiles_cnt = GL_TL_PINCH_OUT_1_CNT;
	} else {
		tiles_cnt = default_cnt;
	}
	return tiles_cnt;
}

static int __gl_timeline_update_zoom_flags(gl_timeline_s *timeline_d,
					   bool b_pout)
{
	GL_CHECK_VAL(timeline_d, -1);

	/* To trigger different transition */
	if (!b_pout) {
		timeline_d->zoom_level++;
		_gl_slider_set_zop(timeline_d->view, GL_SLIDE_ZOP_IN);
	} else {
		timeline_d->zoom_level--;
		_gl_slider_set_zop(timeline_d->view, GL_SLIDE_ZOP_OUT);
	}
	/* Reset zoom level for overscrolling showing in slider */
	if (timeline_d->zoom_level == GL_TM_ZOOM_DEFAULT)
		_gl_slider_set_zlevel(timeline_d->view, GL_SLIDE_ZLEVEL_MAX);
	else if (timeline_d->zoom_level == GL_TM_ZOOM_OUT_TWO)
		_gl_slider_set_zlevel(timeline_d->view, GL_SLIDE_ZLEVEL_MIN);
	else
		_gl_slider_set_zlevel(timeline_d->view, GL_SLIDE_ZLEVEL_MID);
	return 0;
}

static int __gl_timeline_update_zoom_pages(gl_timeline_s *timeline_d,
					   bool b_pout)
{
	GL_CHECK_VAL(timeline_d, -1);
	int i = 0;
	gl_tlp_s *it_d = NULL;
	int index = 0;
	/* Get first image of current page */
	void *sitem = _gl_slider_get_sitem(timeline_d->view);
	GL_CHECK_VAL(sitem, -1);
	it_d = (gl_tlp_s *)_gl_slider_get_sitem_data(sitem);
	GL_CHECK_VAL(it_d, -1);
	int page_first_img_idx = it_d->start_pos;
	int new_cur_page_idx = 0;
	int list_cnt = eina_list_count(timeline_d->count_list);
	gl_dbg("level: %d, count: %d, count_list: %d", timeline_d->zoom_level,
	       timeline_d->count, list_cnt);
	int cnt = 0;

	/* Update flags */
	__gl_timeline_update_zoom_flags(timeline_d, b_pout);
	evas_object_freeze_events_set(timeline_d->view, EINA_TRUE);
	GL_TL_CLEAR_PAGES(timeline_d, false);
	gl_dbgW("evas_object_freeze_events_set EINA_TRUE");

	for (i = 0; i < timeline_d->count;) {
		/* item date is freed in del_cb */
		it_d = (gl_tlp_s *)calloc(1, sizeof(gl_tlp_s));
		/* Create UX item */
		timeline_d->list = eina_list_append(timeline_d->list, it_d);
		it_d->timeline_d = timeline_d;
		if (list_cnt > index) {
			/* Reuse count of page */
			cnt = (int)eina_list_nth(timeline_d->count_list,
						 index);
			gl_dbg("%d", cnt);
		} else {
			/* Add new random count */
			cnt = 1 + rand() % 5;
			timeline_d->count_list = eina_list_append(timeline_d->count_list,
								  (void *)cnt);
		}

		it_d->count = __gl_timeline_update_tiles_cnt(timeline_d->zoom_level,
							     cnt);
		it_d->index = index++;
		it_d->start_pos = i;
		/* Refer to first item of next loop */
		i += it_d->count;
		it_d->end_pos = i - 1;
		/* Get new index of page */
		if (new_cur_page_idx ==0 &&
		    page_first_img_idx >= it_d->start_pos &&
		    page_first_img_idx <= it_d->end_pos) {
			gl_dbgW("New page index got!");
			new_cur_page_idx = it_d->index;
		}
		/* Reset count and end_pos of last page */
		if (timeline_d->zoom_level != GL_TM_ZOOM_OUT_TWO &&
		    i >= timeline_d->count) {
			it_d->end_pos = timeline_d->count - 1;
			it_d->count = it_d->end_pos - it_d->start_pos + 1;
			break;
		}
		it_d = NULL;
	}
	gl_dbg("length: %d", eina_list_count(timeline_d->list));

	_gl_slider_set_list(timeline_d->view, timeline_d->list,
			    new_cur_page_idx);
	_gl_slider_load_first_item(timeline_d->view);
	return 0;
}

static Eina_Bool __gl_timeline_zoom_in(gl_timeline_s *timeline_d)
{
	GL_CHECK_FALSE(timeline_d);
	gl_dbg("");

	if (gl_get_view_mode(timeline_d->ad) != GL_VIEW_TIMELINE)
		return false;

	gl_dbg("level: %d", timeline_d->zoom_level);
	if (timeline_d->zoom_level >= GL_TM_ZOOM_DEFAULT)
		return false;

	/* Update view */
	__gl_timeline_update_zoom_pages(timeline_d, false);
	return true;
}

static Eina_Bool __gl_timeline_zoom_out(gl_timeline_s *timeline_d)
{
	GL_CHECK_FALSE(timeline_d);
	gl_dbg("");

	if (gl_get_view_mode(timeline_d->ad) != GL_VIEW_TIMELINE)
		return false;

	gl_dbg("level: %d", timeline_d->zoom_level);
	if (timeline_d->zoom_level <= GL_TM_ZOOM_OUT_TWO)
		return false;

	/* Update view */
	__gl_timeline_update_zoom_pages(timeline_d, true);
	return true;
}

static void __gl_timeline_slider_clicked_cb(void *data, Evas_Object *obj, void *ei)
{
	gl_dbg("");
}

static void __gl_timeline_slider_long_press_start_cb(void *data,
						     Evas_Object *obj, void *ei)
{
	gl_dbg("");
}

static void __gl_timeline_slider_long_press_end_cb(void *data, Evas_Object *obj,
						   void *ei)
{
	gl_dbg("");
}

static void __gl_timeline_slider_item_changed_cb(void *data, Evas_Object *obj,
						 void *ei)
{
	GL_CHECK(ei);
	GL_CHECK(data);
	gl_dbg("");

	__gl_timeline_update_title((gl_timeline_s *)data,
				   (gl_tlp_s *)_gl_slider_get_sitem_data(ei),
				   false);
}

static void __gl_timeline_slider_item_state_changed_cb(void *data,
						       Evas_Object *obj,
						       void *ei)
{
	gl_dbg("");
}

static void __gl_timeline_slider_items_resized_cb(void *data, Evas_Object *obj,
						  void *ei)
{
	gl_dbgW("start");
	GL_CHECK(data);
	gl_timeline_s *timeline_d = (gl_timeline_s *)data;
	GL_CHECK(timeline_d->ad);
	_gl_main_add_reg_idler(timeline_d->ad);
	if (!timeline_d->b_created) {
		timeline_d->b_created = true;
		_gl_slider_load_first_item(timeline_d->view);
	} else {
		if (timeline_d->tran_op == GL_TRANS_OP_SHOW) {
			gl_dbgW("Change transit operation mode!");
			timeline_d->tran_op = GL_TRANS_OP_START;
		}
	}
	gl_dbgW("done");
}

static void __gl_timeline_slider_items_zoom_out_cb(void *data, Evas_Object *obj,
						   void *ei)
{
	gl_dbgW("start");
	GL_CHECK(data);
	gl_timeline_s *timeline_d = (gl_timeline_s *)data;
	GL_CHECK(timeline_d->ad);
	__gl_timeline_zoom_out(timeline_d);
	gl_dbgW("done");
}

static void __gl_timeline_slider_items_zoom_in_cb(void *data, Evas_Object *obj,
						  void *ei)
{
	gl_dbgW("start");
	GL_CHECK(data);
	gl_timeline_s *timeline_d = (gl_timeline_s *)data;
	GL_CHECK(timeline_d->ad);
	__gl_timeline_zoom_in(timeline_d);
	gl_dbgW("done");
}
#endif

static void __gl_timeline_resize_layout_cb(void *data, Evas *e,
					   Evas_Object *obj, void *ei)
{
	GL_CHECK(data);
	gl_timeline_s *timeline_d = (gl_timeline_s *)data;
	Evas_Coord x,y,w,h;

	evas_object_geometry_get(obj, &x, &y, &w, &h);
	gl_dbg("MainView resized geomtery XYWH(%d,%d,%d,%d)", x, y, w, h);
	evas_object_data_set(timeline_d->layout, "page_w", (void *)w);
	evas_object_data_set(timeline_d->layout, "page_h", (void *)h);
	timeline_d->w = w;
	timeline_d->h = h;
}

/* register mouse callback */
#if 0
static int __gl_timeline_add_slider_cbs(Evas_Object *slider, void *cb_data)
{
	GL_CHECK_VAL(cb_data, -1);
	GL_CHECK_VAL(slider, -1);

	evas_object_smart_callback_add(slider, "slider,clicked",
				       __gl_timeline_slider_clicked_cb,
				       cb_data);
	evas_object_smart_callback_add(slider, "slider,longpress,start",
				       __gl_timeline_slider_long_press_start_cb,
				       cb_data);
	evas_object_smart_callback_add(slider, "slider,longpress,end",
				       __gl_timeline_slider_long_press_end_cb,
				       cb_data);

	evas_object_smart_callback_add(slider, "slider,item,changed",
				       __gl_timeline_slider_item_changed_cb,
				       cb_data);
	evas_object_smart_callback_add(slider, "slider,item,state,changed",
				       __gl_timeline_slider_item_state_changed_cb,
				       cb_data);
	evas_object_smart_callback_add(slider, "slider,items,resized",
				       __gl_timeline_slider_items_resized_cb,
				       cb_data);
	evas_object_smart_callback_add(slider, "slider,items,zoomout",
				       __gl_timeline_slider_items_zoom_out_cb,
				       cb_data);
	evas_object_smart_callback_add(slider, "slider,items,zoomin",
				       __gl_timeline_slider_items_zoom_in_cb,
				       cb_data);
	return 0;
}

static Evas_Object *__gl_timeline_create_slider(gl_timeline_s *timeline_d)
{
	GL_CHECK_NULL(timeline_d);
	Evas_Object *slider = NULL;

	slider = _gl_slider_add(timeline_d->layout);
	GL_CHECK_NULL(slider);

	evas_object_data_set(slider, "create_func",
			     (void *)__gl_timeline_add_page);
	evas_object_data_set(slider, "reset_cks_func",
			     (void *)__gl_timeline_reset_cks);
	evas_object_data_set(slider, "set_cks_state_func",
			     (void *)__gl_timeline_set_cks_state);
	__gl_timeline_add_slider_cbs(slider, (void *)timeline_d);
	int bx = 0;
	int by = 0;
	int bw = 0;
	int bh = 0;
	evas_object_geometry_get(slider, &bx, &by, &bw, &bh);
	gl_dbg("slider. (%d,%d,%d,%d)", bx, by, bw, bh);

	return slider;
}
#endif

static int __gl_timelne_show_nocontents(gl_timeline_s *timeline_d)
{
	GL_CHECK_VAL(timeline_d, -1);
	GL_CHECK_VAL(timeline_d->ad, -1);
	if (timeline_d->nocontents) {
		gl_dbgW("Already created!");
		return 0;
	}

	GL_IF_DEL_OBJ(timeline_d->view);
	Evas_Object *title = elm_object_part_content_get(timeline_d->layout,
							 "title");
	GL_IF_DEL_OBJ(title);

	Evas_Object *noc = _gl_nocontents_create(timeline_d->parent);
	elm_object_part_content_set(timeline_d->layout, "elm.swallow",
					    noc);
	timeline_d->nocontents = noc;

	return 0;
}

static int __gl_timelne_del_nocontents(gl_timeline_s *timeline_d)
{
	GL_CHECK_VAL(timeline_d, -1);
	GL_CHECK_VAL(timeline_d->ad, -1);

	if (timeline_d->nocontents == NULL)
		return 0;

	gl_dbg("Delete nocontents view first");
	GL_IF_DEL_OBJ(timeline_d->nocontents);

	Evas_Object *view = __gl_timeline_create_list_view(timeline_d, false);
	if (view == NULL) {
		gl_dbgE("Failed to create view!");
		__gl_timelne_show_nocontents(timeline_d);
		elm_naviframe_item_title_enabled_set(timeline_d->nf_it, EINA_FALSE, EINA_FALSE);
		return -1;
	}

	elm_naviframe_item_title_enabled_set(timeline_d->nf_it, EINA_TRUE, EINA_TRUE);
	timeline_d->view = view;
	elm_object_part_content_set(timeline_d->layout, "elm.swallow",
				    view);

	return 0;
}

int _gl_timeline_get_item_index(void *data, char* path)
{
	GL_CHECK_VAL(data, 1);
	GL_CHECK_VAL(path, 1);
	gl_appdata *ad = (gl_appdata *)data;
	gl_media_s *gitem = NULL;
	if (ad->tlinfo && ad->tlinfo->data_list) {
		int count = eina_list_count(ad->tlinfo->data_list);
		int i;
		for (i = 0 ; i < count ; i++) {
			gitem = eina_list_nth(ad->tlinfo->data_list, i);
			if (gitem) {
				if (!strcmp(path, gitem->file_url)) {
					return i + 1;
				}
			}
		}
	}
	return 1;
}

static void __gl_timeline_thumbs_sel_cb(void *data, Evas_Object *obj, void *ei)
{
	GL_CHECK(data);
	gl_timeline_s *gitem = (gl_timeline_s *)data;
	GL_CHECK(gitem->ad);
	gl_appdata *ad = (gl_appdata *)gitem->ad;
	elm_gengrid_item_selected_set((Elm_Object_Item *)ei, EINA_FALSE);

	/* Save scroller position before clearing gengrid */
	gl_media_s *item = NULL;
	item = elm_object_item_data_get((Elm_Object_Item *)ei);
	_gl_ui_save_scroller_pos(gitem->view);
	if (gitem->view_m != GL_TL_VIEW_NORMAL) {
		Evas_Object *ck = NULL;
		ck = elm_object_item_part_content_get((Elm_Object_Item *)ei, GL_THUMB_CHECKBOX);
		if (ck) {
			elm_check_state_set(ck, !elm_check_state_get(ck));
			__gl_timeline_thumb_check_op(gitem, ck, item);
		} else {
			gl_dbgE("item checkbox not found");
		}
	} else {
		if (item && item->file_url) {
			int index = _gl_timeline_get_item_index(ad, item->file_url);
			_gl_ext_load_iv_timeline(ad, item->file_url, index, ad->tlinfo->time_media_display_order);
		}
		else {
			gl_dbgW("Image wasn't found!");
		}
	}
	_gl_ui_restore_scroller_pos(gitem->view);
}

Evas_Object *_gl_timeline_thumbs_get_content(void *data, Evas_Object *parent,
				    int w, int h)
{
	GL_CHECK_NULL(parent);
	GL_CHECK_NULL(data);
	gl_media_s *item = (gl_media_s *)data;
	char *path = NULL;
	Evas_Object *layout = NULL;

	if (GL_FILE_EXISTS(item->thumb_url)) {
		path = item->thumb_url;
	} else {
		/* Use default image */
		path = GL_ICON_NO_THUMBNAIL;
	}

	int zoom_level = GL_ZOOM_DEFAULT;

	if (item->type == MEDIA_CONTENT_TYPE_VIDEO) {
		unsigned int v_dur = 0;
		if (item->video_info) {
			v_dur = item->video_info->duration;
		}
		layout = _gl_thumb_show_video(parent, path, v_dur, w, h,
				zoom_level);
	} else {
		if (item->image_info &&
				item->image_info->burstshot_id) {
			layout = _gl_thumb_show_image(parent, path, 0, w, h,
					zoom_level);
			item->mode = GL_CM_MODE_BURSTSHOT;
		} else {
			layout = _gl_thumb_show_image(parent, path, 0, w, h,
					zoom_level);
		}
	}
	return layout;
}

static void __gl_timeline_thumbs_create_thumb_cb(media_content_error_e error,
					const char *path, void *user_data)
{
	GL_CHECK(user_data);
	gl_media_s *thumb_data = (gl_media_s *)user_data;

	thumb_data->b_create_thumb = false;
	if (error == MEDIA_CONTENT_ERROR_NONE && GL_FILE_EXISTS(path) &&
	    g_strcmp0(path, GL_ICON_DB_DEFAULT_THUMB)) {
		GL_CHECK(thumb_data);
		/* Update thumb path */
		GL_FREEIF(thumb_data->thumb_url);
		thumb_data->thumb_url = strdup(path);
		elm_gengrid_item_update(thumb_data->elm_item);
	} else {
		gl_dbgE("[%d]Invalid path[%s]!", error, path);
	}
}

/* Use file to create new thumb if possible */
int _gl_timeline_thumbs_create_thumb(gl_media_s *gitem)
{
	GL_CHECK_VAL(gitem, -1);
	GL_CHECK_VAL(gitem->file_url, -1);

	if (GL_FILE_EXISTS(gitem->file_url)) {
		_gl_data_create_thumb(gitem, __gl_timeline_thumbs_create_thumb_cb,
				      gitem);
		return 0;
	}
	return -1;
}

static void __gl_timeline_thumbs_realized(void *data, Evas_Object *obj, void *ei)
{

	gl_dbg("realized");
	GL_CHECK(ei);
	GL_CHECK(data);
	Elm_Object_Item *it = (Elm_Object_Item *)ei;
	gl_media_s *gitem = NULL;

	gitem = elm_object_item_data_get(it);
	GL_CHECK(gitem);
	/* Checking for local files only */
	if (gitem->storage_type == GL_STORE_T_MMC ||
			gitem->storage_type == GL_STORE_T_ALL ||
			gitem->storage_type == GL_STORE_T_PHONE) {
		/* Use default image */
		if (!GL_FILE_EXISTS(gitem->thumb_url)) {
			_gl_timeline_thumbs_create_thumb(gitem);
		}
	}
}

static void __gl_timeline_thumbs_unrealized(void *data, Evas_Object *obj, void *ei)
{
	gl_dbg("unrealized");
	GL_CHECK(ei);
	GL_CHECK(data); /* It's ad */
	Elm_Object_Item *it = (Elm_Object_Item *)ei;

	gl_media_s *gitem = elm_object_item_data_get(it);
	GL_CHECK(gitem);
	/* Checking for local files only */
	if (gitem->storage_type == GL_STORE_T_MMC ||
	    gitem->storage_type == GL_STORE_T_ALL ||
	    gitem->storage_type == GL_STORE_T_PHONE) {
		if (gitem->b_create_thumb) {
			_gl_data_cancel_thumb(gitem);
		}
	}
}

void _gl_timeline_open_file_select_mode(void *data)
{
	gl_media_s *gitem = (gl_media_s *)data;
	GL_CHECK(gitem);
	GL_CHECK(gitem->file_url);
	GL_CHECK(strlen(gitem->file_url));
	GL_CHECK(gitem->ad);
	gl_appdata *ad = (gl_appdata *)gitem->ad;

	gl_dbg("Loading UG-IMAGE-VIEWER-SELECT-MODE");
	gl_ext_load_iv_time_ug_select_mode(ad, gitem, GL_UG_IV, ad->tlinfo->time_media_display_order);
}

void _gl_timeline_open_image_in_select_mode(void *data, Evas_Object *obj, void *event_info)
{
	GL_CHECK(data);
	_gl_timeline_open_file_select_mode(data);
}

static Evas_Object *__gl_timeline_thumbs_get_content(void *data, Evas_Object *obj,
					    const char *part)
{
	GL_CHECK_NULL(part);
	GL_CHECK_NULL(strlen(part));
	GL_CHECK_NULL(data);
	gl_media_s *gitem = (gl_media_s *)data;
	GL_CHECK_NULL(gitem);
	gl_timeline_s *timeline_d = (gl_timeline_s *)evas_object_data_get(obj, "data");
	GL_CHECK_NULL(timeline_d);
	int mode = timeline_d->view_m;

	if (!g_strcmp0(part, GL_THUMB_ICON)) {
		Evas_Object *layout = _gl_timeline_thumbs_get_content(gitem, obj,
				timeline_d->w,
				timeline_d->h);
		if (gitem->storage_type == GL_STORE_T_MMC) {
			elm_object_item_signal_emit(gitem->elm_item, "show_sd_card_icon", "sd_card_icon_img");
		}
		return layout;
	} else if (!g_strcmp0(part, GL_THUMB_CHECKBOX)) {
		Evas_Object *ck = NULL;
		if (mode == GL_TL_VIEW_EDIT) {
			ck = elm_check_add(obj);
			GL_CHECK_NULL(ck);

#ifdef _USE_GRID_CHECK
			elm_object_style_set(ck, "grid");
#else
			elm_object_style_set(ck, GL_CHECKBOX_STYLE_THUMB);
#endif
			evas_object_propagate_events_set(ck, EINA_FALSE);
			elm_check_state_set(ck, gitem->check_state);
			evas_object_data_set(ck, "data", (void *)timeline_d);
			evas_object_smart_callback_add(ck, "changed", __gl_timeline_thumbs_check_changed, data);
			evas_object_show(ck);
			elm_object_item_signal_emit(gitem->elm_item, "show_image_icon", "elm_image_open_icon_rect");
		} else {
			elm_object_item_signal_emit(gitem->elm_item, "hide_image_icon", "elm_image_open_icon_rect");
		}
		return ck;
	} else if (!g_strcmp0(part, "elm_image_open_icon_swallow_blocker")) {
		Evas_Object *btn1 = NULL;
		if (mode == GL_TL_VIEW_EDIT) {
			btn1 = evas_object_rectangle_add(evas_object_evas_get(obj));
			evas_object_color_set(btn1, 0, 255, 0, 0);
			evas_object_propagate_events_set(btn1, EINA_FALSE);
		} else {
			btn1 = elm_object_item_part_content_get(gitem->elm_item,
					"elm_image_open_icon_swallow_blocker");
			if (btn1) {
				evas_object_del(btn1);
				btn1 = NULL;
			}
		}
		return btn1;
	} else if (!g_strcmp0(part, "elm_image_open_icon_swallow")) {
		Evas_Object *btn = NULL;
		if (mode == GL_TL_VIEW_EDIT) {
			btn = elm_button_add(obj);
			elm_object_style_set(btn, "transparent");
			evas_object_show(btn);
			evas_object_propagate_events_set(btn, EINA_FALSE);
			evas_object_smart_callback_add(btn, "clicked", _gl_timeline_open_image_in_select_mode, gitem);
		} else {
			btn = elm_object_item_part_content_get(gitem->elm_item,
					"elm_image_open_icon_swallow");
			if (btn) {
				evas_object_del(btn);
				btn = NULL;
			}
		}
		return btn;
	}
	return NULL;
}

int _gl_timeline_thumb_set_size(void *data, Evas_Object *view, int *size_w, int *size_h)
{
	GL_CHECK_VAL(view, -1);
	GL_CHECK_VAL(data, -1);
	gl_appdata *ad = (gl_appdata *)data;
	int rotate_mode = ad->maininfo.rotate_mode;
	int _w = 0;
	int _h = 0;
	int _w_l = 0;
	int _h_l = 0;
	int win_w = 0;
	int win_h = 0;
	int items_per_row = 0;
	_gl_get_win_factor(ad->maininfo.win, &win_w, &win_h);

	if ((rotate_mode == APP_DEVICE_ORIENTATION_270) ||
			(rotate_mode == APP_DEVICE_ORIENTATION_90)) {
		if (ad->pinchinfo.zoom_level == GL_ZOOM_IN_TWO) {
			items_per_row = GL_GRID_6_PER_ROW;
		} else if (ad->pinchinfo.zoom_level == GL_ZOOM_IN_ONE) {
			items_per_row = GL_GRID_7_PER_ROW;
		} else if (ad->pinchinfo.zoom_level == GL_ZOOM_DEFAULT) {
			items_per_row = GL_GRID_10_PER_ROW;
		} else {
			items_per_row = GL_GRID_7_PER_ROW;
		}
	} else {
		if (ad->pinchinfo.zoom_level == GL_ZOOM_IN_TWO) {
			items_per_row = GL_GRID_3_PER_ROW;
		} else if (ad->pinchinfo.zoom_level == GL_ZOOM_IN_ONE) {
			items_per_row = GL_GRID_4_PER_ROW;
		} else if (ad->pinchinfo.zoom_level == GL_ZOOM_DEFAULT) {
			items_per_row = GL_GRID_8_PER_ROW;
		} else {
			items_per_row = GL_GRID_4_PER_ROW;
		}
	}

	_w = (int)(win_w / items_per_row);
	_w_l = (int)(win_h / items_per_row);
	_h = _w;
	_h_l = _w_l;

	if ((rotate_mode == APP_DEVICE_ORIENTATION_270) ||
			(rotate_mode == APP_DEVICE_ORIENTATION_90)) {
		elm_gengrid_item_size_set(view, _w_l, _h_l);
		if (size_w)
			*size_w = _w_l-4;
		if (size_h)
			*size_h = _h_l-4;
	} else {
		elm_gengrid_item_size_set(view, _w, _h);
		if (size_w)
			*size_w = _w-4;
		if (size_h)
			*size_h = _h-4;
	}
	if (size_w && size_h)
		gl_dbg("P: %dx%d, size_w: %d,size_h: %d", _w, _h, *size_w, *size_h);
	return 0;
}

Evas_Object *_gl_grid_layout_add(Evas_Object *parent)
{
	Evas_Object *layout = NULL;
	layout = elm_layout_add(parent);
	elm_layout_file_set(layout, GL_EDJ_FILE, "timeline_gridview");
	evas_object_size_hint_align_set(layout, EVAS_HINT_FILL, EVAS_HINT_FILL);
	evas_object_show(layout);

	return layout;
}

Evas_Object *_gl_grid_add(Evas_Object *parent)
{
	GL_CHECK_NULL(parent);
	Evas_Object *grid = elm_gengrid_add(parent);
	GL_CHECK_NULL(grid);

	elm_gengrid_align_set(grid, 0.0, 0.0);
	elm_gengrid_horizontal_set(grid, EINA_FALSE);
	elm_scroller_bounce_set(grid, EINA_FALSE, EINA_TRUE);
	elm_scroller_policy_set(grid, ELM_SCROLLER_POLICY_OFF,
			ELM_SCROLLER_POLICY_AUTO);
	elm_gengrid_multi_select_set(grid, EINA_TRUE);
	evas_object_size_hint_weight_set(grid, EVAS_HINT_EXPAND,
			EVAS_HINT_EXPAND);
	evas_object_show(grid);

	return grid;
}

void _gl_time_finialize_media_data(gl_timeline_s *timeline_d)
{
	GL_CHECK(timeline_d);
	if (timeline_d->data_list) {
		gl_media_s *item = NULL;
		EINA_LIST_FREE(timeline_d->data_list, item) {
			if (!item || !item->uuid)
				continue;
			_gl_data_type_free_glitem((void **)(&item));
			item = NULL;
		}
	}
}

void _gl_time_update_selected_media(gl_timeline_s *timeline_d, Eina_List *list)
{
	GL_CHECK(timeline_d);
	GL_CHECK(list);
	gl_media_s *gitem = NULL;
	int i;
	if (list) {
		int count = eina_list_count(list);
		for (i = 0; i < count ; i++) {
			gitem = eina_list_nth(list, i);
			if (gitem) {
				if (__gl_timeline_is_checked(timeline_d, gitem->uuid)) {
					gitem->check_state = true;
				}
			}
		}
	}
}

static void _gl_time_view_append_gengrid_items(Evas_Object *
		pGengrid,
		int nGenItemIndex,
		gl_timeline_s *timeline_d)
{
	static Elm_Gengrid_Item_Class *gic;
	gic = elm_gengrid_item_class_new();
	gic->item_style = GL_GENGRID_ITEM_STYLE_THUMBNAIL;
	gic->func.text_get = NULL;
	gic->func.content_get = __gl_timeline_thumbs_get_content;
	gic->func.del = NULL;

	int nCount = _gl_time_get_number_of_items_per_row(timeline_d->ad);
	int nIndex = nGenItemIndex;
	int nMaxIndex = nIndex + nCount;
	int nVideoListSize = eina_list_count(timeline_d->data_list);
	gl_media_s *item = NULL;
	char * text = NULL;
	item = eina_list_nth(timeline_d->data_list, nIndex);
	if (item == NULL) {
		gl_dbgE("Invalid item!");
		return;
	}
	int numberOfItemsOnParticularDate = _get_count_of_items_on_same_date(item, timeline_d->data_list, &text, nIndex+1);
	int k =1;
	Elm_Object_Item *gridItem = NULL;
	for (; (nIndex < nMaxIndex) && (nIndex < nVideoListSize) && (k <= numberOfItemsOnParticularDate) ; nIndex++) {
		item = eina_list_nth(timeline_d->data_list, nIndex);
		gridItem =
				elm_gengrid_item_append(pGengrid, gic, item, __gl_timeline_thumbs_sel_cb, timeline_d);
		elm_gengrid_item_select_mode_set(gridItem,
				ELM_OBJECT_SELECT_MODE_ALWAYS);
		item->elm_item = gridItem;
		item->ad = timeline_d->ad;
		k++;
	}
	elm_genlist_item_class_free(gic);
	_gl_timeline_thumb_set_size(timeline_d->ad, pGengrid, &timeline_d->w, &timeline_d->h);
}

static Evas_Object *__gl_get_icon_of_grid_cb(const void
		*pUserData,
		Evas_Object *
		pObject,
		const char
		*pPart)
{
	int nGenItemIndex = (int) pUserData;
	gl_timeline_s *timeline_d = (gl_timeline_s *)evas_object_data_get(pObject,"data");
	if (!timeline_d) {
		return NULL;
	}

	Evas_Object *pGengrid = _gl_grid_add(pObject);
	if (!pGengrid) {
		return NULL;
	}

	evas_object_data_set(pGengrid, "data", (void *)timeline_d);
	evas_object_smart_callback_add(pGengrid, "realized", __gl_timeline_thumbs_realized, timeline_d);
	evas_object_smart_callback_add(pGengrid, "unrealized", __gl_timeline_thumbs_unrealized, timeline_d);

	_gl_time_view_append_gengrid_items(pGengrid, nGenItemIndex, timeline_d);

	return pGengrid;
}

static char *__gl_time_date_get_text(void *data, Evas_Object *obj, const char *part)
{
	GL_CHECK_NULL(data);
	char * text = (char *)data;
	return strdup(text);
}

int _get_count_of_items_on_same_date(gl_media_s *item, Eina_List *list, char **text, int start_index)
{
	int count = 1;
	char *text1 = NULL;
	char *text2 = NULL;
	char *text3 = NULL;
	struct tm t1;
	struct tm t2;
	struct tm ct;
	time_t ctime = 0;
	memset(&ct, 0x00, sizeof(struct tm));
	time(&ctime);
	localtime_r(&ctime, &ct);
	time_t mtime1 = 0;
	time_t mtime2 = 0;
	int i;

	mtime1 = item->mtime;
	mtime2 = item->mtime;
	int item_cnt = eina_list_count(list);

	__gl_timeline_get_tm(mtime1, mtime2, &t1, &t2);
	__gl_timeline_get_mtime_str(t1, ct, &text1, &text2);
	if (text1) {
		text3 = g_strdup_printf("%s", text1);
	}
	for (i = start_index; i < item_cnt ; i++) {
		item = eina_list_nth(list, i);
		if (!item) {
			return 0;
		}
		mtime1 = item->mtime;
		mtime2 = item->mtime;
		__gl_timeline_get_tm(mtime1, mtime2, &t1, &t2);
		__gl_timeline_get_mtime_str(t1, ct, &text1, &text2);
		if (text1 && text3 && (strcmp(text1, text3))) {
			break;
		}
		count ++;
		if (text1) {
			text3 = g_strdup_printf("%s", text1);
		}
	}
	*text = g_strdup_printf("%s", text3);

	return count;
}

int _gl_time_get_number_of_items_per_row(void *data)
{
	GL_CHECK_VAL(data, -1);
	gl_appdata *ad = (gl_appdata *)data;
	int rotate_mode = ad->maininfo.rotate_mode;
	int items_per_row = 0;
	if ((rotate_mode == APP_DEVICE_ORIENTATION_270) ||
			(rotate_mode == APP_DEVICE_ORIENTATION_90)) {
		if (ad->pinchinfo.zoom_level == GL_ZOOM_IN_TWO) {
			items_per_row = GL_GRID_6_PER_ROW;
		} else if (ad->pinchinfo.zoom_level == GL_ZOOM_IN_ONE) {
			items_per_row = GL_GRID_7_PER_ROW;
		} else if (ad->pinchinfo.zoom_level == GL_ZOOM_DEFAULT) {
			items_per_row = GL_GRID_10_PER_ROW;
		} else {
			items_per_row = GL_GRID_7_PER_ROW;
		}
	} else {
		if (ad->pinchinfo.zoom_level == GL_ZOOM_IN_TWO) {
			items_per_row = GL_GRID_3_PER_ROW;
		} else if (ad->pinchinfo.zoom_level == GL_ZOOM_IN_ONE) {
			items_per_row = GL_GRID_4_PER_ROW;
		} else if (ad->pinchinfo.zoom_level == GL_ZOOM_DEFAULT) {
			items_per_row = GL_GRID_8_PER_ROW;
		} else {
			items_per_row = GL_GRID_4_PER_ROW;
		}
	}
	return items_per_row;
}

char *_gl_time_get_genlist_style(void *data)
{
	GL_CHECK_VAL(data, NULL);
	gl_appdata *ad = (gl_appdata *)data;
	int rotate_mode = ad->maininfo.rotate_mode;
	int items_per_row = 0;
	int height = 0;
	int win_w = 0;
	int win_h = 0;
	char *style = NULL;
	_gl_get_win_factor(ad->maininfo.win, &win_w, &win_h);
	if (_gl_is_timeline_edit_mode(data)) {
		ad->pinchinfo.zoom_level = GL_ZOOM_IN_ONE;
	}
	if ((rotate_mode == APP_DEVICE_ORIENTATION_270) ||
			(rotate_mode == APP_DEVICE_ORIENTATION_90)) {
		if (ad->pinchinfo.zoom_level == GL_ZOOM_IN_TWO) {
			items_per_row = GL_GRID_6_PER_ROW;
		} else if (ad->pinchinfo.zoom_level == GL_ZOOM_IN_ONE) {
			items_per_row = GL_GRID_7_PER_ROW;
		} else if (ad->pinchinfo.zoom_level == GL_ZOOM_DEFAULT) {
			items_per_row = GL_GRID_10_PER_ROW;
		} else {
			items_per_row = GL_GRID_7_PER_ROW;
		}
		height = ceil((double)win_h/items_per_row);
	} else {
		if (ad->pinchinfo.zoom_level == GL_ZOOM_IN_TWO) {
			items_per_row = GL_GRID_3_PER_ROW;
		} else if (ad->pinchinfo.zoom_level == GL_ZOOM_IN_ONE) {
			items_per_row = GL_GRID_4_PER_ROW;
		} else if (ad->pinchinfo.zoom_level == GL_ZOOM_DEFAULT) {
			items_per_row = GL_GRID_8_PER_ROW;
		} else {
			items_per_row = GL_GRID_4_PER_ROW;
		}
		height = ceil((double)win_w/items_per_row);
	}
	style = g_strdup_printf(GL_TL_CONTENT_FORMAT,height);

	return style;
}


int _gl_timeline_create_grid_view(gl_timeline_s *timeline_d, Evas_Object *genlist, bool update)
{
	GL_CHECK_VAL(timeline_d, -1);
	int item_cnt = 0;
	int ret = -1;
	int i = 0;
	int k = 0;

	/* Check media-content to show */
	ret = _gl_data_get_item_cnt(GL_ALBUM_ALL_ID, GL_STORE_T_ALL, &item_cnt);
	if (ret != 0 || item_cnt == 0)
		gl_dbgE("Empty!");

	Eina_List *list = NULL;
	if (!update) {
		_gl_data_get_items(-1, -1, &list);
		_gl_time_finialize_media_data(timeline_d);
		if (timeline_d->view_m == GL_TL_VIEW_EDIT) {
			_gl_time_update_selected_media(timeline_d, list);
		}
		timeline_d->data_list = list;
		timeline_d->delete_data = false;
	} else {
		list = timeline_d->data_list;
		timeline_d->delete_data = false;
	}

	if (list == NULL) {
		__gl_timelne_show_nocontents(timeline_d);
		elm_naviframe_item_title_enabled_set(timeline_d->nf_it, EINA_FALSE, EINA_FALSE);
		return -1;
	} else {
		__gl_timelne_del_nocontents(timeline_d);
		elm_naviframe_item_title_enabled_set(timeline_d->nf_it, EINA_TRUE, EINA_TRUE);
	}
	if ((!update || !timeline_d->is_list_reverse) && timeline_d->time_media_display_order == TIME_ORDER_ASC) {
		list = eina_list_reverse(list);
		timeline_d->is_list_reverse = true;
	} else if (timeline_d->is_list_reverse && timeline_d->time_media_display_order == TIME_ORDER_DESC) {
		list = eina_list_reverse(list);
		timeline_d->is_list_reverse = false;
	}

	item_cnt = eina_list_count(list);
	timeline_d->count = item_cnt;
	struct tm ct;
	time_t ctime = 0;
	memset(&ct, 0x00, sizeof(struct tm));
	time(&ctime);
	localtime_r(&ctime, &ct);
	gl_media_s *item = NULL;
	char *style_name = NULL;

	Elm_Genlist_Item_Class *pGenGridItc;
	Elm_Genlist_Item_Class *pGenGridZoomItc;
	pGenGridItc = elm_genlist_item_class_new();
	if (!pGenGridItc) {
		gl_dbg("failed to create genlist item class");
		return -1;
	}

	style_name = _gl_time_get_genlist_style(timeline_d->ad);

	pGenGridItc->item_style = style_name;
	pGenGridItc->func.text_get = NULL;
	pGenGridItc->func.content_get =
			(void *) __gl_get_icon_of_grid_cb;
	pGenGridItc->func.state_get = NULL;
	pGenGridItc->func.del = NULL;
	pGenGridItc->decorate_item_style = NULL;

	pGenGridZoomItc = elm_genlist_item_class_new();
	if (!pGenGridZoomItc) {
		gl_dbg("failed to create genlist item class");
		elm_genlist_item_class_free(pGenGridItc);
		return -1;
	}

	pGenGridZoomItc->item_style = "genlist_group_date";
	pGenGridZoomItc->func.text_get = __gl_time_date_get_text;
	pGenGridZoomItc->func.content_get = NULL;
	pGenGridZoomItc->func.state_get = NULL;
	pGenGridZoomItc->func.del = NULL;

	evas_object_data_set(genlist, "data", (void *)timeline_d);
	int number_of_items_on_particular_date = -1;
	int items_per_row = 0;
	int number_of_rows = 0;
	Elm_Object_Item *it = NULL;
	int item_index = 0;

	char *text = NULL;
	for (i = 0; i < item_cnt; i++) {
		item = eina_list_nth(list, i);

		number_of_items_on_particular_date = _get_count_of_items_on_same_date(item, list, &text, i+1);
		items_per_row = _gl_time_get_number_of_items_per_row(timeline_d->ad);
		if (items_per_row < 0) {
			items_per_row = 4;
		}
		number_of_rows = ceil((double)number_of_items_on_particular_date/items_per_row);
		it = elm_genlist_item_append(genlist, pGenGridZoomItc, (void*)text, NULL, ELM_GENLIST_ITEM_TREE, NULL, NULL);

		for (k = 0; k < number_of_rows; k++) {
			elm_genlist_item_append(genlist, pGenGridItc, (void*)item_index, it, ELM_GENLIST_ITEM_NONE, NULL, NULL);
			if (((k * items_per_row) + items_per_row) <= number_of_items_on_particular_date) {
				item_index = item_index + items_per_row;
			} else {
				int temp = ((k * items_per_row) + items_per_row) - number_of_items_on_particular_date;
				item_index = item_index + items_per_row - temp;
			}
		}
		i = (i + number_of_items_on_particular_date) - 1;
	}
	elm_genlist_item_class_free(pGenGridItc);
	elm_genlist_item_class_free(pGenGridZoomItc);

	return 0;
}

Evas_Object *_gl_box_add(Evas_Object *parent)
{
	Evas_Object *box = NULL;
	box = elm_box_add(parent);
	elm_box_align_set(box,0.0, 0.0);
	evas_object_show(box);

	return box;
}

static void __gl_timeline_genlist_move_cb(void *data, Evas_Object *obj, void *ei)
{
	GL_CHECK(data);
	gl_dbg("");
	gl_timeline_s *timeline_d = (gl_timeline_s *)data;

	if (timeline_d->realized_item) {
		Evas_Object *grid = elm_object_item_part_content_get(timeline_d->realized_item, "elm.swallow");
		if (grid) {
			Elm_Object_Item *it = elm_gengrid_first_item_get(grid);
			if (it) {
				gl_media_s *item = (gl_media_s *)elm_object_item_data_get(it);
				char *text1 = NULL;
				char *text2 = NULL;
				struct tm t1;
				struct tm t2;
				struct tm ct;
				time_t ctime = 0;
				memset(&ct, 0x00, sizeof(struct tm));
				time(&ctime);
				localtime_r(&ctime, &ct);
				time_t mtime1 = 0;
				time_t mtime2 = 0;
				mtime1 = item->mtime;
				mtime2 = item->mtime;

				__gl_timeline_get_tm(mtime1, mtime2, &t1, &t2);
				__gl_timeline_get_mtime_str(t1, ct, &text1, &text2);

				if (text1) {
					if (timeline_d->date_layout) {
						elm_object_part_text_set(timeline_d->date_layout, "text", text1);
						elm_object_part_content_set(timeline_d->parent, "elm.swallow.date.layout", timeline_d->date_layout);
					}
				}
			}
		}
	}
}

static void __gl_timeline_genlist_move_stop_cb(void *data, Evas_Object *obj, void *ei)
{
	gl_dbg("Entry");
	GL_CHECK(data);
	gl_timeline_s *timeline_d = (gl_timeline_s *)data;
	if (timeline_d->date_layout) {
		elm_object_part_content_unset(timeline_d->parent, "elm.swallow.date.layout");
		evas_object_hide(timeline_d->date_layout);
	}
	gl_dbg("Exit");
}

Evas_Object *_gl_genlist_add(Evas_Object *parent)
{
	Evas_Object *genlist = NULL;
	genlist = elm_genlist_add(parent);
	elm_object_style_set(genlist, "handler");
	evas_object_size_hint_align_set(genlist, EVAS_HINT_FILL, EVAS_HINT_FILL);
	evas_object_size_hint_weight_set(genlist, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	elm_scroller_bounce_set(genlist, EINA_FALSE, EINA_TRUE);
	elm_scroller_policy_set(genlist, ELM_SCROLLER_POLICY_OFF, ELM_SCROLLER_POLICY_AUTO);
	elm_genlist_mode_set(genlist, ELM_LIST_COMPRESS);
	elm_scroller_single_direction_set(genlist, ELM_SCROLLER_SINGLE_DIRECTION_HARD);
	evas_object_show(genlist);

	return genlist;
}

static void __gl_timeline_genlist_realized(void *data, Evas_Object *obj, void *ei)
{
	gl_dbg("realize_gl_timeline_create_grid_viewd");
	GL_CHECK(ei);
	GL_CHECK(data);
	gl_timeline_s *timeline_d = (gl_timeline_s *)data;
	Elm_Object_Item *it = (Elm_Object_Item *)ei;
	timeline_d->realized_item = it;
}

static Evas_Object *__gl_timeline_create_list_view(gl_timeline_s *timeline_d, bool update)
{
	GL_CHECK_NULL(timeline_d);
	Evas_Object *genlist = NULL;
	Evas_Object *layout = NULL;

	if (timeline_d->view) {
		evas_object_del(timeline_d->view);
	}

	genlist = _gl_genlist_add(timeline_d->parent);
	GL_CHECK_NULL(genlist);
	evas_object_smart_callback_add(genlist, "vbar,drag", __gl_timeline_genlist_move_cb, timeline_d);
	evas_object_smart_callback_add(genlist, "scroll,anim,stop", __gl_timeline_genlist_move_stop_cb, timeline_d);
	evas_object_smart_callback_add(genlist, "scroll,drag,stop", __gl_timeline_genlist_move_stop_cb, timeline_d);
	evas_object_smart_callback_add(genlist, "realized", __gl_timeline_genlist_realized, timeline_d);

	layout = elm_layout_add(timeline_d->parent);
	GL_CHECK_NULL(layout);
	elm_layout_file_set(layout, GL_EDJ_FILE, "timeline_gridview_date_toast");
	timeline_d->date_layout = layout;

	if (_gl_timeline_create_grid_view(timeline_d, genlist, update) < 0) {
		return NULL;
	}

	return genlist;
}

#if 0
static int __gl_timeline_page_rand(gl_timeline_s *timeline_d)
{
	GL_CHECK_VAL(timeline_d, -1);
	int item_cnt = timeline_d->count;
	int i = 0;
	gl_tlp_s *it_d = NULL;
	int index = 0;

	GL_TL_CLEAR_PAGES(timeline_d, true);

	srand(time(NULL));
	int cnt = 0;
	bool b_new_count = false;
	int list_cnt = eina_list_count(timeline_d->count_list);
	gl_dbg("list_cnt: %d", list_cnt);

	for (i = 0; i < item_cnt;) {
		it_d = (gl_tlp_s *)calloc(1, sizeof(gl_tlp_s));
		timeline_d->list = eina_list_append(timeline_d->list, it_d);
		it_d->timeline_d = timeline_d;
		if (list_cnt > index) {
			/* Reuse count of page */
			cnt = (int)eina_list_nth(timeline_d->count_list,
						 index);
		} else {
			/* Add new random count */
			cnt = 1 + rand() % 5;
			b_new_count = true;
		}

		it_d->count = __gl_timeline_update_tiles_cnt(timeline_d->zoom_level,
							     cnt);
		it_d->index = index++;
		it_d->start_pos = i;
		/* Refer to first item of next loop */
		i += it_d->count;
		it_d->end_pos = i - 1;
		/* Reset count and end_pos of last page */
		if (timeline_d->zoom_level != GL_TM_ZOOM_OUT_TWO &&
		    i >= item_cnt) {
			it_d->end_pos = item_cnt - 1;
			it_d->count = it_d->end_pos - it_d->start_pos + 1;
			if (b_new_count)
				timeline_d->count_list = eina_list_append(timeline_d->count_list,
									  (void *)it_d->count);
			break;
		}
		if (b_new_count)
			timeline_d->count_list = eina_list_append(timeline_d->count_list,
								  (void *)it_d->count);
		it_d = NULL;
	}
	return 0;
}

static int __gl_timeline_update_view(gl_timeline_s *timeline_d, bool b_first)
{
	GL_CHECK_VAL(timeline_d, -1);
	int item_cnt = 0;
	time_t last_mtime = 0;
	int ret = -1;

	/* Check media-content to show */
	ret = _gl_data_get_item_cnt(GL_ALBUM_ALL_ID, GL_STORE_T_ALL, &item_cnt);
	if (ret != 0 || item_cnt == 0)
		gl_dbgE("Empty!");

	Eina_List *list = NULL;
	_gl_data_get_items(0, 0, &list);
	if (list == NULL) {
		GL_TL_CLEAR_PAGES(timeline_d, true);
		__gl_timelne_show_nocontents(timeline_d);
		elm_naviframe_item_title_enabled_set(timeline_d->nf_it, EINA_FALSE, EINA_FALSE);
		return -1;
	} else {
		__gl_timelne_del_nocontents(timeline_d);
		elm_naviframe_item_title_enabled_set(timeline_d->nf_it, EINA_TRUE, EINA_TRUE);
	}

	gl_media_s *item = eina_list_nth(list, 0);
	if (item)
		last_mtime = item->mtime;

	item = NULL;
	EINA_LIST_FREE(list, item) {
		_gl_data_type_free_glitem((void **)(&item));
		item = NULL;
	}

	if (!b_first) {
		if (last_mtime == timeline_d->last_mtime &&
		    item_cnt == timeline_d->count) {
			gl_dbgW("Nothing changed!");
			evas_object_show(timeline_d->view); /* To show slide items */
			__gl_timeline_show_tiles_trans(timeline_d);
			return -1;
		}
	}

	timeline_d->count = item_cnt;
	timeline_d->last_mtime = last_mtime;
	/* Set rand image count for each page */
	__gl_timeline_page_rand(timeline_d);
	/* Reset zoom level for overscrolling showing in slider */
	if (timeline_d->zoom_level == GL_TM_ZOOM_DEFAULT)
		_gl_slider_set_zlevel(timeline_d->view, GL_SLIDE_ZLEVEL_MAX);
	else if (timeline_d->zoom_level == GL_TM_ZOOM_DEFAULT)
		_gl_slider_set_zlevel(timeline_d->view, GL_SLIDE_ZLEVEL_MIN);
	else
		_gl_slider_set_zlevel(timeline_d->view, GL_SLIDE_ZLEVEL_MID);

	_gl_slider_set_list(timeline_d->view, timeline_d->list, 0);
	evas_object_show(timeline_d->view); /* To show slide items */
	if (timeline_d->b_created)
		_gl_slider_start(timeline_d->view);
	return 0;
}
#endif

/* Free data after layout deleted */
static void __gl_timeline_del_layout_cb(void *data, Evas *e, Evas_Object *obj,
					void *ei)
{
	gl_dbg("Delete timeline layout ---");
	evas_object_data_del(obj, "page_w");
	evas_object_data_del(obj, "page_h");
	if (data) {
		gl_timeline_s *timeline_d = (gl_timeline_s *)data;
		GL_IF_DEL_ANIMATOR(timeline_d->animator);
		GL_IF_FREE_ELIST(timeline_d->count_list);
		GL_IF_FREE_ELIST(timeline_d->thumbs);
		if (timeline_d->album) {
			_gl_data_util_free_gcluster(timeline_d->album);
			timeline_d->album = NULL;
		}
		__gl_timeline_clear_sel_list(timeline_d);
		GL_GFREEIF(timeline_d->sel_d);
		_gl_time_finialize_media_data(timeline_d);
		GL_FREE(data);
	}
	gl_dbg("Delete timeline layout +++");
}

static int __gl_timeline_add_cbs(gl_timeline_s *timeline_d)
{
	GL_CHECK_VAL(timeline_d, -1);
	GL_CHECK_VAL(timeline_d->ad, -1);

	/* Add callback for title updating */
	__gl_timeline_add_title_trans_finished_cbs(timeline_d, true);
	/* Register delete callback */
	evas_object_event_callback_add(timeline_d->layout, EVAS_CALLBACK_DEL,
				       __gl_timeline_del_layout_cb, timeline_d);
	evas_object_event_callback_add(timeline_d->layout, EVAS_CALLBACK_RESIZE,
				       __gl_timeline_resize_layout_cb,
				       timeline_d);
	int bx = 0;
	int by = 0;
	int bw = 0;
	int bh = 0;
	evas_object_geometry_get(timeline_d->ad->maininfo.naviframe, &bx, &by,
				 &bw, &bh);
	gl_dbg("naviframe. (%d,%d,%d,%d)", bx, by, bw, bh);
	evas_object_geometry_get(timeline_d->layout, &bx, &by, &bw, &bh);
	gl_dbg("layout. (%d,%d,%d,%d)", bx, by, bw, bh);
	return 0;
}

#ifdef SUPPORT_SLIDESHOW
static int __gl_timeline_start_slideshow(void *data)
{
	GL_CHECK_VAL(data, -1);
	gl_appdata *ad = (gl_appdata *)data;
	ad->gridinfo.grid_type = GL_GRID_T_ALLALBUMS;

	Eina_List *list = NULL;
	gl_item *gitem = NULL;
	gl_media_s *item = NULL;
	_gl_data_get_items(GL_FIRST_VIEW_START_POS,
			   GL_FIRST_VIEW_START_POS, &list);
	if (list != NULL) {
		item = (gl_media_s *)eina_list_nth(list, 0);
		GL_IF_FREE_ELIST(list);
		gitem = _gl_data_new_item_mitem(item);
		GL_CHECK_VAL(gitem, -1);
		gl_ext_load_iv_ug(data, gitem, GL_UG_IV_SLIDESHOW_ALL);
		_gl_data_util_free_gitem(gitem);
	}
	return 0;
}

static int __gl_timeline_slideshow(void *data, const char *label)
{
	GL_CHECK_VAL(label, -1);
	GL_CHECK_VAL(data, -1);
	gl_dbg("label : %s ", label);
	gl_appdata *ad = (gl_appdata *)data;

	if (!g_strcmp0(label, _gl_str(GL_STR_ID_ALL_ITEMS))) {
		__gl_timeline_start_slideshow(data);
	} else if (!g_strcmp0(label, _gl_str(GL_STR_ID_SETTINGS))) {
		evas_object_data_set(ad->maininfo.naviframe,
				     GL_NAVIFRAME_SLIDESHOW_DATA_KEY,
				     __gl_timeline_start_slideshow);
		gl_ext_load_ug(data, GL_UG_GALLERY_SETTING_SLIDESHOW);
	} else if (!g_strcmp0(label, _gl_str(GL_STR_ID_SELECT_ITEMS))) {
#ifdef _USE_APP_SLIDESHOW
		__gl_timeline_edit(data);
		evas_object_data_set(ad->maininfo.naviframe,
				     GL_NAVIFRAME_SELECTED_SLIDESHOW_KEY,
				     gl_pop_to_ctrlbar_ly);
#else
		_gl_ext_launch_gallery_ug(data);
#endif
	} else {
		gl_dbgE("Invalid lable!");
		return -1;
	}
	return 0;
}

static void __gl_timeline_slideshow_cb(void *data, Evas_Object *obj, void *ei)
{
	GL_CHECK(data);
	_gl_ctxpopup_del(data);
	_gl_popup_create_slideshow(data, __gl_timeline_slideshow);
}
#endif

#ifdef _USE_APP_SLIDESHOW
static Eina_Bool __gl_timeline_edit_cancel_cb(void *data, Elm_Object_Item *it)
{
	GL_CHECK_FALSE(data);
	gl_dbg("");
	gl_set_view_mode(data, GL_VIEW_TIMELINE);
	gl_appdata *ad = (gl_appdata *)data;
	if (_gl_timeline_update_view(data) < 0) {
		if (ad->tlinfo->nocontents == NULL) {
			ad->tlinfo->tran_op = GL_TRANS_OP_DEFAULT;
			_gl_slider_update_first_item(ad->tlinfo->view);
		}

	}
	return EINA_TRUE;
}

static int __gl_timeline_edit(void *data)
{
	GL_CHECK_VAL(data, -1);
	gl_appdata *ad = (gl_appdata *)data;
	int item_cnt = 0;
	int ret = -1;

	ret = _gl_data_get_item_cnt(GL_ALBUM_ALL_ID, GL_STORE_T_ALL, &item_cnt);
	if (ret != 0 || item_cnt == 0) {
		gl_dbgE("Empty!");
		return -1;
	}

	if (ad->tlinfo->album == NULL)
		ad->tlinfo->album = _gl_data_util_new_gcluster_all(data,
								   item_cnt);
	GL_CHECK_VAL(ad->tlinfo->album, -1);
	_gl_albums_set_current(data, ad->tlinfo->album);
	Eina_List *medias_elist = NULL;
	_gl_data_get_items_album(data, ad->tlinfo->album,
				 GL_FIRST_VIEW_START_POS, GL_FIRST_VIEW_END_POS,
				 &medias_elist);
	_gl_thumbs_set_list(ad, medias_elist);
	_gl_thumbs_set_edit_mode(data, GL_THUMBS_EDIT_SLIDESHOW);
	_gl_thumbs_create_view(data, GL_NAVI_THUMBS, GL_STR_ID_ALL_ALBUMS, true,
			       __gl_timeline_edit_cancel_cb);

	gl_dbg("Done edit");
	return 0;
}
#endif

void __gl_timeline_desc_mode_set(void *data) {
	gl_dbg("ENTRY");
	GL_CHECK(data);
	gl_appdata *ad = (gl_appdata *)data;
	if (ad->tlinfo->time_media_display_order == TIME_ORDER_DESC) {
		gl_dbg("EXIT 1");
		return;
	}
	ad->tlinfo->time_media_display_order = TIME_ORDER_DESC;
	__gl_update_timeline(ad);
}

void __gl_timeline_asc_mode_set(void *data) {
	gl_dbg("ENTRY");
	GL_CHECK(data);
	gl_appdata *ad = (gl_appdata *)data;
	if (ad->tlinfo->time_media_display_order == TIME_ORDER_ASC) {
		gl_dbg("EXIT 1");
		return;
	}
	ad->tlinfo->time_media_display_order = TIME_ORDER_ASC;
	__gl_update_timeline(ad);
}

static void __gl_timeline_sort_cb(void *data, Evas_Object *obj, void *ei)
{
	gl_dbg("ENTRY");
	GL_CHECK(data);
	gl_appdata *ad = (gl_appdata *)data;
	int state_index = -1;
	state_index = ad->tlinfo->time_media_display_order == TIME_ORDER_ASC ? 1 : 0;
	_gl_list_pop_create(data, obj, ei, GL_STR_SORT, GL_STR_DATE_MOST_RECENT, GL_STR_DATE_OLDEST, state_index);
	gl_dbg("EXIT");
}

static void __gl_timeline_edit_cb(void *data, Evas_Object *obj, void *ei)
{
	GL_CHECK(data);
	gl_appdata *ad = (gl_appdata *)data;
	_gl_ctxpopup_del(data);
	if (ad->uginfo.ug) {
		/**
		* Prevent changed to edit view in wrong way.
		* 1. When invoke imageviewer UG;
		*/
		gl_dbgW("UG invoked!");
		return;
	}
	__gl_timeline_change_mode(data, GL_TL_VIEW_EDIT);
}

static void __gl_albums_viewas_pop_cb(void *data, Evas_Object *obj, void *ei)
{
	gl_dbg("ENTRY");
	GL_CHECK(data);
	gl_appdata *ad = (gl_appdata *)data;
	int state_index = 1; // default is album
	int view_mode = gl_get_view_mode(ad);
	if (view_mode == GL_VIEW_TIMELINE) {
		state_index = 0;
	} else if (view_mode == GL_VIEW_ALBUMS) {
		state_index = 1;
	}
	_gl_list_pop_create(data, obj, ei, GL_STR_ID_VIEW_AS, GL_STR_TIME_VIEW, GL_STR_ALBUMS, state_index);
	gl_dbg("EXIT");
}

#if 0
static void __gl_timeline_share_cb(void *data, Evas_Object *obj, void *ei)
{
	GL_CHECK(data);
	_gl_ctxpopup_del(data);
	gl_appdata *ad = (gl_appdata *)data;
	if (ad->uginfo.ug) {
		/**
		* Prevent changed to edit view in wrong way.
		* 1. When invoke imageviewer UG;
		*/
		gl_dbgW("UG invoked!");
		return;
	}
	__gl_timeline_change_mode(data, GL_TL_VIEW_SHARE);
}
#endif

/* 'Delete medias' is available in Dali view */
static int __gl_timeline_del_op(void *data)
{
	GL_CHECK_VAL(data, -1);
	gl_appdata *ad = (gl_appdata *)data;
	GL_CHECK_VAL(ad->tlinfo, -1);
	gl_timeline_s *timeline_d = ad->tlinfo;
	GL_CHECK_VAL(timeline_d->sel_d, -1);
	int i = 0;
	int popup_op = GL_POPUP_OP_NONE;
	char *item = NULL;
	int ret = -1;

	EINA_LIST_FREE(timeline_d->sel_d->sel_list, item) {
		i++;
		if (item) {
			ret = _gl_del_media_by_id(data, (const char *)item);
			if (ret != 0)
				gl_dbgE("Get media failed[%d]!", ret);
			item = NULL;
		}

		gl_dbg("Write pipe, make progressbar updated!");
		gl_thread_write_pipe(ad, i, popup_op);
		popup_op = GL_POPUP_OP_NONE;
	}
	timeline_d->sel_d->sel_list = NULL;
	return 0;
}

/* Update view after deleting process done */
static int __gl_timeline_update_del_view(void *data)
{
	GL_CHECK_VAL(data, -1);
	gl_dbgW("");

	__gl_timeline_change_mode(data, GL_TL_VIEW_NORMAL);
	/* Update view */
	_gl_timeline_update_view(data);
	/* Add notification */
	_gl_notify_create_notiinfo(GL_STR_DELETED);
	/* Add albums list */
	_gl_update_albums_list(data);
	_gl_db_update_lock_always(data, false);
	return 0;
}

#if 0
static int __gl_timeline_get_path_str(void *data, gchar sep_c, char **path_str,
				      int *sel_cnt)
{
	GL_CHECK_VAL(path_str, -1);
	GL_CHECK_VAL(data, -1);
	gl_appdata *ad = (gl_appdata *)data;
	GL_CHECK_VAL(ad->tlinfo, -1);
	gl_timeline_s *timeline_d = ad->tlinfo;
	GL_CHECK_VAL(timeline_d->sel_d, -1);
	GString *selected_path_list = g_string_new(NULL);
	int selected_item_cnt = 0;
	Eina_List *l = NULL;
	char *item = NULL;
	int ret = -1;
	char *path = NULL;

	EINA_LIST_FOREACH(timeline_d->sel_d->sel_list, l, item) {
		if (item == NULL) {
			gl_dbgE("Invalid item!");
			goto GL_TL_FAILED;
		}

		ret = _gl_local_data_get_path_by_id((const char *)item, &path);
		if (ret != 0 || !path) {
			gl_dbgE("Get path failed[%d]!", ret);
			goto GL_TL_FAILED;
		}

		if (strstr(selected_path_list->str, path)) {
			gl_dbgW("Already appended!");
		} else {
			selected_item_cnt++;
			g_string_append(selected_path_list, path);
			g_string_append_c(selected_path_list, sep_c);
		}
		GL_FREE(path);
	}
	gl_dbg("Selected items count: %d.", selected_item_cnt);
	if (sel_cnt)
		*sel_cnt = selected_item_cnt;
	int len = strlen(selected_path_list->str);
	g_string_truncate(selected_path_list, len - 1);
	/**
	* Frees the memory allocated for the GString.
	* If free_segment is true it also frees the character data.
	* If it's false, the caller gains ownership of the buffer
	* and must free it after use with g_free().
	*/
	*path_str = g_string_free(selected_path_list, false);
	GL_CHECK_VAL(*path_str, -1);
	gl_dbg("Total string:\n\n\t>>@@:> %s <:@@<<\n", *path_str);
	return 0;

 GL_TL_FAILED:

	if (selected_path_list) {
		g_string_free(selected_path_list, true);
		selected_path_list = NULL;
	}
	GL_FREEIF(path);
	return -1;
}

/* To launch Image-editor*/
static void __gl_timeline_ie_cb(void *data, Evas_Object *obj, void *ei)
{
	GL_CHECK(data);
	gl_appdata *ad = (gl_appdata *)data;
	gl_dbg("");

	if (ad->uginfo.ug || ad->uginfo.b_app_called) {
		gl_dbgW("UG or APP is already loaded!");
		return;
	}
	_gl_ctxpopup_del(data);

	GL_CHECK(ad->tlinfo);
	gl_timeline_s *timeline_d = ad->tlinfo;
	if (__gl_timeline_get_sel_cnt(timeline_d) == 0) {
		gl_dbgW("No thumbs selected!");
		gl_popup_create_popup(ad, GL_POPUP_NOBUT,
				      GL_STR_NO_FILES_SELECTED);
		return;
	}

	_gl_ext_load_ie(data, __gl_timeline_get_path_str);
	/* Change to normal mode */
	__gl_timeline_change_mode(data, GL_TL_VIEW_NORMAL);
}

/* 'Move medias' is only available in tab Albums */
static int __gl_timeline_move_copy_op(void *data)
{
	GL_CHECK_VAL(data, -1);
	gl_appdata *ad = (gl_appdata *)data;
	GL_CHECK_VAL(ad->tlinfo, -1);
	gl_timeline_s *timeline_d = ad->tlinfo;
	GL_CHECK_VAL(timeline_d->sel_d, -1);
	int i = 0;
	int popup_op = GL_POPUP_OP_NONE;
	char *item = NULL;

	EINA_LIST_FREE(timeline_d->sel_d->sel_list, item) {
		i++;
		if (item) {
			gl_sdbg("Selected [%s]", item);
			if (ad->maininfo.medias_op_type == GL_MEDIA_OP_COPY_TIMELINE) {
				if (_gl_move_media_thumb_by_id(data, (const char *)item, ad->albuminfo.dest_folder, &popup_op, GL_MC_COPY) != 0)
					gl_dbg("Failed to copy this item");
			} else if (ad->maininfo.medias_op_type == GL_MEDIA_OP_MOVE_TIMELINE) {
				if (_gl_move_media_thumb_by_id(data, (const char *)item, ad->albuminfo.dest_folder, &popup_op, GL_MC_MOVE) != 0)
					gl_dbg("Failed to move this item");
			} else {
				gl_dbgE("Wrong mode!");
			}
			item = NULL;
		}

		gl_dbg("Write pipe, make progressbar updated!");
		gl_thread_write_pipe(ad, i, popup_op);
		popup_op = GL_POPUP_OP_NONE;
	}
	timeline_d->sel_d->sel_list = NULL;
	return 0;
}

/* Update view after moving process done */
static int __gl_timeline_update_move_copy_view(void *data)
{
	GL_CHECK_VAL(data, -1);

	gl_appdata *ad = (gl_appdata *)data;
	const char *noti_str = NULL;
	if (ad->maininfo.medias_op_type == GL_MEDIA_OP_COPY_TIMELINE) {
		noti_str = GL_STR_COPIED;
	} else if (ad->maininfo.medias_op_type == GL_MEDIA_OP_MOVE_TIMELINE) {
		noti_str = GL_STR_MOVED;
	} else {
		gl_dbgE("Wrong mode!");
	}
	if (noti_str)
		_gl_notify_create_notiinfo(noti_str);
	elm_naviframe_item_pop_to(ad->tlinfo->nf_it);
	__gl_timeline_change_mode(data, GL_TL_VIEW_NORMAL);
	/* Update view */
	_gl_timeline_update_view(data);
	/* Update albums list and items list */
	_gl_update_albums_list(ad);
	_gl_db_update_lock_always(data, false);
	return 0;
}

static int __gl_timeline_move_copy(void *data)
{
	GL_CHECK_VAL(data, -1);
	gl_appdata *ad = (gl_appdata *)data;
	char folder_fullpath[GL_DIR_PATH_LEN_MAX] = { 0, };
	GL_CHECK_VAL(ad->tlinfo, -1);
	gl_timeline_s *timeline_d = ad->tlinfo;
	int cnt = __gl_timeline_get_sel_cnt(timeline_d);
	gl_dbg("");

	_gl_db_update_lock_always(data, true);
	/**
	 * 'move_album_id == NULL' is new album case,
	 * other move/save to some existed album.
	 */
	if (ad->albuminfo.path == NULL) {
		gl_dbg("New album");
		if (gl_make_new_album(ad->albuminfo.new_name) != 0) {
			gl_popup_create_popup(ad, GL_POPUP_NOBUT,
					      GL_STR_SAME_NAME_ALREADY_IN_USE);
			gl_dbgE("Failed to make a new directory!");
			goto GL_FAILED;
		}
		snprintf(folder_fullpath, GL_DIR_PATH_LEN_MAX, "%s/%s",
			 GL_DEFAULT_PATH_IMAGES, ad->albuminfo.new_name);
	} else {
		g_strlcpy(folder_fullpath, ad->albuminfo.path,
			  GL_DIR_PATH_LEN_MAX);
		GL_FREE(ad->albuminfo.path);
	}

	memset(ad->albuminfo.dest_folder, 0x00, GL_DIR_PATH_LEN_MAX);
	g_strlcpy(ad->albuminfo.dest_folder, folder_fullpath,
		  GL_DIR_PATH_LEN_MAX);

	/* Check MMC state for cancel operation */
	gl_check_mmc_state(ad, folder_fullpath);
	gl_dbg("MMC state: %d.", ad->maininfo.mmc_state);
	_gl_set_file_op_cbs(data, __gl_timeline_move_copy_op, NULL,
			    __gl_timeline_update_move_copy_view, cnt);

	if (ad->albuminfo.file_mc_mode == GL_MC_MOVE)
		_gl_use_thread_operate_medias(ad, GL_STR_ID_MOVING, cnt,
					      GL_MEDIA_OP_MOVE_TIMELINE);
	else if (ad->albuminfo.file_mc_mode == GL_MC_COPY)
		_gl_use_thread_operate_medias(ad, GL_STR_ID_COPYING, cnt,
					      GL_MEDIA_OP_COPY_TIMELINE);
	else
		gl_dbgE("Wrong mode!");
	return 0;

 GL_FAILED:

	/* Update the label text */
	_gl_ui_update_navi_title_text(timeline_d->nf_it, cnt, false);
	__gl_timeline_check_btns_state(timeline_d, timeline_d->count, cnt);
	_gl_db_update_lock_always(data, false);
	return -1;
}

/* move media to album in edit view */
static void __gl_timeline_move_cb(void *data, Evas_Object *obj, void *ei)
{
	elm_toolbar_item_selected_set((Elm_Object_Item *)ei, EINA_FALSE);
	GL_CHECK(data);
	gl_appdata *ad = (gl_appdata *)data;
	GL_CHECK(ad->tlinfo);
	gl_timeline_s *timeline_d = ad->tlinfo;
	gl_dbg("");

	if (__gl_timeline_get_sel_cnt(timeline_d) == 0) {
		gl_dbgW("No thumbs selected!");
		gl_popup_create_popup(data, GL_POPUP_NOBUT,
				      GL_STR_NO_FILES_SELECTED);
		return;
	}

	ad->albuminfo.file_mc_mode = GL_MC_MOVE;
	_gl_popup_create_move(data, __gl_timeline_move_copy);
}

/* move media to album in edit view */
static void __gl_timeline_copy_cb(void *data, Evas_Object *obj, void *ei)
{
	GL_CHECK(data);
	_gl_ctxpopup_del(data);
	gl_appdata *ad = (gl_appdata *)data;
	GL_CHECK(ad->tlinfo);
	gl_timeline_s *timeline_d = ad->tlinfo;
	gl_dbg("");

	if (__gl_timeline_get_sel_cnt(timeline_d) == 0) {
		gl_dbgW("No thumbs selected!");
		gl_popup_create_popup(data, GL_POPUP_NOBUT,
				      GL_STR_NO_FILES_SELECTED);
		return;
	}

	ad->albuminfo.file_mc_mode = GL_MC_COPY;
	_gl_popup_create_copy(data, __gl_timeline_move_copy);
}
#endif

#ifdef _USE_ROTATE_BG
static int __gl_timeline_rotate_op(void *data)
{
#define GL_ROTATE_DELAY 0.25
	GL_CHECK_VAL(data, -1);
	gl_appdata *ad = (gl_appdata *)data;
	GL_CHECK_VAL(ad->tlinfo, -1);
	gl_timeline_s *timeline_d = ad->tlinfo;
	GL_CHECK_VAL(timeline_d->sel_d, -1);
	int i = 0;
	int popup_op = GL_POPUP_OP_NONE;
	int ret = -1;
	bool b_left = false;
	char *item = NULL;
	int cnt = __gl_timeline_get_sel_cnt(timeline_d);

	if (ad->maininfo.medias_op_type == GL_MEDIA_OP_ROTATING_LEFT_TIMELINE)
		b_left = true;

	EINA_LIST_FREE(timeline_d->sel_d->sel_list, item) {
		i++;
		if (item) {
			ret = _gl_rotate_image_by_id(data, (const char *)item,
						     b_left);
			if (ret != 0)
				gl_dbgE("Rotate image failed[%d]!", ret);

			/* Add some delay for last two images to wait for thumb updated */
			if (i > cnt - 2)
				_gl_delay(GL_ROTATE_DELAY);

			if (i == cnt) {
				gl_dbgW("Last image rotated!");
				/* Add another delay for last thumb */
				_gl_delay(GL_ROTATE_DELAY);
			}
			item = NULL;
		}

		gl_dbg("Write pipe, make progressbar updated!");
		gl_thread_write_pipe(ad, i, popup_op);
		popup_op = GL_POPUP_OP_NONE;
	}
	timeline_d->sel_d->sel_list = NULL;
	return 0;
}

static int __gl_timeline_update_rotate_view(void *data)
{
	GL_CHECK_VAL(data, -1);
	gl_dbgW("");

	__gl_timeline_change_mode(data, GL_TL_VIEW_NORMAL);
	_gl_db_update_lock_always(data, false);
	return 0;
}
static int __gl_timeline_rotate_images(void *data, bool b_left)
{
	gl_dbg("");
	GL_CHECK_VAL(data, -1);
	gl_appdata *ad = (gl_appdata *)data;
	GL_CHECK_VAL(ad->tlinfo, -1);
	gl_timeline_s *timeline_d = ad->tlinfo;

	/* Get all selected medias count */
	int cnt = __gl_timeline_get_sel_cnt(timeline_d);
	/* Check MMC state for cancel operation */
	gl_check_mmc_state(ad, NULL);
	gl_dbg("MMC state: %d.", ad->maininfo.mmc_state);
	/* Rotate left */
	int op_type = GL_MEDIA_OP_ROTATING_LEFT_TIMELINE;
	/* Rotate right */
	if (!b_left)
		op_type = GL_MEDIA_OP_ROTATING_RIGHT_TIMELINE;
	_gl_db_update_lock_always(data, true);
	_gl_set_file_op_cbs(data, __gl_timeline_rotate_op, NULL,
			    __gl_timeline_update_rotate_view, cnt);
	_gl_use_thread_operate_medias(ad, GL_STR_ID_ROTATING, cnt, op_type);

	return 0;
}

static void __gl_timeline_edit_rotate_left_cb(void *data, Evas_Object *obj,
					    void *ei)
{
	GL_CHECK(data);
	_gl_ctxpopup_del(data);

	gl_appdata *ad = (gl_appdata *)data;
	GL_CHECK(ad->tlinfo);
	gl_timeline_s *timeline_d = ad->tlinfo;

	if (__gl_timeline_get_sel_cnt(timeline_d) == 0) {
		gl_dbgW("No thumbs selected!");
		gl_popup_create_popup(data, GL_POPUP_NOBUT,
				      GL_STR_NO_FILES_SELECTED);
		return;
	}
	__gl_timeline_rotate_images(data, true);
}

static void __gl_timeline_edit_rotate_right_cb(void *data, Evas_Object *obj,
					     void *ei)
{
	GL_CHECK(data);
	_gl_ctxpopup_del(data);

	gl_appdata *ad = (gl_appdata *)data;
	GL_CHECK(ad->tlinfo);
	gl_timeline_s *timeline_d = ad->tlinfo;

	if (__gl_timeline_get_sel_cnt(timeline_d) == 0) {
		gl_dbgW("No thumbs selected!");
		gl_popup_create_popup(data, GL_POPUP_NOBUT,
				      GL_STR_NO_FILES_SELECTED);
		return;
	}
	__gl_timeline_rotate_images(data, false);
}
#endif

/* Easy mode: New album/Edit/Slideshow */
static int __gl_timeline_ctxpopup_append(void *data, Evas_Object *parent)
{
	gl_dbg("");
	GL_CHECK_VAL(parent, -1);
	GL_CHECK_VAL(data, -1);
	GL_CHECK_VAL(parent, -1);
	int cnt = 0;
	int ret = -1;
	ret = _gl_data_get_item_cnt(GL_ALBUM_ALL_ID, GL_STORE_T_ALL, &cnt);
	if (ret != 0 || cnt == 0) {
		gl_dbgE("Empty!");
	}

	if (cnt > 0) {
		/* View As */
		_gl_ctxpopup_append(parent, GL_STR_ID_VIEW_AS,
				__gl_albums_viewas_pop_cb, data);
		/* Delete */
		_gl_ctxpopup_append(parent, GL_STR_ID_DELETE,
				    __gl_timeline_edit_cb, data);
		/* Sort */
		_gl_ctxpopup_append(parent, GL_STR_SORT,
				    __gl_timeline_sort_cb, data);
#ifdef SUPPORT_SLIDESHOW
		/* Slide show */
		_gl_ctxpopup_append(parent, GL_STR_ID_SLIDESHOW,
				    __gl_timeline_slideshow_cb, data);
#endif
	}

	return 0;
}

#if 0
static int __gl_timeline_edit_ctxpopup_append(void *data, Evas_Object *parent)
{
	gl_dbg("");
	GL_CHECK_VAL(parent, -1);
	GL_CHECK_VAL(data, -1);
	gl_appdata *ad = (gl_appdata *)data;
	GL_CHECK_VAL(ad->tlinfo, -1);
	GL_CHECK_VAL(ad->tlinfo->sel_d, -1);
	int cnt = __gl_timeline_get_sel_cnt(ad->tlinfo);

	/* Hide items */

#ifdef _USE_ROTATE_BG
	if (ad->tlinfo->sel_d->jpge_cnt == cnt) {
		gl_dbg("Enable more button");
		_gl_ctxpopup_append(parent, GL_STR_ID_ROTATE_LEFT,
				    __gl_timeline_edit_rotate_left_cb,
				    data);
		_gl_ctxpopup_append(parent, GL_STR_ID_ROTATE_RIGHT,
				    __gl_timeline_edit_rotate_right_cb,
				    data);
	}
#endif
	return 0;
}
#endif

static void __gl_timeline_more_cb(void *data, Evas_Object *obj, void *ei)
{
	gl_dbg("more button clicked");
	GL_CHECK(data);
	gl_appdata *ad = (gl_appdata *)data;
	Evas_Object *more = NULL;
	more = _gl_ui_get_btn(NULL, ad->tlinfo->nf_it, GL_NAVIFRAME_MORE);
	if (EINA_TRUE == elm_object_disabled_get(more)) {
		gl_dbg("Menu is disabled");
		return; /* Menu is disabled */
	}

	gl_dbg("Menu is clicked");
	GL_CHECK(ad->tlinfo);
	/* Edit mode */
	if (ad->tlinfo->view_m == GL_TL_VIEW_NORMAL)
		_gl_ctxpopup_create(data, obj, __gl_timeline_ctxpopup_append);
	else
		gl_dbg("Unavailable menu operation");
}

#if 0
static void __gl_timeline_camera_cb(void *data, Evas_Object *obj, void *ei)
{
	GL_CHECK(data);
	gl_dbg("");
	_gl_ext_load_camera(data);
}
#endif

static int __gl_timeline_reset_label(void *data)
{
	GL_CHECK_VAL(data, -1);
	gl_appdata *ad = (gl_appdata *)data;

	if (ad->tlinfo == NULL)
		return -1;
	if (ad->tlinfo->view_m == GL_TL_VIEW_NORMAL) {
		gl_dbg("Normal mode");
		return -1;
	}

	int sel_cnt = __gl_timeline_get_sel_cnt(ad->tlinfo);
	__gl_timeline_check_btns_state(ad->tlinfo, ad->tlinfo->count, sel_cnt);
	/* Update the label text */
	_gl_ui_update_navi_title_text(ad->tlinfo->nf_it, sel_cnt, false);
	return 0;
}

static int __gl_timeline_reset_btns(void *data)
{
	GL_CHECK_VAL(data, -1);
	gl_appdata *ad = (gl_appdata *)data;

	if (ad->tlinfo == NULL)
		return -1;
	/* Check Button menu state in Easymode */
	_gl_ui_disable_menu(ad->tlinfo->nf_it, false);
	return 0;
}

int _gl_time_data_selected_list_finalize(void *data)
{
	GL_CHECK_VAL(data, -1);
	gl_appdata *ad = (gl_appdata *)data;
	gl_dbg("");
	gl_media_s *gitem = NULL;
	GL_CHECK_VAL(ad->tlinfo, -1);
	int i;
	int count;
	if (ad->tlinfo->data_list) {
		count = eina_list_count(ad->tlinfo->data_list);
		for (i = 0; i < count ; i++) {
			gitem = eina_list_nth(ad->tlinfo->data_list, i);
			if (gitem) {
				gitem->check_state = false;
			}
		}
	}

	return 0;
}

static void __gl_timeline_edit_cancel_cb(void *data, Evas_Object *obj, void *ei)
{
	elm_toolbar_item_selected_set((Elm_Object_Item *)ei, EINA_FALSE);
	GL_CHECK(data);
	gl_dbg("");
	__gl_timeline_change_mode(data, GL_TL_VIEW_NORMAL);
}

static int __gl_timeline_add_btns(void *data)
{
	gl_dbg("");
	GL_CHECK_VAL(data, -1);
	gl_appdata *ad = (gl_appdata *)data;
	GL_CHECK_VAL(ad->maininfo.naviframe, -1);
	GL_CHECK_VAL(ad->tlinfo->nf_it, -1);
	Evas_Object *parent = ad->maininfo.naviframe;
	Elm_Object_Item *nf_it = ad->tlinfo->nf_it;

	_gl_ui_clear_btns(data);
	Evas_Object *btn = NULL;

	/* More */
	btn = _gl_but_create_but(parent, NULL, NULL,
				 GL_BUTTON_STYLE_NAVI_MORE,
				 __gl_timeline_more_cb, data);
	GL_CHECK_VAL(btn, -1);
	elm_object_item_part_content_set(nf_it, GL_NAVIFRAME_MORE, btn);
	elm_object_signal_emit(ad->ctrlinfo.ctrlbar_view_ly, "elm,selectall,state,default", "elm");
	return 0;
}

static void __gl_timeline_del_cb(void *data, Evas_Object *obj, void *ei)
{
	elm_toolbar_item_selected_set((Elm_Object_Item *)ei, EINA_FALSE);
	GL_CHECK(data);
	gl_appdata *ad = (gl_appdata *)data;
	GL_CHECK(ad->tlinfo);
	gl_timeline_s *timeline_d = ad->tlinfo;
	/* Get all selected medias count */
	int cnt = __gl_timeline_get_sel_cnt(timeline_d);
	if (cnt == 0) {
		gl_dbgW("No thumbs selected!");
		gl_popup_create_popup(ad, GL_POPUP_NOBUT, GL_STR_NO_FILES_SELECTED);
		return;
	}

	/* Check MMC state for cancel operation */
	gl_check_mmc_state(ad, NULL);
	gl_dbg("MMC state: %d.", ad->maininfo.mmc_state);
	_gl_db_update_lock_always(data, true);
	_gl_set_file_op_cbs(data, __gl_timeline_del_op, NULL,
			    __gl_timeline_update_del_view, cnt);
	_gl_use_thread_operate_medias(ad, GL_STR_ID_DELETING, cnt,
				      GL_MEDIA_OP_DELETE_TIMELINE);
}

/* Select-all checkbox selected/deselected */
#if 0
static void __gl_timeline_edit_selall_cb(void *data, Evas_Object *obj, void *ei)
{
	GL_CHECK(data);
	gl_appdata *ad = (gl_appdata *)data;
	int item_cnt = 0;
	int sel_all_cnt = 0;
	int ret = -1;

	ad->selinfo.ck_state = !ad->selinfo.ck_state;
	if (ad->selinfo.selectall_ck) {
		elm_check_state_set(ad->selinfo.selectall_ck, ad->selinfo.ck_state);
	}
	int state = ad->selinfo.ck_state;
	gl_dbg("Checkbox state: %d", state);
	_gl_slider_set_cks_state(ad->tlinfo->view, state);

	__gl_timeline_clear_sel_list(ad->tlinfo);
	__gl_timeline_clear_sel_cnt(ad->tlinfo);

	if (state == EINA_FALSE) {
		sel_all_cnt = 0;
		item_cnt = ad->tlinfo->count;
	} else {
		Eina_List *list = NULL;
		ret = _gl_data_get_items(GL_GET_ALL_RECORDS, GL_GET_ALL_RECORDS,
					 &list);
		if (ret != 0 || !list)
			gl_dbgW("Empty!");

		gl_media_s *item = NULL;
		EINA_LIST_FREE(list, item) {
			if (!item || !item->uuid)
				continue;
			__gl_timeline_check_special_file(ad->tlinfo, item, true);
			__gl_timeline_sel_append_item(ad->tlinfo, item->uuid);
			_gl_data_type_free_glitem((void **)(&item));
			item_cnt++;
			item = NULL;
		}
		sel_all_cnt = item_cnt;
	}

	__gl_timeline_check_btns_state(ad->tlinfo, item_cnt, sel_all_cnt);
	/* Update the label text */
	_gl_ui_update_navi_title_text(ad->tlinfo->nf_it, sel_all_cnt, false);
}
#endif

int _gl_timeline_update_realized_grid_ck(Evas_Object *view, Eina_Bool state)
{
	GL_CHECK_VAL(view, -1);
	Eina_List *grid = NULL;
	Eina_List *l = NULL;
	Elm_Object_Item *it = NULL;

	grid = elm_gengrid_realized_items_get(view);
	GL_CHECK_VAL(grid, -1);

	EINA_LIST_FOREACH(grid, l, it) {
		if (it == NULL) {
			gl_dbgE("Invalid item!");
			continue;
		}
		Evas_Object *ck = NULL;

		ck = elm_object_item_part_content_get(it, GL_THUMB_CHECKBOX);
		gl_media_s *data = (gl_media_s *)elm_object_item_data_get(it);
		if (!ck) {
			gl_dbgE("Invalid checkbox!");
			return -1;
		}

		if (elm_check_state_get(ck) != state) {
			elm_check_state_set(ck, state);
			gl_timeline_s *td = evas_object_data_get(ck, "data");
			GL_CHECK_VAL(td, -1);
			__gl_timeline_thumb_check_op(td, ck, data);
		}
	}

	return 0;
}

/* Select-all checkbox selected/deselected */
static void __gl_timeline_thumb_edit_selall_cb(void *data, Evas_Object *obj, void *ei)
{
	GL_CHECK(data);
	gl_appdata *ad = (gl_appdata *)data;
	GL_CHECK(ad->tlinfo);

	ad->selinfo.ck_state = !ad->selinfo.ck_state;
	if (ad->selinfo.selectall_ck) {
		elm_check_state_set(ad->selinfo.selectall_ck, ad->selinfo.ck_state);
	}
	int state = ad->selinfo.ck_state;

	GL_CHECK(ad->tlinfo);
	Evas_Object *genlist = ad->tlinfo->view;
	GL_CHECK(genlist);

	gl_media_s *gitem = NULL;
	int i;
	if (ad->tlinfo->data_list) {
		int count = eina_list_count(ad->tlinfo->data_list);
		for (i = 0; i < count ; i++) {
			gitem = eina_list_nth(ad->tlinfo->data_list, i);
			if (gitem) {
				gitem->check_state = state;
				if (state) {
					__gl_timeline_check_special_file(ad->tlinfo, gitem, true);
					__gl_timeline_sel_append_item(ad->tlinfo, gitem->uuid);
				} else {
					__gl_timeline_check_special_file(ad->tlinfo, gitem, false);
					__gl_timeline_sel_remove_item(ad->tlinfo, gitem->uuid);
				}
			}
		}
		int sel_cnt = __gl_timeline_get_sel_cnt(ad->tlinfo);
		_gl_ui_update_navi_title_text(ad->tlinfo->nf_it, sel_cnt, false);

		__gl_timeline_check_btns_state(ad->tlinfo, ad->tlinfo->count,
				sel_cnt);
	}
	elm_genlist_realized_items_update(genlist);
}

/**
 *  Use naviframe api to push albums edit view to stack.
 *  @param obj is the content to be pushed.
 */
static int __gl_timeline_edit_add_btns(void *data)
{
	gl_dbg("EDIT");
	GL_CHECK_VAL(data, -1);
	gl_appdata *ad = (gl_appdata *)data;
	Evas_Object *parent = ad->maininfo.naviframe;
	Elm_Object_Item *nf_it = elm_naviframe_top_item_get(parent);
	GL_CHECK_VAL(nf_it, -1);

	/* More */
	Evas_Object *btn = _gl_but_create_but(parent, NULL, NULL,
					      GL_BUTTON_STYLE_NAVI_MORE,
					      __gl_timeline_more_cb, data);
	GL_CHECK_VAL(btn, -1);
	_gl_ui_disable_btn(btn);
	elm_object_item_part_content_set(nf_it, GL_NAVIFRAME_MORE, btn);

	Evas_Object *btn1 = elm_button_add(parent);
	Evas_Object *btn2 = elm_button_add(parent);
	GL_CHECK_VAL(btn1, -1);
	GL_CHECK_VAL(btn2, -1);
	/* Title Cancel Button */
	elm_object_style_set(btn1, "naviframe/title_left");
	elm_object_item_part_content_set(ad->ctrlinfo.nf_it, GL_NAVIFRAME_TITLE_LEFT_BTN, btn1);
	_gl_ui_set_translate_str(btn1,GL_STR_ID_CANCEL_CAP);
	evas_object_smart_callback_add(btn1, "clicked", __gl_timeline_edit_cancel_cb, ad);
	/* Title Done Button */
	elm_object_style_set(btn2, "naviframe/title_right");
	elm_object_item_part_content_set(ad->ctrlinfo.nf_it, GL_NAVIFRAME_TITLE_RIGHT_BTN, btn2);
	_gl_ui_set_translate_str(btn2,GL_STR_ID_DONE_CAP);
	evas_object_smart_callback_add(btn2, "clicked", __gl_timeline_del_cb, ad);
	elm_object_disabled_set(btn2, EINA_TRUE);

	gl_dbg("Sending signal to EDC");
	elm_object_signal_emit(ad->ctrlinfo.ctrlbar_view_ly, "elm,selectall,state,visible,bg", "elm");
	elm_object_signal_emit(ad->ctrlinfo.ctrlbar_view_ly, "elm,selectall,state,visible", "elm");
	_gl_ui_add_selall_ck(ad->ctrlinfo.ctrlbar_view_ly, "select.all.area.check", "select.all.area.check",
			    __gl_timeline_thumb_edit_selall_cb, data);
	return 0;
}

#ifdef GL_EXTENDED_FEATURES
static int __gl_timeline_share_get_path(void *data, char **files)
{
	GL_CHECK_VAL(files, 0);
	GL_CHECK_VAL(data, 0);
	gl_appdata *ad = (gl_appdata *)data;
	GL_CHECK_VAL(ad->tlinfo, 0);
	gl_timeline_s *timeline_d = ad->tlinfo;
	GL_CHECK_VAL(timeline_d->sel_d, 0);
	int i = 0;
	Eina_List *l = NULL;
	int ret = -1;
	char *item = NULL;
	char *path = NULL;

	EINA_LIST_FOREACH(timeline_d->sel_d->sel_list, l, item) {
		if (item == NULL) {
			gl_dbgE("Invalid item!");
			goto GL_TL_FAILED;
		}

		ret = _gl_local_data_get_path_by_id((const char *)item, &path);
		if (ret != 0 || !path) {
			gl_dbgE("Get path failed[%d]!", ret);
			goto GL_TL_FAILED;
		}

		files[i++] = strdup(path);
		gl_sdbg("file_url: %s!", files[i - 1]);

		GL_FREE(path);
	}
	return i;

 GL_TL_FAILED:

       for (; i > 0; --i)
	       GL_FREEIF(files[i - 1]);
       GL_FREEIF(path);
       return 0;
}

static void __gl_timeline_share_op_cb(void *data, Evas_Object *obj, void *ei)
{
	elm_toolbar_item_selected_set((Elm_Object_Item *)ei, EINA_FALSE);
	GL_CHECK(data);
	gl_appdata *ad = (gl_appdata *)data;
	GL_CHECK(ad->tlinfo);
	gl_timeline_s *timeline_d = ad->tlinfo;
	/* Get all selected medias count */
	int cnt = __gl_timeline_get_sel_cnt(timeline_d);
	_gl_ext_launch_share(data, cnt, __gl_timeline_share_get_path);
	/* Change to normal mode */
	__gl_timeline_change_mode(data, GL_TL_VIEW_NORMAL);
}

/**
 *  Use naviframe api to push albums edit view to stack.
 *  @param obj is the content to be pushed.
 */
static int __gl_timeline_share_add_btns(void *data)
{
	gl_dbg("Share");
	GL_CHECK_VAL(data, -1);
	gl_appdata *ad = (gl_appdata *)data;
	Evas_Object *parent = ad->maininfo.naviframe;
	Elm_Object_Item *nf_it = elm_naviframe_top_item_get(parent);
	GL_CHECK_VAL(nf_it, -1);
	/* Item Toolbar */
	Elm_Object_Item *tb_it = NULL;
	Evas_Object *toolbar = _gl_ctrl_add_toolbar(parent);
	/* Cancel */
	_gl_ctrl_append_item(toolbar, NULL, GL_STR_ID_CANCEL,
			     __gl_timeline_edit_cancel_cb, data);
	/* Share */
	tb_it = _gl_ctrl_append_item(toolbar, NULL, GL_STR_ID_SHARE,
				     __gl_timeline_share_op_cb, data);
	_gl_ctrl_disable_item(tb_it, true);
	elm_object_item_part_content_set(nf_it, "toolbar", toolbar);
	/* Select-all */
	ad->selinfo.ck_state = false;
	_gl_ui_add_xpt_btns(nf_it, GL_UI_XPT_SEL_ALL,
			    __gl_timeline_thumb_edit_selall_cb, NULL, data);
	return 0;
}
#endif

bool __gl_update_timeline(void *data) {
	GL_CHECK_FALSE(data);
	gl_appdata *ad = (gl_appdata *)data;
	GL_CHECK_FALSE(ad->tlinfo);

	if (ad->tlinfo->nocontents) {
		return false;
	}
	Evas_Object *view = __gl_timeline_create_list_view(ad->tlinfo, true);
	if (!view) {
		gl_dbgE("Failed to create view!");
		return false;
	}
	ad->tlinfo->view = view;
	elm_object_part_content_set(ad->tlinfo->layout, "elm.swallow", view);
	return true;
}

static int __gl_timeline_change_mode(void *data, int mode)
{
	GL_CHECK_VAL(data, -1);
	gl_appdata *ad = (gl_appdata *)data;
	GL_CHECK_VAL(ad->tlinfo, -1);

	ad->tlinfo->view_m = mode;
	switch (mode) {
	case GL_TL_VIEW_NORMAL:
		gl_dbg("GL_TL_VIEW_NORMAL");
		_gl_time_data_selected_list_finalize(data);
		_gl_ui_change_navi_title(ad->tlinfo->nf_it, GL_STR_TIME_VIEW, true);
		__gl_timeline_add_btns(data);
		/* Update view to hide checkboxes and clear data */
		__gl_timeline_clear_sel_list(ad->tlinfo);
		GL_GFREEIF(ad->tlinfo->sel_d);
		break;
	case GL_TL_VIEW_EDIT:
	case GL_TL_VIEW_SHARE:
		GL_GFREEIF(ad->tlinfo->sel_d);
		ad->tlinfo->sel_d = g_new0(gl_sel_s, 1);
		GL_CHECK_VAL(ad->tlinfo->sel_d, -1);
		_gl_ui_change_navi_title(ad->tlinfo->nf_it, GL_STR_ID_SELECT_ITEM,
				  false);
		if (GL_TL_VIEW_EDIT == mode) {
			__gl_timeline_edit_add_btns(data);
		}
#ifdef GL_EXTENDED_FEATURES
		else {
			__gl_timeline_share_add_btns(data);
		}
#endif
		break;
	default:
		gl_dbgE("Wrong mode!");
		return -1;
	}
	__gl_update_timeline(data);
	return 0;
}

static Eina_Bool __gl_timeline_pop_op(void *data)
{
	GL_CHECK_FALSE(data);
	gl_appdata *ad = (gl_appdata *)data;
	if (ad->tlinfo && ad->tlinfo->view_m != GL_TL_VIEW_NORMAL) {
		gl_dbg("EDIT");
		__gl_timeline_change_mode(data, GL_TL_VIEW_NORMAL);
		/* Just cancel edit view, dont lower app to background */
		return EINA_TRUE;
	}
	return EINA_FALSE;
}

int _gl_timeline_create_view(void *data, Evas_Object *parent)
{
	GL_CHECK_VAL(parent, -1);
	GL_CHECK_VAL(data, -1);
	gl_appdata *ad = (gl_appdata *)data;
	gl_dbg("");
	int w = 0;
	int h = 0;

	/* Set pop callback for operating when button back clicked */
	evas_object_data_set(ad->maininfo.naviframe, GL_NAVIFRAME_POP_CB_KEY,
			     (void *)__gl_timeline_pop_op);

	evas_object_geometry_get(parent, NULL, NULL, &w, &h);
	gl_dbg("content's size(%dx%d)", w, h);

	gl_set_view_mode(data, GL_VIEW_TIMELINE);
	_gl_ctrl_show_title(data, GL_CTRL_TAB_TIMELINE);

	if (ad->tlinfo) {
		gl_dbgW("Update view");
		/*To show transition*/
		ad->tlinfo->tran_op = GL_TRANS_OP_PREPARE;
		/*Update view*/
		ad->tlinfo->view = __gl_timeline_create_list_view(ad->tlinfo, false);
		elm_object_part_content_set(parent, "elm.swallow.view", ad->tlinfo->layout);
		/*TODO: Update date info if time changed, add timer to check time changed*/
		elm_object_part_content_set(ad->tlinfo->layout, "elm.swallow", ad->tlinfo->view);
		if (ad->tlinfo->view == NULL) {
			gl_dbgE("Failed to create view!");
			return -1;
		}
		evas_object_show(ad->tlinfo->view);
		/*Update buttons*/
		__gl_timeline_add_btns(data);
		/* Update buttons state */
		__gl_timeline_reset_btns(data);
		/* Update naviframe item title */
		__gl_timeline_reset_label(data);
		return 0;
	}

	Evas_Object *layout = NULL;
	Evas_Object *view = NULL;
	gl_timeline_s *timeline_d = NULL;

	timeline_d = (gl_timeline_s *)calloc(1, sizeof(gl_timeline_s));
	GL_CHECK_VAL(timeline_d, -1);

	timeline_d->ad = ad;
	timeline_d->parent = parent;
	timeline_d->nf_it = ad->ctrlinfo.nf_it;
	ad->tlinfo = timeline_d;
	timeline_d->tran_op = GL_TRANS_OP_PREPARE;
	timeline_d->time_media_display_order = TIME_ORDER_DESC;
	ad->pinchinfo.zoom_level = GL_ZOOM_IN_ONE;

	layout = gl_ui_load_edj(parent, GL_EDJ_FILE, "timeline");
	if (layout == NULL) {
		gl_dbgE("Failed to create layout!");
		goto GL_TIMELINE_FAILED;
	}
	evas_object_show(layout);
	elm_object_part_content_set(parent, "elm.swallow.view", layout);
	timeline_d->layout = layout;

	/* Minus title height */
	h -= (GL_TIMELINE_TITLE_H + GL_TIMELINE_PAD_H) * elm_config_scale_get();
	evas_object_data_set(layout, "page_w", (void *)w);
	evas_object_data_set(layout, "page_h", (void *)h);
	timeline_d->w = w;
	timeline_d->h = h;
	timeline_d->is_list_reverse = false;
	gl_dbg("content's size(%dx%d)", w, h);

	view = __gl_timeline_create_list_view(timeline_d, false);
	if (view == NULL) {
		gl_dbgE("Failed to create view!");
		GL_IF_DEL_OBJ(layout);
		return -1;
	}
	timeline_d->view = view;
	elm_object_part_content_set(timeline_d->layout, "elm.swallow", view);

	/*Update buttons*/
	__gl_timeline_add_btns(data);
	/* Update buttons state */
	__gl_timeline_reset_btns(data);
	/* Register callbacks */
	__gl_timeline_add_cbs(timeline_d);
	return 0;

 GL_TIMELINE_FAILED:

	GL_IF_DEL_OBJ(layout);
	return -1;
}

int _gl_timeline_update_view(void *data)
{
	GL_CHECK_VAL(data, -1);
	gl_appdata *ad = (gl_appdata *)data;
	GL_CHECK_VAL(ad->tlinfo, -1);

	/* Update view */
	gl_dbgW("Update view");
	Evas_Object *view = __gl_timeline_create_list_view(ad->tlinfo, false);
	if (view == NULL) {
		gl_dbgE("Failed to create view!");
		return -1;
	}
	ad->tlinfo->view = view;
	elm_object_part_content_set(ad->tlinfo->layout, "elm.swallow", view);
	/* Update buttons state */
	__gl_timeline_reset_btns(data);
	/* Update naviframe item title */
	__gl_timeline_reset_label(data);
	return 0;
}

int _gl_timeline_update_lang(void *data)
{
	GL_CHECK_VAL(data, -1);
	gl_dbg("");
	gl_appdata *ad = (gl_appdata *)data;
	GL_CHECK_VAL(ad->tlinfo, -1);
	GL_CHECK_VAL(ad->tlinfo->nf_it, -1);
	_gl_ui_change_navi_title(ad->tlinfo->nf_it, GL_STR_TIME_VIEW, true);
	_gl_timeline_update_view(ad);

	return 0;
}

int _gl_timeline_hide_view(void *data)
{
	GL_CHECK_VAL(data, -1);
	gl_appdata *ad = (gl_appdata *)data;
	gl_dbg("");

	if (ad->tlinfo == NULL)
		return -1;

	GL_TL_DEL_TRANSITS(ad->tlinfo->tiles);
	/* To hide slide items */
	evas_object_hide(ad->tlinfo->view);
	/* Cancel thumb creation*/
	__gl_timeline_cancel_thumbs(ad->tlinfo);
	/* Hide previous view */
	Evas_Object *pre_view = NULL;
	pre_view = elm_object_part_content_unset(ad->tlinfo->parent,
						 "elm.swallow.view");
	evas_object_hide(pre_view);
	return 0;
}

int _gl_timeline_rotate_view(void *data)
{
	return 0;
}

int _gl_timeline_view_rotate(void *data)
{
	gl_dbgE("ENTRY");
	GL_CHECK_VAL(data, -1);
	gl_appdata *ad = (gl_appdata *)data;
	GL_CHECK_VAL(ad->tlinfo, -1);

	if (ad->tlinfo->nocontents) {
		return -1;
	}

	ad->tlinfo->delete_data = false;
	Evas_Object *view = __gl_timeline_create_list_view(ad->tlinfo, true);
	if (view == NULL) {
		gl_dbgE("Failed to create view!");
		return -1;
	}
	ad->tlinfo->view = view;
	elm_object_part_content_set(ad->tlinfo->layout, "elm.swallow", view);
	if ((ad->maininfo.rotate_mode == APP_DEVICE_ORIENTATION_270) ||
			(ad->maininfo.rotate_mode == APP_DEVICE_ORIENTATION_90)) {
		elm_object_signal_emit(ad->tlinfo->parent, "timelineview,landscape", "");
	} else {
		elm_object_signal_emit(ad->tlinfo->parent, "timelineview,portrait", "");
	}
	if (ad->tlinfo->view_m == GL_TL_VIEW_EDIT) {
		elm_object_signal_emit(ad->ctrlinfo.ctrlbar_view_ly, "elm,selectall,state,visible,bg", "elm");
		elm_object_signal_emit(ad->ctrlinfo.ctrlbar_view_ly, "elm,selectall,state,visible", "elm");
	}

	gl_dbgE("EXIT");
	return 0;
}

int _gl_timeline_update_grid_size(void *data)
{
	gl_dbgE("ENTRY");
	GL_CHECK_VAL(data, -1);
	gl_appdata *ad = (gl_appdata *)data;
	GL_CHECK_VAL(ad->tlinfo, -1);

	if (ad->tlinfo->nocontents) {
		return -1;
	}
	Evas_Object *view = __gl_timeline_create_list_view(ad->tlinfo, true);
	if (view == NULL) {
		gl_dbgE("Failed to create view!");
		return -1;
	}
	ad->tlinfo->view = view;
	elm_object_part_content_set(ad->tlinfo->layout, "elm.swallow", view);

	return 0;
}

void _gl_update_timeview_iv_select_mode_reply(void *data, char **select_result, int count)
{
	gl_dbgE("ENTRY");
	GL_CHECK(data);
	gl_appdata *ad = (gl_appdata *)data;
	GL_CHECK(ad->tlinfo);
	gl_media_s *item = NULL;
	Eina_List *l = NULL;
	int i;
	int sel_count = 0;
	bool in_list = false;

	if (select_result) {
		EINA_LIST_FOREACH(ad->tlinfo->data_list, l, item) {
			if (!item) {
				continue;
			}
			in_list = false;
			for (i = 0; i < count; i++) {
				if (!strcmp(select_result[i], item->file_url)) {
					if (!item->check_state) {
						__gl_timeline_sel_append_item(ad->tlinfo, item->uuid);
						item->check_state = true;
					}
					in_list = true;
					sel_count++;
					break;
				}
			}
			if (!in_list) {
				if (item->check_state) {
					__gl_timeline_sel_remove_item(ad->tlinfo, item->uuid);
					item->check_state = false;
				}
			}
		}
	} else {
		EINA_LIST_FOREACH(ad->tlinfo->data_list, l, item) {
			if (!item) {
				continue;
			}
			if (item->check_state) {
				__gl_timeline_sel_remove_item(ad->tlinfo, item->uuid);
				item->check_state = false;
			}
		}
	}

	_gl_ui_update_navi_title_text(ad->tlinfo->nf_it, sel_count, false);

	if (sel_count == ad->tlinfo->count) {
		ad->selinfo.ck_state = EINA_TRUE;
	} else {
		ad->selinfo.ck_state = EINA_FALSE;
	}

	if (ad->selinfo.selectall_ck) {
		elm_check_state_set(ad->selinfo.selectall_ck, ad->selinfo.ck_state);
	}

	GL_CHECK(ad->tlinfo->view);
	elm_genlist_realized_items_update(ad->tlinfo->view);

	if (select_result) {
		for (i = 0; i < count; i++) {
			if (select_result[i]) {
				free(select_result[i]);
			}
		}
		free(select_result);
	}
}

int _gl_ext_load_time_iv_selected_list(app_control_h service, void *data)
{
	gl_dbgE("ENTRY");
	GL_CHECK_VAL(data, GL_UG_FAIL);
	gl_appdata *ad = (gl_appdata *)data;
	GL_CHECK_VAL(service, GL_UG_FAIL);
	GL_CHECK_VAL(ad->tlinfo, GL_UG_FAIL);
	GL_CHECK_VAL(ad->tlinfo->sel_d, GL_UG_FAIL);
	int i;
	char *gitem = NULL;
	gl_media_s *item = NULL;
	Eina_List *l = NULL;
	char **value = NULL;

	int count = eina_list_count(ad->tlinfo->sel_d->sel_list);

	if (count > 0) {
		(value) = (char**)malloc(count * sizeof(char *));
		if (!value) {
			return GL_UG_FAIL;
		}
	}

	for (i = 0; i < count; i++) {
		gitem = eina_list_nth(ad->tlinfo->sel_d->sel_list, i);
		EINA_LIST_FOREACH(ad->tlinfo->data_list, l, item) {
			if (!item) {
				continue;
			}
			if (!strcmp(item->uuid, gitem)) {
				(value)[i] = strdup(item->file_url);
				break;
			}
		}
	}

	if (count > 0) {
		app_control_add_extra_data_array(service, "Selected index",
				value, count);
	}

	if (value) {
		for (i = 0; i < count; i++) {
			free(value[i]);
		}
		free(value);
	}

	return GL_UG_SUCCESS;
}

bool _gl_is_timeline_edit_mode(void *data)
{
	GL_CHECK_FALSE(data);
	gl_appdata *ad = (gl_appdata *)data;
	GL_CHECK_FALSE(ad->tlinfo);
	if (ad->tlinfo->view_m == GL_TL_VIEW_EDIT) {
		return true;
	}
	return false;
}


