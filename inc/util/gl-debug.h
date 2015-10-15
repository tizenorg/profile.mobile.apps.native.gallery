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

#ifndef _GL_DEBUG_H_
#define _GL_DEBUG_H_

#include <dlog.h>
#include <assert.h>
#include "gl-ta.h"
#include <stdio.h>

#ifdef __cplusplus
extern "C"
{
#endif

#ifdef LOG_TAG
#undef LOG_TAG
#endif

#define LOG_TAG "GALLERY"

/* anci c color type */
#define FONT_COLOR_RESET    "\033[0m"
#define FONT_COLOR_RED      "\033[31m"
#define FONT_COLOR_GREEN    "\033[32m"
#define FONT_COLOR_YELLOW   "\033[33m"
#define FONT_COLOR_BLUE     "\033[34m"
#define FONT_COLOR_PURPLE   "\033[35m"
#define FONT_COLOR_CYAN     "\033[36m"
#define FONT_COLOR_GRAY     "\033[37m"

#ifdef _USE_DLOG_

#define gl_dbg(fmt, arg...)  LOGD(FONT_COLOR_BLUE fmt FONT_COLOR_RESET, ##arg)
#define gl_dbgI(fmt, arg...) LOGI(FONT_COLOR_GREEN fmt FONT_COLOR_RESET, ##arg)
#define gl_dbgW(fmt, arg...) LOGW(FONT_COLOR_CYAN fmt FONT_COLOR_RESET, ##arg)
#define gl_dbgE(fmt, arg...) LOGE(FONT_COLOR_RED fmt FONT_COLOR_RESET, ##arg)

#define GL_PROFILE_IN  LOG(LOG_DEBUG, "LAUNCH", "[gallery:Application:%s:IN]", __FUNCTION__)
#define GL_PROFILE_OUT LOG(LOG_DEBUG, "LAUNCH", "[gallery:Application:%s:OUT]", __FUNCTION__)
#define GL_PROFILE_F_IN(func)  LOG(LOG_DEBUG, "LAUNCH","[gallery:Application:"func":IN]")
#define GL_PROFILE_F_OUT(func) LOG(LOG_DEBUG, "LAUNCH","[gallery:Application:"func":OUT]")

#else

#define gl_dbg(fmt,arg...)
#define gl_dbgW(fmt, arg...)
#define gl_dbgE(fmt, arg...)

#define GL_PROFILE_IN
#define GL_PROFILE_OUT
#define GL_PROFILE_F_IN(func)
#define GL_PROFILE_F_OUT(func)

#endif

#ifdef _USE_SECURE_LOG_
#define gl_sdbg(fmt, arg...)  SECURE_LOGD(FONT_COLOR_BLUE fmt FONT_COLOR_RESET, ##arg)
#define gl_sdbgW(fmt, arg...) SECURE_LOGI(FONT_COLOR_GREEN fmt FONT_COLOR_RESET, ##arg)
#define gl_sdbgE(fmt, arg...) SECURE_LOGE(FONT_COLOR_RED fmt FONT_COLOR_RESET, ##arg)
#endif


#include <time.h>
#include <sys/time.h>

//long gl_get_debug_time(void);
//void gl_reset_debug_time(void);
//void gl_print_debug_time(char* time_string);
void gl_print_debug_time_ex(long start, long end, const char *func_name, char *time_string);



#define gl_retm_if(expr, fmt, arg...) do { \
	if (expr) { \
		gl_dbgE(fmt, ##arg); \
		gl_dbgE("(%s) -> %s() return", #expr, __FUNCTION__); \
		return; \
	} \
} while (0)

#define gl_retvm_if(expr, val, fmt, arg...) do { \
	if (expr) { \
		gl_dbgE(fmt, ##arg); \
		gl_dbgE("(%s) -> %s() return", #expr, __FUNCTION__); \
		return (val); \
	} \
} while (0)

#define GL_CHECK_EXCEP(expr) do { \
	if (!(expr)) { \
		gl_dbgE("Critical ERROR ########################################## Check below item.");\
		goto gl_exception;\
	} \
} while (0)

#define GL_CHECK_VAL(expr, val) 		gl_retvm_if(!(expr), val, "Invalid parameter, return ERROR code!")
#define GL_CHECK_NULL(expr) 			gl_retvm_if(!(expr), NULL, "Invalid parameter, return NULL!")
#define GL_CHECK_FALSE(expr) 			gl_retvm_if(!(expr), false, "Invalid parameter, return FALSE!")
#define GL_CHECK_CANCEL(expr) 			gl_retvm_if(!(expr), ECORE_CALLBACK_CANCEL, "Invalid parameter, return ECORE_CALLBACK_CANCEL!")
#define GL_CHECK(expr) 					gl_retm_if(!(expr), "Invalid parameter, return!")

#define gl_assert(expr) do { \
	if (!(expr)) { \
		gl_dbgE("Critical ERROR ########################################## Check below item.");\
		assert(false); \
	} \
} while (0)

#ifdef __cplusplus
}
#endif

#endif /* _GL_DEBUG_H_ */

