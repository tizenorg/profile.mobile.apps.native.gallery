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
#ifdef _ENABLE_TA
#include <stdio.h>
#include <malloc.h>
#include <string.h>
#include <assert.h>
#include <time.h>		// time, ctime
#include <sys/time.h>		// gettimeofday
#include <sys/utsname.h>
#include <sys/resource.h>
#include <unistd.h>
#include <stdarg.h>
#include "gl-ta.h"
#include <glib.h>
#include "gallery.h"
#include "gl-debug.h"

#define GL_TA_BUF_LEN_MAX 512
#define GL_TA_NAME_LEN_EXT 100
#define GL_TA_HOST_NAME_LEN_MAX 256
#define GL_TA_FORMAT_LEN_MAX 256

// internal func.
static void
__free_cps(void);
static int
__get_cp_index(char *name);
static void
__free_accums(void);
static int
__get_accum_index(char *name);

// global var.
gl_ta_checkpoint **gl_g_cps = NULL;
static int gl_g_cp_index = 0;

gl_ta_accum_item **gl_g_accums = NULL;
static int gl_g_accum_index = 0;
static int gl_g_accum_longest_name = 0;
static unsigned long gl_g_accum_first_time = 0xFFFFFFFF;	// jmlee
static int gl_g_enable = 1;

int
gl_ta_init(void)
{
	if (gl_g_accums)
		return 0;

	gl_g_cps = (gl_ta_checkpoint **) malloc(GL_TA_MAX_CHECKPOINT * sizeof(gl_ta_checkpoint *));
	if (!gl_g_cps)
		return -1;
	memset(gl_g_cps, 0x00, GL_TA_MAX_CHECKPOINT * sizeof(gl_ta_checkpoint *));
	gl_g_accums = (gl_ta_accum_item **) malloc(GL_TA_MAX_CHECKPOINT * sizeof(gl_ta_accum_item *));
	if (!gl_g_accums)
		return -1;
	memset(gl_g_accums, 0x00, GL_TA_MAX_CHECKPOINT * sizeof(gl_ta_accum_item *));

	gl_g_accum_first_time = 0xFFFFFFFF;

	return 0;
}

int
gl_ta_release(void)
{
	if (!gl_g_accums)
		return 0;

	__free_cps();
	__free_accums();

	gl_g_accum_first_time = 0xFFFFFFFF;

	return 0;
}

void
gl_ta_set_enable(int enable)
{
	gl_dbg("GL_TA : setting enable to %d\n", enable);
	gl_g_enable = enable;
}

int
gl_ta_get_numof_checkpoints()
{
	return gl_g_cp_index;
}

char *
gl_ta_fmt(const char *fmt, ...)
{
	static char ta_buf[GL_TA_BUF_LEN_MAX] = { 0, };
	va_list args;

	va_start(args, fmt);
	vsnprintf(ta_buf, GL_TA_BUF_LEN_MAX, fmt, args);
	va_end(args);

	return ta_buf;
}


int
gl_ta_add_checkpoint(char *name, int show, char *filename, int line)
{
	gl_ta_checkpoint *cp = NULL;
	struct timeval t;

	if (!gl_g_enable)
		return -1;

	if (!gl_g_accums)
		return 0;

	if (gl_g_cp_index == GL_TA_MAX_CHECKPOINT)
		return -1;

	if (!name)
		return -1;

	if (strlen(name) == 0)
		return -1;

	cp = (gl_ta_checkpoint *) malloc(sizeof(gl_ta_checkpoint));
	if (!cp)
		return -1;
	memset(cp, 0x00, sizeof(gl_ta_checkpoint));
	cp->name = (char *)malloc(strlen(name) + 1);
	if (!cp->name)
	{
		free(cp);
		return -1;
	}
	memset(cp->name, 0x00, (strlen(name) + 1));
	g_strlcpy(cp->name, name, strlen(cp->name));
	if (show)
		gl_sdbg("[CHECK-POINT] %s...(%s:%d)\n", name, filename, line);

	memset(&t,0x00,sizeof(struct timeval));
	gettimeofday(&t, NULL);
	cp->timestamp = t.tv_sec * GL_TIME_USEC_PER_SEC + t.tv_usec;
#ifdef GL_TA_UNIT_MSEC
	cp->timestamp = (cp->timestamp >= GL_TIME_MSEC_PER_SEC) ? cp->timestamp / GL_TIME_MSEC_PER_SEC : 0;
#endif

	gl_g_cps[gl_g_cp_index] = cp;

	gl_g_cp_index++;

	return 0;
}

