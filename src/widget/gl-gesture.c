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

#include "gl-gesture.h"
#include "gl-util.h"
#include "gl-debug.h"
#include "gl-thumbs.h"
#include "gl-timeline.h"

#define GL_PINCH_TOUCH_HOLD_RANGE 80
#define GL_PINCH_TOUCH_FACTOR 4
#define GL_PINCH_HOLD_TIME_DELAY 0.35f
#define GL_GESTURE_KEY_DATA "gesture_data"

typedef struct _gl_gesture_t gl_gesture_s;
typedef struct _gl_pinch_event_t gl_pinch_event_s;
typedef enum _gl_pinch_plan_t gl_pinch_plan_e;

struct _gl_gesture_t {
	gl_appdata *ad;
	Evas_Object *parent;
	Evas_Object *gesture;
	gl_gesture_cb zoom_in_cb;
	gl_gesture_cb zoom_out_cb;
	void *zoom_in_data;
	void *zoom_out_data;

	int dis_old;
	int pinch_dis_old;
	Eina_List *s_event_elist;
	Ecore_Job *pinch_job;
	int next_plan;
};

struct _gl_pinch_event_t {
	int device;

	struct gl_prev {
		Evas_Coord x;
		Evas_Coord y;
	} prev;

	Evas_Coord x;
	Evas_Coord y;
	Evas_Coord w;
	Evas_Coord h;
	Evas_Object *object;
	Ecore_Timer *hold_timer;
	int ts;		/* Time stamp to calculate whether scrolling or moving */
	int v;		/* Velocity */
	int pinch_dis;
	Evas_Object *pinch_obj;	/* Object between thumb and index */
	Evas_Object *test;	/* To see dis center */
};

enum _gl_pinch_plan_t {
	GL_PINCH_PLAN_NONE,
	GL_PINCH_PLAN_OUT,
	GL_PINCH_PLAN_IN,
	GL_PINCH_PLAN_MAX
};

static gl_pinch_event_s *__gl_gesture_create_event_obj(void *data,
							Evas_Object *object,
							int device)
{
	GL_CHECK_NULL(data);
	gl_gesture_s *gesture_d = (gl_gesture_s *)data;
	gl_pinch_event_s *ev = NULL;
	/*gl_dbg(""); */

	ev = calloc(1, sizeof(gl_pinch_event_s));
	if (ev == NULL) {
		gl_dbg("Cannot allocate event_t");
		return NULL;
	}

	ev->object = object;
	ev->device = device;
	evas_object_geometry_get(object, &ev->x, &ev->y, &ev->w, &ev->h);

	gesture_d->s_event_elist = eina_list_append(gesture_d->s_event_elist,
	                           ev);
	return ev;
}

static int __gl_gesture_destroy_event_obj(void *data, gl_pinch_event_s *ev)
{
	GL_CHECK_VAL(ev, -1);
	GL_CHECK_VAL(data, -1);
	gl_gesture_s *gesture_d = (gl_gesture_s *)data;
	/*gl_dbg(""); */

	ev->pinch_obj = NULL;
	ev->pinch_dis = 0;
	gesture_d->s_event_elist = eina_list_remove(gesture_d->s_event_elist,
	                           ev);
	GL_IF_DEL_TIMER(ev->hold_timer);
	/* We don't need to reset the hold_timer object */

	GL_FREE(ev);
	return 0;
}

static gl_pinch_event_s *__gl_gesture_get_event_obj(void *data, int device)
{
	GL_CHECK_NULL(data);
	gl_gesture_s *gesture_d = (gl_gesture_s *)data;
	Eina_List *l = NULL;
	gl_pinch_event_s *ev = NULL;

	EINA_LIST_FOREACH(gesture_d->s_event_elist, l, ev) {
		if (ev && ev->device == device) {
			break;
		}
		ev = NULL;
	}

	return ev;
}

static int __gl_gesture_get_distance(Evas_Coord x1, Evas_Coord y1, Evas_Coord x2,
                                     Evas_Coord y2)
{
	int dis, dx, dy;

	dx = x1 - x2;
	dy = y1 - y2;

	dis = (int)sqrt(dx * dx + dy * dy);
	return dis;
}

