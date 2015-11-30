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
#include "gallery.h"
#include "gl-popup.h"
#include "gl-ui-util.h"
#include "gl-util.h"
#include "gl-debug.h"
#include "gl-albums.h"
#include "gl-albums-new.h"
#include "gl-progressbar.h"
#include "gl-strings.h"
#include "gl-button.h"
#include "gl-controlbar.h"
#include "gl-editfield.h"
#include "gl-ctxpopup.h"
#include "gl-thumbs.h"
#include "gl-timeline.h"

enum  gl_popup_obj_del_cb_type {
	GL_POPUP_OBJ_DEL_CB_TYPE_ITEM,
	GL_POPUP_OBJ_DEL_CB_TYPE_ALBUM
};

#define GL_POPUP_GENLIST_ITEM_W 618

#define GL_POPUP_TIMEOUT_1S 1.0
#define GL_POPUP_TIMEOUT_2S 2.0
#define GL_POPUP_TIMEOUT_3S 3.0
#define GL_POPUP_OBJ_DATA_KEY "gl_popup_data_key"
#define GL_POPUP_OBJ_DEL_CB_KEY "gl_popup_cb_key"
#define GL_POPUP_OBJ_DEL_CB_DATA_KEY "gl_popup_cb_data_key"
#define GL_POPUP_OBJ_DEL_CB_TYPE_KEY "gl_popup_cb_type_key"
#define GL_POPUP_GENLIST_DATA_KEY "gl_popup_genlist_data_key"
#define SETTINGS_STORAGE_UG_NAME "setting-storage-efl"
#define GL_POPUP_GENLIST_HEIGHT_HD 260
#define GL_POPUP_GENLIST_WIDTH_HD 630
#define GL_POPUP_GENLIST_HEIGHT_QHD 185
#define GL_POPUP_GENLIST_WIDTH_QHD 450
#define GL_POPUP_GENLIST_HEIGHT_WVGA 202
#define GL_POPUP_GENLIST_WIDTH_WVGA 480

typedef struct _gl_popup_t gl_popup_s;
typedef struct _gl_popup_menu_t gl_popup_menu_s;
typedef struct _gl_popup_resize_t gl_popup_resize_s;

struct _gl_popup_t {
	gl_appdata *ad;
	Evas_Object *popup;
	Evas_Smart_Cb cb1;
	Evas_Smart_Cb cb2;
	Evas_Smart_Cb cb3;
	const void *data1;
	const void *data2;
	const void *data3;
};

struct _gl_popup_menu_t {
	gl_appdata *ad;
	Evas_Object *popup;
	void *op_func;
};

struct _gl_popup_resize_t {
	gl_appdata *ad;
	Evas_Object *genlist;
	Evas_Object *box;
	Ecore_Job *job;
	int item_height;
};

static void __gl_popup_resp(void *data, const char *text);

static void __gl_popup_genlist_lang_changed(void *data, Evas_Object *obj, void *ei)
{
	GL_CHECK(obj);
	elm_genlist_realized_items_update(obj);
}

/* Delete popup contents */
static int __gl_popup_clear_content(Evas_Object *popup)
{
	GL_CHECK_VAL(popup, -1);
	gl_dbg("");

	Evas_Object *content = elm_object_content_get(popup);
	GL_CHECK_VAL(content, -1);
	Eina_List *chidren = elm_box_children_get(content);
	GL_CHECK_VAL(chidren, -1);
	Evas_Object *genlist = eina_list_nth(chidren, 0);
	GL_CHECK_VAL(genlist, -1);
	elm_genlist_clear(genlist);
	elm_box_clear(content);
	return 0;
}

static Eina_Bool __gl_popup_timeout_cb(void *data)
{
	GL_CHECK_FALSE(data);
	gl_appdata *ad = (gl_appdata *)data;
	GL_IF_DEL_TIMER(ad->popupinfo.del_timer);
	GL_IF_DEL_OBJ(ad->popupinfo.popup);
	return ECORE_CALLBACK_CANCEL;
}

static int __gl_popup_add_timer(void *data, double to_inc)
{
	GL_CHECK_VAL(data, -1);
	gl_appdata *ad = (gl_appdata *)data;
	GL_IF_DEL_TIMER(ad->popupinfo.del_timer);
	ad->popupinfo.del_timer = ecore_timer_add(to_inc, __gl_popup_timeout_cb,
	                          data);
	return 0;
}

static void __gl_popup_del_resp_cb(void *data, Evas_Object *obj,
                                   void *event_info)
{
	GL_CHECK(obj);
	GL_CHECK(data);
	gl_appdata *ad = (gl_appdata *)data;
	const char *text = NULL;

	text = elm_object_text_get(obj);
	GL_CHECK(text);
	gl_dbg("Button: %s", text);

	if (!g_strcmp0(text, GL_STR_DELETE)) {
		void *del_cb = evas_object_data_get(ad->popupinfo.popup,
		                                    GL_POPUP_OBJ_DEL_CB_KEY);
		void *cb_data = evas_object_data_get(ad->popupinfo.popup,
		                                     GL_POPUP_OBJ_DEL_CB_DATA_KEY);
		GL_IF_DEL_OBJ(ad->popupinfo.popup);
		if (del_cb && cb_data) {
			int (*_del_cb)(void * cb_data);
			_del_cb = del_cb;
			_del_cb(cb_data);
		} else {
			gl_dbgE("Invalid callback!");
		}
	} else {
		__gl_popup_resp(data, text);
	}
}

static void __gl_popup_resp(void *data, const char *text)
{
	GL_CHECK(data);
	gl_appdata *ad = (gl_appdata *)data;
	int mode = ad->popupinfo.mode;
	GL_CHECK(text);
	gl_dbg("Button: %s, mode: %d", text, mode);

	switch (mode) {
	case GL_POPUP_ALBUM_NEW_EMPTY:
	case GL_POPUP_ALBUM_NEW_DUPLICATE:
	case GL_POPUP_ALBUM_NEW_INVALID:
	case GL_POPUP_ALBUM_RENAME_EMPTY:
	case GL_POPUP_ALBUM_RENAME_DUPLICATE:
	case GL_POPUP_ALBUM_RENAME_INVALID:
		if (!g_strcmp0(text, _gl_str(GL_STR_ID_YES)) ||
		        !g_strcmp0(text, _gl_str(GL_STR_ID_OK))) {
			_gl_editfield_show_imf(data);
		} else if (!g_strcmp0(text, _gl_str(GL_STR_ID_NO)) ||
		           !g_strcmp0(text, _gl_str(GL_STR_ID_CANCEL))) {
			elm_naviframe_item_pop(ad->maininfo.naviframe);
		}
		break;
	case GL_POPUP_NOBUT_APPEXIT:
		if (ad->popupinfo.popup) {
			__gl_popup_clear_content(ad->popupinfo.popup);
			evas_object_del(ad->popupinfo.popup);
			ad->popupinfo.popup = NULL;
		}
		elm_exit();
		return;
	case GL_POPUP_SLIDESHOW:
	case GL_POPUP_LONGPRESSED:
	case GL_POPUP_MEDIA_DELETE: {
		_gl_free_selected_list(data);
		break;
	}
	case GL_POPUP_ALBUM_MEMORY_FULL:
		GL_IF_DEL_OBJ(ad->popupinfo.memory_full_popup);
		elm_naviframe_item_pop(ad->maininfo.naviframe);

		if (!g_strcmp0(text, _gl_str(GL_STR_SETTINGS))) {
			app_control_h app_control;
			int ret = 0;

			ret = app_control_create(&app_control);

			if (ret != APP_CONTROL_ERROR_NONE) {
				gl_dbgE("app_control create failed");
				return;
			}

			ret = app_control_set_operation(app_control, APP_CONTROL_OPERATION_VIEW);
			if (ret != APP_CONTROL_ERROR_NONE) {
				gl_dbgE("app_control_set_operation()... [0x%x]", ret);
				goto END;
			}

			ret = app_control_set_app_id(app_control, SETTINGS_STORAGE_UG_NAME);
			if (ret != APP_CONTROL_ERROR_NONE) {
				gl_dbgE("app_control_set_uri()... [0x%x]", ret);
				goto END;
			}

			ret = app_control_send_launch_request(app_control, NULL, NULL);
			if (ret != APP_CONTROL_ERROR_NONE) {
				gl_dbgE("app_control launch Failed... [0x%x]", ret);
				goto END;
			}
END:
			if (app_control) {
				app_control_destroy(app_control);
			}
		}
		break;
	default:
		break;
	}

	GL_IF_DEL_OBJ(ad->popupinfo.popup);
}

static void __gl_popup_resp_cb(void *data, Evas_Object *obj, void *ei)
{
	GL_CHECK(data);
	GL_CHECK(obj);

	const char *text = NULL;

	text = elm_object_text_get(obj);
	GL_CHECK(text);
	gl_dbg("Button: %s", text);
	__gl_popup_resp(data, text);
}

static void __gl_popup_block_clicked_cb(void *data, Evas_Object *obj, void *ei)
{
	GL_CHECK(data);
	gl_appdata *ad = (gl_appdata *)data;
	_gl_free_selected_list(data);
	GL_IF_DEL_OBJ(ad->popupinfo.popup);
}