void
gl_ta_show_checkpoints(void)
{
	int i = 0;

	if (!gl_g_accums)
		return;

	gl_dbg("BEGIN RESULT ============================\n");
	for (i = 0; i < gl_g_cp_index; i++)
	{
		gl_sdbg("[%d] %s : %ld us.\n", i, gl_g_cps[i]->name, gl_g_cps[i]->timestamp);
	}
	gl_dbg("END RESULT   ============================\n");
}

void
gl_ta_show_diff(char *name1, char *name2)
{
	if (!gl_g_accums)
		return;


	gl_sdbg("Time takes from [%s] to [%s] : %ld us.\n", name1, name2, gl_ta_get_diff(name1, name2));
}

unsigned long
gl_ta_get_diff(char *name1, char *name2)
{
	int cp1, cp2;

	if (!gl_g_accums)
		return 0;


	// fail if bad param.
	if (!name1 || !name2)
		return -1;

	// fail if same.
	if (strcmp(name1, name2) == 0)
		return -1;

	// get index
	if ((cp1 = __get_cp_index(name1)) == -1)
		return -1;

	if ((cp2 = __get_cp_index(name2)) == -1)
		return -1;

	// NOTE :
	// return value must be positive value.
	// bcz the value of higher index of gl_g_cps always higher than lower one.
	return gl_g_cps[cp2]->timestamp - gl_g_cps[cp1]->timestamp;

}

static int
__get_cp_index(char *name)
{
	int i;

	//assert(name);
	if (!name)
		return -1;

	// find index
	for (i = 0; i < gl_g_cp_index; i++)
	{
		if (strcmp(name, gl_g_cps[i]->name) == 0)
			return i;
	}

	return -1;
}

static int
__get_accum_index(char *name)
{
	int i;

	//assert(name);
	if (!name)
		return -1;


	// find index
	for (i = 0; i < gl_g_accum_index; i++)
	{
		if (strcmp(name, gl_g_accums[i]->name) == 0)
			return i;
	}

	return -1;
}

static void
__free_cps(void)
{
	int i = 0;

	if (!gl_g_cps)
		return;

	for (i = 0; i < gl_g_cp_index; i++)
	{
		if (gl_g_cps[i])
		{
			if (gl_g_cps[i]->name)
				free(gl_g_cps[i]->name);

			free(gl_g_cps[i]);

			gl_g_cps[i] = NULL;
		}
	}

	free(gl_g_cps);
	gl_g_cps = NULL;

	gl_g_cp_index = 0;
}

static void
__free_accums(void)
{
	int i = 0;

	if (!gl_g_accums)
		return;

	for (i = 0; i < gl_g_accum_index; i++)
	{
		if (gl_g_accums[i])
		{
			if (gl_g_accums[i]->name)
				free(gl_g_accums[i]->name);

			free(gl_g_accums[i]);

			gl_g_accums[i] = NULL;
		}
	}

	gl_g_accum_index = 0;
	gl_g_accum_longest_name = 0;

	free(gl_g_accums);
	gl_g_accums = NULL;
}


