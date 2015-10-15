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

#ifndef _GL_TILE_H_
#define _GL_TILE_H_

#include "gl-data.h"

#define GL_TILE_ICON "elm.swallow.icon"
#define GL_TILE_CHECKBOX "elm.swallow.check"
#define GL_TILE_CHECKBOX_GRID "elm.swallow.check_grid"
#define GL_TILE_TYPE_ICON "elm.swallow.type_icon"
#define GL_TILE_SD_ICON "elm.swallow.sd_icon"
#define GL_TILE_PABR "elm.swallow.progressbar"
#define GL_TILE_DIM "elm.swallow.dim"

#define GL_GENGRID_STYLE_ALBUM_VIEW  "gallery/albums_view"
#define GL_GENGRID_STYLE_ALBUM_SPLIT_VIEW  "gallery/albums_split_view"

/* Album type for showing different icon in snsicon part in edc */
typedef enum _gl_tile_type {
	GL_TILE_TYPE_NONE,
	GL_TILE_TYPE_MMC_STORAGE,		       /* MMC album */
	GL_TILE_TYPE_CAMERA, /* Camera album */
	GL_TILE_TYPE_DOWNLOAD, /* Download album */
	GL_TILE_TYPE_FOLDER, /* Folder album */
} gl_tile_type;

typedef int (*bg_file_set_cb) (Evas_Object *bg, void *data);

Evas_Object *_gl_tile_show_part_icon(Evas_Object *obj, const char *part,
				     bg_file_set_cb func, void *data);
Evas_Object *_gl_tile_show_part_dim(Evas_Object *obj);
Evas_Object *_gl_tile_show_part_checkbox_grid(Evas_Object *obj, bool checked,
					      Evas_Smart_Cb func,
					      const void *data);
Evas_Object *_gl_tile_show_part_checkbox(Evas_Object *obj, bool checked,
					 Evas_Smart_Cb func, const void *data);
Evas_Object *_gl_tile_show_part_rename(Evas_Object *obj, Evas_Smart_Cb func,
				       const void *data);
Evas_Object *_gl_tile_show_part_type_icon(Evas_Object *obj, int sns_type);
int _gl_tile_get_mtime(time_t *mtime1, time_t *mtime2, char *buf, int max_len);
Evas_Object *_gl_tile_add_gengrid(Evas_Object *parent);
int _gl_tile_update_item_size(void *data, Evas_Object *grid, bool b_update);
Evas_Object *_gl_tile_add_gesture(void *data, Evas_Object *parent);

#endif