static void __gl_popup_genlist_delete_cb(void *data, Evas *e, Evas_Object *obj,
			void *ei)
{
	gl_dbg("Delete genlist ---");
	if (obj) {
		evas_object_data_del(obj, GL_POPUP_OBJ_DATA_KEY);
		gl_popup_resize_s *resize_d = evas_object_data_get(obj,
		                              GL_POPUP_GENLIST_DATA_KEY);
		if (resize_d) {
			GL_IF_DEL_JOB(resize_d->job);
			GL_FREE(resize_d);
		}
	}
	if (data) {
		elm_genlist_item_class_free((Elm_Genlist_Item_Class *)data);
	}

	gl_dbg("Delete genlist +++");
}

/* This works if mini_menustyle used */
static int __gl_popup_set_min_size(Evas_Object *box, int cnt)
{
	GL_CHECK_VAL(box, -1);

	/* #define GENLIST_HEIGHT_1LINE  110  // (114 - top/bottom padding) */
	/*modify the hight to solve the pop up scroll issue*/
#define GL_POPUP_GENLIST_ITEM_H 114
#define GL_POPUP_GENLIST_ITEM_H2 136

	int item_height = GL_POPUP_GENLIST_ITEM_H;
	int font_size = _gl_get_font_size();
	gl_dbg("font size: %d", font_size);
	if (font_size == 4) {
		item_height = GL_POPUP_GENLIST_ITEM_H2;
	}
	int min_h = 0;
	if (cnt > 3) {
		min_h = item_height * 4;
	} else {
		min_h = item_height * cnt;
	}

	evas_object_size_hint_min_set(box,
	                              GL_POPUP_GENLIST_ITEM_W * elm_config_scale_get(),
	                              min_h * elm_config_scale_get());
	return item_height;
}

static void __gl_popup_mouseup_cb(void *data, Evas *e, Evas_Object *obj,
                                  void *event_info)
{
	gl_dbg("");
	GL_CHECK(data);
	GL_CHECK(event_info);
	Evas_Event_Mouse_Up *ev = event_info;
	if (ev->button == 3) {
		gl_dbg("Delete popup!");
		__gl_popup_resp(data, GL_STR_ID_CANCEL);
	}
}

/*static void __gl_popup_keydown_cb(void *data, Evas *e, Evas_Object *obj,
				  void *event_info)
{
	gl_dbg("");
	GL_CHECK(data);
	GL_CHECK(event_info);
	Evas_Event_Key_Down *ev = event_info;
	if (!strcmp(ev->keyname, KEY_BACK)) {
		gl_dbg("Delete popup!");
		__gl_popup_resp(data, _gl_str(GL_STR_ID_CANCEL));
	}
}*/


/* We need to perform cancellation, not just delete popup */
static inline void __gl_popup_ea_back_cb(void *data, Evas_Object *obj, void *ei)
{
	gl_dbg("Use customized callback");
	__gl_popup_resp(data, _gl_str(GL_STR_ID_CANCEL));
}

static void __gl_popup_local_resp_cb(void *data, Evas_Object *obj, void *ei)
{
	evas_object_del((Evas_Object *)data);
}

static void __gl_popup_local_mouseup_cb(void *data, Evas *e, Evas_Object *obj, void *ei)
{
	gl_dbg("");
	GL_CHECK(data);
	GL_CHECK(ei);
	Evas_Event_Mouse_Up *ev = ei;
	if (ev->button == 3) {
		gl_dbg("Delete popup!");
		GL_IF_DEL_OBJ(obj);
	}
}

/*static void __gl_popup_local_keydown_cb(void *data, Evas *e, Evas_Object *obj,  void *ei)
{
	gl_dbg("");
	GL_CHECK(data);
	GL_CHECK(ei);
	Evas_Event_Key_Down *ev = ei;
	if (!strcmp(ev->keyname, KEY_BACK)) {
		gl_dbg("Delete popup!");
		GL_IF_DEL_OBJ(obj);
	}
}*/

static void __gl_popup_local_delete_cb(void *data, Evas *e, Evas_Object *obj, void *ei)
{
	gl_dbg("Delete popup ---");
	GL_CHECK(data);
	gl_appdata *ad = (gl_appdata *)data;
	GL_IF_DEL_TIMER(ad->popupinfo.del_timer);
	evas_object_event_callback_del(obj, EVAS_CALLBACK_MOUSE_UP,
	                               __gl_popup_local_mouseup_cb);
	/* Disable EVAS_CALLBACK_KEY_DOWN event */
	/*evas_object_event_callback_del(obj, EVAS_CALLBACK_KEY_DOWN,
				       __gl_popup_local_keydown_cb);*/
	_gl_editfield_show_imf(data);
	gl_dbg("Delete popup +++");
}

static void __gl_popup_local_block_clicked_cb(void *data, Evas_Object *obj, void *ei)
{
	GL_IF_DEL_OBJ(obj);
}

static Eina_Bool __gl_popup_local_timeout_cb(void *data)
{
	GL_CHECK_CANCEL(data);
	Evas_Object *popup = (Evas_Object *)data;
	gl_appdata *ad = (gl_appdata *)evas_object_data_get(popup,
	                 GL_POPUP_OBJ_DATA_KEY);
	GL_IF_DEL_OBJ(popup);

	GL_CHECK_CANCEL(ad);
	GL_IF_DEL_TIMER(ad->popupinfo.del_timer);
	return ECORE_CALLBACK_CANCEL;
}

static Evas_Object *__gl_popup_add(Evas_Object *parent, const char *style,
                                   const char *title, const char *text,
                                   void *data)
{
	GL_CHECK_NULL(parent);
	Evas_Object *popup = elm_popup_add(parent);
	GL_CHECK_NULL(popup);
	if (style) {
		elm_object_style_set(popup, style);
	}
	if (title) {
		_gl_ui_set_translate_part_str(popup, "title,text", title);
	}
	if (text) {
		_gl_ui_set_translate_str(popup, text);
	}
	evas_object_size_hint_weight_set(popup, EVAS_HINT_EXPAND,
	                                 EVAS_HINT_EXPAND);

	/*Add callback for Hardware Key*/
	evas_object_event_callback_add(popup, EVAS_CALLBACK_MOUSE_UP,
	                               __gl_popup_mouseup_cb, data);
	/* Disable EVAS_CALLBACK_KEY_DOWN event */
	/*evas_object_event_callback_add(popup, EVAS_CALLBACK_KEY_DOWN,
				       __gl_popup_keydown_cb, data);*/
	/*Delete the Popup if the Popup has a BACK event.*/
	eext_object_event_callback_add(popup, EEXT_CALLBACK_BACK,
	                               __gl_popup_ea_back_cb, data);
	return popup;
}

static Evas_Object *__gl_popup_add_local(Evas_Object *parent, const char *style,
					const char *title, const char *text,
					void *data)
{
	GL_CHECK_NULL(parent);
	Evas_Object *popup = elm_popup_add(parent);
	GL_CHECK_NULL(popup);
	if (style) {
		elm_object_style_set(popup, style);
	}
	if (title) {
		_gl_ui_set_translate_part_str(popup, "title,text", title);
	}
	if (text) {
		_gl_ui_set_translate_str(popup, text);
	}
	evas_object_size_hint_weight_set(popup, EVAS_HINT_EXPAND,
	                                 EVAS_HINT_EXPAND);

	/*Add callback for Hardware Key*/
	evas_object_event_callback_add(popup, EVAS_CALLBACK_MOUSE_UP,
	                               __gl_popup_local_mouseup_cb, data);
	/* Disable EVAS_CALLBACK_KEY_DOWN event */
	/*evas_object_event_callback_add(popup, EVAS_CALLBACK_KEY_DOWN,
				       __gl_popup_local_keydown_cb, data);*/
	/*Delete the Popup if the Popup has a BACK event.*/
	eext_object_event_callback_add(popup, EEXT_CALLBACK_BACK,
	                               eext_popup_back_cb, NULL);
	return popup;
}

static void __gl_popup_genlist_realized(void *data, Evas_Object *obj, void *ei)
{
	if (elm_genlist_first_item_get(obj) == ei) {
		gl_dbg("Emit signal");
		evas_object_smart_callback_call(obj, "popup,genlist,realized",
		                                data);
	}
}

static Evas_Object *__gl_popup_add_genlist(void *data, Evas_Object *parent,
					const char *style,
					Elm_Gen_Item_Text_Get_Cb text_get,
					Elm_Gen_Item_Content_Get_Cb content_get,
					void *append_func)
{
	GL_CHECK_NULL(parent);

	/* Set item of Genlist.*/
	Elm_Genlist_Item_Class *gic = NULL;
	gic = elm_genlist_item_class_new();
	GL_CHECK_NULL(gic);
	gic->item_style = style;
	gic->func.text_get = text_get;
	gic->func.content_get = content_get;
	gic->func.state_get = NULL;
	gic->func.del = NULL;

	/* Create genlist */
	Evas_Object *genlist = elm_genlist_add(parent);
	if (genlist == NULL) {
		elm_genlist_item_class_free(gic);
		return NULL;
	}
	evas_object_smart_callback_add(genlist, "realized",
	                               __gl_popup_genlist_realized, data);
	/* Register delete callback */
	evas_object_event_callback_add(genlist, EVAS_CALLBACK_DEL,
	                               __gl_popup_genlist_delete_cb,
	                               (void *)gic);
	evas_object_smart_callback_add(genlist, "language,changed",
	                               __gl_popup_genlist_lang_changed, NULL);
	evas_object_size_hint_weight_set(genlist, EVAS_HINT_EXPAND,
	                                 EVAS_HINT_EXPAND);
	evas_object_size_hint_align_set(genlist, EVAS_HINT_FILL,
	                                EVAS_HINT_FILL);
	if (append_func) {
		void (*append_cb)(void * popup_data, Evas_Object * gl, Elm_Genlist_Item_Class * gic);
		append_cb = append_func;
		append_cb(data, genlist, gic);
	}

	if (elm_genlist_items_count(genlist) <= 3)
		elm_scroller_policy_set(genlist, ELM_SCROLLER_POLICY_OFF,
		                        ELM_SCROLLER_POLICY_OFF);
	return genlist;
}

