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

/**
 * The Exif specification defines an Orientation Tag to indicate the orientation of the
 * camera relative to the captured scene. This can be used by the camera either to
 * indicate the orientation automatically by an orientation sensor,
 * or to allow the user to indicate the orientation manually by a menu switch,
 * without actually transforming the image data itself.
 * Here is an explanation given by TsuruZoh Tachibanaya in
 * his description of the Exif file format:
 *
 * The orientation of the camera relative to the scene, when the image was captured.
 * The relation of the '0th row' and '0th column' to visual position is shown as below.
 *
 * Value	 0th Row	        0th Column
 *   1	  top	          left side
 *   2	  top	          right side
 *   3	  bottom	          right side
 *   4	  bottom	          left side
 *   5	  left side          top
 *   6	  right side        top
 *   7	  right side        bottom
 *   8	  left side          bottom
 *
 * Read this table as follows (thanks to Peter Nielsen for clarifying this - see also below):
 * Entry #6 in the table says that the 0th row in the stored image is the right side of
 * the captured scene, and the 0th column in the stored image is the top side of
 * the captured scene.
 *
 * Here is another description given by Adam M. Costello:
 *
 * For convenience, here is what the letter F would look like if it were tagged correctly
 * and displayed by a program that ignores the orientation tag
 * (thus showing the stored image):
 *
 *       1             2         3      4            5                 6                   7                  8
 *
 *  888888  888888       88    88      8888888888  88                             88  8888888888
 *  88               88        88    88      88  88          88  88                  88  88          88  88
 *  8888        8888     8888   8888   88                8888888888  8888888888               88
 *  88               88        88    88
 *  88               88  888888   888888
*/

#ifdef _USE_ROTATE_BG

#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <Elementary.h>
#include "gl-exif.h"
#include "gl-debug.h"
#include "gallery.h"
#include "gl-file-util.h"

#define GL_EXIF_BUF_LEN_MAX 65536L
#define GL_EXIF_BUF_TIME_LEN_MAX 20
#define GL_EXIF_DEFAULT_YEAR 1900

#define GL_EXIF_SOI 0xD8
#define GL_EXIF_TAG 0xFF
#define GL_EXIF_APP0 0xE0
#define GL_EXIF_APP1 0xE1
#define GL_EXIF_JFIF_00 0x00
#define GL_EXIF_JFIF_01 0x01
#define GL_EXIF_JFIF_02 0x02

#define GL_EXIF_IFD_DATA_FORMAT_UNSIGNED_BYTE 1
#define GL_EXIF_IFD_DATA_FORMAT_ASCII_STRINGS 1
#define GL_EXIF_IFD_DATA_FORMAT_UNSIGNED_SHORT 2
#define GL_EXIF_IFD_DATA_FORMAT_UNSIGNED_LONG 4
#define GL_EXIF_IFD_DATA_FORMAT_UNSIGNED_RATIONAL 8
#define GL_EXIF_IFD_DATA_FORMAT_SIGNED_BYTE 1
#define GL_EXIF_IFD_DATA_FORMAT_UNDEFINED 1
#define GL_EXIF_IFD_DATA_FORMAT_SIGNED_SHORT 2
#define GL_EXIF_IFD_DATA_FORMAT_SIGNED_LONG 4
#define GL_EXIF_IFD_DATA_FORMAT_SIGNED_RATIONAL 8
#define GL_EXIF_IFD_DATA_FORMAT_SIGNED_FLOAT 4
#define GL_EXIF_IFD_DATA_FORMAT_DOUBLE_FLOAT 8

#define GL_EXI_TMP_JPEG_FILE GL_DATA_FOLDER_PATH"/.gallery_tmp_write_exif.jpg"

/* Write one byte, testing for EOF */
static int __gl_exif_write_1_byte(FILE *fd, int c)
{
	if (fputc(c, fd) < 0) {
		gl_dbgE("fputc failed!");
		return -1;
	}

	return 0;
}

/* Read one byte, testing for EOF */
static int __gl_exif_read_1_byte(FILE *fd)
{
	int c = 0;

	/* Return next input byte, or EOF if no more */
	c = getc(fd);
	if (c == EOF) {
		gl_dbgE("Premature EOF in JPEG file!");
		return -1;
	}

	return c;
}

/* Read 2 bytes, convert to unsigned int */
/* All 2-byte quantities in JPEG markers are MSB first */
static int __gl_exif_read_2_bytes(FILE *fd, unsigned int *len)
{
	int c1 = 0;
	int c2 = 0;

	/* Return next input byte, or EOF if no more */
	c1 = getc(fd);
	if (c1 == EOF) {
		gl_dbgE("Premature EOF in JPEG file!");
		return -1;
	}

	/* Return next input byte, or EOF if no more */
	c2 = getc(fd);
	if (c2 == EOF) {
		gl_dbgE("Premature EOF in JPEG file!");
		return -1;
	}

	if (len) {
		*len = (((unsigned int)c1) << 8) + ((unsigned int)c2);
	}

	return 0;
}

