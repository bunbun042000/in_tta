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

#include "in_tta.h"

#include <Shlwapi.h>
#include <stdlib.h>
#include <type_traits>

#include <Winamp/in2.h>
#include <Agave/Language/api_language.h>

#include <taglib/tag.h>
#include <taglib/trueaudiofile.h>
#include <taglib/tstring.h>

#include "AlbumArt.h"
#include "DecodeFile.h"
#include "MediaLibrary.h"

#include "..\common\VersionNo.h"
#include "resource.h"


// For Support Transcoder input (2007/10/15)
static __declspec(align(16)) CDecodeFile playing_ttafile;
static __declspec(align(16)) CDecodeFile transcode_ttafile;
CMediaLibrary m_ReadTag;
CMediaLibrary m_WriteTag;

static const __int32 PLAYING_BUFFER_SIZE = PLAYING_BUFFER_LENGTH * MAX_DEPTH * MAX_NCH;
static const __int32 TRANSCODING_BUFFER_SIZE = TRANSCODING_BUFFER_LENGTH * MAX_DEPTH * MAX_NCH;

static long	vis_buffer[PLAYING_BUFFER_SIZE * MAX_NCH];	// vis buffer
static BYTE pcm_buffer[PLAYING_BUFFER_SIZE];

static HANDLE decoder_handle = INVALID_HANDLE_VALUE;
static DWORD WINAPI __stdcall DecoderThread (void *p);
static volatile int killDecoderThread = 0;

void config(HWND hwndParent);
void about(HWND hwndParent);
void init();
void quit();
void getfileinfo(const in_char *file, in_char *title, int *length_in_ms);
int  infodlg(const in_char *file, HWND hwndParent);
int  isourfile(const in_char *fn);
int  play(const in_char *fn);
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

static void tta_error_message(int error, const in_char *filename)
{
	wchar_t message[1024];

#ifdef UNICODE_INPUT_PLUGIN
	std::wstring name(filename);
#else
	std::string name(filename);
#endif
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
			L"Winamp plug-in version " PLUGIN_VERSION "\nbased on " 
			LIBTTA_VERSION "\n" PROJECT_URL);
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