static void __gl_popup_delete_cb(void *data, Evas *e, Evas_Object *obj, void *ei)
{
	gl_dbg("Delete popup ---");
	evas_object_event_callback_del(obj, EVAS_CALLBACK_MOUSE_UP,
	                               __gl_popup_mouseup_cb);
	/* Disable EVAS_CALLBACK_KEY_DOWN event */
	/*evas_object_event_callback_del(obj, EVAS_CALLBACK_KEY_DOWN,
				       __gl_popup_keydown_cb);*/
	GL_FREEIF(data);
	gl_dbg("Delete popup +++");
}

static void __gl_popup_menu_delete_cb(void *data, Evas *e, Evas_Object *obj,
                                      void *ei)
{
	gl_dbg("Delete popup ---");

	evas_object_event_callback_del(obj, EVAS_CALLBACK_MOUSE_UP,
	                               __gl_popup_mouseup_cb);
	/* Disable EVAS_CALLBACK_KEY_DOWN event */
	/*evas_object_event_callback_del(obj, EVAS_CALLBACK_KEY_DOWN,
				       __gl_popup_keydown_cb);*/
	/* Remove key data for popup */
	evas_object_data_del(obj, GL_POPUP_OBJ_DATA_KEY);

	GL_FREEIF(data);
	gl_dbg("Delete popup +++");
}

static void __gl_popup_menu_sel_cb(void *data, Evas_Object *obj, void *ei)
{
	GL_CHECK(ei);
	GL_CHECK(data);
	gl_popup_menu_s *share_d = (gl_popup_menu_s *)data;
	GL_CHECK(share_d->ad);
	gl_appdata *ad = share_d->ad;
	char *id = NULL;

	id = (char *)elm_object_item_data_get(ei);
	GL_CHECK(id);
	gl_sdbg("label id: %s", id);

	int (*op_cb)(void * data, const char * label) = NULL;
	if (share_d->op_func) {
		op_cb = share_d->op_func;
	}
	GL_IF_DEL_OBJ(ad->popupinfo.popup);
	ad->popupinfo.mode = GL_POPUP_NOBUT;
	if (op_cb) {
		op_cb(ad, _gl_str(id));
	}
}

static char *__gl_popup_menu_get_text(void *data, Evas_Object *obj,
                                      const char *part)
{
	if (!strcmp(part, "elm.text")) {
		char *str = NULL;
		GL_CHECK_NULL(data);
		str = _gl_str((char *)data);
		if (str) {
			return strdup(str);
		}
	}
	return NULL;
}

#ifdef SUPPORT_SLIDESHOW
static void __gl_popup_slideshow_append(void *data, Evas_Object *gl,
                                        Elm_Genlist_Item_Class *gic)
{
	GL_CHECK(gic);
	GL_CHECK(gl);
	GL_CHECK(data);
	gl_dbg("append items");

	elm_genlist_item_append(gl, gic, (void *)GL_STR_ID_ALL_ITEMS,
	                        NULL, ELM_GENLIST_ITEM_NONE,
	                        __gl_popup_menu_sel_cb, data);
	elm_genlist_item_append(gl, gic, (void *)GL_STR_ID_SELECT_ITEMS,
	                        NULL, ELM_GENLIST_ITEM_NONE,
	                        __gl_popup_menu_sel_cb, data);
	elm_genlist_item_append(gl, gic, (void *)GL_STR_ID_SETTINGS,
	                        NULL, ELM_GENLIST_ITEM_NONE,
	                        __gl_popup_menu_sel_cb, data);
}
#endif

#if 0
static void __gl_popup_album_longpressed_append(void *data, Evas_Object *gl,
			Elm_Genlist_Item_Class *gic)
{
	GL_CHECK(gic);
	GL_CHECK(gl);
	GL_CHECK(data);
	gl_dbg("append items");
	gl_popup_menu_s *share_d = (gl_popup_menu_s *)data;
	GL_CHECK(share_d->ad);
	gl_appdata *ad = share_d->ad;
	gl_cluster *album = _gl_albums_get_current(ad);
	GL_CHECK(album);

	elm_genlist_item_append(gl, gic, (void *)GL_STR_ID_SHARE,
	                        NULL, ELM_GENLIST_ITEM_NONE,
	                        __gl_popup_menu_sel_cb, data);
#ifdef SUPPORT_SLIDESHOW
	elm_genlist_item_append(gl, gic, (void *)GL_STR_ID_SLIDESHOW,
	                        NULL, ELM_GENLIST_ITEM_NONE,
	                        __gl_popup_menu_sel_cb, data);
#endif

	if (GL_STORE_T_ALL != album->cluster->type)
		elm_genlist_item_append(gl, gic, (void *)GL_STR_ID_DELETE,
		                        NULL, ELM_GENLIST_ITEM_NONE,
		                        __gl_popup_menu_sel_cb, data);
	if ((GL_STORE_T_PHONE == album->cluster->type ||
	        GL_STORE_T_MMC == album->cluster->type) &&
	        !_gl_data_is_camera_album(album->cluster))
		elm_genlist_item_append(gl, gic, (void *)GL_STR_ID_RENAME,
		                        NULL, ELM_GENLIST_ITEM_NONE,
		                        __gl_popup_menu_sel_cb, data);
}
#endif

static void __gl_popup_thumb_longpressed_append(void *data, Evas_Object *gl,
			Elm_Genlist_Item_Class *gic)
{
	GL_CHECK(gic);
	GL_CHECK(gl);
	GL_CHECK(data);
	gl_dbg("append items");
	gl_popup_menu_s *share_d = (gl_popup_menu_s *)data;
	GL_CHECK(share_d->ad);
	gl_appdata *ad = share_d->ad;
	GL_CHECK(ad->selinfo.elist);
	gl_item *gitem = (gl_item *)eina_list_nth(ad->selinfo.elist, 0);
	GL_CHECK(gitem);
	GL_CHECK(gitem->item);
	gl_dbg("item storage type: %d", gitem->item->storage_type);
	elm_genlist_item_append(gl, gic, (void *)GL_STR_ID_SHARE,
	                        NULL, ELM_GENLIST_ITEM_NONE,
	                        __gl_popup_menu_sel_cb, data);
	if ((GL_STORE_T_PHONE == gitem->item->storage_type ||
	        GL_STORE_T_MMC == gitem->item->storage_type) &&
	        MEDIA_CONTENT_TYPE_IMAGE == gitem->item->type)
		elm_genlist_item_append(gl, gic, (void *)GL_STR_ID_EDIT,
		                        NULL, ELM_GENLIST_ITEM_NONE,
		                        __gl_popup_menu_sel_cb, data);
	if (GL_STORE_T_PHONE == gitem->item->storage_type ||
	        GL_STORE_T_MMC == gitem->item->storage_type) {
		elm_genlist_item_append(gl, gic, (void *)GL_STR_ID_DELETE,
		                        NULL, ELM_GENLIST_ITEM_NONE,
		                        __gl_popup_menu_sel_cb, data);
	}

	if ((GL_STORE_T_PHONE == gitem->item->storage_type ||
	        GL_STORE_T_MMC == gitem->item->storage_type)) {
		elm_genlist_item_append(gl, gic, (void *)GL_STR_ID_MOVE,
		                        NULL, ELM_GENLIST_ITEM_NONE,
		                        __gl_popup_menu_sel_cb, data);
		elm_genlist_item_append(gl, gic, (void *)GL_STR_ID_COPY,
		                        NULL, ELM_GENLIST_ITEM_NONE,
		                        __gl_popup_menu_sel_cb, data);
	}
	/*
	if (MEDIA_CONTENT_TYPE_IMAGE == gitem->item->type)
		elm_genlist_item_append(gl, gic, (void *)GL_STR_ID_DETAILS,
					NULL, ELM_GENLIST_ITEM_NONE,
					__gl_popup_menu_sel_cb, data);*/
}

static void __gl_popup_btn_new_album_cb(void *data, Evas_Object *obj, void *ei)
{
	gl_dbg("");
	GL_CHECK(data);
	gl_popup_menu_s *menu_d = NULL;
	menu_d = (gl_popup_menu_s *)data;
	GL_CHECK(menu_d->ad);
	gl_appdata *ad = menu_d->ad;

	_gl_albums_new_create_view(ad, menu_d->op_func);
	ad->popupinfo.mode = GL_POPUP_NOBUT;
}