static Eina_Bool __gl_gesture_hold_timer_cb(void *data)
{
	gl_pinch_event_s *ev0 = (gl_pinch_event_s *)data;
	GL_IF_DEL_TIMER(ev0->hold_timer);
	return ECORE_CALLBACK_CANCEL;
}

static int __gl_gesture_get_multi_device(void *data)
{
	GL_CHECK_VAL(data, -1);
	gl_gesture_s *gesture_d = (gl_gesture_s *)data;
	Eina_List *l = NULL;
	gl_pinch_event_s *ev = NULL;

	EINA_LIST_FOREACH(gesture_d->s_event_elist, l, ev) {
		if (ev && ev->device != 0) {
			return ev->device;
		}
	}
	return 0;
}

static void __gl_gesture_mouse_down_event(void *data, Evas *e,
			Evas_Object *evas_obj, void *ei)
{
	GL_CHECK(ei);
	GL_CHECK(data);
	Evas_Event_Mouse_Down *ev = (Evas_Event_Mouse_Down *)ei;
	/*gl_dbg(""); */

	gl_pinch_event_s *ev0;
	ev0 = __gl_gesture_get_event_obj(data, 0);
	if (ev0) {
		return;
	}

	ev0 = __gl_gesture_create_event_obj(data, evas_obj, 0);
	if (!ev0) {
		return;
	}

	ev0->hold_timer = NULL;
	ev0->prev.x = ev->output.x;
	ev0->prev.y = ev->output.y;
}

static void __gl_gesture_multi_down_event(void *data, Evas *evas,
        Evas_Object *obj, void *ei)
{
	GL_CHECK(ei);
	GL_CHECK(data);
	gl_pinch_event_s *ev;
	Evas_Event_Multi_Down *down = (Evas_Event_Multi_Down *)ei;
	/*gl_dbg(""); */

	ev = __gl_gesture_get_event_obj(data, down->device);
	if (ev) {
		return;
	}

	ev = __gl_gesture_create_event_obj(data, obj, down->device);
	if (!ev) {
		return;
	}

	ev->hold_timer = NULL;
	ev->prev.x = down->output.x;
	ev->prev.y = down->output.y;
}

static void __gl_gesture_mouse_up_event(void *data, Evas *e,
                                        Evas_Object *obj, void *ei)
{
	GL_CHECK(data);
	gl_gesture_s *gesture_d = (gl_gesture_s *)data;
	int mdevice;
	gl_pinch_event_s *ev0;
	gl_pinch_event_s *ev = NULL;
	/*gl_dbg(""); */
	Evas_Event_Mouse_Up *p_ev = (Evas_Event_Mouse_Up *)ei;

	ev0 = __gl_gesture_get_event_obj(data, 0);
	if (ev0 == NULL) {
		gl_dbg("Cannot get event0");
		return;
	}
	/* check whether device multi is on or off */
	mdevice = __gl_gesture_get_multi_device(data);
	/*gl_dbg("mdevice[%d]", mdevice); */
	if (mdevice == 0) {
	} else {
		/*gl_dbg(" device multi is on"); */
		/* when device multi is on */
		ev = __gl_gesture_get_event_obj(data, mdevice);
		if (ev == NULL) {
			gl_dbg("Cannot get event multi");
			return;
		}

		GL_IF_DEL_TIMER(ev->hold_timer);
		ev->hold_timer = ecore_timer_add(GL_PINCH_HOLD_TIME_DELAY,
		                                 __gl_gesture_hold_timer_cb, ev);
	}

	if (!gesture_d->ad->gridinfo.multi_touch) {
		_gl_thumbs_change_view(gesture_d->ad, ev0->prev.x, p_ev->output.x);
	} else {
		gesture_d->ad->gridinfo.multi_touch = false;
	}
	__gl_gesture_destroy_event_obj(data, ev0);
}