/* Add raw exif tag and data */
static int __gl_exif_add_header(FILE *fd, unsigned int *orientation)
{
	GL_CHECK_VAL(orientation, -1);
	GL_CHECK_VAL(fd, -1);
	int i = 0;
	int ret = -1;
	char *time_buf = NULL;
	unsigned int offset = 0;

	/* raw EXIF header data */
	const unsigned char exif1[] = {
		GL_EXIF_TAG, GL_EXIF_SOI, GL_EXIF_TAG, GL_EXIF_APP1
	};
	/* Write File head, check for JPEG SOI + Exif APP1 */
	for (i = 0; i < 4; i++) {
		if (__gl_exif_write_1_byte(fd, exif1[i]) < 0) {
			goto GL_EXIF_FAILED;
		}
	}
	/* SET the marker parameter length count */
	/* Length includes itself, so must be at least 2
	    Following Exif data length must be at least 6; 30+36 bytes*/
	const unsigned char exif2[] = { 0x00, 0x42 };
	for (i = 0; i < 2; i++) {
		if (__gl_exif_write_1_byte(fd, exif2[i]) < 0) {
			goto GL_EXIF_FAILED;
		}
	}

	/* Write Exif head -- "Exif" */
	const unsigned char exif3[] = { 0x45, 0x78, 0x69, 0x66, 0x00, 0x00 };
	for (i = 0; i < 6; i++) {
		if (__gl_exif_write_1_byte(fd, exif3[i]) < 0) {
			goto GL_EXIF_FAILED;
		}
	}

	/* Set byte order and Tag Mark , "II(0x4949)" */
	const unsigned char exif4[] = { 0x49, 0x49, 0x2A, 0x00 };
	for (i = 0; i < 4; i++) {
		if (__gl_exif_write_1_byte(fd, exif4[i]) < 0) {
			goto GL_EXIF_FAILED;
		}
	}
	offset += 4;

	/* Set first IFD offset (offset to IFD0) , II-08000000 */
	const unsigned char exif5[] = { 0x08, 0x00, 0x00, 0x00 };
	for (i = 0; i < 4; i++) {
		if (__gl_exif_write_1_byte(fd, exif5[i]) < 0) {
			goto GL_EXIF_FAILED;
		}
	}
	offset += 4;

	/* IFD: Image File Directory */
	/* Set the number of directory entries contained in this IFD, - EEEE ;
	  * 2 entry: orientation, data time */
	const unsigned char exif6[] = { 0x02, 0x00 };
	for (i = 0; i < 2; i++) {
		if (__gl_exif_write_1_byte(fd, exif6[i]) < 0) {
			goto GL_EXIF_FAILED;
		}
	}
	offset += 2;

	/* Add Orientation Tag in IFD0; 0x0112 */
	const unsigned char exif7[] = { 0x12, 0x01 };
	for (i = 0; i < 2; i++) {
		if (__gl_exif_write_1_byte(fd, exif7[i]) < 0) {
			goto GL_EXIF_FAILED;
		}
	}
	offset += 2;

	gl_dbg("Write: %d", *orientation);
	const unsigned char exif8[] = { 0x03, 0x00, 0x01, 0x00, 0x00, 0x00 };
	for (i = 0; i < 6; i++) {
		if (__gl_exif_write_1_byte(fd, exif8[i]) < 0) {
			goto GL_EXIF_FAILED;
		}
	}
	offset += 6;

	/* Set the Orientation value */
	if (__gl_exif_write_1_byte(fd, (unsigned char)(*orientation)) < 0) {
		goto GL_EXIF_FAILED;
	}

	const unsigned char exif9[] = { 0x00, 0x00, 0x00 };
	for (i = 0; i < 3; i++) {
		if (__gl_exif_write_1_byte(fd, exif9[i]) < 0) {
			goto GL_EXIF_FAILED;
		}
	}
	offset += 4;

	/* Add Data Time Tag in IFD0; 0x0132 */
	const unsigned char exif10[] = { 0x32, 0x01 };
	for (i = 0; i < 2; i++) {
		if (__gl_exif_write_1_byte(fd, exif10[i]) < 0) {
			goto GL_EXIF_FAILED;
		}
	}
	offset += 2;

	/* Type: strings */
	const unsigned char exif11[] = { 0x02, 0x00 };
	for (i = 0; i < 2; i++) {
		if (__gl_exif_write_1_byte(fd, exif11[i]) < 0) {
			goto GL_EXIF_FAILED;
		}
	}
	offset += 2;

	/* Data lengh, byte count */
	const unsigned char exif12[] = { 0x14, 0x00, 0x00, 0x00 };
	for (i = 0; i < 4; i++) {
		if (__gl_exif_write_1_byte(fd, exif12[i]) < 0) {
			goto GL_EXIF_FAILED;
		}
	}
	offset += 8;

	/* 20 bytes larger than 4 bytes,
	  * so next 4 bytes is data offset start from "II"(0x4949)*/

	gl_dbg("offset: %2X", offset + 8);
	/* Too add data offset, plus 4 bytes self and plus 4 bytes IFD terminator */
	if (__gl_exif_write_1_byte(fd, (unsigned char)(offset + 4)) < 0) {
		goto GL_EXIF_FAILED;
	}

	const unsigned char exif13[] = { 0x00, 0x00, 0x00 };
	for (i = 0; i < 3; i++) {
		if (__gl_exif_write_1_byte(fd, exif13[i]) < 0) {
			goto GL_EXIF_FAILED;
		}
	}

	/*After last directory entry, there is a 4bytes of data('LLLLLLLL' at the chart),
	  * it means an offset to next IFD. If its value is '0x00000000',
	  * it means this is the last IFD and there is no linked IFD */
	const unsigned char exif14[] = { 0x00, 0x00, 0x00, 0x00 };
	for (i = 0; i < 4; i++) {
		if (__gl_exif_write_1_byte(fd, exif14[i]) < 0) {
			goto GL_EXIF_FAILED;
		}
	}

	/* Date Time of image was last modified.
	  * Data format is "YYYY:MM:DD HH:MM:SS"+0x00, total 20bytes
	  */
	time_t t;
	struct tm tms;
	struct tm *tm;

	t = time(NULL);
	tm = localtime_r(&t, &tms);
	if (!tm) {
		gl_dbgE("Faild to allocate memory!");
		goto GL_EXIF_FAILED;
	}

	time_buf = (char *)calloc(1, GL_EXIF_BUF_TIME_LEN_MAX);
	if (!time_buf) {
		gl_dbgE("Faild to allocate memory!");
		goto GL_EXIF_FAILED;
	}
	snprintf(time_buf, GL_EXIF_BUF_TIME_LEN_MAX,
	         "%04i:%02i:%02i %02i:%02i:%02i",
	         tm->tm_year + GL_EXIF_DEFAULT_YEAR, tm->tm_mon + 1,
	         tm->tm_mday, tm->tm_hour, tm->tm_min, tm->tm_sec);

	gl_dbg("time_buf: %s", time_buf);
	if (fwrite(time_buf, 1, GL_EXIF_BUF_TIME_LEN_MAX, fd) != GL_EXIF_BUF_TIME_LEN_MAX) {
		gl_dbgW("Write size are diff!");
		goto GL_EXIF_FAILED;
	}

	ret = 0;

GL_EXIF_FAILED:

	gl_dbg("All done");
	GL_FREEIF(time_buf);
	return ret;
}

