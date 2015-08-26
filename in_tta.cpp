// in_tta.cpp : Defines the initialization routines for the DLL.
//
/* Description:	 TTA input plug-in for upper Winamp 2.91
 *               with MediaLibrary Extension version
 * Developed by: Alexander Djourik <ald@true-audio.com>
 *               Pavel Zhilin <pzh@true-audio.com>
 *               (MediaLibrary Extension Yamagata Fumihiro <bunbun042000@gmail.com> )
 *
 * Copyright (c) 2005 Alexander Djourik. All rights reserved.
 *
 */

/*
The ttaplugin-winamp project.
Copyright (C) 2005-2015 Yamagata Fumihiro

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/


#include <Shlwapi.h>
#include <stdlib.h>
#include <type_traits>
#include <windows.h>

#include "AlbumArt.h"
//#include "DecodeFile.h"
#include "MediaLibrary.h"
#include "in_tta.h"

#include <Winamp/in2.h>
#include <Agave/Language/api_language.h>

#include <taglib/tag.h>
#include <taglib/trueaudiofile.h>
#include <taglib/tstring.h>

#include "..\common\VersionNo.h"
#include "resource.h"


// For Support Transcoder input (2007/10/15)
CMediaLibrary m_ReadTag;
CMediaLibrary m_WriteTag;

static const __int32 PLAYING_BUFFER_SIZE = PLAYING_BUFFER_LENGTH * MAX_DEPTH * MAX_NCH;
static const __int32 TRANSCODING_BUFFER_SIZE = TRANSCODING_BUFFER_LENGTH * MAX_DEPTH * MAX_NCH;

static int paused = 0;
static int seek_needed = -1;
static unsigned int decode_pos_ms = 0;

static BYTE pcm_buffer[PLAYING_BUFFER_SIZE];	// PCM buffer
static short vis_buffer[PLAYING_BUFFER_SIZE*MAX_NCH];	// vis buffer

static decoder_TTA player;
static decoder_TTA transcoder;

static HANDLE decoder_handle = INVALID_HANDLE_VALUE;
static DWORD WINAPI __stdcall DecoderThread (void *p);
static volatile int killDecoderThread = 0;

void config(HWND hwndParent);
void about(HWND hwndParent);
void init();
void quit();
void getfileinfo(const wchar_t *file, wchar_t *title, int *length_in_ms);
int  infodlg(const wchar_t *file, HWND hwndParent);
int  isourfile(const wchar_t *fn);
int  play(const wchar_t *fn);
void pause();
void unpause();
int  ispaused();
void stop();
int  getlength();
int  getoutputtime();
void setoutputtime(int time_in_ms);
void setvolume(int volume);
void setpan(int pan);
void eq_set(int on, char data[10], int preamp);

static void skip_id3v2_tag(TTAinfo *TTAInfo);

//void SetPlayingTitle(const char *filename, char *title);

In_Module mod = {
	IN_VER,
	"TTA Audio Decoder " PLUGIN_VERSION,
	NULL,	// hMainWindow
	NULL,	// hDllInstance
	"TTA\0TTA Audio File (*.TTA)\0",
	1,	// is_seekable
	1,	// uses output
	config,
	about,
	init,
	quit,
	getfileinfo,
	infodlg,
	isourfile,
	play,
	pause,
	unpause,
	ispaused,
	stop,
	getlength,
	getoutputtime,
	setoutputtime,
	setvolume,
	setpan,
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, // vis stuff
	NULL, NULL,	// dsp
	eq_set,
	NULL,	// setinfo
	NULL		// out_mod
};

typedef struct _buffer {
	long		data_length;
	BYTE	   *buffer;
} data_buf;

static data_buf remain_data; // for transcoding (buffer for data that is decoded but is not copied)

///////////////////////////////////////////////////////////////////////
// Description:	 TTAv1 lossless audio decoder.                       //
// Copyright (c) 1999-2009 Aleksander Djuric. All rights reserved.   //
///////////////////////////////////////////////////////////////////////

///////////////////// constants and definitions ///////////////////////

#define PREDICTOR1(x, k)	((int)((((__int64)x << k) - x) >> k))
#define DEC(x)			(((x)&1)?(++(x)>>1):(-(x)>>1))
#define WRITE_BUFFER2(x, y) { *x-- = (BYTE) (*y >> 8); *x-- = (BYTE) *y; }
#define WRITE_BUFFER3(x, y) { *x-- = (BYTE) (*y >> 16); *x-- = (BYTE) (*y >> 8); *x-- = (BYTE) *y; }

const unsigned int bit_mask[] = {
	0x00000000, 0x00000001, 0x00000003, 0x00000007,
	0x0000000f, 0x0000001f, 0x0000003f, 0x0000007f,
	0x000000ff, 0x000001ff, 0x000003ff, 0x000007ff,
	0x00000fff, 0x00001fff, 0x00003fff, 0x00007fff,
	0x0000ffff, 0x0001ffff, 0x0003ffff, 0x0007ffff,
	0x000fffff, 0x001fffff, 0x003fffff, 0x007fffff,
	0x00ffffff, 0x01ffffff, 0x03ffffff, 0x07ffffff,
	0x0fffffff, 0x1fffffff, 0x3fffffff, 0x7fffffff,
	0xffffffff
};

const unsigned int bit_shift[] = {
	0x00000001, 0x00000002, 0x00000004, 0x00000008,
	0x00000010, 0x00000020, 0x00000040, 0x00000080,
	0x00000100, 0x00000200, 0x00000400, 0x00000800,
	0x00001000, 0x00002000, 0x00004000, 0x00008000,
	0x00010000, 0x00020000, 0x00040000, 0x00080000,
	0x00100000, 0x00200000, 0x00400000, 0x00800000,
	0x01000000, 0x02000000, 0x04000000, 0x08000000,
	0x10000000, 0x20000000, 0x40000000, 0x80000000,
	0x80000000, 0x80000000, 0x80000000, 0x80000000,
	0x80000000, 0x80000000, 0x80000000, 0x80000000
};

const unsigned int *shift_16 = bit_shift + 4;

const unsigned int crc32_table[256] = {
	0x00000000, 0x77073096, 0xee0e612c, 0x990951ba,
	0x076dc419, 0x706af48f, 0xe963a535, 0x9e6495a3,
	0x0edb8832, 0x79dcb8a4, 0xe0d5e91e, 0x97d2d988,
	0x09b64c2b, 0x7eb17cbd, 0xe7b82d07, 0x90bf1d91,
	0x1db71064, 0x6ab020f2, 0xf3b97148, 0x84be41de,
	0x1adad47d, 0x6ddde4eb, 0xf4d4b551, 0x83d385c7,
	0x136c9856, 0x646ba8c0, 0xfd62f97a, 0x8a65c9ec,
	0x14015c4f, 0x63066cd9, 0xfa0f3d63, 0x8d080df5,
	0x3b6e20c8, 0x4c69105e, 0xd56041e4, 0xa2677172,
	0x3c03e4d1, 0x4b04d447, 0xd20d85fd, 0xa50ab56b,
	0x35b5a8fa, 0x42b2986c, 0xdbbbc9d6, 0xacbcf940,
	0x32d86ce3, 0x45df5c75, 0xdcd60dcf, 0xabd13d59,
	0x26d930ac, 0x51de003a, 0xc8d75180, 0xbfd06116,
	0x21b4f4b5, 0x56b3c423, 0xcfba9599, 0xb8bda50f,
	0x2802b89e, 0x5f058808, 0xc60cd9b2, 0xb10be924,
	0x2f6f7c87, 0x58684c11, 0xc1611dab, 0xb6662d3d,
	0x76dc4190, 0x01db7106, 0x98d220bc, 0xefd5102a,
	0x71b18589, 0x06b6b51f, 0x9fbfe4a5, 0xe8b8d433,
	0x7807c9a2, 0x0f00f934, 0x9609a88e, 0xe10e9818,
	0x7f6a0dbb, 0x086d3d2d, 0x91646c97, 0xe6635c01,
	0x6b6b51f4, 0x1c6c6162, 0x856530d8, 0xf262004e,
	0x6c0695ed, 0x1b01a57b, 0x8208f4c1, 0xf50fc457,
	0x65b0d9c6, 0x12b7e950, 0x8bbeb8ea, 0xfcb9887c,
	0x62dd1ddf, 0x15da2d49, 0x8cd37cf3, 0xfbd44c65,
	0x4db26158, 0x3ab551ce, 0xa3bc0074, 0xd4bb30e2,
	0x4adfa541, 0x3dd895d7, 0xa4d1c46d, 0xd3d6f4fb,
	0x4369e96a, 0x346ed9fc, 0xad678846, 0xda60b8d0,
	0x44042d73, 0x33031de5, 0xaa0a4c5f, 0xdd0d7cc9,
	0x5005713c, 0x270241aa, 0xbe0b1010, 0xc90c2086,
	0x5768b525, 0x206f85b3, 0xb966d409, 0xce61e49f,
	0x5edef90e, 0x29d9c998, 0xb0d09822, 0xc7d7a8b4,
	0x59b33d17, 0x2eb40d81, 0xb7bd5c3b, 0xc0ba6cad,
	0xedb88320, 0x9abfb3b6, 0x03b6e20c, 0x74b1d29a,
	0xead54739, 0x9dd277af, 0x04db2615, 0x73dc1683,
	0xe3630b12, 0x94643b84, 0x0d6d6a3e, 0x7a6a5aa8,
	0xe40ecf0b, 0x9309ff9d, 0x0a00ae27, 0x7d079eb1,
	0xf00f9344, 0x8708a3d2, 0x1e01f268, 0x6906c2fe,
	0xf762575d, 0x806567cb, 0x196c3671, 0x6e6b06e7,
	0xfed41b76, 0x89d32be0, 0x10da7a5a, 0x67dd4acc,
	0xf9b9df6f, 0x8ebeeff9, 0x17b7be43, 0x60b08ed5,
	0xd6d6a3e8, 0xa1d1937e, 0x38d8c2c4, 0x4fdff252,
	0xd1bb67f1, 0xa6bc5767, 0x3fb506dd, 0x48b2364b,
	0xd80d2bda, 0xaf0a1b4c, 0x36034af6, 0x41047a60,
	0xdf60efc3, 0xa867df55, 0x316e8eef, 0x4669be79,
	0xcb61b38c, 0xbc66831a, 0x256fd2a0, 0x5268e236,
	0xcc0c7795, 0xbb0b4703, 0x220216b9, 0x5505262f,
	0xc5ba3bbe, 0xb2bd0b28, 0x2bb45a92, 0x5cb36a04,
	0xc2d7ffa7, 0xb5d0cf31, 0x2cd99e8b, 0x5bdeae1d,
	0x9b64c2b0, 0xec63f226, 0x756aa39c, 0x026d930a,
	0x9c0906a9, 0xeb0e363f, 0x72076785, 0x05005713,
	0x95bf4a82, 0xe2b87a14, 0x7bb12bae, 0x0cb61b38,
	0x92d28e9b, 0xe5d5be0d, 0x7cdcefb7, 0x0bdbdf21,
	0x86d3d2d4, 0xf1d4e242, 0x68ddb3f8, 0x1fda836e,
	0x81be16cd, 0xf6b9265b, 0x6fb077e1, 0x18b74777,
	0x88085ae6, 0xff0f6a70, 0x66063bca, 0x11010b5c,
	0x8f659eff, 0xf862ae69, 0x616bffd3, 0x166ccf45,
	0xa00ae278, 0xd70dd2ee, 0x4e048354, 0x3903b3c2,
	0xa7672661, 0xd06016f7, 0x4969474d, 0x3e6e77db,
	0xaed16a4a, 0xd9d65adc, 0x40df0b66, 0x37d83bf0,
	0xa9bcae53, 0xdebb9ec5, 0x47b2cf7f, 0x30b5ffe9,
	0xbdbdf21c, 0xcabac28a, 0x53b39330, 0x24b4a3a6,
	0xbad03605, 0xcdd70693, 0x54de5729, 0x23d967bf,
	0xb3667a2e, 0xc4614ab8, 0x5d681b02, 0x2a6f2b94,
	0xb40bbe37, 0xc30c8ea1, 0x5a05df1b, 0x2d02ef8d
};
///////////////////////// crc32 functions /////////////////////////////

#define UPDATE_CRC32(x, crc) crc = ((crc>>8) ^ crc32_table[(crc^x) & 0xFF])

static unsigned int
crc32(BYTE *buffer, unsigned int len) {
	unsigned long i;
	unsigned long crc = 0xFFFFFFFF;

	for (i = 0; i < len; i++) UPDATE_CRC32(buffer[i], crc);

	return (crc ^ 0xFFFFFFFF);
}

static int read_fileinfo(TTAinfo *TTAInfo) {
	TTAhdr TTAHdr;
	unsigned int checksum, datasize, origsize;
	DWORD result;

	// get file size
	TTAInfo->FileSize = GetFileSize(TTAInfo->hFile, NULL);

	// skip metadata
	skip_id3v2_tag(TTAInfo);

	// read TTA header
	if (!ReadFile(TTAInfo->hFile, &TTAHdr, sizeof(TTAhdr), &result, NULL) ||
		result != sizeof(TTAhdr)) {
		CloseHandle(TTAInfo->hFile);
		TTAInfo->State = TTA_READ_ERROR;
		return -1;
	}

	// check for TTA signature
	if (TTAHdr.TTAid != TTA1_SIGN) {
		CloseHandle(TTAInfo->hFile);
		TTAInfo->State = TTA_FORMAT_ERROR;
		return -1;
	}

	checksum = crc32((BYTE *)&TTAHdr, sizeof(TTAhdr) - 4);
	if (checksum != TTAHdr.CRC32) {
		CloseHandle(TTAInfo->hFile);
		TTAInfo->State = TTA_FILE_ERROR;
		return -1;
	}

	// check for player supported formats
	if (TTAHdr.AudioFormat != WAVE_FORMAT_PCM ||
		TTAHdr.BitsPerSample < MIN_BPS ||
		TTAHdr.BitsPerSample > MAX_BPS ||
		TTAHdr.NumChannels > MAX_NCH) {
		CloseHandle(TTAInfo->hFile);
		TTAInfo->State = TTA_FILE_ERROR;
		return -1;
	}

	// fill the File Info
	TTAInfo->Nch = TTAHdr.NumChannels;
	TTAInfo->Bps = TTAHdr.BitsPerSample;
	TTAInfo->BSize = (TTAHdr.BitsPerSample + 7) / 8;
	TTAInfo->SampleRate = TTAHdr.SampleRate;
	TTAInfo->DataLength = TTAHdr.DataLength;
	TTAInfo->Length = TTAHdr.DataLength / TTAHdr.SampleRate * 1000;

	datasize = TTAInfo->FileSize - TTAInfo->Offset;
	origsize = TTAInfo->DataLength * TTAInfo->BSize * TTAInfo->Nch;

	TTAInfo->Compress = (float)datasize / origsize;
	TTAInfo->BitRate = (int)((TTAInfo->Compress *
		TTAInfo->SampleRate * TTAInfo->Nch * TTAInfo->Bps) / 1000);

	return 0;
}

static int open_TTA_file(const wchar_t *filename, TTAinfo *TTA_Info) {
	ZeroMemory(TTA_Info, sizeof(TTAinfo));

	lstrcpyn(TTA_Info->FName, filename, MAX_PATH - 1);
	TTA_Info->hFile = CreateFile(filename, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE,
		NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (TTA_Info->hFile == INVALID_HANDLE_VALUE) {
		TTA_Info->State = TTA_OPEN_ERROR;
		return -1;
	}

	return read_fileinfo(TTA_Info);
}

//////////////////////// TTA hybrid filter ////////////////////////////

///////// Filter Settings //////////
static int flt_set[3] = { 10, 9, 10 };

__inline void memshl(int *pA, int *pB) {
	*pA++ = *pB++;
	*pA++ = *pB++;
	*pA++ = *pB++;
	*pA++ = *pB++;
	*pA++ = *pB++;
	*pA++ = *pB++;
	*pA++ = *pB++;
	*pA = *pB;
}

__inline void hybrid_filter(TTAfltst *fs, int *in) {
	int *pA = fs->dl;
	int *pB = fs->qm;
	int *pM = fs->dx;
	int sum = fs->round;

	if (!fs->error) {
		sum += *pA++ * *pB, pB++;
		sum += *pA++ * *pB, pB++;
		sum += *pA++ * *pB, pB++;
		sum += *pA++ * *pB, pB++;
		sum += *pA++ * *pB, pB++;
		sum += *pA++ * *pB, pB++;
		sum += *pA++ * *pB, pB++;
		sum += *pA++ * *pB, pB++; pM += 8;
	}
	else if (fs->error < 0) {
		sum += *pA++ * (*pB -= *pM++), pB++;
		sum += *pA++ * (*pB -= *pM++), pB++;
		sum += *pA++ * (*pB -= *pM++), pB++;
		sum += *pA++ * (*pB -= *pM++), pB++;
		sum += *pA++ * (*pB -= *pM++), pB++;
		sum += *pA++ * (*pB -= *pM++), pB++;
		sum += *pA++ * (*pB -= *pM++), pB++;
		sum += *pA++ * (*pB -= *pM++), pB++;
	}
	else {
		sum += *pA++ * (*pB += *pM++), pB++;
		sum += *pA++ * (*pB += *pM++), pB++;
		sum += *pA++ * (*pB += *pM++), pB++;
		sum += *pA++ * (*pB += *pM++), pB++;
		sum += *pA++ * (*pB += *pM++), pB++;
		sum += *pA++ * (*pB += *pM++), pB++;
		sum += *pA++ * (*pB += *pM++), pB++;
		sum += *pA++ * (*pB += *pM++), pB++;
	}

	*(pM - 0) = ((*(pA - 1) >> 30) | 1) << 2;
	*(pM - 1) = ((*(pA - 2) >> 30) | 1) << 1;
	*(pM - 2) = ((*(pA - 3) >> 30) | 1) << 1;
	*(pM - 3) = ((*(pA - 4) >> 30) | 1);

	fs->error = *in;
	*in += (sum >> fs->shift);
	*pA = *in;

	*(pA - 1) = *(pA - 0) - *(pA - 1);
	*(pA - 2) = *(pA - 1) - *(pA - 2);
	*(pA - 3) = *(pA - 2) - *(pA - 3);

	memshl(fs->dl, fs->dl + 1);
	memshl(fs->dx, fs->dx + 1);
}

static void filter_init(TTAfltst *fs, int shift) {
	ZeroMemory(fs, sizeof(TTAfltst));
	fs->shift = shift;
	fs->round = 1 << (shift - 1);
}

//////////////////////// decoder functions ////////////////////////////

static void rice_init(TTAadapt *rice, unsigned int k0, unsigned int k1) {
	rice->k0 = k0;
	rice->k1 = k1;
	rice->sum0 = shift_16[k0];
	rice->sum1 = shift_16[k1];
}

__inline void reader_init(TTA_reader *Reader) { Reader->Pos = &Reader->End; }

__inline BYTE reader_read_byte(TTA_reader *Reader, TTAinfo *tta_info) {
	DWORD result;

	if (Reader->Pos == &Reader->End) {
		if (!ReadFile(tta_info->hFile, Reader->Buffer,
			READER_BUFFER_SIZE, &result, NULL) || !result) return 0;
		Reader->Pos = Reader->Buffer;
	}

	return *Reader->Pos++;
}

__inline unsigned int reader_read_crc32(TTA_reader *Reader, TTAinfo *tta_info) {
	unsigned int data_size;
	DWORD result;

	data_size = &Reader->End - Reader->Pos;
	if (data_size < 4) {
		memcpy(Reader->Buffer, Reader->Pos, data_size);
		if (!ReadFile(tta_info->hFile, &Reader->Buffer[data_size],
			READER_BUFFER_SIZE - data_size, &result, NULL) || !result) return 0;
		Reader->Pos = Reader->Buffer;
	}

	memcpy(&result, Reader->Pos, 4);
	Reader->Pos += 4;

	return result;
}

void decoder_init(TTAcodec *Current, TTAinfo tta_info) {
	unsigned int i;

	if (Current->FrameNum == Current->FrameTotal - 1)
		Current->FrameLen = Current->FrameLenL;
	else Current->FrameLen = Current->FrameLenD;

	// init entropy decoder
	for (i = 0; i < tta_info.Nch; i++) {
		filter_init(&Current->TTA[i].fst, flt_set[tta_info.BSize - 1]);
		rice_init(&Current->TTA[i].rice, 10, 10);
		Current->TTA[i].last = 0;
	}

	Current->FrameCRC = 0xFFFFFFFFUL;
	Current->FramePos = 0;
	Current->BitCount = 0;
	Current->BitCache = 0;
}

int set_position(int move_needed, TTA_reader *Reader, TTAcodec *Current, TTAinfo *tta_info) {
	unsigned int result;

	if (Current->FrameNum >= Current->FrameTotal) return 0;

	if (move_needed && tta_info->Seekable) {
		result = SetFilePointer(tta_info->hFile, Current->SeekTable[Current->FrameNum], NULL, FILE_BEGIN);
		if (result == INVALID_SET_FILE_POINTER && GetLastError() != NO_ERROR) {
			tta_info->State = TTA_READ_ERROR;
			return -1;
		}
		reader_init(Reader);
	}

	decoder_init(Current, *tta_info);

	return 0;
}


int decoder_new(HANDLE *heap, TTAinfo *tta_info, TTAcodec *Current, TTA_reader *Reader)
{
	unsigned int crc, data_offset, size;
	unsigned int i;
	DWORD result;

	// check for buffers is free
//	if (Current->SeekTable) stop();

	Current->FrameLenD = (int)(FRAME_TIME * tta_info->SampleRate);
	Current->FrameLenL = tta_info->DataLength % Current->FrameLenD;
	if (!Current->FrameLenL) Current->FrameLenL = Current->FrameLenD;
	Current->FrameTotal = tta_info->DataLength / Current->FrameLenD + (Current->FrameLenL ? 1 : 0);

	// allocate memory for seek table
	size = (Current->FrameTotal * 4) + 4;
	Current->SeekTable = (unsigned int *)HeapAlloc(*heap, 0, size);
	if (!Current->SeekTable) {
		tta_info->State = TTA_MEMORY_ERROR;
		return -1;
	}

	// read seek table
	if (!ReadFile(tta_info->hFile, Current->SeekTable, size, &result, NULL) ||
		result != size) {
		tta_info->State = TTA_READ_ERROR;
		return -1;
	}

	crc = crc32((BYTE *)Current->SeekTable, size - 4);
	tta_info->Seekable = (crc == Current->SeekTable[Current->FrameTotal]);

	data_offset = tta_info->Offset + sizeof(TTAhdr) + size;
	for (i = 0; i < Current->FrameTotal; i++) {
		size = Current->SeekTable[i];
		Current->SeekTable[i] = data_offset;
		data_offset += size;
	}

	Current->FrameNum = 0;

	reader_init(Reader);
	set_position(false, Reader, Current, tta_info);

	return 0;
}

int decode_to_pcmbuffer(BYTE *output, TTAinfo *tta_info, TTAcodec *Current, TTA_reader *Reader, int count) {
	TTA_channel_codec *dec = Current->TTA;
	TTAfltst *fst;
	TTAadapt *rice;
	BYTE *p = output;
	BYTE *w;
	int cache[MAX_NCH];
	int *s = cache;
	int *r, *last;
	unsigned int k, depth, unary, binary;
	unsigned int sample_size, buffer_size;
	int tmp, value, crc_flag, ret;

	sample_size = tta_info->BSize * tta_info->Nch;
	buffer_size = count * sample_size;

	for (ret = 0; p < output + buffer_size;) {
		fst = &dec->fst;
		rice = &dec->rice;
		last = &dec->last;
		unary = 0;

		if (Current->FramePos == Current->FrameLen) {
			if (Current->FrameNum == Current->FrameTotal) break;

			// check frame crc
			Current->FrameCRC ^= 0xFFFFFFFFUL;
			crc_flag = (Current->FrameCRC != reader_read_crc32(Reader, tta_info));
			Current->FrameNum++;

			if (crc_flag) ZeroMemory(output, buffer_size);
			if (set_position(crc_flag, Reader, Current, tta_info) < 0) break;
		}

		// decode Rice unsigned
		while (!(Current->BitCache ^ bit_mask[Current->BitCount])) {
			unary += Current->BitCount;
			Current->BitCache = reader_read_byte(Reader, tta_info);
			UPDATE_CRC32(Current->BitCache, Current->FrameCRC);
			Current->BitCount = 8;
		}

		while (Current->BitCache & 1) {
			unary++;
			Current->BitCache >>= 1;
			Current->BitCount--;
		}
		Current->BitCache >>= 1;
		Current->BitCount--;

		switch (unary) {
		case 0: depth = 0; k = rice->k0; break;
		default:
			depth = 1; k = rice->k1;
			unary--;
		}

		if (k) {
			while (Current->BitCount < k) {
				tmp = reader_read_byte(Reader, tta_info);
				UPDATE_CRC32(tmp, Current->FrameCRC);
				Current->BitCache |= tmp << Current->BitCount;
				Current->BitCount += 8;
			}
			binary = Current->BitCache & bit_mask[k];
			Current->BitCache >>= k;
			Current->BitCount -= k;
			Current->BitCache &= bit_mask[Current->BitCount];
			value = (unary << k) + binary;
		}
		else value = unary;

		switch (depth) {
		case 1:
			rice->sum1 += value - (rice->sum1 >> 4);
			if (rice->k1 > 0 && rice->sum1 < shift_16[rice->k1])
				rice->k1--;
			else if (rice->sum1 > shift_16[rice->k1 + 1])
				rice->k1++;
			value += bit_shift[rice->k0];
		default:
			rice->sum0 += value - (rice->sum0 >> 4);
			if (rice->k0 > 0 && rice->sum0 < shift_16[rice->k0])
				rice->k0--;
			else if (rice->sum0 > shift_16[rice->k0 + 1])
				rice->k0++;
		}

		value = DEC(value);

		// decompress stage 1: adaptive hybrid filter
		hybrid_filter(fst, &value);

		// decompress stage 2: fixed order 1 prediction
		value += PREDICTOR1(*last, 5); *last = value;

		if (dec < Current->TTA + tta_info->Nch - 1) {
			*s++ = value;
			dec++;
		}
		else {
			*s = value;
			if (tta_info->Nch == 1) {
				if (tta_info->BSize == 2) {
					WRITE_BUFFER2(p, s);
				}
				else { // bsize == 3
					WRITE_BUFFER3(p, s);
				}
			}
			else {
				r = s - 1;

				p += sample_size;
				w = p - 1;

				if (tta_info->BSize == 2) {
					*s += *r / 2; WRITE_BUFFER2(w, s);
					while (r > cache) {
						*r = *s-- - *r;	WRITE_BUFFER2(w, r); *r--;
					}
					*r = *s - *r; WRITE_BUFFER2(w, r);
				}
				else { // bsize == 3
					*s += *r / 2; WRITE_BUFFER3(w, s);
					while (r > cache) {
						*r = *s-- - *r;	WRITE_BUFFER3(w, r); *r--;
					}
					*r = *s - *r; WRITE_BUFFER3(w, r);
				}
			}

			s = cache;
			Current->FramePos++; ret++;
			dec = Current->TTA;
		}
	}

	return ret;
}

static int get_info_data(TTAinfo *fInfo, wchar_t *title) {
//	wchar_t temp[MAX_PATH];

	_wsplitpath_s(fInfo->FName, NULL, 0, NULL, 0, title, MAX_PATH, NULL, 0);

	return (fInfo->Length);
}

static void tta_error_message(int error, const wchar_t *filename)
{
	wchar_t message[1024];

	std::wstring name(filename);

	switch (error) {
		case TTA_OPEN_ERROR:	
			wsprintf(message, L"Can't open file:\n%ls", name.c_str());
			break;
		case TTA_FORMAT_ERROR:
			wsprintf(message, L"Unknown TTA format version:\n%ls", name.c_str());
			break;
		case TTA_NOT_SUPPORTED:
			wsprintf(message, L"Not supported file format:\n%ls", name.c_str());
			break;
		case TTA_FILE_ERROR:
			wsprintf(message, L"File is corrupted:\n%ls", name.c_str());
			break;
		case TTA_READ_ERROR:
			wsprintf(message, L"Can't read from file:\n%ls", name.c_str());
			break;
		case TTA_WRITE_ERROR:
			wsprintf(message, L"Can't write to file:\n%ls", name.c_str());
			break;
		case TTA_MEMORY_ERROR:	
			wsprintf(message, L"Insufficient memory available");
			break;
		case TTA_SEEK_ERROR:
			wsprintf(message, L"file seek error");
			break;
		case TTA_PASSWORD_ERROR:
			wsprintf(message, L"password protected file");
			break;
		default:
			wsprintf(message, L"Unknown TTA decoder error");
			break;
	}

	MessageBox(mod.hMainWindow, message, L"TTA Decoder Error",
		MB_OK|MB_ICONERROR|MB_SYSTEMMODAL);

}

static BOOL CALLBACK config_dialog(HWND dialog, UINT message,
	WPARAM wparam, LPARAM lparam)
{

	switch (message) {
	case WM_INITDIALOG:
		return TRUE;
	case WM_COMMAND:
		switch (LOWORD(wparam)) {
		case IDOK:
		case IDCANCEL:
			EndDialog(dialog, wparam);
			return TRUE;
		}
		break;
	}
	return FALSE;
}
static BOOL CALLBACK about_dialog(HWND dialog, UINT message,
	WPARAM wparam, LPARAM lparam)
{
	switch (message) {
	case WM_INITDIALOG:
		SetDlgItemText(dialog, IDC_PLUGIN_VERSION,
			L"Winamp plug-in version " PLUGIN_VERSION "\n" PROJECT_URL);
		SetDlgItemText(dialog, IDC_PLUGIN_CREADIT,
			ORIGINAL_CREADIT01 ORIGINAL_CREADIT02 ORIGINAL_CREADIT03
			CREADIT01 CREADIT02);
		return TRUE;
	case WM_COMMAND:
		switch (LOWORD(wparam)) {
		case IDOK:
		case IDCANCEL:
			EndDialog(dialog, wparam);
			return TRUE;
		}
		break;
	}
	return FALSE;
}

void config(HWND hwndParent) 
{
	DialogBox(mod.hDllInstance, MAKEINTRESOURCE(IDD_CONFIG),
		hwndParent, config_dialog);
}

void about(HWND hwndParent)
{
	DialogBox(mod.hDllInstance, MAKEINTRESOURCE(IDD_ABOUT),
		hwndParent, about_dialog);
}


void init()
{
	remain_data.data_length = 0;
	remain_data.buffer = NULL;

	player.heap = GetProcessHeap();
	ZeroMemory(&player.info, sizeof(TTAinfo));

	Wasabi_Init();
}

void quit()
{
	remain_data.data_length = 0;

	if (remain_data.buffer != NULL)
	{
		delete [] remain_data.buffer;
		remain_data.buffer = NULL;
	}
	else
	{
		// Do nothing
	}

	Wasabi_Quit();
}


void getfileinfo(const wchar_t *file, wchar_t *title, int *length_in_ms)
{

	if (!file || !*file)
	{
		// invalid filename may be playing file
			*length_in_ms = get_info_data(&player.info, title);
			title = L"";
	} else {
		TagLib::FileName fn(file);
		TagLib::TrueAudio::File f(fn);
		if (f.isValid() == true) {
			*length_in_ms = f.audioProperties()->length() * 1000;
		} else {
			// cannot get fileinfo
			*length_in_ms = 0;
		}
		title = L"";
	}
}

int infodlg(const in_char *filename, HWND parent)
{
	return 0;
}

int isourfile(const in_char *filename)
{
	return 0;
} 

void show_bitrate(int bitrate) {
	mod.SetInfo(bitrate, player.info.SampleRate / 1000, player.info.Nch, 1);
}

int play(const wchar_t *filename)
{
	int maxlatency;
	DWORD decoder_thread_id;

	// check for required data presented
	if (!filename || !*filename) return -1;

	// open TTA file
	if (open_TTA_file(filename, &player.info) < 0) {
		tta_error_message(player.info.State, filename);
		return 1;
	}

	if (decoder_new(&player.heap, &player.info, &player.Current, &player.Reader) < 0) {
		tta_error_message(player.info.State, filename);
		return 1;
	}

	paused = 0;
	decode_pos_ms = 0;
	seek_needed = -1;
	mod.is_seekable = player.info.Seekable;

	maxlatency = mod.outMod->Open(player.info.SampleRate, player.info.Nch, player.info.Bps, -1, -1);
	if (maxlatency < 0) {
		CloseHandle(player.info.hFile);
		player.info.hFile = INVALID_HANDLE_VALUE;
		return 1;
	}

	// setup information display
	show_bitrate(player.info.BitRate);

	// initialize vis stuff
	mod.SAVSAInit(maxlatency, player.info.SampleRate);
	mod.VSASetInfo(player.info.Nch, player.info.SampleRate);

	// set the output plug-ins default volume
	mod.outMod->SetVolume(-666);

	killDecoderThread = 0;

	decoder_handle = CreateThread(NULL, 0, DecoderThread, NULL, 0, &decoder_thread_id);
	if (!decoder_handle) { stop(); return 1; }

	return 0;
}

void pause() 
{
	paused = 1;
	mod.outMod->Pause(1);
}

void unpause() 
{
	paused = 0;
	mod.outMod->Pause(0);
}

int ispaused()
{
	return paused;

}

void stop()
{

	remain_data.data_length = 0;

	if (remain_data.buffer != NULL)
	{
		delete [] remain_data.buffer;
		remain_data.buffer = NULL;
	}
	else
	{
		// Do nothing
	}

	if (INVALID_HANDLE_VALUE != decoder_handle) {
		killDecoderThread = 1;
		WaitForSingleObject(decoder_handle, INFINITE);
		CloseHandle(decoder_handle);
		decoder_handle = INVALID_HANDLE_VALUE;
	}
	else
	{
		// Do nothing
	}


	if (player.info.hFile != INVALID_HANDLE_VALUE) 
	{
		CloseHandle(player.info.hFile);
		player.info.hFile = INVALID_HANDLE_VALUE;
	}
	else
	{
		// Do nothing
	}


	mod.outMod->Close();
	mod.SAVSADeInit();

	if (player.Current.SeekTable) {
		HeapFree(player.heap, 0, player.Current.SeekTable);
		player.Current.SeekTable = NULL;
	}

}
int  getlength() { return player.info.Length; }
int  getoutputtime() { return decode_pos_ms + (mod.outMod->GetOutputTime() - mod.outMod->GetWrittenTime()); }
void setoutputtime(int time_in_ms) { seek_needed = time_in_ms; }
void setvolume(int volume) { mod.outMod->SetVolume(volume); }
void setpan(int pan) { mod.outMod->SetPan(pan); }
void eq_set(int on, char data[10], int preamp) {}

static void do_vis(unsigned char *data, int count, int bps, long double position)
{
	mod.SAAddPCMData(data, player.info.Nch, bps, (int)position);
	mod.VSAAddPCMData(data, player.info.Nch, bps, (int)position);
}

static DWORD WINAPI DecoderThread(void *p) {
	int done = 0;
	int len;

	while (!killDecoderThread) {
		if (seek_needed != -1) {
			if (seek_needed >= player.info.Length) {
				decode_pos_ms = player.info.Length;
				mod.outMod->Flush(decode_pos_ms);
				done = 1;
			}
			else {
				player.Current.FrameNum = seek_needed / SEEK_STEP;
				decode_pos_ms = player.Current.FrameNum * SEEK_STEP;
				seek_needed = -1;
				mod.outMod->Flush(decode_pos_ms);
				player.Current.FramePos = -1;
			}
			if (set_position(true, &player.Reader, &player.Current, &player.info)) return 0;
		}
		if (done) {
			mod.outMod->CanWrite();
			if (!mod.outMod->IsPlaying()) {
				PostMessage(mod.hMainWindow, WM_WA_MPEG_EOF, 0, 0);
				return 0;
			}
			else Sleep(10);
		}
		else if (mod.outMod->CanWrite() >=
			((576 * player.info.Nch * player.info.BSize) << (mod.dsp_isactive() ? 1 : 0))) {
			if (!(len = decode_to_pcmbuffer(pcm_buffer, &player.info, &player.Current, &player.Reader, 576))) done = 1;
			else {
				decode_pos_ms += (len * 1000) / player.info.SampleRate;
				do_vis(pcm_buffer, len, player.info.Bps, decode_pos_ms);
				if (mod.dsp_isactive())
					len = mod.dsp_dosamples((short *)pcm_buffer, len, player.info.Bps,
						player.info.Nch, player.info.SampleRate);
				mod.outMod->Write((char *)pcm_buffer, len * player.info.Nch * (player.info.Bps >> 3));
			}
		}
		else Sleep(20);
	}

	return 0;
}

extern "C"
{
	__declspec(dllexport) In_Module* __cdecl winampGetInModule2(void)
	{
		return &mod;
	}


	__declspec(dllexport) int __cdecl
		winampGetExtendedFileInfoW(const wchar_t *fn, const wchar_t *data, wchar_t *dest, size_t destlen)
	{

		return m_ReadTag.GetExtendedFileInfo(fn, data, dest, destlen);
	}

	__declspec(dllexport) int __cdecl winampUseUnifiedFileInfoDlg(const char * fn)
	{
  // this will be called when Winamp is requested to show a File Info dialog for the selected file(s)
  // and this will allow you to override or forceable ignore the handling of a file or format
  // e.g. this will allow streams/urls to be ignored
	if (!_strnicmp(fn, "file://", 7)) 
	{
		fn += 7;
	}
	
	if (PathIsURLA(fn)) 
	{ 
		return 0;
	}
	else
	{
		// Do nothing
	}

	return 1;
	}


	__declspec(dllexport) int __cdecl
		winampSetExtendedFileInfoW(const wchar_t *fn, const wchar_t *data, const wchar_t *val)
	{
		return m_WriteTag.SetExtendedFileInfo(fn, data, val);
	}

	__declspec(dllexport) int __cdecl winampWriteExtendedFileInfo()
	{
		return m_WriteTag.WriteExtendedFileInfo();
	}

	__declspec(dllexport) intptr_t __cdecl
		winampGetExtendedRead_openW(const wchar_t *filename, int *size, int *bps, int *nch, int *srate)
	{

		transcoder.heap = GetProcessHeap();
		ZeroMemory(&transcoder.info, sizeof(TTAinfo));

		// open TTA file
		if (open_TTA_file(filename, &transcoder.info) < 0) {
			tta_error_message(transcoder.info.State, filename);
			return 1;
		}

		if (decoder_new(&transcoder.heap, &transcoder.info, &transcoder.Current, &transcoder.Reader) < 0) {
			tta_error_message(transcoder.info.State, filename);
			return 1;
		}

		paused = 0;
		decode_pos_ms = 0;
		seek_needed = -1;
		mod.is_seekable = transcoder.info.Seekable;

		*bps = transcoder.info.Bps;
		*nch = transcoder.info.Nch;
		*srate = transcoder.info.SampleRate;
		*size = transcoder.info.DataLength * (*bps / 8) * (*nch);
		remain_data.data_length = 0;
		if (NULL != remain_data.buffer) {
			delete[] remain_data.buffer;
			remain_data.buffer = NULL;
		}
		else {
			// do nothing
		}

		return (intptr_t)&transcoder;
	}

	__declspec(dllexport) intptr_t __cdecl winampGetExtendedRead_getData(intptr_t handle, char *dest, int len, int *killswitch)
	{
		decoder_TTA *trans = (decoder_TTA *)handle;
		unsigned char *buf = NULL;
		int dest_used = 0;
		int n = 0;
		int32_t decoded_bytes = 0;

		if (trans->info.hFile == INVALID_HANDLE_VALUE)
		{
			return (intptr_t)-1;
		}
		else
		{
			// do nothing
		}

		buf = new unsigned char[TRANSCODING_BUFFER_SIZE];

		// restore remain (not copied) data
		if (remain_data.data_length != 0) {
			while (min(len - dest_used, remain_data.data_length) > 0 && !*killswitch) {
				n = min(len - dest_used, remain_data.data_length);
				memcpy_s(dest + dest_used, len - dest_used, remain_data.buffer + dest_used, n);
				dest_used += n;
				remain_data.data_length -= n;
			}

			if (remain_data.data_length != 0) {
				memcpy_s(buf, TRANSCODING_BUFFER_SIZE, remain_data.buffer + dest_used, remain_data.data_length);
				delete[] remain_data.buffer;
				remain_data.buffer = new BYTE[remain_data.data_length];
				memcpy_s(remain_data.buffer, remain_data.data_length, buf, remain_data.data_length);
				return (intptr_t)dest_used;
			}
			else {
				delete[] remain_data.buffer;
				remain_data.buffer = NULL;
			}
		}

		while (dest_used < len && !*killswitch) {
			// do we need to decode more?
			if (n >= decoded_bytes) 
			{
				decoded_bytes = decode_to_pcmbuffer(buf, &trans->info, &trans->Current, &trans->Reader, TRANSCODING_BUFFER_LENGTH);

				if (0 == decoded_bytes) 
				{
					break; // end of stream
				}
				else
				{
					decoded_bytes = decoded_bytes * trans->info.Bps / 8 * trans->info.Nch;
					n = min(len - dest_used, decoded_bytes);
					if (n > 0)
					{
						memcpy_s(dest + dest_used, len - dest_used, buf, n);
						dest_used += n;
					}
					else
					{
						// do nothing
					}
				}
			}
			else 
			{
				// do nothing
			}
		}

		// copy as much as we can back to winamp
		if (n > 0 && n < decoded_bytes)
		{
			remain_data.data_length = decoded_bytes - n;
			if (NULL != remain_data.buffer)
			{
				delete[] remain_data.buffer;
				remain_data.buffer = NULL;
			}
			else
			{
				// Do nothing
			}
			remain_data.buffer = new BYTE[remain_data.data_length];
			memcpy_s(remain_data.buffer, remain_data.data_length, buf + n, remain_data.data_length);
		}
		else
		{
			// Do nothing
		}

		if (NULL != buf)
		{
			delete[] buf;
			buf = NULL;
		}
		else
		{
			// Do nothing
		}

		return (intptr_t)dest_used;
	}

	// return nonzero on success, zero on failure
	__declspec( dllexport ) int __cdecl winampGetExtendedRead_setTime(intptr_t handle, int millisecs)
	{
		int done = 0;
		decoder_TTA *trans = (decoder_TTA *)handle;
		if (NULL != trans && trans->info.hFile != INVALID_HANDLE_VALUE)
		{
			trans->Current.FrameNum = millisecs / SEEK_STEP;
			decode_pos_ms = player.Current.FrameNum * SEEK_STEP;
			trans->Current.FramePos = -1;
			set_position(true, &trans->Reader, &trans->Current, &trans->info);
		} 
		else
		{
			return 0;
		}
		return 1;
	}

	__declspec( dllexport ) void __cdecl winampGetExtendedRead_close(intptr_t handle)
	{
		if (remain_data.buffer != NULL) {
			delete [] remain_data.buffer;
			remain_data.buffer = NULL;
		} else {
			// nothing to do
		}

		decoder_TTA *trans = (decoder_TTA *)handle;
		if (NULL != trans && trans->info.hFile != INVALID_HANDLE_VALUE)
		{
			::CloseHandle(trans->info.hFile);
			trans->info.hFile = INVALID_HANDLE_VALUE;
		}
		else
		{
			// do nothing
		}

	}
}

static unsigned int unpack_sint28(const char *ptr) {
	unsigned int value = 0;

	if (ptr[0] & 0x80) return 0;

	value = value | (ptr[0] & 0x7f);
	value = (value << 7) | (ptr[1] & 0x7f);
	value = (value << 7) | (ptr[2] & 0x7f);
	value = (value << 7) | (ptr[3] & 0x7f);

	return value;
}

static void skip_id3v2_tag(TTAinfo *TTAInfo) {
	id3v2_tag id3v2;
	int id3_size;
	DWORD result;

	if (!ReadFile(TTAInfo->hFile, &id3v2, sizeof(id3v2_tag), &result, NULL) ||
		result != sizeof(id3v2_tag) || memcmp(id3v2.Id, "ID3", 3)) {
		SetFilePointer(TTAInfo->hFile, 0, NULL, FILE_BEGIN);
		return;
	}

	id3_size = unpack_sint28(id3v2.Size);
	TTAInfo->Offset = id3_size + sizeof(id3v2_tag);

	SetFilePointer(TTAInfo->hFile, TTAInfo->Offset, NULL, FILE_BEGIN);
}

