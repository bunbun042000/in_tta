/*
The ttaplugins-winamp project.
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

#pragma once
#define UNICODE_INPUT_PLUGIN

static const __int32 PLAYING_BUFFER_LENGTH = 576;
static const __int32 TRANSCODING_BUFFER_LENGTH = 5120;
static const unsigned __int32 TTA1_SIGN = 0x31415454;
static const __int32 READER_BUFFER_SIZE = 5184;
static const long double FRAME_TIME = 1.04489795918367346939;
static const int SEEK_STEP = (int)(FRAME_TIME * 1000);
static const __int32 MAX_DEPTH = 3;
static const __int32 MAX_BPS = (MAX_DEPTH * 8);
static const __int32 MIN_BPS = 16;
static const __int32 MAX_NCH = 6;

typedef enum {
	TTA_SUCCESS,	// setjmp data saved
	TTA_NO_ERROR,	// no known errors found
	TTA_OPEN_ERROR,	// can't open file
	TTA_FORMAT_ERROR,	// not compatible file format
	TTA_FILE_ERROR,	// file is corrupted
	TTA_READ_ERROR,	// can't read from input file
	TTA_WRITE_ERROR,	// can't write to output file
	TTA_SEEK_ERROR,	// file seek error
	TTA_MEMORY_ERROR,	// insufficient memory available
	TTA_PASSWORD_ERROR,	// password protected file
	TTA_NOT_SUPPORTED	// unsupported architecture
} TTA_CODEC_STATUS;

typedef struct {
	unsigned __int32 TTAid;
	unsigned __int16 AudioFormat;
	unsigned __int16 NumChannels;
	unsigned __int16 BitsPerSample;
	unsigned __int32 SampleRate;
	unsigned __int32 DataLength;
	unsigned __int32 CRC32;
} TTAhdr;

typedef struct {
	unsigned int k0;
	unsigned int k1;
	unsigned int sum0;
	unsigned int sum1;
} TTAadapt;

typedef struct {
	int shift;
	int round;
	int error;
	int qm[12];
	int dx[12];
	int dl[12];
} TTAfltst;

typedef struct {
	TTAfltst fst;
	TTAadapt rice;
	int last;
} TTA_channel_codec;

typedef struct {
	TTA_channel_codec TTA[2 * MAX_NCH];	// decoder (1 per channel)
	unsigned int BitCount;	// count of bits in cache
	unsigned int BitCache;	// bit cache
	unsigned int FrameCRC;	// CRC32
	unsigned int FrameTotal;	// total count of frames
	unsigned int FrameLenD;	// default frame length in samples
	unsigned int FrameLenL;	// last frame length in samples
	unsigned int FrameLen;	// current frame length in samples
	unsigned int FrameNum;	// currently playing frame index
	unsigned int FramePos;	// the playing position in frame
	unsigned int *SeekTable;	// the playing position table
	unsigned __int32 read_bytes;  // read bytes
} TTAcodec;

typedef struct {
	BYTE Buffer[READER_BUFFER_SIZE];
	BYTE End;
	BYTE *Pos;
} TTA_reader;

typedef struct {
	wchar_t			FName[MAX_PATH];
	HANDLE			hFile;		// file handle
	unsigned short	Nch;		// number of channels
	unsigned short	Bps;		// bits per sample
	unsigned short	BSize;		// byte size
	unsigned int	SampleRate;	// samplerate (sps)
	unsigned int	DataLength;	// data length in samples
	unsigned int	Length;		// playback time (sec)
	unsigned int	FileSize;	// file size (byte)
	float			Compress;	// compression ratio
	unsigned int	BitRate;	// bitrate (kbps)
	unsigned int	Seekable;	// is seekable?
	TTA_CODEC_STATUS State;		// return code
	unsigned int	Offset;		// header size
} TTAinfo;

typedef struct {
	unsigned char  Id[3];
	unsigned char  majorVersion;
	unsigned char  minorVersion;
	unsigned char  Flags;
	char  Size[4];
} id3v2_tag;

typedef struct {
	HANDLE		heap;
	TTAinfo		info;			// currently playing file info
	TTA_reader	Reader;
	TTAcodec	Current;
} decoder_TTA;