/* Add exif to jfif , don't have exif */
static int __gl_exif_add_exif_to_jfif(char *file_path, unsigned int *orientation)
{
	GL_CHECK_VAL(orientation, -1);
	GL_CHECK_VAL(file_path, -1);
	unsigned char tmp[GL_EXIF_BUF_LEN_MAX] = { 0, };
	FILE *fd = NULL;
	int ret = -1;

	if ((fd = fopen(file_path, "rb+")) == NULL) {
		gl_sdbgE("Can't open %s!", file_path);
		return -1;
	}

	char *tmp_file = GL_EXI_TMP_JPEG_FILE;
	FILE *tmp_fd = NULL;
	if ((tmp_fd = fopen(tmp_file, "wb+")) == NULL) {
		gl_dbgE("Can't open %s!", tmp_file);
		goto GL_EXIF_FAILED;
	}

	/* Add raw EXIF header data */
	if (__gl_exif_add_header(tmp_fd, orientation) < 0) {
		goto GL_EXIF_FAILED;
	}

	size_t r_size = 0;
	/* Remove start of JPEG image data section, 20 bytes */
	r_size = fread(tmp, sizeof(char), 20, fd);

	if (r_size > 0) {
		gl_dbg("r_size: %ld", r_size);
	}

	memset(tmp, 0x00, GL_EXIF_BUF_LEN_MAX);
	/* Write JPEG image data to tmp file after EXIF header */
	while ((r_size = fread(tmp, 1, sizeof(tmp), fd)) > 0) {
		gl_dbg("r_size: %ld", r_size);
		if (fwrite(tmp, 1, r_size, tmp_fd) != r_size) {
			gl_dbgW("Write and read size are diff!");
		}

		memset(tmp, 0x00, GL_EXIF_BUF_LEN_MAX);
	}

	fclose(fd);
	fd = fopen(file_path, "wb");
	if (!fd) {
		gl_sdbgE("Error creating file %s!", file_path);
		goto GL_EXIF_FAILED;
	}

	memset(tmp, 0x00, GL_EXIF_BUF_LEN_MAX);
	/* Write back tmp file after to JPEG image */
	int val = fseek(tmp_fd, 0, SEEK_SET);
	gl_dbg("return value is : %d", val);
	while ((r_size = fread(tmp, 1, sizeof(tmp), tmp_fd)) > 0) {
		gl_dbg("r_size: %ld", r_size);
		if (fwrite(tmp, 1, r_size, fd) != r_size) {
			gl_dbgW("Write and read size are diff!");
		}

		memset(tmp, 0x00, GL_EXIF_BUF_LEN_MAX);
	}

	ret = 0;

GL_EXIF_FAILED:

	if (fd) {
		fclose(fd);
	}
	if (tmp_fd) {
		fclose(tmp_fd);
	}

	/* Delete tmp file */
	if (!gl_file_unlink(tmp_file)) {
		gl_dbgE("Delete file failed");
	}

	gl_dbg("All done");
	return ret;
}



