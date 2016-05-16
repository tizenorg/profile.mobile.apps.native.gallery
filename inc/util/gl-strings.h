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

/* This header file includes all multi language strings which need display */
#ifndef _GL_STRINGS_H_
#define _GL_STRINGS_H_

#ifdef __cplusplus
extern "C"
{
#endif

#define GL_STR_DOMAIN_SYS "sys_string"
#define GL_STR_DOMAIN_LOCAL PACKAGE

/* Special album name saved in DB  */
#define GL_STR_ALBUM_CAMERA _("Camera")
#define GL_STR_ALBUM_DOWNLOADS _("Downloads")
#define GL_STR_ALBUM_SCREENSHOTS _("Screenshots")

/* hard code strings and already translated strings in gallery po files */
#define GL_STR_UNDER_IMPLEMENTATION _("Under Implementation!")

#define GL_STR_INVALID_INPUT_PARAMETER _("IDS_MEDIABR_POP_INVALID_INPUT_PARAMETER")
#define GL_STR_ENTRY_IS_EMPTY _("IDS_MEDIABR_POP_ENTRY_IS_EMPTY")
#define GL_STR_SAME_NAME_ALREADY_IN_USE _("IDS_MEDIABR_POP_SAME_NAME_ALREADY_IN_USE")

#define GL_STR_UNABLE_tO_RENAME _("IDS_MEDIABR_POP_UNABLE_TO_RENAME")
#define GL_STR_NO_ALBUMS_SELECTED _("IDS_MEDIABR_POP_NO_ALBUMS_SELECTED")
#define GL_STR_SELECT_ITEM _(GL_STR_ID_SELECT_ITEM)
#define GL_STR_NO_ALBUMS _(GL_STR_ID_NO_ALBUMS)

#define GL_STR_EMPTY _("")
#define GL_STR_NO_NAME GL_STR_ID_NO_NAME

#define GL_STR_MORE dgettext("gallery", "IDS_COM_BODY_MORE")

#define GL_STR_DELETE_1_FILE _("IDS_MEDIABR_POP_1_ITEM_WILL_BE_DELETED")
#define GL_STR_DELETE_PD_FILES _("IDS_GALLERY_POP_PD_ITEMS_WILL_BE_DELETED_ABB")

#define GL_STR_DELETE_1_ALBUM _("IDS_MEDIABR_POP_1_ALBUM_WILL_BE_DELETED")
#define GL_STR_DELETE_PD_ALBUMS _("IDS_MEDIABR_POP_PD_ALBUMS_WILL_BE_DELETED")

#define GL_STR_PD_SELECTED _("IDS_GALLERY_HEADER_PD_SELECTED_ABB")

#define GL_STR_CREATE_FOLDER_Q _("IDS_MEDIABR_POP_CREATE_FOLDER_Q")
#define GL_STR_ITEMS _("items")
#define GL_STR_ITEM _("item")
#define GL_STR_RETRY_Q _("IDS_MEDIABR_POP_RETRY_Q")
#define GL_STR_REMOVE_COMFIRMATION _("IDS_MEDIABR_POPUP_REMOVE_Q")

/* system strings which are included in sys-string-0 po files */
#define GL_STR_OPERATION_FAILED dgettext("gallery", GL_STR_ID_OPERATION_FAILED)
#define GL_STR_CAMERA dgettext("gallery", GL_STR_ID_CAMERA)
#define GL_STR_CANCELLED dgettext("gallery", "IDS_COM_POP_CANCELLED")
#define GL_STR_SUCCESS dgettext("gallery", "IDS_COM_POP_SUCCESS")
#define GL_STR_FAILED dgettext("gallery", "IDS_COM_POP_FAILED")
#define GL_STR_BACK dgettext("gallery", "IDS_COM_SK_BACK")
#define GL_STR_DELETE dgettext("gallery", GL_STR_ID_DELETE)
#define GL_STR_ALBUMS "IDS_GALLERY_BUTTON_ALBUMS"
#define GL_STR_ID_TIME_LINE "IDS_MEDIABR_HEADER_TIMELINE_ABB"
#define GL_STR_TIME_LINE dgettext("gallery","IDS_MEDIABR_HEADER_TIMELINE_ABB")
#define GL_STR_DELETED dgettext("gallery", "IDS_COM_POP_DELETED")
#define GL_STR_DELETE_COMFIRMATION dgettext("gallery", "IDS_COM_POP_DELETE_Q")
#define GL_STR_SELECTED  dgettext("gallery", "IDS_COM_POP_SELECTED")
#define GL_STR_ADDED dgettext("gallery", "IDS_COM_POP_ADDED")
#define GL_STR_MOVED dgettext("gallery", "IDS_COM_POP_MOVED")
#define GL_STR_COPIED dgettext("gallery", "IDS_COM_POP_COPIED_P")
#define GL_STR_SELECT_ALL dgettext("gallery", "IDS_COM_BODY_SELECT_ALL")
#define GL_STR_NO_FILES_SELECTED dgettext("gallery", GL_STR_ID_NO_FILES_SELECTED)
#define GL_STR_APPLICATION_NOT_INSTALLED dgettext("gallery", "IDS_COM_BODY_APPLICATION_NOT_INSTALLED")
#define GL_STR_PROCESSING dgettext("gallery", "IDS_COM_POP_PROCESSING")
#define GL_STR_TODAY dgettext("gallery", "IDS_COM_BODY_TODAY")
#define GL_STR_YESTERDAY dgettext("gallery", "IDS_COM_BODY_YESTERDAY")
#define GL_STR_SUN dgettext("gallery", "IDS_COM_POP_SUN")
#define GL_STR_MON dgettext("gallery", "IDS_COM_POP_MON")
#define GL_STR_TUE dgettext("gallery", "IDS_COM_POP_TUE")
#define GL_STR_WED dgettext("gallery", "IDS_COM_POP_WED")
#define GL_STR_THU dgettext("gallery", "IDS_COM_POP_THU")
#define GL_STR_FRI dgettext("gallery", "IDS_COM_POP_FRI")
#define GL_STR_SAT dgettext("gallery", "IDS_COM_POP_SAT")
#define GL_STR_HELP dgettext("gallery", "IDS_COM_BODY_HELP")

/*string for screen reader*/
#define GL_STR_SR_TAB               _("tab")
#define GL_STR_SR_DOUBLE_TAP        _("double tap to move to contents")
#define GL_STR_SR_THUMBNAIL_VIEW    _("Thumbnail view")
#define GL_STR_SR_FOUCUSED_POINT    _("Focused point")
#define GL_STR_SR_UNFOUCUSED_POINT  _("Unfocused point")
#define GL_STR_SR_THUMBS_DOUBLE_TAP _("double tap to preview it")
#define GL_STR_SR_ALBUM_DOUBLE_TAP  _("double tap to open the album")
#define GL_STR_SR_BUTTON            _("button")
#define GL_STR_SR_TITLE _("title")

/* Add ID to make update text easily if language changed */
#define GL_STR_ID_DELETE_ACCOUNT_COMFIRMATION "IDS_MEDIABR_POP_ALL_DATA_RELATED_TO_THIS_ACCOUNT_WILL_BE_DELETED_CONTINUE_Q"
#define GL_STR_ID_DELETE_WEB_ALBUMS "Some web albums will be deleted"
#define GL_STR_ID_DELETE "IDS_COM_OPT_DELETE"
#define GL_STR_ID_CANCEL "IDS_COM_SK_CANCEL"
#define GL_STR_ID_CANCEL_ABB "IDS_MEDIABR_BUTTON_CANCEL_ABB"
#define GL_STR_ID_MOVE_TO_ALBUM dgettext(GL_STR_DOMAIN_LOCAL, "IDS_GALLERY_OPT_MOVE_TO_ALBUM")
#define GL_STR_ID_COPY_TO_ALBUM dgettext(GL_STR_DOMAIN_LOCAL, "IDS_GALLERY_OPT_COPY_TO_ALBUM")
#define GL_STR_ID_MOVE "IDS_COM_BODY_MOVE"
#define GL_STR_ID_COPY "IDS_COM_BODY_COPY"
#define GL_STR_ID_NEW_ALBUM "IDS_GALLERY_OPT_CREATE_ALBUM"
#define GL_STR_ID_NEW_ALBUM_CONFIRMATION "IDS_MEDIABR_POP_DO_YOU_WANT_TO_COPY_OR_MOVE_THE_PICTURES_FROM_THE_ORIGINAL_ALBUM_Q"
#define GL_STR_ID_OPERATION_FAILED "IDS_COM_BODY_OPERATION_FAILED"
#define GL_STR_ID_CLOSE "IDS_COM_BODY_CLOSE"
#define GL_STR_ID_SHARE "IDS_MEDIABR_OPT_SHARE"
#define GL_STR_ID_START "IDS_COM_BODY_START"
#define GL_STR_ID_SETTINGS "IDS_COM_BODY_SETTINGS"
#ifdef SUPPORT_SLIDESHOW
#define GL_STR_ID_SLIDESHOW "IDS_MEDIABR_OPT_SLIDESHOW"
#endif
#define GL_STR_ID_START_SLIDESHOW "IDS_MEDIABR_OPT_START_SLIDESHOW"
#define GL_STR_ID_SLIDESHOW_SETTINGS "IDS_MEDIABR_BODY_SLIDESHOW_SETTINGS"
//#define GL_STR_ID_ALBUMS "IDS_COM_BODY_ALBUMS"
#define GL_STR_ID_ALBUMS GL_STR_ALBUMS

#define GL_STR_ID_GALLERY "IDS_COM_BODY_GALLERY"
#define GL_STR_ID_CHNAGE_NAME "IDS_COM_BODY_CHANGE_NAME"
#define GL_STR_ID_MOVING "IDS_COM_POP_MOVING"
#define GL_STR_ID_COPYING "IDS_COM_POP_COPYING_ING"
#define GL_STR_ID_DELETING "IDS_COM_POP_DELETING"
#define GL_STR_ID_YES "IDS_COM_SK_YES"
#define GL_STR_ID_NO "IDS_COM_SK_NO"
#define GL_STR_ID_OK "IDS_COM_SK_OK"
#define GL_STR_ID_DONE "IDS_COM_SK_DONE"
#define GL_STR_ID_RENAME "IDS_COM_SK_RENAME"
#define GL_STR_ID_CAMERA "IDS_COM_BODY_CAMERA"
#define GL_STR_ID_EDIT "IDS_COM_BODY_EDIT"
#define GL_STR_ID_SAVE "IDS_COM_SK_SAVE"
#define GL_STR_ID_DOWNLOADS "IDS_MF_BODY_DOWNLOADS"
#define GL_STR_ID_SCREENSHOTS "IDS_MF_BODY_SCREENSHOTS"
#define GL_STR_ID_NO_ITEMS dgettext(GL_STR_DOMAIN_LOCAL, "IDS_MEDIABR_POP_NO_ITEMS")
#define GL_STR_ID_NO_ITEMS_SECOND dgettext(GL_STR_DOMAIN_LOCAL, "IDS_GALLERY_BODY_AFTER_YOU_TAKE_PICTURES_THEY_WILL_BE_SHOWN_HERE")
#define GL_STR_ID_NO_CONTENTS "IDS_COM_BODY_NO_CONTENTS"
#define GL_STR_ID_NO_FILES_SELECTED "IDS_COM_POP_NO_FILES_SELECTED"
#define GL_STR_ID_NO_NAME "IDS_MEDIABR_POP_NO_NAME"
#define GL_STR_ID_ALL_ALBUMS dgettext(GL_STR_DOMAIN_LOCAL, "IDS_MEDIABR_BODY_ALL_ALBUMS")
#define GL_STR_ID_FAVOURITE_ALBUMS "IDS_TW_BODY_FAVORITES"
#define GL_STR_ID_SELECT_ALBUM "IDS_MEDIABR_HEADER_SELECT_ALBUM"
#define GL_STR_TIME_VIEW "IDS_GALLERY_HEADER_TIME"
#define GL_STR_SCREEN_HEIGHT_HD 1280
#define GL_STR_SCREEN_WIDTH_HD 720
#define GL_STR_SCREEN_HEIGHT_QHD 960
#define GL_STR_SCREEN_WIDTH_QHD 540
#define GL_STR_SCREEN_HEIGHT_WVGA 800
#define GL_STR_SCREEN_WIDTH_WVGA 480
#define GL_STR_SORT "IDS_MF_OPT_SORT_BY_ABB"
#define GL_STR_DATE_OLDEST "IDS_GALLERY_OPT_DATE_HOLDEST_ABB3"
#define GL_STR_DATE_MOST_RECENT "IDS_GALLERY_OPT_DATE_HMOST_RECENT_ABB2"
#define GL_STR_ID_ALBUM_DEFAULT dgettext(GL_STR_DOMAIN_LOCAL, "IDS_GALLERY_HEADER_ALBUM")
#define GL_STR_ID_REORDER dgettext(GL_STR_DOMAIN_LOCAL, "IDS_CLOCK_HEADER_REORDER_ABB")
#define GL_STR_ID_SAME_DESTINATION_ERROR_MSG dgettext(GL_STR_DOMAIN_LOCAL, "IDS_GALLERY_TPOP_SELECTED_ALBUM_SAME_AS_SOURCE_ALBUM_SELECT_ANOTHER_ALBUM")
#define GL_STR_ID_VIEW_AS "IDS_GALLERY_OPT_VIEW_AS"
#define GL_STR_ID_CANCEL_CAP	"IDS_TPLATFORM_ACBUTTON_CANCEL_ABB"
#define GL_STR_ID_DONE_CAP	"IDS_TPLATFORM_ACBUTTON_DONE_ABB"

#define GL_STR_ID_ALBUM dgettext(GL_STR_DOMAIN_LOCAL, "IDS_GALLERY_BUTTON_ALBUMS")
#define GL_STR_ID_SELECT_ITEM "IDS_MEDIABR_HEADER_SELECT_ITEM"
#define GL_STR_ID_NO_ALBUMS "IDS_MEDIABR_BODY_NO_ALBUMS"
#define GL_STR_ID_JAN dgettext(GL_STR_DOMAIN_LOCAL, "IDS_COM_BODY_JAN_ABB")
#define GL_STR_ID_FEB dgettext(GL_STR_DOMAIN_LOCAL, "IDS_COM_BODY_FEB_ABB")
#define GL_STR_ID_MAR dgettext(GL_STR_DOMAIN_LOCAL, "IDS_COM_BODY_MAR_ABB")
#define GL_STR_ID_APR dgettext(GL_STR_DOMAIN_LOCAL, "IDS_COM_BODY_APR_ABB")
#define GL_STR_ID_MAY dgettext(GL_STR_DOMAIN_LOCAL, "IDS_COM_BODY_MAY_ABB")
#define GL_STR_ID_JUN dgettext(GL_STR_DOMAIN_LOCAL, "IDS_COM_BODY_JUN_ABB")
#define GL_STR_ID_JUL dgettext(GL_STR_DOMAIN_LOCAL, "IDS_COM_BODY_JUL_ABB")
#define GL_STR_ID_AUG dgettext(GL_STR_DOMAIN_LOCAL, "IDS_COM_BODY_AUG_ABB")
#define GL_STR_ID_SEP dgettext(GL_STR_DOMAIN_LOCAL, "IDS_COM_BODY_SEP_ABB")
#define GL_STR_ID_OCT dgettext(GL_STR_DOMAIN_LOCAL, "IDS_COM_BODY_OCT_ABB")
#define GL_STR_ID_NOV dgettext(GL_STR_DOMAIN_LOCAL, "IDS_COM_BODY_NOV_ABB")
#define GL_STR_ID_DEC dgettext(GL_STR_DOMAIN_LOCAL, "IDS_COM_BODY_DEC_ABB")

#define GL_STR_ID_ROTATE_LEFT "IDS_MEDIABR_OPT_ROTATE_LEFT"
#define GL_STR_ID_ROTATE_RIGHT "IDS_MEDIABR_OPT_ROTATE_RIGHT"
#define GL_STR_ID_EDIT_IMAGE "IDS_MEDIABR_OPT_EDIT_IMAGE"
#define GL_STR_ID_ROTATING _("IDS_MEDIABR_POP_ROTATING_ING")

/*String for slideshow*/
#define GL_STR_ID_ALL_ITEMS "IDS_MEDIABR_OPT_ALL_ITEMS"
#define GL_STR_ID_SELECT_ITEMS "IDS_MEDIABR_BODY_SELECT_ITEMS"
#define GL_STR_ID_SELECT_ALL "IDS_GALLERY_HEADER_SELECT_ALL_ABB"

#define GL_STR_ID_DETAILS "IDS_COM_BODY_DETAILS"
#define GL_STR_ID_MAX_NO_CHARS_REACHED "IDS_COM_POP_MAXIMUM_NUMBER_OF_CHARACTERS_REACHED"

#define GL_STR_ID_SHARE_LIMIT _("IDS_IV_POP_YOU_CAN_SELECT_UP_TO_PD_ITEMS")

#define GL_STR_ID_UNSUPPORTED_FILE_TYPE "IDS_COM_BODY_UNSUPPORTED_FILE_TYPE"

#define GL_STR_ID_DELETE_COMFIRMATION     "IDS_COM_POP_DELETE_Q"

#define GL_STR_ID_DELETE_1_FILE           "IDS_MEDIABR_POP_1_ITEM_WILL_BE_DELETED"
#define GL_STR_ID_DELETE_PD_FILES         "IDS_MEDIABR_POP_PD_ITEMS_WILL_BE_DELETED"
#define GL_STR_PD_ITEMS _("IDS_MEDIABR_BODY_PD_ITEMS")

#define GL_STR_ID_DELETE_1_ALBUM          "IDS_MEDIABR_POP_1_ALBUM_WILL_BE_DELETED"
#define GL_STR_ID_DELETE_PD_ALBUMS        "IDS_MEDIABR_POP_PD_ALBUMS_WILL_BE_DELETED"

#define GL_STR_ID_INVALID_INPUT_PARAMETER "IDS_MEDIABR_POP_INVALID_INPUT_PARAMETER"
#define GL_STR_ID_ENTRY_IS_EMPTY "IDS_MEDIABR_POP_ENTRY_IS_EMPTY"
#define GL_STR_ID_SAME_NAME_ALREADY_IN_USE "IDS_MEDIABR_POP_SAME_NAME_ALREADY_IN_USE"

#define GL_STR_ID_THERE_IS_ONLY_ONE_ALBUM "IDS_IV_BODY_THERE_IS_ONLY_ONE_ALBUM"

/* Design ID from Email and Myfiles. Only for temporary use. */
#define GL_STR_1_ITEM _("IDS_EMAIL_BODY_1_ITEM")
#define GL_STR_REMOVED _("IDS_MF_POP_REMOVED")
#define GL_STR_ID_ENTER_TAG_NAME "IDS_MEDIABR_POP_ENTER_TAG_NAME"
#define GL_STR_MEMORY_FULL "IDS_MF_POP_NOT_ENOUGH_MEMORY_DELETE_SOME_ITEMS"
#define GL_DEVICE_MEMORY_FULL "IDS_MF_POP_THERE_IS_NOT_ENOUGH_SPACE_IN_YOUR_DEVICE_STORAGE_GO_TO_SETTINGS_POWER_AND_STORAGE_STORAGE_THEN_DELETE_SOME_FILES_AND_TRY_AGAIN"
#define GL_STR_SETTINGS "IDS_MF_BUTTON_SETTINGS"
#define GL_DATA_SAVE_FAILED "IDS_MF_HEADER_UNABLE_TO_SAVE_DATA_ABB"

#ifdef __cplusplus
}
#endif
#endif