static void __gl_popup_move_cb(void *data, Evas_Object *obj, void *ei)
{
	gl_dbg("");
	/* Get 'menu data' via 'GL_POPUP_OBJ_DATA_KEY' */
	GL_CHECK(obj);
	gl_popup_menu_s *menu_d = NULL;
	menu_d = (gl_popup_menu_s *)evas_object_data_get(obj, GL_POPUP_OBJ_DATA_KEY);
	GL_CHECK(menu_d);
	GL_CHECK(menu_d->ad);
	gl_appdata *ad = menu_d->ad;
	GL_FREEIF(ad->albuminfo.path);

	if (data) {
		GL_CHECK(menu_d->op_func);
		ad->albuminfo.path = strdup((char *)data);
		gl_sdbg("Dest album path: %s", ad->albuminfo.path);
		int (*mc_cb)(void * data);
		mc_cb = menu_d->op_func;
		mc_cb(ad);
	}

	GL_IF_DEL_OBJ(ad->popupinfo.popup);
	ad->popupinfo.mode = GL_POPUP_NOBUT;
}

static void __gl_popup_move_append(void *data, Evas_Object *gl,
                                   Elm_Genlist_Item_Class *gic)
{
	GL_CHECK(gic);
	GL_CHECK(gl);
	GL_CHECK(data);
	gl_popup_menu_s *menu_d = (gl_popup_menu_s *)data;
	GL_CHECK(menu_d->ad);
	gl_appdata *ad = menu_d->ad;
	GL_CHECK(menu_d->op_func);
	GL_CHECK(ad->maininfo.naviframe);
	int i = 0;
	Eina_List *album_list = NULL;
	int all_count = 0;
	char *label = NULL;
	gl_cluster *album = NULL;
	gl_cluster *cur_album = _gl_albums_get_current(ad);
	evas_object_data_set(gl, GL_POPUP_OBJ_DATA_KEY, menu_d);
	/*elm_genlist_item_append(gl, gic, (void *)GL_STR_ID_NEW_ALBUM,
				NULL, ELM_GENLIST_ITEM_NONE,
				__gl_popup_move_cb, NULL);*/

	if (ad->albuminfo.elist && ad->albuminfo.elist->clist) {
		album_list = ad->albuminfo.elist->clist;
		all_count = eina_list_count(album_list);
	} else {
		gl_dbgE("cluster list is NULL");
		return;
	}

	gl_dbg("cluster list length: %d", all_count);
	for (i = 0; i < all_count; i++) {
		album = eina_list_nth(album_list, i);
		GL_CHECK(album);
		GL_CHECK(album->cluster);
		GL_CHECK(album->cluster->display_name);
		GL_CHECK(album->cluster->uuid);

		if (cur_album && cur_album->cluster &&
		        cur_album->cluster->uuid &&
		        !g_strcmp0(album->cluster->uuid, cur_album->cluster->uuid)) {
			continue;
		}
		if (album->cluster->type == GL_STORE_T_MMC ||
		        album->cluster->type == GL_STORE_T_PHONE) {
			label = _gl_get_i18n_album_name(album->cluster);
			elm_genlist_item_append(gl, gic, (void *)label,
			                        NULL, ELM_GENLIST_ITEM_NONE,
			                        __gl_popup_move_cb,
			                        album->cluster->path);
		}
	}
}

static void __gl_popup_genlist_resize_job_cb(void *data)
{
	GL_CHECK(data);
	gl_popup_resize_s *resize_d = (gl_popup_resize_s *)data;
	Evas_Object *track = NULL;
	Elm_Object_Item *item = elm_genlist_first_item_get(resize_d->genlist);
	track = elm_object_item_track(item);
	if (track) {
		Evas_Coord h = 0;
		evas_object_geometry_get(track, NULL, NULL, NULL, &h);
		gl_dbg("size diff: %d-%d", h, resize_d->item_height);
		if (resize_d->item_height != h) {
			int cnt = elm_genlist_items_count(resize_d->genlist);
			int min_h = 0;
			if (cnt > 3) {
				min_h = h * 4;
			} else {
				min_h = h * cnt;
			}

			evas_object_size_hint_min_set(resize_d->box,
			                              GL_POPUP_GENLIST_ITEM_W * elm_config_scale_get(),
			                              min_h);
		}
	}
	elm_object_item_untrack(item);
	GL_IF_DEL_JOB(resize_d->job);
}

static void __gl_popup_genlist_realized_resize(void *data, Evas_Object *obj, void *ei)
{
	GL_CHECK(data);
	gl_popup_resize_s *resize_d = (gl_popup_resize_s *)data;
	GL_IF_DEL_JOB(resize_d->job);
	resize_d->job = ecore_job_add(__gl_popup_genlist_resize_job_cb, data);
}

static int __gl_popup_add_resize_op(gl_appdata *ad, Evas_Object *box, Evas_Object *genlist)
{
	gl_popup_resize_s *resize_d = NULL;
	resize_d = (gl_popup_resize_s *)calloc(1, sizeof(gl_popup_resize_s));
	GL_CHECK_VAL(resize_d, -1);

	resize_d->box = box;
	resize_d->genlist = genlist;
	resize_d->ad = ad;
	evas_object_smart_callback_add(genlist, "popup,genlist,realized",
	                               __gl_popup_genlist_realized_resize,
	                               (void *)resize_d);
	evas_object_data_set(genlist, GL_POPUP_GENLIST_DATA_KEY,
	                     (void *)resize_d);

	resize_d->item_height = __gl_popup_set_min_size(box,
	                        elm_genlist_items_count(genlist));
	return 0;
}

/* Title + menustyle(genlist) + one button */
static int __gl_popup_create_menu(void *data, const char *title,
                                  void *append_func, void *op_func, int mode)
{
	gl_dbg("");
	GL_CHECK_VAL(data, -1);
	gl_appdata *ad = (gl_appdata *)data;

	if (ad->popupinfo.popup) {
		gl_dbg("The existed popup is deleted");
		evas_object_del(ad->popupinfo.popup);
		ad->popupinfo.popup = NULL;
	}

	gl_popup_menu_s *menu_d = NULL;
	menu_d = (gl_popup_menu_s *)calloc(1, sizeof(gl_popup_menu_s));
	GL_CHECK_VAL(menu_d, -1);

	Evas_Object *box = NULL;
	Evas_Object *genlist = NULL;
	Evas_Object *popup = __gl_popup_add(ad->maininfo.win,
	                                    "content_no_vhpad",
	                                    title, NULL, data);
	if (popup == NULL) {
		GL_FREE(menu_d);
		return -1;
	}
	/* Register delete callback */
	evas_object_event_callback_add(popup, EVAS_CALLBACK_DEL,
	                               __gl_popup_menu_delete_cb, menu_d);
	menu_d->ad = ad;
	menu_d->popup = popup;
	menu_d->op_func = op_func;
	evas_object_data_set(popup, GL_POPUP_OBJ_DATA_KEY, menu_d);

	box = elm_box_add(popup);
	evas_object_size_hint_weight_set(box, EVAS_HINT_EXPAND,
	                                 EVAS_HINT_EXPAND);
	/* Create genlist */
	genlist = __gl_popup_add_genlist(menu_d, box, "1text/popup",
	                                 __gl_popup_menu_get_text, NULL,
	                                 append_func);
	__gl_popup_add_resize_op(ad, box, genlist);

	elm_box_pack_end(box, genlist);
	evas_object_show(genlist);
	evas_object_show(box);
	elm_object_content_set(popup, box);
	evas_object_show(popup);
	ad->popupinfo.popup = popup;
	ad->popupinfo.mode = mode;
	return 0;
}

/* Title + menustyle(genlist) + one button */
static int __gl_popup_create_album_menu(void *data, const char *title,
                                        void *append_func, void *op_func, int mode)
{
	gl_dbg("");
	GL_CHECK_VAL(data, -1);
	gl_appdata *ad = (gl_appdata *)data;

	if (ad->popupinfo.popup) {
		gl_dbg("The existed popup is deleted");
		evas_object_del(ad->popupinfo.popup);
		ad->popupinfo.popup = NULL;
	}

	gl_popup_menu_s *menu_d = NULL;
	menu_d = (gl_popup_menu_s *)calloc(1, sizeof(gl_popup_menu_s));
	GL_CHECK_VAL(menu_d, -1);

	Evas_Object *box = NULL;
	Evas_Object *genlist = NULL;
	Evas_Object *popup = __gl_popup_add(ad->maininfo.win, NULL,
	                                    title, NULL, data);
	if (popup == NULL) {
		GL_FREE(menu_d);
		return -1;
	}
	/* Register delete callback */
	evas_object_event_callback_add(popup, EVAS_CALLBACK_DEL,
	                               __gl_popup_menu_delete_cb, menu_d);
	menu_d->ad = ad;
	menu_d->popup = popup;
	menu_d->op_func = op_func;
	evas_object_data_set(popup, GL_POPUP_OBJ_DATA_KEY, menu_d);

	box = elm_box_add(popup);
	evas_object_size_hint_weight_set(box, EVAS_HINT_EXPAND,
	                                 EVAS_HINT_EXPAND);
	/* Create genlist */
	genlist = __gl_popup_add_genlist(menu_d, box, "1text/popup",
	                                 __gl_popup_menu_get_text, NULL,
	                                 append_func);

	if (elm_genlist_items_count(genlist) > 0) {
//		__gl_popup_add_resize_op(ad, box, genlist);
		elm_box_pack_end(box, genlist);
		evas_object_show(genlist);
		evas_object_show(box);

		elm_object_style_set(popup, "content_no_vhpad");
		elm_object_content_set(popup, box);
	} else {
		GL_IF_DEL_OBJ(genlist);
		GL_IF_DEL_OBJ(box);

		_gl_ui_set_translate_str(popup, GL_STR_ID_THERE_IS_ONLY_ONE_ALBUM);
	}

	evas_object_show(popup);
	ad->popupinfo.popup = popup;
	ad->popupinfo.mode = mode;
	return 0;
}