/* Add  orientation tag to jpegs which have exif tag but do not have orientation tag: include jfif and exif*/
static int __gl_exif_add_orientation_tag(char *file_path,
			unsigned int *orientation)
{

	GL_CHECK_VAL(orientation, -1);
	GL_CHECK_VAL(file_path, -1);
	unsigned char tmp[GL_EXIF_BUF_LEN_MAX] = { 0, };
	FILE *fd = NULL;
	int ret = -1;
	int tmp_exif = -1;
	int i = 0;
	unsigned int length = 0;
	bool is_motorola = false; /* Flag for byte order */
	unsigned int offset = 0;
	size_t r_size = 0;
	const unsigned char ifd_data_format[13] = {
		/*add 0 to ifd_data_format[0] ,for  easy to use*/
		0,
		GL_EXIF_IFD_DATA_FORMAT_UNSIGNED_BYTE,
		GL_EXIF_IFD_DATA_FORMAT_ASCII_STRINGS,
		GL_EXIF_IFD_DATA_FORMAT_UNSIGNED_SHORT,
		GL_EXIF_IFD_DATA_FORMAT_UNSIGNED_LONG,
		GL_EXIF_IFD_DATA_FORMAT_UNSIGNED_RATIONAL,
		GL_EXIF_IFD_DATA_FORMAT_SIGNED_BYTE,
		GL_EXIF_IFD_DATA_FORMAT_UNDEFINED,
		GL_EXIF_IFD_DATA_FORMAT_SIGNED_SHORT,
		GL_EXIF_IFD_DATA_FORMAT_SIGNED_LONG,
		GL_EXIF_IFD_DATA_FORMAT_SIGNED_RATIONAL,
		GL_EXIF_IFD_DATA_FORMAT_SIGNED_FLOAT,
		GL_EXIF_IFD_DATA_FORMAT_DOUBLE_FLOAT

	};

	if ((fd = fopen(file_path, "rb+")) == NULL) {
		gl_sdbgE("Can't open %s!", file_path);
		return -1;
	}

	char *tmp_file = GL_EXI_TMP_JPEG_FILE;
	FILE *tmp_fd = NULL;
	if ((tmp_fd = fopen(tmp_file, "wb+")) == NULL) {
		gl_dbgE("Can't open %s!", tmp_file);
		goto GL_EXIF_FAILED;
	}
	/* Find APP1 */
	bool b_tag_ff = false;
	while (1) {
		tmp_exif = __gl_exif_read_1_byte(fd);
		if (tmp_exif < 0) {
			goto GL_EXIF_FAILED;
		}

		/*copy data from jpeg to tmp_fd (from "FF D8" to " FF E1",because those data we needn't modify)*/
		if (__gl_exif_write_1_byte(tmp_fd, tmp_exif) < 0) {
			goto GL_EXIF_FAILED;
		}

		tmp[0] = (unsigned char)tmp_exif;

		gl_dbg("- %02X", tmp[0]);
		if (!b_tag_ff) {
			/* Get first tag */
			if (tmp[0] == GL_EXIF_TAG) {
				gl_dbgW("0xFF!");
				b_tag_ff = true;
			}
			continue;
		}

		/* Get APP1 */
		if (tmp[0] == GL_EXIF_APP1) {
			gl_dbgW("Exif in APP1!");
			break;
		} else {
			gl_dbgW("0x%02X!", tmp[0]);
			b_tag_ff = false;
		}
	}

	/* Get the marker parameter length count */
	if (__gl_exif_read_2_bytes(fd, &length) < 0) {
		goto GL_EXIF_FAILED;
	}
	gl_dbg("length: %d", length);
	/* Length includes itself, so must be at least 2
	    Following Exif data length must be at least 6 */
	if (length < 8) {
		gl_dbgE("length < 8");
		goto GL_EXIF_FAILED;
	}
	/*modify  the marker parameter length, orientation tag is 12*/
	length += 12;
	gl_dbgW("modified length: %d", length);
	tmp[0] = (length >> 8) & 0xff ;
	tmp[1] = length & 0xff ;
	for (i = 0; i < 2; i++) {
		if (__gl_exif_write_1_byte(tmp_fd, tmp[i]) < 0) {
			goto GL_EXIF_FAILED;
		}
	}

	for (i = 0; i < 6; i++) {
		tmp_exif = __gl_exif_read_1_byte(fd);
		if (tmp_exif < 0) {
			goto GL_EXIF_FAILED;
		}

		tmp[i] = (unsigned char)tmp_exif;
		gl_dbg("- %02X", tmp[i]);
		if (__gl_exif_write_1_byte(tmp_fd, tmp[i]) < 0) {
			goto GL_EXIF_FAILED;
		}
	}
	if (tmp[0] == 0x45 && tmp[1] == 0x78 && tmp[2] == 0x69 && tmp[3] == 0x66 &&
	        tmp[4] == 0x00 && tmp[5] == 0x00) {
		gl_dbgW("Met Exif!");
	} else {
		gl_dbgW("Not met Exif!");
		goto GL_EXIF_FAILED;
	}
	/* Read Exif body */
	for (i = 0; i < 4; i++) {
		tmp_exif = __gl_exif_read_1_byte(fd);
		if (tmp_exif < 0) {
			goto GL_EXIF_FAILED;
		}
		tmp[i] = (unsigned char)tmp_exif;
		if (__gl_exif_write_1_byte(tmp_fd, tmp[i]) < 0) {
			goto GL_EXIF_FAILED;
		}
	}

	/* Check byte order and Tag Mark , "II(0x4949)" or "MM(0x4d4d)" */
	if (tmp[0] == 0x49 && tmp[1] == 0x49 && tmp[2] == 0x2A &&
	        tmp[3] == 0x00) {
		gl_dbg("Intel");
		is_motorola = false;
	} else if (tmp[0] == 0x4D && tmp[1] == 0x4D && tmp[2] == 0x00 &&
	           tmp[3] == 0x2A) {
		gl_dbg("Motorola");
		is_motorola = true;
	} else {
		goto GL_EXIF_FAILED;
	}

	for (i = 0; i < 4; i++) {
		tmp_exif = __gl_exif_read_1_byte(fd);
		if (tmp_exif < 0) {
			goto GL_EXIF_FAILED;
		}

		tmp[i] = (unsigned char)tmp_exif;
		gl_dbg("- %02X", tmp[i]);
		if (__gl_exif_write_1_byte(tmp_fd, tmp[i]) < 0) {
			goto GL_EXIF_FAILED;
		}
	}

	/* Get first IFD offset (offset to IFD0) , MM-08000000, II-00000008 */
	if (is_motorola) {
		if (tmp[0] != 0 && tmp[1] != 0) {
			goto GL_EXIF_FAILED;
		}
		offset = tmp[2];
		offset <<= 8;
		offset += tmp[3];
	} else {
		if (tmp[3] != 0 && tmp[2] != 0) {
			goto GL_EXIF_FAILED;
		}
		offset = tmp[1];
		offset <<= 8;
		offset += tmp[0];
	}
	gl_dbg("offset: %d", offset);
	/*if offset >8, copy data from there to IFD start position*/
	if (offset > 8) {
		for (i = 0; i < (offset - 8); i++) {
			tmp_exif = __gl_exif_read_1_byte(fd);
			if (tmp_exif < 0) {
				goto GL_EXIF_FAILED;
			}

			tmp[i] = (unsigned char)tmp_exif;
			gl_dbg("- %02X", tmp[i]);
			if (__gl_exif_write_1_byte(tmp_fd, tmp[i]) < 0) {
				goto GL_EXIF_FAILED;
			}
		}
	}

	/* IFD: Image File Directory */
	/* Get the number of directory entries contained in this IFD, - 2 bytes, EE */
	unsigned int tags_cnt = 0;
	for (i = 0; i < 2; i++) {
		tmp_exif = __gl_exif_read_1_byte(fd);
		if (tmp_exif < 0) {
			goto GL_EXIF_FAILED;
		}

		tmp[i] = (unsigned char)tmp_exif;
	}
	if (is_motorola) {
		tags_cnt = tmp[0];
		tags_cnt <<= 8;
		tags_cnt += tmp[1];
	} else {
		tags_cnt = tmp[1];
		tags_cnt <<= 8;
		tags_cnt += tmp[0];
	}
	gl_dbg("tags_cnt: %d", tags_cnt);
	/*modify tags num,add orientation tag */
	tags_cnt += 1;
	gl_dbg("modified tags_cnt: %d", tags_cnt);
	if (is_motorola) {
		tmp[0] = (tags_cnt >> 8) & 0xff;
		tmp[1] = tags_cnt & 0xff;
	} else {
		tmp[0] = tags_cnt & 0xff;
		tmp[1] = (tags_cnt >> 8) & 0xff;
	}
	for (i = 0; i < 2; i++) {
		gl_dbg("modified- %02X", tmp[i]);
		if (__gl_exif_write_1_byte(tmp_fd, tmp[i]) < 0) {
			goto GL_EXIF_FAILED;
		}

	}
	/* Add   Orientation Tag in IFD0 */
	unsigned int tag_num = 0;
	unsigned char orientation_tag[12] = { 0, };
	bool b_found_position = false;
	int j = 0;
	unsigned int data_type = 0;
	unsigned int unit_num = 0;
	unsigned int data_length = 0;
	unsigned int offset_value = 0;
	/*initialize orientation_tag value*/
	if (is_motorola) {
		orientation_tag[0] = 0x01;
		orientation_tag[1] = 0x12;

		orientation_tag[2] = 0x00;
		orientation_tag[3] = 0x03;

		orientation_tag[4] = 0x00;
		orientation_tag[5] = 0x00;
		orientation_tag[6] = 0x00;
		orientation_tag[7] = 0x01;

		orientation_tag[8] = 0x00;
		orientation_tag[9] = (unsigned char)(*orientation);
		orientation_tag[10] = 0x00;
		orientation_tag[11] = 0x00;

	} else {
		orientation_tag[0] = 0x12;
		orientation_tag[1] = 0x01;
		orientation_tag[2] = 0x03;
		orientation_tag[3] = 0x00;
		orientation_tag[4] = 0x01;
		orientation_tag[5] = 0x00;
		orientation_tag[6] = 0x00;
		orientation_tag[7] = 0x00;
		orientation_tag[8] = (unsigned char)(*orientation);
		orientation_tag[9] = 0x00;
		orientation_tag[10] = 0x00;
		orientation_tag[11] = 0x00;
	}
	/*if there is no other tag, then only insert orientation_tag,don't go to the while (1)*/
	if (tags_cnt == 1) {
		for (j = 0; j < 12 ; j++) {
			gl_dbg("orientation_tag- %02X", orientation_tag[j]);
			if (__gl_exif_write_1_byte(tmp_fd, orientation_tag[j]) < 0) {
				goto GL_EXIF_FAILED;
			}
		}
	}
	while (1) {
		if (--tags_cnt == 0) {
			break;
		}

		/* Every directory entry size is 12 */
		for (i = 0; i < 12; i++) {
			tmp_exif = __gl_exif_read_1_byte(fd);
			if (tmp_exif < 0) {
				goto GL_EXIF_FAILED;
			}

			tmp[i] = (unsigned char)tmp_exif;
		}
		/* Get Tag number */
		if (is_motorola) {
			tag_num = tmp[0];
			tag_num <<= 8;
			tag_num += tmp[1];
		} else {
			tag_num = tmp[1];
			tag_num <<= 8;
			tag_num += tmp[0];
		}
		gl_dbgW("tag num %02X!" , tag_num);
		/* to find Orientation Tag position */
		if (tag_num < 0x0112) {

		} else if (tag_num > 0x0112) {
			if (!b_found_position) {
				for (j = 0; j < 12 ; j++) {
					gl_dbg("orientation_tag- %02X", orientation_tag[j]);
					if (__gl_exif_write_1_byte(tmp_fd, orientation_tag[j]) < 0) {
						goto GL_EXIF_FAILED;
					}
				}
				b_found_position = true;
			}
			if (is_motorola) {
				data_type = tmp[2];
				data_type <<= 8;
				data_type += tmp[3];

				unit_num = tmp[4];
				unit_num <<= 8;
				unit_num += tmp[5];
				unit_num <<= 8;
				unit_num += tmp[6];
				unit_num <<= 8;
				unit_num += tmp[7];
			} else {
				data_type = tmp[3];
				data_type <<= 8;
				data_type += tmp[2];

				unit_num = tmp[7];
				unit_num <<= 8;
				unit_num += tmp[6];
				unit_num <<= 8;
				unit_num += tmp[5];
				unit_num <<= 8;
				unit_num += tmp[4];
			}
			gl_dbgW("data_type %02X!" , data_type);
			gl_dbgW("unit_num %02X!" , unit_num);
			if ((data_type < 1) || (data_type > 12)) {
				gl_dbgE("Wrong data type!");
				goto GL_EXIF_FAILED;
			}

			data_length = ifd_data_format[data_type] * unit_num;
			gl_dbgW("data_length %02X!" , data_length);
			/*data_length >4 ,next 4 bytes  store the offset, so need to modify the offset*/
			if (data_length > 4) {
				if (is_motorola) {
					offset_value = tmp[8];
					offset_value <<= 8;
					offset_value += tmp[9];
					offset_value <<= 8;
					offset_value += tmp[10];
					offset_value <<= 8;
					offset_value += tmp[11];
					gl_dbgW("offset_value %02X!" , offset_value);
					/*add orientation offset*/
					offset_value += 12;
					gl_dbgW("changed offset_value %02X!" , offset_value);
					tmp[8] = (offset_value >> 24) & 0xff;
					tmp[9] = (offset_value >> 16) & 0xff;
					tmp[10] = (offset_value >> 8) & 0xff;
					tmp[11] = offset_value & 0xff;
					gl_dbg("tmp[8] %02X!" , tmp[8]);
					gl_dbg("tmp[9] %02X!" , tmp[9]);
					gl_dbg("tmp[10] %02X!" , tmp[10]);
					gl_dbg("tmp[11] %02X!" , tmp[11]);
				} else {
					offset_value = tmp[11];
					offset_value <<= 8;
					offset_value += tmp[10];
					offset_value <<= 8;
					offset_value += tmp[9];
					offset_value <<= 8;
					offset_value += tmp[8];
					gl_dbgW("offset_value %02X!" , offset_value);
					/*add orientation offset*/
					offset_value += 12;
					gl_dbgW("changed offset_value %02X!" , offset_value);

					tmp[11] = (offset_value >> 24) & 0xff;
					tmp[10] = (offset_value >> 16) & 0xff;
					tmp[9] = (offset_value >> 8) & 0xff;
					tmp[8] = offset_value & 0xff;
					gl_dbg("tmp[8] %02X!" , tmp[8]);
					gl_dbg("tmp[9] %02X!" , tmp[9]);
					gl_dbg("tmp[10] %02X!" , tmp[10]);
					gl_dbg("tmp[11] %02X!" , tmp[11]);

				}

			}

		}
		for (i = 0; i < 12 ; i++) {
			gl_dbg("- %02X", tmp[i]);
			if (__gl_exif_write_1_byte(tmp_fd, tmp[i]) < 0) {
				goto GL_EXIF_FAILED;
			}

		}
		memset(tmp, 0x00, 12);

	}
	memset(tmp, 0x00, GL_EXIF_BUF_LEN_MAX);
	/* Write JPEG image data to tmp file after EXIF header */
	while ((r_size = fread(tmp, 1, sizeof(tmp), fd)) > 0) {
		gl_dbg("r_size: %ld", r_size);
		if (fwrite(tmp, 1, r_size, tmp_fd) != r_size) {
			gl_dbgW("Write and read size are diff!");
		}

		memset(tmp, 0x00, GL_EXIF_BUF_LEN_MAX);
	}
	fclose(fd);
	fd = NULL;
	fd = fopen(file_path, "wb");
	if (!fd) {
		gl_sdbgE("Error creating file %s!", file_path);
		goto GL_EXIF_FAILED;
	}

	memset(tmp, 0x00, GL_EXIF_BUF_LEN_MAX);
	/* Write back tmp file after to JPEG image */
	int val = fseek(tmp_fd, 0, SEEK_SET);
	gl_dbg("return value is : %d", val);
	while ((r_size = fread(tmp, 1, sizeof(tmp), tmp_fd)) > 0) {
		gl_dbg("r_size: %ld", r_size);
		if (fwrite(tmp, 1, r_size, fd) != r_size) {
			gl_dbgW("Write and read size are diff!");
		}
		memset(tmp, 0x00, GL_EXIF_BUF_LEN_MAX);
	}

	ret = 0;

GL_EXIF_FAILED:

	if (fd) {
		fclose(fd);
		fd = NULL;
	}

	if (tmp_fd) {
		fclose(tmp_fd);
		tmp_fd = NULL;
	}

	/* Delete tmp file */
	if (!gl_file_unlink(tmp_file)) {
		gl_dbgE("Delete file failed");
	}

	gl_dbg("All done");
	return ret;
}

