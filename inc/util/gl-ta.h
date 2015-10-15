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

#ifndef _GL_TA_H_
#define _GL_TA_H_


#ifdef _ENABLE_TA
// defs.
#define GL_TA_MAX_CHECKPOINT	500
#define GL_TA_MAX_ACCUM		500
#define GL_TA_TIME_BUF_LEN 256

typedef struct _gl_ta_checkpoint
{
	unsigned long timestamp;
	char *name;
} gl_ta_checkpoint;

typedef struct _gl_ta_accum_item
{
	unsigned long elapsed_accum;
	unsigned long num_calls;
	unsigned long elapsed_min;
	unsigned long elapsed_max;
	unsigned long first_start;
	unsigned long last_end;

	char *name;

	unsigned long timestamp;
	int on_estimate;
	int num_unpair;
} gl_ta_accum_item;

#define GL_TA_SHOW_STDOUT	0
#define GL_TA_SHOW_STDERR	1
#define GL_TA_SHOW_FILE	2
#define GL_TA_RESULT_FILE "/tmp/gallery-ta.log"

/////////////////////////////
// COMMON
int gl_ta_init(void);
int gl_ta_release(void);
void gl_ta_set_enable(int enable);
char *gl_ta_fmt(const char *fmt, ...);

/////////////////////////////
// CHECK POINT
int gl_ta_add_checkpoint(char *name, int show, char *filename, int line);
void gl_ta_show_checkpoints(void);
void gl_ta_show_diff(char *name1, char *name2);

int gl_ta_get_numof_checkpoints();
unsigned long gl_ta_get_diff(char *name1, char *name2);
char *gl_ta_get_name(int idx);


/////////////////////////////
// ACCUM ITEM
int gl_ta_accum_item_begin(char *name, int show, char *filename, int line);
int gl_ta_accum_item_end(char *name, int show, char *filename, int line);
void gl_ta_accum_show_result(int direction);

// macro.
#define GL_TA_INIT()								(	gl_ta_init()												)
#define GL_TA_RELEASE()							(	gl_ta_release()											)
#define GL_TA_SET_ENABLE(enable)				(	gl_ta_set_enable(enable)								)

// checkpoint handling
#define GL_TA_ADD_CHECKPOINT(name,show)		(	gl_ta_add_checkpoint(name,show,__FILE__,__LINE__)		)
#define GL_TA_SHOW_CHECKPOINTS()				(	gl_ta_show_checkpoints()								)
#define GL_TA_SHOW_DIFF(name1, name2)			(	gl_ta_show_diff(name1, name2)							)
#define GL_TA_GET_NUMOF_CHECKPOINTS()			(	gl_ta_get_numof_checkpoints()							)
#define GL_TA_GET_DIFF(name1, name2)			(	gl_ta_get_diff(name1, name2)							)
#define GL_TA_GET_NAME(idx)						(	gl_ta_get_name(idx)									)

// accum item handling
#define GL_TA_ACUM_ITEM_BEGIN(name,show)		(	gl_ta_accum_item_begin(name,show,__FILE__,__LINE__)	)
#define GL_TA_ACUM_ITEM_END(name,show)		(	gl_ta_accum_item_end(name,show,__FILE__,__LINE__)		)
#define GL_TA_ACUM_ITEM_SHOW_RESULT()		(	gl_ta_accum_show_result(GL_TA_SHOW_STDOUT)			)
#define GL_TA_ACUM_ITEM_SHOW_RESULT_TO(x)	(	gl_ta_accum_show_result(x)							)

#define __gl_ta__(name, x) \
GL_TA_ACUM_ITEM_BEGIN(name, 0); \
x \
GL_TA_ACUM_ITEM_END(name, 0);

#define __gl_tafmt__(fmt, args...) 			(	gl_ta_fmt(fmt, ##args)	)

#else // _ENABLE_TA

#define GL_TA_INIT()
#define GL_TA_RELEASE()
#define GL_TA_SET_ENABLE(enable)

// checkpoint handling
#define GL_TA_ADD_CHECKPOINT(name,show)
#define GL_TA_SHOW_CHECKPOINTS()
#define GL_TA_SHOW_DIFF(name1, name2)
#define GL_TA_GET_NUMOF_CHECKPOINTS()
#define GL_TA_GET_DIFF(name1, name2)
#define GL_TA_GET_NAME(idx)

// accum item handling
#define GL_TA_ACUM_ITEM_BEGIN(name,show)
#define GL_TA_ACUM_ITEM_END(name,show)
#define GL_TA_ACUM_ITEM_SHOW_RESULT()
#define GL_TA_ACUM_ITEM_SHOW_RESULT_TO(x)
#define __gl_ta__(name, x) x
#define __gl_tafmt__(fmt, args...)

#endif // _ENABLE_TA

#endif // _GALLERY_TA_H_