int _gl_popup_create(void *data, const char *title, const char *text,
                     const char *str1, Evas_Smart_Cb cb1, const void *data1,
                     const char *str2, Evas_Smart_Cb cb2, const void *data2,
                     const char *str3, Evas_Smart_Cb cb3, const void *data3)
{
	GL_CHECK_VAL(data, -1);
	gl_appdata *ad = (gl_appdata *)data;

	if (ad->popupinfo.popup) {
		gl_dbg("Delte the existed popup first");
		evas_object_del(ad->popupinfo.popup);
		ad->popupinfo.popup = NULL;
	}

	Evas_Object *popup = NULL;
	popup = __gl_popup_add(ad->maininfo.win, NULL, title, text, data);
	GL_CHECK_VAL(popup, -1);
	gl_popup_s *popup_d = (gl_popup_s *)calloc(1, sizeof(gl_popup_s));
	if (popup_d == NULL) {
		gl_dbgE("Calloc failed!");
		evas_object_del(popup);
		return -1;
	}
	evas_object_data_set(popup, GL_POPUP_OBJ_DATA_KEY, popup_d);
	/* Register delete callback */
	evas_object_event_callback_add(popup, EVAS_CALLBACK_DEL,
	                               __gl_popup_delete_cb, popup_d);
	popup_d->ad = ad;
	popup_d->popup = popup;

	if (str1 && cb1) {
		Evas_Object *btn1 = NULL;
		btn1 = _gl_but_create_but_popup(popup, str1, cb1,
		                                data1);
		elm_object_part_content_set(popup, "button1", btn1);
		popup_d->cb1 = cb1;
		popup_d->data1 = data1;
	}
	if (str2 && cb2) {
		Evas_Object *btn2 = NULL;
		btn2 = _gl_but_create_but_popup(popup, str2, cb2,
		                                data2);
		elm_object_part_content_set(popup, "button2", btn2);
		popup_d->cb2 = cb2;
		popup_d->data2 = data2;
	}
	if (str3 && cb3) {
		Evas_Object *btn3 = NULL;
		btn3 = _gl_but_create_but_popup(popup, str3, cb3,
		                                data3);
		elm_object_part_content_set(popup, "button3", btn3);
		popup_d->cb3 = cb3;
		popup_d->data3 = data3;
	}

	evas_object_show(popup);
	ad->popupinfo.popup = popup;
	return 0;
}

static void __gl_popup_del_lang_changed(void *data, Evas_Object *obj, void *ei)
{
	GL_CHECK(obj);
	int sel_cnt = (int)data;
	if (sel_cnt < 1) {
		gl_dbg("Do not need to update the laungue manual");
		return;
	}
	int del_type = (int)evas_object_data_get(obj,
	               GL_POPUP_OBJ_DEL_CB_TYPE_KEY);
	char *popup_str = NULL;
	if (del_type == GL_POPUP_OBJ_DEL_CB_TYPE_ITEM) {
		popup_str = g_strdup_printf(GL_STR_DELETE_PD_FILES, sel_cnt);
	} else {
		popup_str = g_strdup_printf(GL_STR_DELETE_PD_ALBUMS, sel_cnt);
	}
	elm_object_text_set(obj, popup_str);
}

int _gl_popup_create_del(void *data, int mode, void *sel_cb, void *del_cb,
                         void *cb_data)
{
	GL_CHECK_VAL(cb_data, -1);
	GL_CHECK_VAL(del_cb, -1);
	GL_CHECK_VAL(data, -1);
	gl_appdata *ad = (gl_appdata *)data;
	int sel_cnt = 0;
	char *fmt_str = NULL;
	char *popup_str = NULL;
	char *one_selected = NULL;
	char *mul_selected = NULL;

	if (ad->popupinfo.popup != NULL) {
		gl_dbg("The existed popup is deleted");
		evas_object_del(ad->popupinfo.popup);
	}

	if (sel_cb) {
		int (*_sel_cb)(void * data);
		_sel_cb = sel_cb;
		sel_cnt = _sel_cb(cb_data);
		gl_dbg("mode: %d", mode);
		switch (mode) {
		case GL_POPUP_DEL_FILE :
			one_selected = GL_STR_ID_DELETE_1_FILE;
			mul_selected = GL_STR_DELETE_PD_FILES;
			break;
		case GL_POPUP_DEL_ALBUM:
			one_selected = GL_STR_ID_DELETE_1_ALBUM;
			mul_selected = GL_STR_DELETE_PD_ALBUMS;
			break;
		default:
			gl_dbgE("Wrong delete mode!");
			return -1;
		}

		if (sel_cnt == 1) {
			popup_str = g_strdup(one_selected);
		} else {
			fmt_str = mul_selected;
			popup_str = g_strdup_printf(fmt_str, sel_cnt);
		}
	} else {
		popup_str = g_strdup(GL_STR_ID_DELETE_COMFIRMATION);
	}
	gl_dbg("popup string: %s", popup_str);

	Evas_Object *popup = __gl_popup_add(ad->maininfo.win, NULL,
	                                    GL_STR_ID_DELETE,
	                                    popup_str, data);
	GL_CHECK_VAL(popup, -1);
	evas_object_event_callback_add(popup, EVAS_CALLBACK_DEL,
	                               __gl_popup_delete_cb, NULL);


	/*only when sel_cnt > 1, we need to change langue manual*/
	if (sel_cnt > 1) {
		evas_object_smart_callback_add(popup, "language,changed",
		                               __gl_popup_del_lang_changed,
		                               (void *)sel_cnt);
		if (mode == GL_POPUP_DEL_FILE)
			evas_object_data_set(popup,
			                     GL_POPUP_OBJ_DEL_CB_TYPE_KEY,
			                     (void *)GL_POPUP_OBJ_DEL_CB_TYPE_ITEM);
		else
			evas_object_data_set(popup,
			                     GL_POPUP_OBJ_DEL_CB_TYPE_KEY,
			                     (void *)GL_POPUP_OBJ_DEL_CB_TYPE_ALBUM);
	}
	evas_object_data_set(popup, GL_POPUP_OBJ_DEL_CB_KEY, del_cb);
	evas_object_data_set(popup, GL_POPUP_OBJ_DEL_CB_DATA_KEY, cb_data);

	Evas_Object *btn1 = NULL;
	Evas_Object *btn2 = NULL;
	btn1 = _gl_but_create_but_popup(popup, GL_STR_ID_CANCEL,
	                                __gl_popup_del_resp_cb, data);
	btn2 = _gl_but_create_but_popup(popup, GL_STR_ID_DELETE,
	                                __gl_popup_del_resp_cb, data);
	elm_object_part_content_set(popup, "button1", btn1);
	elm_object_part_content_set(popup, "button2", btn2);

	evas_object_show(popup);
	ad->popupinfo.popup = popup;
	ad->popupinfo.mode = GL_POPUP_MEDIA_DELETE;

	GL_FREE(popup_str);
	return 0;
}

int gl_popup_create_popup(void *data, gl_popup_mode mode, char *desc)
{
	GL_CHECK_VAL(data, -1);
	gl_appdata *ad = (gl_appdata *)data;
	gl_dbg("");

	if (ad->popupinfo.popup) {
		gl_dbg("The existed popup is deleted");
		evas_object_del(ad->popupinfo.popup);
		ad->popupinfo.popup = NULL;
	}

	gl_pb_del_pbar(ad);
	Evas_Object *popup = NULL;

	if (mode == GL_POPUP_ALBUM_MEMORY_FULL) {
		popup = __gl_popup_add(ad->maininfo.win, NULL, GL_DATA_SAVE_FAILED, desc,
		                       data);
	} else {
		popup = __gl_popup_add(ad->maininfo.win, NULL, NULL, desc,
		                       data);
	}

	GL_CHECK_VAL(popup, -1);
	evas_object_event_callback_add(popup, EVAS_CALLBACK_DEL,
	                               __gl_popup_delete_cb, NULL);

	Evas_Object *btn1 = NULL;
	Evas_Object *btn2 = NULL;

	switch (mode) {
	case GL_POPUP_NOBUT:
	case GL_POPUP_NOBUT_APPEXIT:
		__gl_popup_add_timer(ad, GL_POPUP_TIMEOUT_3S);
		break;
	case GL_POPUP_ONEBUT:
		btn1 = _gl_but_create_but_popup(popup, GL_STR_ID_CLOSE,
		                                __gl_popup_resp_cb, data);
		elm_object_part_content_set(popup, "button1", btn1);
		break;
	case GL_POPUP_ALBUM_NEW_EMPTY:
	case GL_POPUP_ALBUM_NEW_DUPLICATE:
	case GL_POPUP_ALBUM_NEW_INVALID:
	case GL_POPUP_ALBUM_RENAME_EMPTY:
	case GL_POPUP_ALBUM_RENAME_DUPLICATE:
	case GL_POPUP_ALBUM_RENAME_INVALID:
		btn1 = _gl_but_create_but_popup(popup, GL_STR_ID_NO,
		                                __gl_popup_resp_cb, data);
		elm_object_part_content_set(popup, "button1", btn1);
		btn2 = _gl_but_create_but_popup(popup, GL_STR_ID_YES,
		                                __gl_popup_resp_cb, data);
		elm_object_part_content_set(popup, "button2", btn2);
		gl_dbg("Hide Entry cursor and IMF");
		_gl_editfield_hide_imf(ad);
		break;
	case GL_POPUP_ALBUM_MEMORY_FULL:
		btn1 = _gl_but_create_but_popup(popup, GL_STR_ID_CANCEL_ABB,
		                                __gl_popup_resp_cb, data);
		elm_object_part_content_set(popup, "button1", btn1);
		btn2 = _gl_but_create_but_popup(popup, GL_STR_SETTINGS,
		                                __gl_popup_resp_cb, data);
		elm_object_part_content_set(popup, "button2", btn2);
		break;

	default:
		gl_dbgE("Other popup mode...");
		break;
	}

	evas_object_show(popup);

	/* Focus popup and IMF hide */
	elm_object_focus_set(popup, EINA_TRUE);

	if (mode == GL_POPUP_ALBUM_MEMORY_FULL) {
		ad->popupinfo.memory_full_popup = popup;
	} else {
		ad->popupinfo.popup = popup;
	}
	ad->popupinfo.mode = mode;

	return 0;
}