static int __gl_exif_rw_jfif(FILE *fd, char *file_path,
                             unsigned int *orientation, bool b_write)
{
	GL_CHECK_VAL(fd, -1);
	GL_CHECK_VAL(file_path, -1);
	GL_CHECK_VAL(orientation, -1);
	unsigned char tmp[GL_EXIF_BUF_LEN_MAX] = { 0, };
	int i = 0;
	unsigned int length = 0;
	int tmp_exif = -1;
	bool is_motorola = false; /* Flag for byte order */
	unsigned int offset = 0;
	int ret = -1;
	/*unsigned char version = 0x00; */

	if (__gl_exif_read_2_bytes(fd, &length) < 0) {
		goto GL_EXIF_FAILED;
	}
	gl_dbg("length: %d", length);

	for (i = 0; i < 5; i++) {
		tmp_exif = __gl_exif_read_1_byte(fd);
		if (tmp_exif < 0) {
			goto GL_EXIF_FAILED;
		}
		tmp[i] = (unsigned char)tmp_exif;
	}

	/* JFIF0 */
	if (tmp[0] != 0x4A || tmp[1] != 0x46 || tmp[2] != 0x49 ||
	        tmp[3] != 0x46 || tmp[4] != 0x00) {
		gl_dbgE("Not met Jfif!");
		goto GL_EXIF_FAILED;
	}

	for (i = 0; i < 2; i++) {
		tmp_exif = __gl_exif_read_1_byte(fd);
		if (tmp_exif < 0) {
			goto GL_EXIF_FAILED;
		}
		tmp[i] = (unsigned char)tmp_exif;
	}

	/* Check JFIF version */
	if (tmp[0] == 0x01 && tmp[1] == GL_EXIF_JFIF_00) {
		gl_dbg("Jfif 1.00");
	} else if (tmp[0] == 0x01 && tmp[1] == GL_EXIF_JFIF_01) {
		gl_dbg("Jfif 1.01");
	} else if (tmp[0] == 0x01 && tmp[1] == GL_EXIF_JFIF_02) {
		gl_dbg("Jfif 1.02");
	} else {
		gl_dbgE("Unknow Jfif version[%d.%d]!", tmp[0], tmp[1]);
		goto GL_EXIF_FAILED;
	}

	/* Save version */
	/*version = tmp[1]; */

	/* Find APP1 */
	bool b_tag_ff = false;
	while (1) {
		tmp_exif = __gl_exif_read_1_byte(fd);
		if (tmp_exif < 0) {
			goto GL_EXIF_FAILED;
		}

		tmp[0] = (unsigned char)tmp_exif;

		gl_dbg("- %02X", tmp[0]);
		if (!b_tag_ff) {
			/* Get first tag */
			if (tmp[0] == GL_EXIF_TAG) {
				gl_dbgW("0xFF!");
				b_tag_ff = true;
			}
			continue;
		}

		/* Get APP1 */
		if (tmp[0] == GL_EXIF_APP1) {
			gl_dbgW("Exif in APP1!");
			break;
		}

		gl_dbgW("No Exif in APP1!");

		/* Close file */
		fclose(fd);
		if (!b_write) {
			/* Normal orientation = 0degree = 1 */
			*orientation = 1;
			return 0;
		}
		return __gl_exif_add_exif_to_jfif(file_path, orientation);
	}

	/* Find Exif */
	while (1) {
		for (i = 0; i < 6; i++) {
			tmp_exif = __gl_exif_read_1_byte(fd);
			if (tmp_exif < 0) {
				gl_dbgW("Not met Exif!");
				goto GL_EXIF_FAILED;
			}

			tmp[i] = (unsigned char)tmp_exif;
			gl_dbg("- %02X", tmp[i]);
		}
		if (tmp[0] == 0x45 && tmp[1] == 0x78 && tmp[2] == 0x69 && tmp[3] == 0x66 &&
		        tmp[4] == 0x00 && tmp[5] == 0x00) {
			gl_dbgW("Met Exif!");
			break;
		} else {
			gl_dbg("Not met Exif!");
			if (fseek(fd, -5, SEEK_CUR) < 0) {
				gl_dbgE("fseek failed!");
				goto GL_EXIF_FAILED;
			}
			continue;
		}
	}

	/* Read Exif body */
	for (i = 0; i < 4; i++) {
		tmp_exif = __gl_exif_read_1_byte(fd);
		if (tmp_exif < 0) {
			goto GL_EXIF_FAILED;
		}
		tmp[i] = (unsigned char)tmp_exif;
	}

	/* Check byte order and Tag Mark , "II(0x4949)" or "MM(0x4d4d)" */
	if (tmp[0] == 0x49 && tmp[1] == 0x49 && tmp[2] == 0x2A &&
	        tmp[3] == 0x00) {
		gl_dbg("Intel");
		is_motorola = false;
	} else if (tmp[0] == 0x4D && tmp[1] == 0x4D && tmp[2] == 0x00 &&
	           tmp[3] == 0x2A) {
		gl_dbg("Motorola");
		is_motorola = true;
	} else {
		goto GL_EXIF_FAILED;
	}

	for (i = 0; i < 4; i++) {
		tmp_exif = __gl_exif_read_1_byte(fd);
		if (tmp_exif < 0) {
			goto GL_EXIF_FAILED;
		}

		tmp[i] = (unsigned char)tmp_exif;
		gl_dbg("- %02X", tmp[i]);
	}

	/* Get first IFD offset (offset to IFD0) , MM-08000000, II-00000008 */
	if (is_motorola) {
		if (tmp[0] != 0 && tmp[1] != 0) {
			goto GL_EXIF_FAILED;
		}
		offset = tmp[2];
		offset <<= 8;
		offset += tmp[3];
	} else {
		if (tmp[3] != 0 && tmp[2] != 0) {
			goto GL_EXIF_FAILED;
		}
		offset = tmp[1];
		offset <<= 8;
		offset += tmp[0];
	}
	gl_dbg("offset: %d", offset);

	/* IFD: Image File Directory */
	/* Get the number of directory entries contained in this IFD, - 2 bytes, EE */
	unsigned int tags_cnt = 0;
	for (i = 0; i < 2; i++) {
		tmp_exif = __gl_exif_read_1_byte(fd);
		if (tmp_exif < 0) {
			goto GL_EXIF_FAILED;
		}

		tmp[i] = (unsigned char)tmp_exif;
	}
	if (is_motorola) {
		tags_cnt = tmp[0];
		tags_cnt <<= 8;
		tags_cnt += tmp[1];
	} else {
		tags_cnt = tmp[1];
		tags_cnt <<= 8;
		tags_cnt += tmp[0];
	}
	gl_dbg("tags_cnt: %d", tags_cnt);
	if (tags_cnt == 0) {
		gl_dbgE("tags_cnt == 0,no found orientation tag!");
		if (b_write) {
			gl_dbgW("to add an orientation tag!");
			fclose(fd);
			fd = NULL;
			return __gl_exif_add_orientation_tag(file_path, orientation);

		} else {
			/* Normal orientation = 0degree = 1 */
			*orientation = 1;
			ret = 0;
		}
		goto GL_EXIF_FAILED;
	}

	/* Search for Orientation Tag in IFD0 */
	unsigned int tag_num = 0;
	while (1) {
		/* Every directory entry size is 12 */
		for (i = 0; i < 12; i++) {
			tmp_exif = __gl_exif_read_1_byte(fd);
			if (tmp_exif < 0) {
				goto GL_EXIF_FAILED;
			}

			tmp[i] = (unsigned char)tmp_exif;
		}
		/* Get Tag number */
		if (is_motorola) {
			tag_num = tmp[0];
			tag_num <<= 8;
			tag_num += tmp[1];
		} else {
			tag_num = tmp[1];
			tag_num <<= 8;
			tag_num += tmp[0];
		}
		/* found Orientation Tag */
		if (tag_num == 0x0112) {
			gl_dbgW("Found orientation tag!");
			break;
		}
		if (--tags_cnt == 0) {
			gl_dbgW("tags_cnt == 0, no found orientation tag!");
			if (b_write) {
				gl_dbgW("to add an orientation tag!");
				fclose(fd);
				fd = NULL;
				return __gl_exif_add_orientation_tag(file_path, orientation);

			} else {
				/* Normal orientation = 0degree = 1 */
				*orientation = 1;
				ret = 0;
			}
			goto GL_EXIF_FAILED;
		}
	}

	/* |TT|ff|NNNN|DDDD|  ---  TT - 2 bytes, tag NO. ;ff - 2 bytes, data format
	     NNNN - 4 bytes, entry count;  DDDD - 4 bytes Data value */
	if (b_write) {
		gl_dbg("Write: %d", *orientation);
		/* Set the Orientation value */
		if (is_motorola) {
			tmp[9] = (unsigned char)(*orientation);
		} else {
			tmp[8] = (unsigned char)(*orientation);
		}

		/* Move pointer back to the entry start point */
		if (fseek(fd, -12, SEEK_CUR) < 0) {
			gl_dbgE("fseek failed!");
			goto GL_EXIF_FAILED;
		}
		fwrite(tmp, 1, 10, fd);
	} else {
		/* Get the Orientation value */
		if (is_motorola) {
			if (tmp[8] != 0) {
				gl_dbgE("tmp[8] != 0");
				goto GL_EXIF_FAILED;
			}
			*orientation = (unsigned int)tmp[9];
		} else {
			if (tmp[9] != 0) {
				gl_dbgE("tmp[9] != 0");
				goto GL_EXIF_FAILED;
			}
			*orientation = (unsigned int)tmp[8];
		}
		if (*orientation > 8) {
			gl_dbgE("*orient > 8");
			goto GL_EXIF_FAILED;
		}
		gl_dbg("Read: %d", *orientation);
	}

	ret = 0;

GL_EXIF_FAILED:

	fclose(fd);
	gl_dbg("All done");
	return ret;
}
bool _gl_exif_check_img_type(char *file_path)
{
	GL_CHECK_VAL(file_path, -1);
	int tmp_exif = -1;
	unsigned int i = 0;
	unsigned char exif_data[4] = { 0, };
	FILE *fd = NULL;
	bool ret = false;

	if ((fd = fopen(file_path, "rb")) == NULL) {
		gl_sdbgE("Can't open %s!", file_path);
		return -1;
	}


	/* Read File head, check for JPEG SOI + Exif APP1 */
	for (i = 0; i < 4; i++) {
		tmp_exif = __gl_exif_read_1_byte(fd);
		if (tmp_exif < 0) {
			goto GL_EXIF_FAILED;
		}

		exif_data[i] = (unsigned char)tmp_exif;
	}

	if (exif_data[0] == GL_EXIF_TAG && exif_data[1] == GL_EXIF_SOI) {
		gl_dbg("JPEG file");
	} else {
		gl_dbgE("Not a JPEG file!");
		goto GL_EXIF_FAILED;
	}

	if (exif_data[2] == GL_EXIF_TAG && exif_data[3] == GL_EXIF_APP1) {
		gl_dbgW("Exif in APP1!");
		ret = true;
	} else if (exif_data[2] == GL_EXIF_TAG &&
	           exif_data[3] == GL_EXIF_APP0) {
		gl_dbgW("Jfif in APP0!");
		ret = true;
	} else {
		gl_dbgE("Not a Exif in APP1 or Jiff in APP2[%d]!", exif_data[3]);
		ret = false;
	}
GL_EXIF_FAILED:

	fclose(fd);
	gl_dbg("");
	return ret;
}