static void __gl_gesture_multi_up_event(void *data, Evas *evas,
                                        Evas_Object *obj, void *ei)
{
	GL_CHECK(ei);
	GL_CHECK(data);
	gl_gesture_s *gesture_d = (gl_gesture_s *)data;
	Evas_Event_Multi_Up *up = (Evas_Event_Multi_Up *)ei;
	gl_pinch_event_s *ev0 = NULL;
	gl_pinch_event_s *ev = NULL;
	/*gl_dbg(""); */

	ev = __gl_gesture_get_event_obj(data, up->device);
	if (ev == NULL) {
		gl_dbg("Cannot get device multi");
		return;
	}

	gesture_d->dis_old = 0;
	gesture_d->pinch_dis_old = 0;
	/*dis_old = 0; */

	/* act depends on device 0 on or off */
	ev0 = __gl_gesture_get_event_obj(data, 0);
	if (ev0) {
		/* up when device 0 is on */
		/* timer for pinch */
		GL_IF_DEL_TIMER(ev0->hold_timer);
		ev0->hold_timer = ecore_timer_add(GL_PINCH_HOLD_TIME_DELAY,
		                                  __gl_gesture_hold_timer_cb,
		                                  ev0);
	} else {
		/* up when device 0 is off */
	}
	__gl_gesture_destroy_event_obj(data, ev);
}

static void __gl_gesture_mouse_move_event(void *data, Evas *e,
			Evas_Object *obj, void *ei)
{
	GL_CHECK(ei);
	GL_CHECK(data);
//	Evas_Event_Mouse_Move *ev = (Evas_Event_Mouse_Move *)ei;
	gl_pinch_event_s *ev0;
	/*gl_dbg(""); */

	ev0 = __gl_gesture_get_event_obj(data, 0);
	if (ev0 == NULL) {
		/*gl_dbg("Cannot get device0"); */
		return;
	}

	__gl_gesture_get_multi_device(data);
}

static void __gl_gesture_zoom_out_job_cb(void *data)
{
	GL_CHECK(data);
	gl_gesture_s *gesture_d = (gl_gesture_s *)data;
	if (gesture_d->next_plan != GL_PINCH_PLAN_OUT) {
		gl_dbgW("State is wrong[plan:%d]!", gesture_d->next_plan);
		GL_IF_DEL_JOB(gesture_d->pinch_job);
		return;
	}
	gl_dbg("Pinch zoom out");

	if (gesture_d->zoom_out_cb)
		gesture_d->zoom_out_cb(gesture_d->gesture,
		                       gesture_d->zoom_out_data);

	GL_IF_DEL_JOB(gesture_d->pinch_job);
}

static void __gl_gesture_zoom_in_job_cb(void *data)
{
	GL_CHECK(data);
	gl_gesture_s *gesture_d = (gl_gesture_s *)data;
	if (gesture_d->next_plan != GL_PINCH_PLAN_IN) {
		gl_dbgW("State is wrong[plan:%d]!", gesture_d->next_plan);
		GL_IF_DEL_JOB(gesture_d->pinch_job);
		return;
	}
	gl_dbg("Pinch zoom in");

	if (gesture_d->zoom_in_cb)
		gesture_d->zoom_in_cb(gesture_d->gesture,
		                      gesture_d->zoom_in_data);

	GL_IF_DEL_JOB(gesture_d->pinch_job);
}