Evas_Object *_gl_popup_create_local(void *data, gl_popup_mode mode, char *desc)
{
	GL_CHECK_NULL(data);
	gl_appdata *ad = (gl_appdata *)data;
	gl_dbg("");

	Evas_Object *win = ad->maininfo.win;
	if (ad->maininfo.entry_win) {
		win = ad->maininfo.entry_win;
	}

	Evas_Object *popup = __gl_popup_add_local(win, NULL, NULL, desc, data);
	GL_CHECK_NULL(popup);
	evas_object_data_set(popup, GL_POPUP_OBJ_DATA_KEY, data);
	evas_object_event_callback_add(popup, EVAS_CALLBACK_DEL,
	                               __gl_popup_local_delete_cb, data);
	Evas_Object *btn1 = NULL;

	switch (mode) {
	case GL_POPUP_NOBUT:
		GL_IF_DEL_TIMER(ad->popupinfo.del_timer);
		ad->popupinfo.del_timer = ecore_timer_add(GL_POPUP_TIMEOUT_3S,
		                          __gl_popup_local_timeout_cb,
		                          popup);
		evas_object_smart_callback_add(popup, "block,clicked",
		                               __gl_popup_local_block_clicked_cb,
		                               popup);
		break;
	case GL_POPUP_ONEBUT:
		btn1 = _gl_but_create_but_popup(popup, GL_STR_ID_OK,
		                                __gl_popup_local_resp_cb, popup);
		elm_object_part_content_set(popup, "button1", btn1);
		break;
	default:
		gl_dbgW("Other popup mode[%d]", mode);
		break;
	}

	evas_object_show(popup);
	elm_object_focus_set(popup, EINA_FALSE);
	return popup;
}

#ifdef SUPPORT_SLIDESHOW
int _gl_popup_create_slideshow(void *data, void *op_func)
{
	gl_dbg("");
	GL_CHECK_VAL(data, -1);
	__gl_popup_create_menu(data, GL_STR_ID_SLIDESHOW,
	                       __gl_popup_slideshow_append, op_func,
	                       GL_POPUP_SLIDESHOW);
	_gl_popup_add_block_callback(data);
	return 0;
}
#endif

int _gl_popup_create_move(void *data, void *op_func)
{
	gl_dbg("");
	GL_CHECK_VAL(data, -1);
	__gl_popup_create_album_menu(data, GL_STR_ID_MOVE,
	                             __gl_popup_move_append, op_func,
	                             GL_POPUP_MEDIA_MOVE);
	_gl_popup_add_buttons(data, GL_STR_ID_NEW_ALBUM,
	                      __gl_popup_btn_new_album_cb);
	return 0;
}

int _gl_popup_create_copy(void *data, void *op_func)
{
	gl_dbg("");
	GL_CHECK_VAL(data, -1);
	__gl_popup_create_album_menu(data, GL_STR_ID_COPY,
	                             __gl_popup_move_append, op_func,
	                             GL_POPUP_MEDIA_MOVE);
	_gl_popup_add_buttons(data, GL_STR_ID_NEW_ALBUM,
	                      __gl_popup_btn_new_album_cb);
	return 0;
}

int _gl_popup_create_move_with_append(void *data, void *op_func, void *append_func, const char *title)
{
	gl_dbg("");
	GL_CHECK_VAL(data, -1);
	__gl_popup_create_album_menu(data, title, append_func, op_func,
	                             GL_POPUP_MEDIA_MOVE);
	return 0;
}

int _gl_popup_create_longpressed_album_with_append(void *data, void *op_func, void *append_func, const char *title)
{
	gl_dbg("");
	GL_CHECK_VAL(data, -1);
	__gl_popup_create_menu(data, title, append_func, op_func,
	                       GL_POPUP_LONGPRESSED);
	_gl_popup_add_block_callback(data);
	return 0;
}

int _gl_popup_create_longpressed_thumb_with_append(void *data, void *op_func, void *append_func, const char *title)
{
	gl_dbg("");
	GL_CHECK_VAL(data, -1);
	__gl_popup_create_menu(data, title, append_func, op_func,
	                       GL_POPUP_LONGPRESSED);
	_gl_popup_add_block_callback(data);
	return 0;
}

int _gl_popup_create_longpressed_thumb(void *data, void *op_func, char *file_name)
{
	gl_dbg("");
	GL_CHECK_VAL(data, -1);
	__gl_popup_create_menu(data, file_name,
	                       __gl_popup_thumb_longpressed_append, op_func,
	                       GL_POPUP_LONGPRESSED);
	_gl_popup_add_block_callback(data);
	return 0;
}

int _gl_popup_add_buttons(void *data, const char *text, Evas_Smart_Cb cb_func)
{
	Evas_Object *btn1 = NULL;
	Evas_Object *btn2 = NULL;
	gl_appdata *ad = (gl_appdata *)data;
	/* Button 'cancel' */
	btn1 = _gl_but_create_but_popup(ad->popupinfo.popup, GL_STR_ID_CANCEL,
	                                __gl_popup_resp_cb, data);
	elm_object_part_content_set(ad->popupinfo.popup, "button1", btn1);

	void *menu_data = evas_object_data_get(ad->popupinfo.popup,
	                                       GL_POPUP_OBJ_DATA_KEY);
	btn2 = _gl_but_create_but_popup(ad->popupinfo.popup, text, cb_func,
	                                menu_data);
	elm_object_part_content_set(ad->popupinfo.popup, "button2", btn2);
	return 0;
}

int _gl_popup_add_block_callback(void *data)
{
	gl_appdata *ad = (gl_appdata *)data;
	evas_object_smart_callback_add(ad->popupinfo.popup, "block,clicked",
	                               __gl_popup_block_clicked_cb, data);
	return 0;
}

void _gl_time_view_selected_cb(void *data, Evas_Object *obj, void *event_info)
{
	gl_dbg("ENTRY");
	GL_CHECK(data);
	gl_appdata *ad = (gl_appdata *)data;
	if (ad->popupinfo.popup) {
		evas_object_del(ad->popupinfo.popup);
		ad->popupinfo.popup = NULL;
	}
	int view_mode = gl_get_view_mode(ad);
	if (view_mode == GL_VIEW_TIMELINE) {
		gl_dbg("EXIT 1");
		return;
	}
	_gl_ctrl_show_view(data, "Timeline");
	gl_dbg("EXIT");
}

void _gl_album_view_selected_cb(void *data, Evas_Object *obj, void *event_info)
{
	gl_dbg("ENTRY");
	GL_CHECK(data);
	gl_appdata *ad = (gl_appdata *)data;
	if (ad->popupinfo.popup) {
		evas_object_del(ad->popupinfo.popup);
		ad->popupinfo.popup = NULL;
	}
	int view_mode = gl_get_view_mode(ad);
	if (view_mode == GL_VIEW_ALBUMS) {
		gl_dbg("EXIT 1");
		return;
	}
	_gl_ctrl_show_view(data, GL_STR_ALBUMS);
	gl_dbg("EXIT");
}

void _gl_asc_sort_selected_cb(void *data, Evas_Object *obj, void *event_info)
{
	gl_dbg("ENTRY");
	GL_CHECK(data);
	gl_appdata *ad = (gl_appdata *)data;
	if (ad->popupinfo.popup) {
		evas_object_del(ad->popupinfo.popup);
		ad->popupinfo.popup = NULL;
	}
	if (ad->maininfo.view_mode == GL_VIEW_THUMBS) {
		if (ad->gridinfo.media_display_order == ORDER_ASC) {
			gl_dbg("EXIT 1");
			return;
		}
		ad->gridinfo.media_display_order = ORDER_ASC;
		_gl_thumbs_update_items(ad);
	} else if (ad->maininfo.view_mode == GL_VIEW_TIMELINE) {
		__gl_timeline_asc_mode_set(ad);
	}
	gl_dbg("EXIT");
}