static int __gl_exif_rw_orient(char *file_path, unsigned int *orient, bool b_write)
{
	GL_CHECK_VAL(file_path, -1);
	gl_dbg("b_write: %d", b_write);
	unsigned int length = 0;
	unsigned int i = 0;
	bool is_motorola = false; /* Flag for byte order */
	unsigned int offset = 0;
	unsigned int jfif_offset = 0;
	unsigned int tags_cnt = 0;
	unsigned int tag_num = 0;
	int tmp_exif = -1;
	unsigned char exif_data[GL_EXIF_BUF_LEN_MAX] = { 0, };
	FILE *fd = NULL;
	int ret = -1;

	if (b_write) {
		if ((fd = fopen(file_path, "rb+")) == NULL) {
			gl_sdbgE("Can't open %s!", file_path);
			return -1;
		}
	} else {
		if ((fd = fopen(file_path, "rb")) == NULL) {
			gl_sdbgE("Can't open %s!", file_path);
			return -1;
		}
	}

	/* Read File head, check for JPEG SOI + Exif APP1 */
	for (i = 0; i < 4; i++) {
		tmp_exif = __gl_exif_read_1_byte(fd);
		if (tmp_exif < 0) {
			goto GL_EXIF_FAILED;
		}

		exif_data[i] = (unsigned char)tmp_exif;
	}

	if (exif_data[0] == GL_EXIF_TAG && exif_data[1] == GL_EXIF_SOI) {
		gl_dbg("JPEG file");
	} else {
		gl_dbgE("Not a JPEG file!");
		goto GL_EXIF_FAILED;
	}

	if (exif_data[2] == GL_EXIF_TAG && exif_data[3] == GL_EXIF_APP1) {
		gl_dbgW("Exif in APP1!");
	} else if (exif_data[2] == GL_EXIF_TAG &&
	           exif_data[3] == GL_EXIF_APP0) {
		gl_dbgW("Jfif in APP0!");
		int ret = __gl_exif_rw_jfif(fd, file_path, orient, b_write);
		return ret;
	} else {
		gl_dbgE("Not a Exif in APP1 or Jiff in APP2[%d]!", exif_data[3]);
		goto GL_EXIF_FAILED;
	}

	/* Get the marker parameter length count */
	if (__gl_exif_read_2_bytes(fd, &length) < 0) {
		goto GL_EXIF_FAILED;
	}
	gl_dbg("length: %d", length);
	/* Length includes itself, so must be at least 2
	    Following Exif data length must be at least 6 */
	if (length < 8) {
		gl_dbgE("length < 8");
		goto GL_EXIF_FAILED;
	}
	length -= 8;

	/* Length of an IFD entry */
	if (length < 12) {
		gl_dbgE("length < 12");
		goto GL_EXIF_FAILED;
	}

	/* Read Exif head, check for "Exif" */
	for (i = 0; i < 6; i++) {
		tmp_exif = __gl_exif_read_1_byte(fd);
		if (tmp_exif < 0) {
			goto GL_EXIF_FAILED;
		}

		exif_data[i] = (unsigned char)tmp_exif;
	}

	if (exif_data[0] != 0x45 || exif_data[1] != 0x78 ||
	        exif_data[2] != 0x69 || exif_data[3] != 0x66 ||
	        exif_data[4] != 0x00 || exif_data[5] != 0x00) {
		gl_dbgE("Not met Exif!");
		for (i = 0; i < 6; i++) {
			gl_dbg("- %02X", exif_data[i]);
		}
		goto GL_EXIF_FAILED;
	}

	/* Read Exif body */
	for (i = 0; i < length; i++) {
		tmp_exif = __gl_exif_read_1_byte(fd);
		if (tmp_exif < 0) {
			goto GL_EXIF_FAILED;
		}
		exif_data[i] = (unsigned char)tmp_exif;
	}

	/* Check byte order and Tag Mark , "II(0x4949)" or "MM(0x4d4d)" */
	if (exif_data[0] == 0x49 && exif_data[1] == 0x49 &&
	        exif_data[2] == 0x2A && exif_data[3] == 0x00) {
		gl_dbg("Intel");
		is_motorola = false;
	} else if (exif_data[0] == 0x4D && exif_data[1] == 0x4D &&
	           exif_data[2] == 0x00 && exif_data[3] == 0x2A) {
		gl_dbg("Motorola");
		is_motorola = true;
	} else {
		goto GL_EXIF_FAILED;
	}

	/* Get first IFD offset (offset to IFD0) , MM-00000008, II-08000000 */
	if (is_motorola) {
		if (exif_data[4] != 0 && exif_data[5] != 0) {
			goto GL_EXIF_FAILED;
		}
		offset = exif_data[6];
		offset <<= 8;
		offset += exif_data[7];
	} else {
		if (exif_data[7] != 0 && exif_data[6] != 0) {
			goto GL_EXIF_FAILED;
		}
		offset = exif_data[5];
		offset <<= 8;
		offset += exif_data[4];
	}
	/* check end of data segment */
	if (offset > length - 2) {
		gl_dbgE("offset > length - 2");
		goto GL_EXIF_FAILED;
	}

	/* IFD: Image File Directory */
	/* Get the number of directory entries contained in this IFD, - EEEE */
	if (is_motorola) {
		tags_cnt = exif_data[offset];
		tags_cnt <<= 8;
		tags_cnt += exif_data[offset + 1];
	} else {
		tags_cnt = exif_data[offset + 1];
		tags_cnt <<= 8;
		tags_cnt += exif_data[offset];
	}
	if (tags_cnt == 0) {
		gl_dbgE("tags_cnt == 0 - 2");
		goto GL_EXIF_FAILED;
	}
	offset += 2;

	/* check end of data segment */
	if (offset > length - 12) {
		gl_dbgE("offset > length - 12");
		goto GL_EXIF_FAILED;
	}

	/* Search for Orientation Tag in IFD0 */
	while (1) {
		/* Get Tag number */
		if (is_motorola) {
			tag_num = exif_data[offset];
			tag_num <<= 8;
			tag_num += exif_data[offset + 1];
		} else {
			tag_num = exif_data[offset + 1];
			tag_num <<= 8;
			tag_num += exif_data[offset];
		}
		/* found Orientation Tag */
		if (tag_num == 0x0112) {
			gl_dbgW("Found orientation tag!");
			break;
		}
		if (--tags_cnt == 0) {
			gl_dbgW("tags_cnt == 0, no found orientation tag!");
			if (b_write) {
				gl_dbgW("to add an orientation tag!");
				fclose(fd);
				fd = NULL;
				return __gl_exif_add_orientation_tag(file_path, orient);

			} else {
				/* Normal orientation = 0degree = 1 */
				*orient = 1;
				ret = 0;
			}
			goto GL_EXIF_FAILED;
		}

		/* Every directory entry size is 12 */
		offset += 12;
	}

	if (b_write) {
		gl_dbg("Write: %d", *orient);
		/* Set the Orientation value */
		if (is_motorola) {
			exif_data[offset + 9] = (unsigned char)(*orient);
		} else {
			exif_data[offset + 8] = (unsigned char)(*orient);
		}

		if (fseek(fd, jfif_offset + (4 + 2 + 6 + 2) + offset, SEEK_SET) < 0) {
			gl_dbgE("fseek failed!");
			goto GL_EXIF_FAILED;
		}
		fwrite(exif_data + 2 + offset, 1, 10, fd);
	} else {
		/* Get the Orientation value */
		if (is_motorola) {
			if (exif_data[offset + 8] != 0) {
				gl_dbgE("exif_data[offset+8] != 0");
				goto GL_EXIF_FAILED;
			}
			*orient = (unsigned int)exif_data[offset + 9];
		} else {
			if (exif_data[offset + 9] != 0) {
				gl_dbgE("exif_data[offset+9] != 0");
				goto GL_EXIF_FAILED;
			}
			*orient = (unsigned int)exif_data[offset + 8];
		}
		if (*orient > 8) {
			gl_dbgE("*orient > 8");
			goto GL_EXIF_FAILED;
		}
		gl_dbg("Read: %d", *orient);
	}

	ret = 0;

GL_EXIF_FAILED:

	fclose(fd);
	gl_dbg("All done");
	return ret;
}