static void __gl_gesture_multi_move_event(void *data, Evas *evas,
			Evas_Object *obj, void *ei)
{
	GL_CHECK(data);
	gl_gesture_s *gesture_d = (gl_gesture_s *)data;
	Evas_Event_Multi_Move *move = (Evas_Event_Multi_Move *)ei;
	int dis_new;
	gl_pinch_event_s *ev0;
	gl_pinch_event_s *ev;
	/*(gl_dbg(""); */

	ev = __gl_gesture_get_event_obj(data, move->device);
	if (ev == NULL) {
		gl_dbg("Cannot get multi device");
		return;
	}
	gesture_d->ad->gridinfo.multi_touch = true;

	if (_gl_is_timeline_edit_mode(gesture_d->ad)) {
		gl_dbg("Prevent multi move in timeline edit mode");
		return;
	}

	if (gl_get_view_mode(gesture_d->ad) == GL_VIEW_THUMBS
	        || gl_get_view_mode(gesture_d->ad) == GL_VIEW_THUMBS_EDIT
	        || gl_get_view_mode(gesture_d->ad) == GL_VIEW_THUMBS_SELECT) {
		if (gl_get_view_mode(gesture_d->ad) == GL_VIEW_THUMBS_EDIT) {
			gl_dbg("Prevent multi move in thumbs edit view");
			return;
		}
		if (gesture_d->ad->gridinfo.split_view_mode == SPLIT_VIEW) {
			gl_dbg("Prevent multi move in split view");
			return;
		}
		if (gl_get_view_mode(gesture_d->ad) == GL_VIEW_THUMBS_SELECT) {
			gl_dbg("Prevent multi move in thumbs select view");
			return;
		}
	}
	ev->prev.x = move->cur.output.x;
	ev->prev.y = move->cur.output.y;

	ev0 = __gl_gesture_get_event_obj(data, 0);
	if (ev0 == NULL) {
		gl_dbg("Cannot get device0");
		return;
	}

	dis_new = __gl_gesture_get_distance(ev0->prev.x, ev0->prev.y,
	                                    ev->prev.x, ev->prev.y);

	int dis_old = gesture_d->dis_old;
	if (dis_old != 0) {
		if (dis_old - dis_new > 0 &&
		        ev->pinch_dis > GL_PINCH_TOUCH_HOLD_RANGE) {
			if (gesture_d->pinch_dis_old &&
			        ev->pinch_dis < (gesture_d->pinch_dis_old * GL_PINCH_TOUCH_FACTOR)) {
				ev->pinch_dis += (dis_old - dis_new);
				gesture_d->dis_old = dis_new;
				return;
			}

			gesture_d->next_plan = GL_PINCH_PLAN_OUT; /* plan to zoom-out */
			if (!gesture_d->pinch_job) {
				gl_dbgW("Add job pinch zoom out");
				gesture_d->pinch_job = ecore_job_add(__gl_gesture_zoom_out_job_cb,
				                                     data);
			} else {
				gl_dbgW("Added job pinch zoom out");
			}

			gesture_d->pinch_dis_old = ev->pinch_dis;
			ev->pinch_dis = 0;
		} else if (dis_old - dis_new < 0 &&
		           ev->pinch_dis < -GL_PINCH_TOUCH_HOLD_RANGE) {
			if (gesture_d->pinch_dis_old &&
			        ev->pinch_dis > (gesture_d->pinch_dis_old * GL_PINCH_TOUCH_FACTOR)) {
				ev->pinch_dis += (dis_old - dis_new);
				gesture_d->dis_old = dis_new;
				return;
			}

			gesture_d->next_plan = GL_PINCH_PLAN_IN; /* plan to zoom-in */
			if (!gesture_d->pinch_job) {
				gl_dbgW("Add job pinch zoom in");
				gesture_d->pinch_job = ecore_job_add(__gl_gesture_zoom_in_job_cb,
				                                     data);
			} else {
				gl_dbgW("Added job pinch zoom in");
			}

			gesture_d->pinch_dis_old = ev->pinch_dis;
			ev->pinch_dis = 0;
		}
		ev->pinch_dis += (dis_old - dis_new);
	}
	gl_dbg("dis_new: %d, dis_old: %d, pinch_dis %d", dis_new, dis_old,
	       ev->pinch_dis);

	/* Reset dis_old value */
	gesture_d->dis_old = dis_new;
}

static void __gl_gesture_del_cb(void *data, Evas *e, Evas_Object *obj,
                                void *ei)
{
	gl_dbg("Delete gesture ---");
	if (data) {
		gl_gesture_s *gesture_d = (gl_gesture_s *)data;
		GL_IF_DEL_JOB(gesture_d->pinch_job);
		evas_object_data_del(gesture_d->gesture, GL_GESTURE_KEY_DATA);
		GL_FREE(gesture_d);
	}
	gl_dbg("Delete gesture +++");
}