void _gl_desc_sort_selected_cb(void *data, Evas_Object *obj, void *event_info)
{
	gl_dbg("ENTRY");
	GL_CHECK(data);
	gl_appdata *ad = (gl_appdata *)data;
	if (ad->popupinfo.popup) {
		evas_object_del(ad->popupinfo.popup);
		ad->popupinfo.popup = NULL;
	}
	if (ad->maininfo.view_mode == GL_VIEW_THUMBS) {
		if (ad->gridinfo.media_display_order == ORDER_DESC) {
			gl_dbg("EXIT 1");
			return;
		}
		ad->gridinfo.media_display_order = ORDER_DESC;
		_gl_thumbs_update_items(ad);
	} else if (ad->maininfo.view_mode == GL_VIEW_TIMELINE) {
		__gl_timeline_desc_mode_set(ad);
	}
	gl_dbg("EXIT");
}

void _gl_list_viewas_popup_cb_add(Evas_Object *btn, Evas_Object *btn1, void *ad)
{
	evas_object_smart_callback_add(btn, "clicked", _gl_time_view_selected_cb, ad);
	evas_object_smart_callback_add(btn1, "clicked", _gl_album_view_selected_cb, ad);
}

void _gl_list_sortby_popup_cb_add(Evas_Object *btn, Evas_Object *btn1, void *ad)
{
	evas_object_smart_callback_add(btn, "clicked", _gl_desc_sort_selected_cb, ad);
	evas_object_smart_callback_add(btn1, "clicked", _gl_asc_sort_selected_cb, ad);
}

void _gl_list_pop_create(void *data, Evas_Object *obj, void *ei, char *title, char *first_text, char *second_text, int state_index)
{
	gl_dbg("ENTRY");
	GL_CHECK(data);
	gl_appdata *ad = (gl_appdata *)data;
	int w, h;
	_gl_ctxpopup_del(data);
	Evas_Object * popup = elm_popup_add(ad->maininfo.win);
	evas_object_size_hint_weight_set(popup, EVAS_HINT_EXPAND,
	                                 EVAS_HINT_EXPAND);
	_gl_ui_set_translate_part_str(popup, "title,text", title);

	Evas_Object *ly = elm_layout_add(popup);
	Evas_Object *ly1 = elm_layout_add(ly);
	elm_layout_file_set(ly, GL_EDJ_FILE, "list_popup_swallow_ly");
	elm_layout_file_set(ly1, GL_EDJ_FILE, "list_popup_content_ly");
	_gl_ui_set_translate_part_str(ly1, "elm.text", first_text);
	_gl_ui_set_translate_part_str(ly1, "elm.text.second", second_text);

	Evas_Object *group_radio = elm_radio_add(ly1);
	elm_radio_state_value_set(group_radio, 0);
	elm_radio_value_set(group_radio, 0);

	Evas_Object *radio = elm_radio_add(ly1);
	Evas_Object *radio1 = elm_radio_add(ly1);
	elm_radio_group_add(radio, group_radio);
	elm_radio_state_value_set(radio, 0);
	evas_object_propagate_events_set(radio, EINA_TRUE);
	evas_object_show(radio);
	elm_object_part_content_set(ly1, "elm.swallow", radio);

	elm_radio_group_add(radio1, group_radio);
	elm_radio_state_value_set(radio1, 1);
	elm_radio_value_set(group_radio, state_index);
	evas_object_propagate_events_set(radio1, EINA_TRUE);
	evas_object_show(radio1);
	elm_object_part_content_set(ly1, "elm.swallow.second", radio1);

	Evas_Object *btn = elm_button_add(ly1);
	Evas_Object *btn1 = elm_button_add(ly1);
	elm_object_style_set(btn, "transparent");
	elm_object_style_set(btn1, "transparent");
	elm_object_part_content_set(ly1, "button1", btn);
	elm_object_part_content_set(ly1, "button2", btn1);

	if (strcmp(title, GL_STR_SORT)) {
		_gl_list_viewas_popup_cb_add(btn, btn1, ad);
	} else {
		_gl_list_sortby_popup_cb_add(btn, btn1, ad);
	}
	ad->popupinfo.popup = popup;
	elm_object_part_content_set(ly, "list", ly1);
	elm_object_content_set(popup, ly);
	evas_object_show(popup);
	evas_object_geometry_get(ad->maininfo.win, NULL, NULL, &w, &h);
	Edje_Message_Int *msg = (Edje_Message_Int *)malloc(sizeof(Edje_Message_Int) + sizeof(int));
	GL_CHECK(msg);
	if ((w == GL_STR_SCREEN_WIDTH_WVGA && h == GL_STR_SCREEN_HEIGHT_WVGA) || (w == GL_STR_SCREEN_HEIGHT_WVGA && h == GL_STR_SCREEN_WIDTH_WVGA)) {
		msg->val = GL_POPUP_GENLIST_HEIGHT_WVGA;
		edje_object_message_send(elm_layout_edje_get(ly) , EDJE_MESSAGE_INT, 2, msg);
	} else if ((w == GL_STR_SCREEN_WIDTH_QHD && h == GL_STR_SCREEN_HEIGHT_QHD) || (w == GL_STR_SCREEN_HEIGHT_QHD && h == GL_STR_SCREEN_WIDTH_QHD)) {
		msg->val = GL_POPUP_GENLIST_HEIGHT_QHD;
		edje_object_message_send(elm_layout_edje_get(ly), EDJE_MESSAGE_INT, 2, msg);
	} else if ((w == GL_STR_SCREEN_WIDTH_HD && h == GL_STR_SCREEN_HEIGHT_HD) || (w == GL_STR_SCREEN_HEIGHT_HD && h == GL_STR_SCREEN_WIDTH_HD)) {
		msg->val = GL_POPUP_GENLIST_HEIGHT_HD;
		edje_object_message_send(elm_layout_edje_get(ly), EDJE_MESSAGE_INT, 2, msg);
	}
	free(msg);
	eext_object_event_callback_add(popup, EEXT_CALLBACK_BACK,
	                               eext_popup_back_cb, NULL);
	evas_object_repeat_events_set(popup, EINA_FALSE);
	gl_dbg("EXIT");
}

static void __gl_thumbs_edit_cancel_cb(void *data, Evas_Object *obj, void *ei)
{
	GL_CHECK(data);
	gl_appdata *ad = (gl_appdata *)data;
	gl_dbg("");
	if (ad->popupinfo.popup) {
		evas_object_del(ad->popupinfo.popup);
	}
	ad->popupinfo.popup = NULL;
}

static void __gl_thumbs_edit_move_cb(void *data, Evas_Object *obj, void *ei)
{
	GL_CHECK(data);
	gl_appdata *ad = (gl_appdata *)data;
	gl_dbg("");

	if (ad->popupinfo.popup) {
		evas_object_del(ad->popupinfo.popup);
	}
	ad->popupinfo.popup = NULL;
	ad->albuminfo.file_mc_mode = GL_MC_MOVE;
	if (ad->gridinfo.it) {
		gl_item *gitem = NULL;
		int cnt = _gl_data_selected_list_count(ad);
		int i;
		for (i = 0; i < cnt; i++) {
			gitem = eina_list_nth(ad->selinfo.elist, i);
			GL_CHECK(gitem);
			GL_CHECK(gitem->album);
			GL_CHECK(gitem->album->cluster);
			if (!strcmp(ad->albuminfo.path, gitem->album->cluster->path)) {
				_gl_popup_create_local(data, GL_POPUP_NOBUT,
				                       GL_STR_ID_SAME_DESTINATION_ERROR_MSG);
				return;
			}
		}
		gl_move_copy_to_album(ad);
	}
}

static void __gl_thumbs_edit_create_album_cb(void *data, Evas_Object *obj, void *ei)
{
	GL_CHECK(data);
	gl_appdata *ad = (gl_appdata *)data;
	gl_dbg("");

	if (ad->popupinfo.popup) {
		evas_object_del(ad->popupinfo.popup);
	}
	ad->popupinfo.popup = NULL;
	ad->albuminfo.path = NULL;
	ad->albuminfo.file_mc_mode = GL_MC_COPY;
	if (_gl_thumbs_get_edit_mode(data) == GL_THUMBS_EDIT_COPY) {
		ad->albuminfo.file_mc_mode = GL_MC_COPY;
	} else if (_gl_thumbs_get_edit_mode(data) == GL_THUMBS_EDIT_MOVE) {
		ad->albuminfo.file_mc_mode = GL_MC_MOVE;
	}
	_gl_albums_new_create_view(ad, gl_move_copy_to_album);
	return;
}

static void __gl_thumbs_edit_copy_cb(void *data, Evas_Object *obj, void *ei)
{
	GL_CHECK(data);
	gl_appdata *ad = (gl_appdata *)data;
	gl_dbg("");

	if (ad->popupinfo.popup) {
		evas_object_del(ad->popupinfo.popup);
	}
	ad->popupinfo.popup = NULL;
	ad->albuminfo.file_mc_mode = GL_MC_COPY;
	if (ad->gridinfo.it) {
		gl_move_copy_to_album(ad);
	}
}

