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

#ifndef _GALLERY_H_
#define _GALLERY_H_

#include <pthread.h>
#include <app.h>
#include <efl_extension.h>

#include "gl-data.h"
#include "gl-util.h"

#ifndef EXPORT_API
#define EXPORT_API __attribute__ ((visibility("default")))
#endif

#ifdef __cplusplus
extern "C"
{
#endif

#if !defined(APPDIR)
#  define APPDIR "/usr/apps/org.tizen.gallery"
#endif

#if !defined(PACKAGE)
#  define PACKAGE "gallery"
#endif

#if !defined(LOCALEDIR)
#  define LOCALEDIR APPDIR"/res/locale"
#endif

#if !defined(EDJDIR)
#  define EDJDIR APPDIR"/res/edje"
#endif

#if !defined(IMAGEDIR)
#  define IMAGEDIR APPDIR"/res/images/"
#endif

#define PKGNAME_GALLERY "org.tizen.gallery"
#define GL_APP_DEFAULT_ICON_DIR "/usr/share/icons/default/small"

/******** EDJ File path ********/

#define GL_EDJ_FILE "gallery.edj"

/******** Groups name ********/

#define GL_GRP_CONTROLBAR "gallery/layout"
#define GL_GRP_GRIDVIEW "gallery/gridview"
#define GL_GRP_PLACES_MARKER "gallery/place_marker"
#define GL_GRP_PROGRESSBAR "gallery/progressbar"
#define GL_GRP_PHOTOFRAME "photoframe"

/******* Part name ***********/

#define GL_NAVIFRAME_PREV_BTN "prev_btn"
#define GL_NAVIFRAME_TITLE_PREV_BTN "title_prev_btn"
#define GL_NAVIFRAME_TITLE_RIGHT_BTN "title_right_btn"
#define GL_NAVIFRAME_TITLE_LEFT_BTN "title_left_btn"
#define GL_NAVIFRAME_TITLE_MORE_BTN "title_more_btn"
#define GL_NAVIFRAME_TITLE_BTN1 "title_toolbar_button1"
#define GL_NAVIFRAME_TITLE_BTN2 "title_toolbar_button2"

#define GL_NAVIFRAME_BTN1 "toolbar_button1"
#define GL_NAVIFRAME_BTN2 "toolbar_button2"
#define GL_NAVIFRAME_MORE "toolbar_more_btn"
#define GL_NAVIFRAME_TOOLBAR "toolbar"

#define GL_POPUP_TEXT "title,text"
#define GL_NOCONTENTS_TEXT "elm.text"
#define GL_NOCONTENTS_TEXT2 "elm.text.2"
#define GL_GRID_TEXT "elm.text"
#define GL_NAAVIFRAME_TEXT "elm.text.title"

#define GL_IMF_ON "virtualkeypad,state,on"
#define GL_IMF_OFF "virtualkeypad,state,off"
#define GL_STA_ON "clipboard,state,on"
#define GL_STA_OFF "clipboard,state,off"

/******** Style name ********/

#define GL_CLASS_GENGRID "gengrid"
#ifdef _USE_CUSTOMIZED_GENGRID_STYLE
#define GL_GENGRID_STYLE_GALLERY "gallery"
#endif

#define GL_BUTTON_STYLE_RENAME "rename" //"gallery/rename"
#define GL_BUTTON_STYLE_NAVI_MORE "naviframe/more/default"
#define GL_BUTTON_STYLE_NAVI_TITLE "naviframe/title_text"
#define GL_BUTTON_STYLE_NAVI_TITLE_ICON "naviframe/title_icon"
#define GL_BUTTON_STYLE_NAVI_TOOLBAR "naviframe/toolbar/default"
#define GL_BUTTON_STYLE_NAVI_TOOLBAR_LEFT "naviframe/toolbar/left"
#define GL_BUTTON_STYLE_NAVI_TOOLBAR_RIGHT "naviframe/toolbar/right"

/* Button object style of previous button of naviframe */
#define GL_BUTTON_STYLE_NAVI_PRE "naviframe/back_btn/default"
#ifdef _USE_ROTATE_BG
#define GL_BUTTON_STYLE_NAVI_CTRL "naviframe_control/multiline"
#endif

#define GL_CHECKBOX_STYLE_DEFAULT "gallery/default"
#define GL_GENLIST_STYLE_DEFAULT GL_CHECKBOX_STYLE_DEFAULT
#define GL_POPUP_STYLE_DEFAULT GL_CHECKBOX_STYLE_DEFAULT
#define GL_CONTROLBAR_STYLE_NAVI "naviframe"
#define GL_CBAR_STYLE_DEFAULT GL_CHECKBOX_STYLE_DEFAULT

/******** String ********/

#define GL_SEPERATOR_BT '?'
#define GL_ARGV_MSS_TYPE "MSS_Type"
#define GL_ARGV_MSS_3 "3"
#define GL_ARGV_MSS_SORT_TYPE "MSS_Sort_type"
#define GL_ARGV_MSS_SORT_1 "1"

/* MIME type */
#define GL_MIME_IMAGE_ALL	"image/*"
#define GL_MIME_VIDEO_ALL	"video/*"

#define GL_AUL_PREFIX "gallery:"
#define GL_AUL_IMAGE "imageviewer"

/******** Numerical value ********/

/* Window width and height */
#define GL_WIN_WIDTH 720
#define GL_WIN_HEIGHT 1280
/*naviframe title + naviframe bottom toobar */
#define GL_FIXED_HEIGHT (111 + 98)

/* Specified ID for customed items, which don't exist in DB */
#define GL_TAG_FAVORITES_ID 0
#define GL_ALBUM_ALL_ID "GALLERY_ALBUM_ALL_ALBUMS_ID"
#define GL_ALBUM_FAVOURITE_ID "GALLERY_ALBUM_FAVOURITE_ALBUMS_ID"
#define GL_ALBUM_ALLSHARE_ID "GALLERY_ALBUM_ALLSHARE_ID"
#define GL_ALBUM_PTP_ID "GALLERY_ALBUM_PTP_ID"

/* Size of thumbnails in gridview */
#define GL_GRID_ITEM_CNT 32
/* Thumbnail would be enlarged if medias count is less than 7 */
#define GL_GRID_ITEM_ZOOM_LEVEL_02 2
#define GL_GRID_ITEM_ZOOM_LEVEL_03 (1.35f)
#define GL_GRID_ITEM_ZOOM_LEVEL_02_CNT 8

#define GL_MOUSE_RANGE 10

/* Range of longitude and latitude */
#define GL_MAP_LONG_MAX 180.0
#define GL_MAP_LONG_MIN (-180.0)
#define GL_MAP_LAT_MAX 90.0
#define GL_MAP_LAT_MIN (-90.0)
#define GL_MAP_GEO_INIT 1000.0

/* String length of mtye item id */
#define GL_MTYPE_ITEN_ID_LEN_MAX 37

/* String length of album name */
#define GL_ALBUM_NAME_LEN_MAX GL_FILE_NAME_LEN_MAX

#define GL_DATE_INFO_LEN_MAX 256
#define GL_POPUP_DESC_LEN_MAX 256
#define GL_EXT_STR_COUNT_LEN 20
#define GL_FACE_DETECT_INFO_LEN_MAX 128

/* String length for webalbum */
#define GL_WEB_ALBUM_TITLE_LEN_MAX 256
#define GL_WEB_ALBUM_ID_LEN_MAX GL_WEB_ALBUM_TITLE_LEN_MAX
#define GL_WEB_WEBNODE_NAME_LEN_MAX 128
#define GL_WEB_WEBNODE_ALBUM_ID_LEN_MAX GL_WEB_WEBNODE_NAME_LEN_MAX
#define GL_WEB_WEBNODE_TOKEN_LEN_MAX 512

#define GL_PLACE_DATA_STR_LEN_MAX 64

/* String length for PTP album */
#define GL_PTP_MODEL_LEN_MAX 128

/* 1900 */
#define GL_DEFAULT_YEAR 1900
/* 1s=1000ms */
#define GL_TIME_MSEC_PER_SEC 1000
/*1s = 1000000us*/
#define GL_TIME_USEC_PER_SEC 1000000L
/* 1ms=1000us */
#define GL_TIME_USEC_PER_MSEC 1000
/* 1min=60s */
#define GL_TIME_SEC_PER_MIN 60
/* 1h=60min */
#define GL_TIME_MIN_PER_HOUR 60
/* 1h=(60x60)s */
#define GL_TIME_SEC_PER_HOUR (GL_TIME_MIN_PER_HOUR * GL_TIME_SEC_PER_MIN)

/* itoa length max ----> 1(sign) + 10(2^31) + 1(NULL) */
#define GL_INTERGER_LEN_MAX 12

#define GL_ERROR_BUF_LEN 1024

/* File system related String definition */
#define GL_ROOT_PATH_PHONE		"/opt/usr/media"
#define GL_ROOT_PATH_MMC	"/opt/storage/sdcard"
#define GL_DEFAULT_PATH_IMAGES GL_ROOT_PATH_PHONE"/Pictures" // refer to s4
#define GL_DEFAULT_PATH_PICTURES GL_ROOT_PATH_PHONE"/Images"
#define GL_DOWNLOADS_PATH GL_ROOT_PATH_PHONE"/Downloads"
#define GL_DATA_FOLDER_PATH "/opt/usr/apps/"PKGNAME_GALLERY"/data"

#define GL_NAVIFRAME_OBJ_DATA_KEY "gl_naviframe_data_key"
#define GL_NAVIFRAME_UG_UPDATE_KEY "gl_naviframe_ug_update_key"
#define GL_NAVIFRAME_UG_RESULT_KEY "gl_naviframe_ug_result_key"
#define GL_NAVIFRAME_SLIDESHOW_DATA_KEY "gl_naviframe_slideshow_data_key"
#ifdef _USE_APP_SLIDESHOW
#define GL_NAVIFRAME_SELECTED_SLIDESHOW_KEY "gl_naviframe_selected_slideshow_key"
#endif
#define GL_NAVIFRAME_POP_CB_KEY "gl_naviframe_pop_cb_key"

/******** Enumeration ********/
typedef enum _gl_view_mode
{
	GL_VIEW_NONE,
	GL_VIEW_ALBUMS,
	GL_VIEW_ALBUMS_EDIT,
	GL_VIEW_ALBUMS_RENAME,
	GL_VIEW_ALBUMS_SELECT,
	GL_VIEW_TIMELINE,
	GL_VIEW_THUMBS,
	GL_VIEW_THUMBS_EDIT,
	GL_VIEW_THUMBS_SELECT,
	GL_VIEW_ALBUM_REORDER,
} gl_view_mode;

typedef enum _gl_app_exit_mode
{
	GL_APP_EXIT_NONE,
	GL_APP_EXIT_MASS_STORAGE,
} gl_app_exit_mode;

typedef enum _gl_entry_mode
{
	GL_ENTRY_NONE,
	GL_ENTRY_NEW_ALBUM,
	GL_ENTRY_RENAME_ALBUM,
	GL_ENTRY_RENAME_TAG,
} gl_entry_mode;

/* Grid view created for different thumbnails view types */
enum _gl_gird_t {
	GL_GRID_T_NONE,
	GL_GRID_T_LOCAL,
	GL_GRID_T_ALLALBUMS,
	GL_GRID_T_FAV,
};
typedef enum _gl_gird_t gl_grid_t_e;

enum _gl_edit_mode_t {
	GL_GRID_EDIT = 0,
	GL_GRID_SHARE,
};
typedef enum _gl_edit_mode_t _gl_edit_mode_t_e;

typedef enum  _gl_detail_view_display_order {
	ORDER_ASC = 0,
	ORDER_DESC
}_gl_view_order;

typedef enum _gl_album_detail_view_mode {
	DETAIL_VIEW = 0,
	SPLIT_VIEW
}_gl_detail_view_mode;
/******** Structure ********/

typedef struct gl_db_noti_t gl_db_noti_s;
typedef struct _gl_timeline_t gl_timeline_s;

typedef struct
{
	Evas_Object *ctrlbar_ly;
	/* Layout of albums view/tags view/places view */
	Evas_Object *ctrlbar_view_ly;
	/* Naviframe item pushed to stack */
	Elm_Object_Item *nf_it;
	int tab_mode;	 /* State of controlbar item: Albums, Places or Tags */
	Ecore_Timer *avoid_multi_tap;
} gl_ctrl_info;

typedef struct
{
	Evas_Object *gesture_sel;
	Evas_Object *gesture;
	int zoom_level;
} gl_pinch_info;

typedef struct _gl_mouse_t gl_mouse_s;
struct _gl_mouse_t {
	Evas_Coord x;
	Evas_Coord y;
	bool b_pressed;
};

typedef struct
{
	Elm_Gengrid_Item_Class date_gic; /* Gengrid item class of date view */
	Evas_Object *layout;
	/* Thumbnail selection view for Add tag to photo */
	Evas_Object *select_view;
	Evas_Object *select_layout;
	Evas_Object *view;
	Evas_Object *nocontents;
	bool b_slideshow_disable;
	/* Edit */
	bool is_append;
	/* It's in edit view */
	int edit_mode;
	/* Naviframe item pushed to stack */
	Elm_Object_Item *nf_it;
	Elm_Object_Item *nf_it_select;
	char *title;
	gl_mouse_s mouse;	/* Added for shrink effect while select thumbnail */
	Ecore_Timer *clicked_timer;
	/* It's thumbnails icon size, not grid item size */
	int icon_w;
	int icon_h;
					/* if album contains more than (GL_FIRST_VIEW_END_POS+1) medias */
	Eina_List *medias_elist;
	int count;		/* Media items count appended to gridview including burst shot*/
	bool back_to_normal;	/* When back from edit mode, reset checkbox state */
	_gl_view_order media_display_order; /* keep track of the media items display order */
	gl_grid_t_e grid_type;
	Elm_Object_Item *it; /* Keep track of selected item in move copy popup*/
	_gl_detail_view_mode split_view_mode;
	bool multi_touch;
	bool update_split_view;
} gl_grid_info;

typedef struct
{
	bool b_app_called; /* Indicates if any application invoked by Gallery */
	app_control_h ug;
	int ug_type;
	int iv_type;
	void *data;
	int sort_type;	/* Types: wminfo_media_sort_type; pass it to imageviewer */
	bool b_start_slideshow; /* Start slideshow from slideshow-setting ug */
	bool b_ug_launched;
#ifdef _USE_APP_SLIDESHOW
	bool b_selected_slideshow;
#endif
} gl_ug_info;

typedef struct
{
	Elm_Gengrid_Item_Class gic;	/* Gengrid item class of albums view */
	Evas_Object *view;	 /* Albums view object */
	Evas_Object *nocontents; /* Nocontents object */
	/* Albums view object for Add tag to photo */
	Evas_Object *select_view;
	/* Albums view layout object for Add tag to photo */
	Evas_Object *select_layout;

	int b_share_mode; /* Direct share mode */
	gl_cluster *current; /* Album selected for showing thumbnails view/list view */
	/* Album selected for rename/open locked album */
	gl_cluster *selected;
	/* Save destination album id while move/save */
	char *path;
	char new_name[GL_ALBUM_NAME_LEN_MAX];
	bool b_new_album;
	char *temp_album_name;
	int file_mc_mode;
	Eina_List *sel_elist;	/* List of all selected albums */
	gl_cluster_list *elist; /* List of all albums */
	int albums_cnt;		 /* Albums count appended to gridview */
	char dest_folder[GL_DIR_PATH_LEN_MAX];	/* The full path of destination album of movement */

	Elm_Object_Item *nf_it_select;
	Elm_Object_Item *grid_sel_item;	/* currently selected album grid item will be stored here */
	char *selected_uuid; /* currently selected album's uuid will be stored here */
} gl_album_info;

typedef struct
{
	Evas_Object *editfield;
	Evas_Object *entry;
	Evas_Object *popup;
	Ecore_IMF_Context *context;
	Elm_Entry_Filter_Limit_Size *limit_filter;

	/* Type: gl_entry_mode; Indicate usage of entry */
	int mode;
	/* Button 'Done' in new album/tag view */
	Evas_Object *done_btn;
	Elm_Object_Item *nf_it;
	void *op_cb; /* Operation after done pressed */
	bool b_hide_toolbar;
	bool b_hide_sip;
} gl_entry_info;

typedef struct
{
	Evas_Object *popup;
	Evas_Object *memory_full_popup;
	int mode;
	/* Selectioninfo popup layout */
	Ecore_Timer *del_timer;
} gl_popup_info;

typedef struct {
	Eina_List *elist; /* List of all selected medias of a album/tag/marker */
	Eina_List *fav_elist; /* List of fav medias of a album/tag/marker */
	int images_cnt;	/* Images count selected */
	int jpeg_cnt;	/* JEPG files count selected */
	int disable_hide_cnt;	/* Disable hide files count selected */
	/* All media count */
	int sel_cnt;
	/* Checkbox state of select-all */
	Eina_Bool ck_state;
	Evas_Object *selectall_ck;
	Eina_List *copy_elist;
} gl_selected_info;

typedef struct {
	Evas_Object *popup;
	Evas_Object *pbar;
	Evas_Object *status_label;
	Ecore_Timer *pbar_timer;
	/**
	* After progressbar showed completely, use timer to
	* emit next signal to write pipe to continue to operate medias.
	*/
	Ecore_Timer *start_thread_timer;
	/**
	* After thread operation done, use idler to
	* delete progressbar after it's showed completely(status is 100%).
	*/
	Ecore_Job *del_pbar_job;
	Ecore_Timer *del_pbar_timer;
	Ecore_Pipe *sync_pipe;	   /* Pipe for communication between main and child thread */
	pthread_mutex_t pbar_lock; /* Lock for state of 'Cancel' button */
	pthread_mutex_t refresh_lock; /* Lock for progressbar refreshing */
	pthread_cond_t refresh_cond;  /* Condition for progressbar refreshing */
	int refresh_flag; /* Indicates progressbar updated or not while moving or deleting */
	int pbar_cancel;  /* State of 'Cancel' button on progressbar popup */
	/* Medias count already operated */
	int finished_cnt;
	int total_cnt;
	void *op_cb;
	void *update_cb;
	void *del_item_cb;
} gl_pbar_info;

typedef struct
{
	Evas_Object *win;
	Evas_Object *layout;
	Evas_Object *naviframe;
	Evas_Object *bg;
	Evas_Object *ctxpopup;
	Evas_Object *entry_win; /* For 3D view */
	int rotate_mode; /* Type: appcore_rm; Indicate rotation mode of whole application */
	int view_mode;	 /* Type: gl_view_mode; Indicate view mode of whole application */
	bool reentrant;
	bool b_paused;
	/* Mouse down event handle */
	Ecore_Event_Handler *keydown_handler;
	/* Font type change event handle */
	Ecore_Event_Handler *font_handler;
	void *dlopen_iv_handle; /* Handle for dlopen imageviewer lib .so */
	/* Use idler to register ASF when launching Gallery */
	Ecore_Timer *reg_idler;
	bool b_reged_idler;

	int all_medias_cnt;
	time_t last_mtime;
	int medias_op_type;	 /* Type: Move or Delete medias */
	int mmc_state;		 /* MMC state(Inserted, Removed) */
	/* SVI, for playing touch sound */
	int svi_handle;
	int sound_status;
	Ecore_Idler *mmc_idler;
	int externalStorageId;
	bool lang_changed;
	bool hide_noti;
} gl_main_info;


struct _gl_appdata
{
	gl_main_info maininfo;	 /* Global variables about webalbum */
	gl_ctrl_info ctrlinfo;	 /* Global variables about controlbar */
	gl_album_info albuminfo; /* Global variables about albums view */
	gl_grid_info gridinfo;	 /* Global variables about thumbnails view */
	gl_pinch_info pinchinfo; /* Global variables about pinch zoom out */
	gl_entry_info entryinfo; /* Global variables about entry */
	gl_popup_info popupinfo; /* Global variables about popup */
	gl_pbar_info pbarinfo;	 /* Global variables about progressbar */
	gl_ug_info uginfo;	 /* Global variables about ug */
	gl_selected_info selinfo; /* Global variables about files selected */
	gl_timeline_s *tlinfo; /* Global variables about TIMELINE view */
	/* Data about using inotify */
	gl_db_noti_s *db_noti_d;
};

typedef struct _gl_appdata gl_appdata;

#ifdef __cplusplus
}
#endif

#endif /* _GALLERY_H_ */