Evas_Object *_gl_gesture_add(void *data, Evas_Object *parent)
{
	GL_CHECK_NULL(parent);
	GL_CHECK_NULL(data);
	Evas_Object *gesture = NULL;

	gesture = evas_object_rectangle_add(evas_object_evas_get(parent));
	GL_CHECK_NULL(gesture);
	evas_object_color_set(gesture, 0, 0, 0, 0);

	gl_gesture_s *gesture_d = (gl_gesture_s *)calloc(1, sizeof(gl_gesture_s));
	if (gesture_d == NULL) {
		evas_object_del(gesture);
		return NULL;
	}

	gesture_d->ad = data;
	gesture_d->parent = parent;
	gesture_d->gesture = gesture;

	evas_object_event_callback_add(gesture, EVAS_CALLBACK_MOUSE_DOWN,
	                               __gl_gesture_mouse_down_event,
	                               gesture_d);
	evas_object_event_callback_add(gesture, EVAS_CALLBACK_MOUSE_UP,
	                               __gl_gesture_mouse_up_event, gesture_d);
	evas_object_event_callback_add(gesture, EVAS_CALLBACK_MOUSE_MOVE,
	                               __gl_gesture_mouse_move_event,
	                               gesture_d);

	evas_object_event_callback_add(gesture, EVAS_CALLBACK_MULTI_DOWN,
	                               __gl_gesture_multi_down_event,
	                               gesture_d);
	evas_object_event_callback_add(gesture, EVAS_CALLBACK_MULTI_UP,
	                               __gl_gesture_multi_up_event, gesture_d);
	evas_object_event_callback_add(gesture, EVAS_CALLBACK_MULTI_MOVE,
	                               __gl_gesture_multi_move_event,
	                               gesture_d);

	evas_object_data_set(gesture, GL_GESTURE_KEY_DATA, (void *)gesture_d);
	evas_object_event_callback_add(gesture, EVAS_CALLBACK_DEL,
	                               __gl_gesture_del_cb, gesture_d);
	return gesture;
}

int _gl_gesture_set_zoom_in_cb(Evas_Object *gesture, gl_gesture_cb cb,
                               void *data)
{
	GL_CHECK_VAL(cb, -1);
	GL_CHECK_VAL(gesture, -1);
	gl_gesture_s *gesture_d = NULL;

	gesture_d = (gl_gesture_s *)evas_object_data_get(gesture,
	            GL_GESTURE_KEY_DATA);
	GL_CHECK_VAL(gesture_d, -1);
	gesture_d->zoom_in_cb = cb;
	gesture_d->zoom_in_data = data;
	return 0;
}

int _gl_gesture_set_zoom_out_cb(Evas_Object *gesture, gl_gesture_cb cb,
                                void *data)
{
	GL_CHECK_VAL(cb, -1);
	GL_CHECK_VAL(gesture, -1);
	gl_gesture_s *gesture_d = NULL;

	gesture_d = (gl_gesture_s *)evas_object_data_get(gesture,
	            GL_GESTURE_KEY_DATA);
	GL_CHECK_VAL(gesture_d, -1);
	gesture_d->zoom_out_cb = cb;
	gesture_d->zoom_out_data = data;
	return 0;
}

int _gl_gesture_reset_flags(Evas_Object *gesture)
{
	GL_CHECK_VAL(gesture, -1);
	gl_gesture_s *gesture_d = NULL;

	gesture_d = (gl_gesture_s *)evas_object_data_get(gesture,
	            GL_GESTURE_KEY_DATA);
	GL_CHECK_VAL(gesture_d, -1);
	GL_IF_DEL_JOB(gesture_d->pinch_job);
	gesture_d->next_plan = GL_PINCH_PLAN_NONE;
	return 0;
}

Evas_Object *_gl_gesture_get_parent(Evas_Object *gesture)
{
	GL_CHECK_NULL(gesture);
	gl_gesture_s *gesture_d = NULL;

	gesture_d = (gl_gesture_s *)evas_object_data_get(gesture,
	            GL_GESTURE_KEY_DATA);
	GL_CHECK_NULL(gesture_d);
	return gesture_d->parent;
}