int
gl_ta_accum_item_begin(char *name, int show, char *filename, int line)
{
	gl_ta_accum_item *accum = NULL;
	int index = 0;
	int name_len = 0;
	struct timeval t;

	if (!gl_g_enable)
		return -1;

	if (!gl_g_accums)
		return 0;



	if (gl_g_accum_index == GL_TA_MAX_ACCUM)
		return -1;

	if (!name)
		return -1;

	name_len = strlen(name);
	if (name_len == 0)
		return -1;

	// if 'name' is new one. create new item.
	if ((index = __get_accum_index(name)) == -1)
	{
		accum = (gl_ta_accum_item *) malloc(sizeof(gl_ta_accum_item));
		if (!accum)
			return -1;

		// clear first.
		memset(accum, 0x00, sizeof(gl_ta_accum_item));
		accum->elapsed_min = 0xFFFFFFFF;

		accum->name = (char *)malloc(name_len + GL_TA_NAME_LEN_EXT);
		if (!accum->name)
		{
			free(accum);
			return -1;
		}
		memset(accum->name, 0x00, name_len + GL_TA_NAME_LEN_EXT);
		g_strlcpy(accum->name, name, name_len);
		// add it to list.
		gl_g_accums[gl_g_accum_index] = accum;
		gl_g_accum_index++;

		if (gl_g_accum_longest_name < name_len)
			gl_g_accum_longest_name = name_len;

	}
	else
	{
		accum = gl_g_accums[index];
	}

	// verify pairs of begin, end.
	if (accum->on_estimate)
	{
		gl_sdbg("[%s] is not 'end'ed!\n", accum->name);
		accum->num_unpair++;
		return -1;
	}

	memset(&t,0x00,sizeof(struct timeval));
	// get timestamp
	gettimeofday(&t, NULL);
	accum->timestamp = t.tv_sec * GL_TIME_USEC_PER_SEC + t.tv_usec;
#ifdef GL_TA_UNIT_MSEC
	accum->timestamp = (accum->timestamp >= GL_TIME_MSEC_PER_SEC) ? accum->timestamp / GL_TIME_MSEC_PER_SEC : 0;
#endif
	accum->on_estimate = 1;

	if (accum->first_start == 0)
	{			// assum that timestamp never could be zero.
		accum->first_start = accum->timestamp;

		if (gl_g_accum_first_time > accum->first_start)
			gl_g_accum_first_time = accum->first_start;
	}

	if (show)
		gl_sdbg("[ACCUM BEGIN] %s : %ld ---(%s:%d)\n", name, accum->timestamp, filename, line);

	accum->num_calls++;

	return 0;
}

int
gl_ta_accum_item_end(char *name, int show, char *filename, int line)
{
	gl_ta_accum_item *accum = NULL;
	long int tval = 0;
	int index = 0;
	struct timeval t;

	if (!gl_g_enable)
		return -1;

	if (!gl_g_accums)
		return 0;

	memset(&t,0x00,sizeof(struct timeval));
	// get time first for more accuracy.
	gettimeofday(&t, NULL);

	if (gl_g_accum_index == GL_TA_MAX_ACCUM)
		return -1;

	if (!name)
		return -1;

	if (strlen(name) == 0)
		return -1;

	// varify the 'name' is already exist.
	if ((index = __get_accum_index(name)) == -1)
	{
		//printf("[%s] is not added before!\n", name);
		return -1;
	}

	accum = gl_g_accums[index];

	// verify pairs of begin, end.
	if (!accum->on_estimate)
	{
		gl_sdbg("[%s] is not 'begin' yet!\n", accum->name);
		accum->num_unpair++;
		return -1;
	}

	// get current timestamp.
	tval = t.tv_sec * GL_TIME_USEC_PER_SEC + t.tv_usec;
#ifdef GL_TA_UNIT_MSEC
	tval = (tval >= GL_TIME_MSEC_PER_SEC) ? tval / GL_TIME_MSEC_PER_SEC : 0;
#endif

	// update last_end
	accum->last_end = tval;

	// make get elapsed time.
	tval = tval - accum->timestamp;

	// update min/max
	accum->elapsed_max = tval > accum->elapsed_max ? tval : accum->elapsed_max;
	accum->elapsed_min = tval < accum->elapsed_min ? tval : accum->elapsed_min;

	if (show)
		gl_sdbg("[ACCUM END] %s : %ld + %ld ---(%s:%d)\n", name, accum->elapsed_accum, tval, filename, line);

	// add elapsed time
	accum->elapsed_accum += tval;
	accum->on_estimate = 0;

	return 0;
}