void getfileinfo(const in_char *file, in_char *title, int *length_in_ms)
{

#ifdef UNICODE_INPUT_PLUGIN
	title = L"";
#else
	title = "";
#endif

	if (!file || !*file) {
		// invalid filename may be playing file
		if(playing_ttafile.isValid() && playing_ttafile.isDecodable()) {
			*length_in_ms = playing_ttafile.GetLengthbymsec();
		} else {
			*length_in_ms = 0;
		}
	} else {
		TagLib::FileName fn(file);
		TagLib::TrueAudio::File f(fn);
		if (f.isValid() == true) {
			*length_in_ms = f.audioProperties()->length() * 1000;
		} else {
			// cannot get fileinfo
			*length_in_ms = 0;
		}
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

int play(const in_char *fn)
{
	int maxlatency;
	unsigned long decoder_thread_id;
	int return_number;

	if(!playing_ttafile.isValid()) {
		return 1;
	} else {
		// do nothing
	}

	try {
		return_number = playing_ttafile.SetFileName(fn);
	}

	catch (CDecodeFile_exception &ex) {
		tta_error_message(ex.code(), fn);
		return -1;
	}

	maxlatency = mod.outMod->Open(playing_ttafile.GetSampleRate(), 
		playing_ttafile.GetNumberofChannel(), playing_ttafile.GetOutputBPS(), -1, -1);
	if (maxlatency < 0) {
		stop();
		return 1;
	} else {
		//do nothing
	}

	// setup information display
	mod.SetInfo(playing_ttafile.GetBitrate(), playing_ttafile.GetSampleRate() / 1000, playing_ttafile.GetNumberofChannel(), 1);

	// initialize vis stuff
	mod.SAVSAInit(maxlatency, playing_ttafile.GetSampleRate());
	mod.VSASetInfo(playing_ttafile.GetNumberofChannel(), playing_ttafile.GetSampleRate());

	// set the output plug-ins default volume
	mod.outMod->SetVolume(-666);

	killDecoderThread = 0;

	decoder_handle = CreateThread(NULL, 0, DecoderThread, NULL, 0, &decoder_thread_id);
	if (!decoder_handle) {
		stop(); 
		return 1;
	}

	return 0;
}

void pause() 
{
	if (playing_ttafile.isValid() && playing_ttafile.isDecodable()) {
		playing_ttafile.SetPaused(1);
	} else {
		// do nothing
	}

	mod.outMod->Pause(1);
}

void unpause() 
{
	if (playing_ttafile.isValid() && playing_ttafile.isDecodable()) {
		playing_ttafile.SetPaused(0);
	} else {
		// do nothing
	}

	mod.outMod->Pause(0);
}

int ispaused()
{
	if (playing_ttafile.isValid() && playing_ttafile.isDecodable()) {
		return playing_ttafile.GetPaused();
	} else {
		return 0;
	}

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

	mod.SetInfo(0, 0, 0, 1);
	mod.outMod->Close();
	mod.SAVSADeInit();

}

int getlength()
{
	if (playing_ttafile.isValid() && playing_ttafile.isDecodable()) {
		return playing_ttafile.GetLengthbymsec();
	} else {
		return 0;
	}
}

int getoutputtime()
{
	if (playing_ttafile.isValid() && playing_ttafile.isDecodable()) { 
		return (int)(playing_ttafile.GetDecodePosMs() 
		+ (mod.outMod->GetOutputTime() - mod.outMod->GetWrittenTime())); 
	} else {
		return 0;
	}
}

void setoutputtime(int time_in_ms)
{
	if (playing_ttafile.isValid() && playing_ttafile.isDecodable()) {
		playing_ttafile.SetSeekNeeded(time_in_ms); 
	} else {
		// do nothing
	}

}

void setvolume(int volume)
{
	mod.outMod->SetVolume(volume);
}

void setpan(int pan)
{
	mod.outMod->SetPan(pan);
}
void eq_set(int on, char data[10], int preamp)
{
	// do nothing.
}


static void do_vis(unsigned char *data, int count, int bps, long double position)
{

	if (playing_ttafile.isValid() && playing_ttafile.isDecodable()) {
		mod.SAAddPCMData(data, playing_ttafile.GetNumberofChannel(), bps, (int)position);
		mod.VSAAddPCMData(data, playing_ttafile.GetNumberofChannel(), bps, (int)position);
	}
}


DWORD WINAPI __stdcall DecoderThread (void *p) 
{

	int done = 0;
	int len;

	if (!playing_ttafile.isValid() || !playing_ttafile.isDecodable())
	{
#ifdef UNICODE_INPUT_PLUGIN
		tta_error_message(-1, L"");
#else
		tta_error_message(-1, "");
#endif
		done = 1;
		return 0;
	}
	else
	{
		// do nothing
	}

	int bitrate = playing_ttafile.GetBitrate();

	while (!killDecoderThread) {
		if(!playing_ttafile.isDecodable())
		{
#ifdef UNICODE_INPUT_PLUGIN
			tta_error_message(-1, L"");
#else
			tta_error_message(-1, "");
#endif
			PostMessage(mod.hMainWindow, WM_WA_MPEG_EOF, 0, 0);
			return 0;
		} 
		else 
		{
			// do nothing
		}
		
		if (playing_ttafile.GetSeekNeeded() != -1) 
		{
			mod.outMod->Flush((int)playing_ttafile.SeekPosition(&done));
		} 
		else
		{
			// do nothing
		}

		if (done) {
			if (!mod.outMod->IsPlaying()) 
			{
				PostMessage(mod.hMainWindow, WM_WA_MPEG_EOF, 0, 0);
				return 0;
			}
			else
			{
				::Sleep(1);
			}
		} 
		else if (mod.outMod->CanWrite() >= 
			((PLAYING_BUFFER_LENGTH * playing_ttafile.GetNumberofChannel() * 
			playing_ttafile.GetByteSize()) << (mod.dsp_isactive()? 1:0))) 
		{
				try
				{
					len = playing_ttafile.GetSamples(pcm_buffer, PLAYING_BUFFER_SIZE, &bitrate);
				}
				catch (CDecodeFile_exception &ex)
				{
#ifdef UNICODE_INPUT_PLUGIN
					tta_error_message(ex.code(), playing_ttafile.GetFileNameW());
#else
					tta_error_message(ex.code(), playing_ttafile.GetFileName());
#endif
					PostMessage(mod.hMainWindow, WM_WA_MPEG_EOF, 0, 0);
					mod.SetInfo(0, 0, 0, 1);
					mod.outMod->Close();
					mod.SAVSADeInit();
					return 0;
				}
				if (len == 0) 
				{
					done = 1;
				} 
				else
				{
					do_vis(pcm_buffer, len, playing_ttafile.GetOutputBPS(), playing_ttafile.GetDecodePosMs());
					if (mod.dsp_isactive()) 
					{
						len = mod.dsp_dosamples((short *)pcm_buffer, len, playing_ttafile.GetOutputBPS(),
							playing_ttafile.GetNumberofChannel(), playing_ttafile.GetSampleRate());
					} 
					else
					{
						// do nothing
					}
					mod.outMod->Write((char *)pcm_buffer, len * playing_ttafile.GetNumberofChannel()
						* (playing_ttafile.GetOutputBPS() >> 3));
				}
				mod.SetInfo(bitrate, playing_ttafile.GetSampleRate() / 1000, playing_ttafile.GetNumberofChannel(), 1);
		} 
		else
		{
			::Sleep(1);
		}
	}

	return 0;
}

extern "C"
{
	__declspec(dllexport) In_Module* __cdecl winampGetInModule2(void)
	{
		return &mod;
	}


#ifdef  UNICODE_INPUT_PLUGIN
	__declspec(dllexport) int __cdecl
		winampGetExtendedFileInfoW(const wchar_t *fn, const wchar_t *data, wchar_t *dest, size_t destlen)
	{

		return m_ReadTag.GetExtendedFileInfo(fn, data, dest, destlen);
	}
#else
	__declspec(dllexport) int __cdecl
	winampGetExtendedFileInfo(const char *fn, const char *data, char *dest, size_t destlen)
	{

		return m_ReadTag.GetExtendedFileInfo(fn, data, dest, destlen);
	}
#endif

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


#ifdef  UNICODE_INPUT_PLUGIN
	__declspec(dllexport) int __cdecl
		winampSetExtendedFileInfoW(const wchar_t *fn, const wchar_t *data, const wchar_t *val)
	{
		return m_WriteTag.SetExtendedFileInfo(fn, data, val);
	}
#else
	__declspec(dllexport) int __cdecl
		winampSetExtendedFileInfo(const char *fn, const char *data, const char *val)
	{
		return m_WriteTag.SetExtendedFileInfo(fn, data, val);
	}
#endif

	__declspec(dllexport) int __cdecl winampWriteExtendedFileInfo()
	{
		return m_WriteTag.WriteExtendedFileInfo();
	}

#ifdef  UNICODE_INPUT_PLUGIN
	__declspec(dllexport) intptr_t __cdecl
		winampGetExtendedRead_open(const wchar_t *filename, int *size, int *bps, int *nch, int *srate)
	{

		CDecodeFile *dec = &transcode_ttafile;
		if (!dec->isValid()) {
			return (intptr_t)0;
		}
		else {
			// do nothing
		}
		const char *mbFile = reinterpret_cast<const char*>(filename);
		wchar_t wcsFileName[MAX_PATH + 1];
		size_t strlen = 0;
		mbstowcs_s(&strlen, wcsFileName, MAX_PATH + 1, mbFile, _TRUNCATE);

		try {
			dec->SetFileName(wcsFileName);
		}

		catch (CDecodeFile_exception &ex) {
			tta_error_message(ex.code(), wcsFileName);
			return (intptr_t)0;
		}

		*bps = dec->GetBitsperSample();
		*nch = dec->GetNumberofChannel();
		*srate = dec->GetSampleRate();
		*size = dec->GetDataLength() * (*bps / 8) * (*nch);
		remain_data.data_length = 0;
		if (NULL != remain_data.buffer) {
			delete[] remain_data.buffer;
			remain_data.buffer = NULL;
		}
		else {
			// do nothing
		}

		return (intptr_t)dec;
	}
#else
	__declspec(dllexport) intptr_t __cdecl
		winampGetExtendedRead_open(const char *filename, int *size, int *bps, int *nch, int *srate)
	{

		CDecodeFile *dec = new (&CDecodeFile_mem)CDecodeFile;
		if (!dec->isValid()) {
			return (intptr_t) 0;
		} else {
			// do nothing
		}

		try {
			dec->SetFileName(filename);
		}

		catch (CDecodeFile_exception &ex) {
			tta_error_message(ex.code(), filename);
			return (intptr_t) 0;
		}

		*bps = dec->GetBitsperSample();
		*nch = dec->GetNumberofChannel();
		*srate = dec->GetSampleRate();
		*size = dec->GetDataLength() * (*bps / 8) * (*nch);
		remain_data.data_length = 0;
		if (NULL != remain_data.buffer) {
			delete [] remain_data.buffer;
			remain_data.buffer = NULL;
		} else {
			// do nothing
		}
	
		return (intptr_t)dec;
	}
#endif

#ifdef  UNICODE_INPUT_PLUGIN
	__declspec(dllexport) intptr_t __cdecl winampGetExtendedRead_getData(intptr_t handle, char *dest, int len, int *killswitch)
	{
		CDecodeFile *dec = (CDecodeFile *)handle;
		unsigned char *buf = NULL;
		int dest_used = 0;
		int n = 0;
		int bitrate;
		int32_t decoded_bytes = 0;

		if (!dec->isDecodable()) {
			return (intptr_t)-1;
		}
		else {
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
			if (n >= decoded_bytes) {
				try {
					decoded_bytes = dec->GetSamples(buf, TRANSCODING_BUFFER_SIZE, &bitrate);
				}
				catch (CDecodeFile_exception &ex) {
					tta_error_message(ex.code(), dec->GetFileNameW());
					dest_used = -1;
					break;
				}

				if (0 == decoded_bytes) {
					break; // end of stream
				}
				else {
					decoded_bytes = decoded_bytes * dec->GetBitsperSample() / 8 * dec->GetNumberofChannel();
					n = min(len - dest_used, decoded_bytes);
					if (n > 0) {
						memcpy_s(dest + dest_used, len - dest_used, buf, n);
						dest_used += n;
					}
					else {
						// do nothing
					}
				}
			}
			else {
				// do nothing
			}
		}

		// copy as much as we can back to winamp
		if (n > 0 && n < decoded_bytes) {
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
#else

	__declspec(dllexport) intptr_t __cdecl winampGetExtendedRead_getData(intptr_t handle, char *dest, int len, int *killswitch)
	{
		CDecodeFile *dec = (CDecodeFile *)handle;
		unsigned char buf[BUFFER_SIZE];
		int dest_used = 0;
		int n = 0;
		int bitrate;
		int32_t decoded_bytes = 0;

		if (!dec->isDecodable()) {
			return (intptr_t)-1;
		}
		else {
			// do nothing
		}

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
			if (n >= decoded_bytes) {
				try {
					decoded_bytes = dec->GetSamples(buf, BUFFER_SIZE, &bitrate);
				}
				catch (CDecodeFile_exception &ex) {
					tta_error_message(ex.code(), dec->GetFileName());
					dest_used = -1;
					break;
				}

				if (0 == decoded_bytes) {
					break; // end of stream
				}
				else {
					decoded_bytes = decoded_bytes * dec->GetBitsperSample() / 8 * dec->GetNumberofChannel();
					n = min(len - dest_used, decoded_bytes);
					if (n > 0) {
						memcpy_s(dest + dest_used, len - dest_used, buf, n);
						dest_used += n;
					}
					else {
						// do nothing
					}
				}
			}
			else {
				// do nothing
			}
		}

		// copy as much as we can back to winamp
		if (n > 0 && n < decoded_bytes) {
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

		return (intptr_t)dest_used;
	}
#endif

	// return nonzero on success, zero on failure
	__declspec( dllexport ) int __cdecl winampGetExtendedRead_setTime(intptr_t handle, int millisecs)
	{
		int done = 0;
		CDecodeFile *dec = (CDecodeFile *)handle;
		if (NULL != dec && dec->isValid() && dec->isDecodable()) {
			dec->SetSeekNeeded(millisecs);
			dec->SeekPosition(&done);
		} else {
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

		CDecodeFile *dec = (CDecodeFile *)handle;
		if (NULL != dec && dec->isValid()) {
			dec = NULL;
		} else {
			// do nothing
		}

	}
}
