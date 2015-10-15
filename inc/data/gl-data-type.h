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

#ifndef _GL_DATA_TYPE_H_
#define _GL_DATA_TYPE_H_

#include <stdlib.h>
#include <media_content.h>
#include <eina_list.h>
#include <Elementary.h>

#ifdef __cplusplus
extern "C"
{
#endif

#define GL_FREEIF(ptr) \
	do { \
		if(ptr != NULL) { \
			free(ptr); \
			ptr = NULL; \
		} \
	} while (0)

#define GL_FREE(ptr) \
	do { \
		free(ptr); \
		ptr = NULL; \
	} while (0)

#define GL_GFREEIF(ptr) \
	do { \
		if(ptr != NULL) { \
			g_free(ptr); \
			ptr = NULL; \
		} \
	} while (0)

#define GL_GFREE(ptr) \
	do { \
		g_free(ptr); \
		ptr = NULL; \
	} while (0)

#define GL_TYPE_ALBUM (0x55551)
#define GL_TYPE_MEDIA (0x55553)
#define GL_TYPE_WEB_MEDIA (0x55554)

typedef enum {
	GL_STORE_T_PHONE = 0,	/**< Stored only in phone */
	GL_STORE_T_MMC,		/**< Stored only in MMC */
	GL_STORE_T_ALL,		/**< Stored only in All albums album */
	GL_STORE_T_FAV,		/**< Stored only in Favorites album */
} gl_store_type_e;


typedef struct _gl_album_t gl_album_s;
typedef struct _gl_media_t gl_media_s;
typedef struct _gl_image_t gl_image_s;
typedef struct _gl_video_t gl_video_s;

struct _gl_album_t {
	int gtype;                         /*self-defination type, when free this struct space, use it*/
	media_folder_h folder_h;           /*the handle of operating this folder*/
	char *uuid;                        /*folder  UUID*/
	char *display_name;                /*album name*/
	char *path;                        /*the full path of this folder*/
	int type;                          /*storage type*/
	time_t mtime;                      /*modified time*/
	int count;                         /*the media count in this folder*/
};

struct _gl_media_t {
	void *ad;
	int gtype;                         /*self-defination type, when free this struct space, use it*/
	media_info_h media_h;              /*the handle of operating this media*/
	char *uuid;                        /*meida id*/
	int type;                          /*meida type, image or video*/
	char *thumb_url;                   /*the thumbnail full path of this meida file*/
	char *file_url;                    /*the full path of this meida file*/
	time_t mtime;                      /*modified time*/
	char *display_name;                /*item name*/
	int mode; /* Show icon indicates different camera shot mode */
	char *ext;
	gl_store_type_e storage_type;      /* Storage type got from DB */
	bool b_create_thumb;   /* Request DB to create thumbnail */
	bool check_state;   /* store check state */
	Elm_Object_Item *elm_item;   /*store item handle */

	union {
		gl_image_s *image_info;    /*image information*/
		gl_video_s *video_info;    /*video information*/
	};
};

struct _gl_image_t {
	char *media_uuid;                  /*media uuid*/
	image_meta_h image_h;              /*the handle of operating this image*/
	int orientation;                   /*the orientation of this image*/
	char *burstshot_id;                /*the tag of burst shot*/
};

struct _gl_video_t {
	char *media_uuid;                  /*media uuid*/
	video_meta_h video_h;              /*the handle of operating this video*/
	char *title;                       /*the title of video*/
	time_t last_played_pos;            /*the last played position*/
	int duration;                      /*the duration of this video*/
	int bookmarks;                     /*whether exist bookmarks*/
};

int _gl_data_type_new_media(gl_media_s **item);
int _gl_data_type_new_album(gl_album_s **album);

int _gl_data_type_free_media_list(Eina_List **list);
int _gl_data_type_free_album_list(Eina_List **list);
int _gl_data_type_free_glitem(void **item);

#ifdef __cplusplus
}
#endif


#endif /* _GL_DATA_TYPE_H_ */