void
__print_some_info(FILE * fp)
{
	if (!fp)
		return;

	// comment
	{
		gl_dbg("\nb~ b~ b~\n\n");
	}

	// General infomation
	{
		time_t t_val;
		char hostname[GL_TA_HOST_NAME_LEN_MAX] = { 0, };
		char time_buf[GL_TA_TIME_BUF_LEN] = { 0,};
		struct utsname uts;
		struct rusage r_usage;

		gl_dbg("\n[[ General info ]]\n");

		// time and date
		time(&t_val);
		gl_dbg("Date : %s",ctime_r(&t_val, time_buf));

		memset(&uts,0x00,sizeof(struct utsname));
		// system
		if (gethostname(hostname, GL_TA_HOST_NAME_LEN_MAX - 1) == 0 && uname(&uts) >= 0)
		{
			gl_dbg("Hostname : %s\n", hostname);
			gl_dbg("System : %s\n", uts.sysname);
			gl_dbg("Machine : %s\n", uts.machine);
			gl_dbg("Nodename : %s\n", uts.nodename);
			gl_dbg("Release : %s \n", uts.release);
			gl_dbg("Version : %s \n", uts.version);
		}

		memset(&r_usage,0x00,sizeof(struct rusage));
		// process info.
		gl_dbg("Process priority : %d\n", getpriority(PRIO_PROCESS, getpid()));
		getrusage(RUSAGE_SELF, &r_usage);
		gl_dbg("CPU usage : User = %ld.%06ld, System = %ld.%06ld\n",
			r_usage.ru_utime.tv_sec, r_usage.ru_utime.tv_usec,
			r_usage.ru_stime.tv_sec, r_usage.ru_stime.tv_usec);


	}

	// host environment variables
	{
		extern char **environ;
		char **env = environ;

		gl_dbg("\n[[ Host environment variables ]]\n");
		while (*env)
		{
			gl_dbg("%s\n", *env);
			env++;
		}
	}

	// applied samsung feature
	{
	}


}

void
gl_ta_accum_show_result(int direction)
{
	int i = 0;
	char format[GL_TA_FORMAT_LEN_MAX] = { 0, };
	FILE *fp = stderr;

	if (!gl_g_accums)
		return;

	switch (direction)
	{
	case GL_TA_SHOW_STDOUT:
		fp = stdout;
		break;
	case GL_TA_SHOW_STDERR:
		fp = stderr;
		break;
	case GL_TA_SHOW_FILE:
		{
			fp = fopen(GL_TA_RESULT_FILE, "wt");
			if (!fp)
				return;
		}
	}
	__print_some_info(fp);

#ifdef GL_TA_UNIT_MSEC
	snprintf(format, sizeof(format),
		 "[%%3d] %%-%ds | \ttotal : %%4ld\tcalls : %%3ld\tavg : %%4ld\tmin : %%4ld\tmax : %%4ld\tstart : %%4lu\tend : %%4lu\tunpair : %%3ld\n",
		 gl_g_accum_longest_name);
	gl_dbg("BEGIN RESULT ACCUM============================ : NumOfItems : %d, unit(msec)\n", gl_g_accum_index);
#else
	snprintf(format, sizeof(format),
		 "[%%3d] %%-%ds | \ttotal : %%ld\tcalls : %%ld\tavg : %%ld\tmin : %%ld\tmax : %%ld\tstart : %%lu\tend : %%lu\tunpair : %%ld\n",
		 gl_g_accum_longest_name);
	gl_dbg("BEGIN RESULT ACCUM============================ : NumOfItems : %d, unit(usec)\n", gl_g_accum_index);
#endif

	for (i = 0; i < gl_g_accum_index; i++)
	{
		// prevent 'devide by zero' error
		if (gl_g_accums[i]->num_calls == 0)
			gl_g_accums[i]->num_calls = 1;

		/*fprintf(fp,
			format,
			i,
			gl_g_accums[i]->name,
			gl_g_accums[i]->elapsed_accum,
			gl_g_accums[i]->num_calls,
			(gl_g_accums[i]->elapsed_accum == 0) ? 0 : (int)(gl_g_accums[i]->elapsed_accum / gl_g_accums[i]->num_calls),	// Fix it! : devide by zero.
			gl_g_accums[i]->elapsed_min,
			gl_g_accums[i]->elapsed_max,
			gl_g_accums[i]->first_start - gl_g_accum_first_time,
			gl_g_accums[i]->last_end - gl_g_accum_first_time,
			gl_g_accums[i]->num_unpair);*/
	}
	gl_dbg("END RESULT ACCUM  ============================\n");

	if (direction == GL_TA_SHOW_FILE)
		fclose(fp);
}

#endif //_ENABLE_TA