int _gl_exif_get_orientation(char *file_path, unsigned int *orientation)
{
	GL_CHECK_VAL(orientation, -1);
	GL_CHECK_VAL(file_path, -1);
	gl_sdbg("file_path: %s", file_path);

	return __gl_exif_rw_orient(file_path, orientation, false);
}

int _gl_exif_set_orientation(char *file_path, unsigned int orientation)
{
	GL_CHECK_VAL(file_path, -1);
	gl_sdbg("file_path: %s", file_path);

	return __gl_exif_rw_orient(file_path, &orientation, true);
}

int _gl_exif_get_rotated_orientation(unsigned int orientation, bool b_left)
{
	int rotated_orientataion;
	gl_dbg("b_left: %d", b_left);

	switch (orientation) {
	case GL_ORIENTATION_ROT_0:
		/* true: 0 -> 270, false: 0 -> 90 */
		if (b_left) {
			rotated_orientataion = GL_ORIENTATION_ROT_270;
		} else {
			rotated_orientataion = GL_ORIENTATION_ROT_90;
		}
		break;
	case GL_ORIENTATION_ROT_90:
		/* true: 90 -> 0, false: 90 -> 180 */
		if (b_left) {
			rotated_orientataion = GL_ORIENTATION_ROT_0;
		} else {
			rotated_orientataion = GL_ORIENTATION_ROT_180;
		}
		break;
	case GL_ORIENTATION_ROT_180:
		/* true: 180 -> 90, false: 180 -> 270 */
		if (b_left) {
			rotated_orientataion = GL_ORIENTATION_ROT_90;
		} else {
			rotated_orientataion = GL_ORIENTATION_ROT_270;
		}
		break;
	case GL_ORIENTATION_ROT_270:
		/* true: 270 -> 180, false: 270 -> 0 */
		if (b_left) {
			rotated_orientataion = GL_ORIENTATION_ROT_180;
		} else {
			rotated_orientataion = GL_ORIENTATION_ROT_0;
		}
		break;
	default:
		gl_dbgE("Wrong oriectation: %d!", orientation);
		/* true: 0 -> 270, false: 0 -> 90 */
		if (b_left) {
			rotated_orientataion = GL_ORIENTATION_ROT_270;
		} else {
			rotated_orientataion = GL_ORIENTATION_ROT_90;
		}
		break;
	}

	gl_dbg("New orientation: %d", rotated_orientataion);
	return rotated_orientataion;
}

#endif