static void __gl_popup_move_copy_sel_cb(void *data, Evas_Object *obj, void *ei)
{
	gl_dbg("");
	GL_CHECK(obj);
	GL_CHECK(data);
	gl_popup_menu_s *menu_d = evas_object_data_get(obj, GL_POPUP_OBJ_DATA_KEY);
	GL_CHECK(menu_d);
	GL_CHECK(menu_d->ad);
	gl_appdata *ad = menu_d->ad;
	GL_FREEIF(ad->albuminfo.path);

	Elm_Object_Item *it = (Elm_Object_Item *)ei;
	Elm_Object_Item *it_f = NULL;
	Elm_Object_Item *it_l = NULL;
	bool flag = false;

	it_f = elm_genlist_first_item_get(obj);
	it_l = elm_genlist_last_item_get(obj);

	if (it_f && it_l) {
		if (elm_genlist_item_selected_get(it_l)) {
			flag = true;
		}
		while (it_f != NULL && it_f != it_l && !flag) {
			if (elm_genlist_item_selected_get(it_f)) {
				flag = true;
				break;
			}
			it_f = elm_genlist_item_next_get(it_f);
		}
	}

	if (flag) {
		ad->gridinfo.it = it;
		if (data) {
			ad->albuminfo.path = strdup((char *)data);
		}
		if (_gl_thumbs_get_edit_mode(ad) == GL_THUMBS_EDIT_COPY) {
			__gl_thumbs_edit_copy_cb(ad, NULL, NULL);
		} else if (_gl_thumbs_get_edit_mode(ad) == GL_THUMBS_EDIT_MOVE) {
			__gl_thumbs_edit_move_cb(ad, NULL, NULL);
		}
		return;
	}
}

static char *__gl_popup_menu_get_genlist_text(void *data, Evas_Object *obj,
				const char *part)
{
	gl_dbg("");
	gl_cluster *album = (gl_cluster *)data;
	char *label = NULL;
	label = _gl_get_i18n_album_name(album->cluster);

	if (!strcmp(part, "elm.text")) {
		char *str = NULL;
		GL_CHECK_NULL(label);
		str = _gl_str((char *)label);
		if (str) {
			return strdup(str);
		}
	}
	return NULL;
}

Evas_Object *__gl_popup_menu_get_genlist_content(void *data, Evas_Object *obj,
				const char *part)
{
	gl_cluster *album_item = (gl_cluster *)data;
	Evas_Object *ic = elm_icon_add(obj);

	if (!strcmp(part, "elm.swallow.icon")) {
		if (GL_STORE_T_MMC == album_item->cluster->type) {
			GL_ICON_SET_FILE(ic, GL_ICON_MMC);
			evas_object_size_hint_aspect_set(ic, EVAS_ASPECT_CONTROL_VERTICAL, 1, 1);
		} else {
			GL_ICON_SET_FILE(ic, GL_ICON_FOLDER);
			evas_object_size_hint_aspect_set(ic, EVAS_ASPECT_CONTROL_VERTICAL, 1, 1);
		}
	}
	return ic;
}

void _gl_genlist_item_apend(void *data, Evas_Object *gl, Elm_Genlist_Item_Class *gic)
{
	GL_CHECK(gic);
	GL_CHECK(gl);
	GL_CHECK(data);
	gl_popup_menu_s *menu_d = (gl_popup_menu_s *)data;
	GL_CHECK(menu_d->ad);
	gl_appdata *ad = menu_d->ad;
	GL_CHECK(ad->maininfo.win);
	int i = 0;
	Eina_List *album_list = NULL;
	int all_count = 0;
	gl_cluster *album = NULL;

	evas_object_data_set(gl, GL_POPUP_OBJ_DATA_KEY, menu_d);
	if (ad->albuminfo.elist && ad->albuminfo.elist->clist) {
		album_list = ad->albuminfo.elist->clist;
		all_count = eina_list_count(album_list);
	} else {
		gl_dbgE("cluster list is NULL");
		return;
	}

	for (i = 0; i < all_count; i++) {
		album = eina_list_nth(album_list, i);
		GL_CHECK(album);
		GL_CHECK(album->cluster);
		GL_CHECK(album->cluster->display_name);
		GL_CHECK(album->cluster->uuid);

		if (album->cluster->type == GL_STORE_T_MMC ||
		        album->cluster->type == GL_STORE_T_PHONE) {
			elm_genlist_item_append(gl, gic, (void *)album,
			                        NULL, ELM_GENLIST_ITEM_NONE,
			                        __gl_popup_move_copy_sel_cb,
			                        album->cluster->path);
		}
	}
}

int _gl_popup_create_copy_move(void *data, void *sel_cb, void *cb_data)
{
	GL_CHECK_VAL(cb_data, -1);
	GL_CHECK_VAL(data, -1);
	gl_dbg("");
	gl_appdata *ad = (gl_appdata *)data;

	if (ad->popupinfo.popup) {
		evas_object_del(ad->popupinfo.popup);
		ad->popupinfo.popup = NULL;
	}

	Evas_Object *parent = ad->maininfo.win;
	Evas_Object *popup = elm_popup_add(parent);
	evas_object_size_hint_weight_set(popup, EVAS_HINT_EXPAND,
	                                 EVAS_HINT_EXPAND);
	if (_gl_thumbs_get_edit_mode(data) == GL_THUMBS_EDIT_COPY) {
		_gl_ui_set_translate_part_str(popup, "title,text", GL_STR_ID_COPY);
	} else if (_gl_thumbs_get_edit_mode(data) == GL_THUMBS_EDIT_MOVE) {
		_gl_ui_set_translate_part_str(popup, "title,text", GL_STR_ID_MOVE);
	}
	evas_object_show(popup);
	ad->popupinfo.popup = popup;

	Evas_Object *btn1 = elm_button_add(parent);
	Evas_Object *btn2 = elm_button_add(parent);
	elm_object_style_set(btn1, "default");
	elm_object_style_set(btn2, "default");
	evas_object_size_hint_weight_set(btn1, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	evas_object_size_hint_weight_set(btn2, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	evas_object_size_hint_align_set(btn1, 1, 1);
	evas_object_size_hint_align_set(btn2, 1, 1);
	elm_object_part_content_set(popup, "button1", btn1);
	elm_object_part_content_set(popup, "button2", btn2);
	evas_object_smart_callback_add(btn1, "clicked", __gl_thumbs_edit_cancel_cb, ad);
	evas_object_smart_callback_add(btn2, "clicked", __gl_thumbs_edit_create_album_cb, ad);
	_gl_ui_set_translate_part_str(btn1, "default", GL_STR_ID_CANCEL);
	_gl_ui_set_translate_part_str(btn2, "default", GL_STR_ID_NEW_ALBUM);

	gl_popup_menu_s *menu_d = NULL;
	menu_d = (gl_popup_menu_s *)calloc(1, sizeof(gl_popup_menu_s));
	GL_CHECK_VAL(menu_d, -1);
	menu_d->ad = ad;
	Evas_Object *genlist = __gl_popup_add_genlist(menu_d, popup, "default",
	                       __gl_popup_menu_get_genlist_text, __gl_popup_menu_get_genlist_content,
	                       _gl_genlist_item_apend);
	Evas_Object *ly = elm_layout_add(popup);
	elm_layout_file_set(ly, GL_EDJ_FILE, "list_popup_swallow_ly");
	Evas_Object *ly1 = elm_layout_add(popup);
	elm_layout_file_set(ly1, GL_EDJ_FILE, "list_popup_content_ly");
	elm_object_part_content_set(ly, "list", genlist);
	elm_object_content_set(popup, ly);
	int w, h;
	evas_object_geometry_get(ad->maininfo.win, NULL, NULL, &w, &h);
	Edje_Message_Int *msg = (Edje_Message_Int *)malloc(sizeof(Edje_Message_Int) + sizeof(int));
	GL_CHECK_VAL(msg, -1);
	if ((w == GL_STR_SCREEN_WIDTH_WVGA && h == GL_STR_SCREEN_HEIGHT_WVGA) || (w == GL_STR_SCREEN_HEIGHT_WVGA && h == GL_STR_SCREEN_WIDTH_WVGA)) {
		msg->val = GL_POPUP_GENLIST_HEIGHT_WVGA;
		edje_object_message_send(elm_layout_edje_get(ly) , EDJE_MESSAGE_INT, 2, msg);
	} else if ((w == GL_STR_SCREEN_WIDTH_QHD && h == GL_STR_SCREEN_HEIGHT_QHD) || (w == GL_STR_SCREEN_HEIGHT_QHD && h == GL_STR_SCREEN_WIDTH_QHD)) {
		msg->val = GL_POPUP_GENLIST_HEIGHT_QHD;
		edje_object_message_send(elm_layout_edje_get(ly), EDJE_MESSAGE_INT, 2, msg);
	} else if ((w == GL_STR_SCREEN_WIDTH_HD && h == GL_STR_SCREEN_HEIGHT_HD) || (w == GL_STR_SCREEN_HEIGHT_HD && h == GL_STR_SCREEN_WIDTH_HD)) {
		msg->val = GL_POPUP_GENLIST_HEIGHT_HD;
		edje_object_message_send(elm_layout_edje_get(ly), EDJE_MESSAGE_INT, 2, msg);
	}
	free(msg);
	evas_object_show(genlist);
	eext_object_event_callback_add(popup, EEXT_CALLBACK_BACK,
	                               eext_popup_back_cb, NULL);

	return 0;
}
