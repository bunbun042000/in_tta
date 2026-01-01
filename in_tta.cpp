/*
 * in_tta.c
 *
 * Description:	 TTA input plug-in for Winamp 2
 *
 * Copyright (c) 2005-2009 Aleksander Djuric. All rights reserved.
 *
 */
 
 /*
The ttaplugins-winamp project.
Copyright (C) 2005-2026 Yamagata Fumihiro

This file is part of in_tta.

in_tta is free software: you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation, either
version 3 of the License, or any later version.

in_tta is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
General Public License for more details.

You should have received a copy of the GNU General Public License along with enc_tta.
If not, see <https://www.gnu.org/licenses/>.
*/

#include "in_tta.h"

#include <Shlwapi.h>
#include <type_traits>
#include <strsafe.h>

#include <Winamp/in2.h>
#include <Agave/Language/api_language.h>

#include <taglib/tag.h>
#include <taglib/trueaudiofile.h>
#include <taglib/tstring.h>

#include "TTADecoder.h"

#include "wasabi.h"
#include "AlbumArt.h"
#include "ttaTag.h"

#include "VersionNo.h"
#include "resource.h"

const static int MAX_MESSAGE_LENGTH = 1024;
const static __int32 PLAYING_BUFFER_LENGTH = 576;

// for playing static variables
static TTADecoder *decoder_tta = nullptr;

static HANDLE decoder_handle = INVALID_HANDLE_VALUE;
static DWORD WINAPI __stdcall DecoderThread(void *p);
static volatile int killDecoderThread = 0;

// for MetaData static variables
ttaTag m_ReadTag;
ttaTag m_WriteTag;

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

In_Module mod =
{
	IN_VER,
	const_cast<char *>("TTA Audio Decoder " PLUGIN_VERSION_CHAR),			// description of module, with version string	description,
	nullptr,		// hMainWindow
	nullptr,		// hDllInstance
	const_cast<char *>("TTA\0TTA Audio File (*.TTA)\0"),				// "mp3\0Layer 3 MPEG\0mp2\0Layer 2 MPEG\0mpg\0Layer 1 MPEG\0"
	1,			// is_seekable
	1,			// uses output
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
	nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, // vis stuff
	nullptr, nullptr,	// dsp
	eq_set,
	nullptr,		// setinfo
	nullptr			// out_mod
};

static void tta_error_message(int error, const wchar_t *filename)
{
	wchar_t message[MAX_MESSAGE_LENGTH];

	std::wstring name(filename);

	switch (error)
	{
	case TTA_OPEN_ERROR:
		StringCbPrintf(message, MAX_MESSAGE_LENGTH, L"Can't open file:\n%ls", name.c_str());
		break;
	case TTA_FORMAT_ERROR:
		StringCbPrintf(message, MAX_MESSAGE_LENGTH, L"Unknown TTA format version:\n%ls", name.c_str());
		break;
	case TTA_NOT_SUPPORTED:
		StringCbPrintf(message, MAX_MESSAGE_LENGTH, L"Not supported file format:\n%ls", name.c_str());
		break;
	case TTA_FILE_ERROR:
		StringCbPrintf(message, MAX_MESSAGE_LENGTH, L"File is corrupted:\n%ls", name.c_str());
		break;
	case TTA_READ_ERROR:
		StringCbPrintf(message, MAX_MESSAGE_LENGTH, L"Can't read from file:\n%ls", name.c_str());
		break;
	case TTA_WRITE_ERROR:
		StringCbPrintf(message, MAX_MESSAGE_LENGTH, L"Can't write to file:\n%ls", name.c_str());
		break;
	case TTA_MEMORY_ERROR:
		StringCbPrintf(message, MAX_MESSAGE_LENGTH, L"Insufficient memory available");
		break;
	case TTA_SEEK_ERROR:
		StringCbPrintf(message, MAX_MESSAGE_LENGTH, L"file seek error");
		break;
	case TTA_PASSWORD_ERROR:
		StringCbPrintf(message, MAX_MESSAGE_LENGTH, L"password protected file");
		break;
	default:
		StringCbPrintf(message, MAX_MESSAGE_LENGTH, L"Unknown TTA decoder error");
		break;
	}

	MessageBox(mod.hMainWindow, message, L"TTA Decoder Error",
		MB_OK | MB_ICONERROR | MB_SYSTEMMODAL);

}

static INT_PTR CALLBACK about_dialog(HWND dialog, UINT message, WPARAM wparam, LPARAM lparam)
{
	switch (message)
	{
	case WM_INITDIALOG:
		SetDlgItemText(dialog, IDC_PLUGIN_VERSION,
			IN_TTA_PLUGIN_VERSION_CREADIT);
		SetDlgItemText(dialog, IDC_PLUGIN_CREADIT,
			IN_TTA_PLUGIN_COPYRIGHT_CREADIT);
		return TRUE;
	case WM_COMMAND:
		switch (LOWORD(wparam))
		{
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
	DialogBox(mod.hDllInstance, MAKEINTRESOURCE(IDD_ABOUT),
		hwndParent, about_dialog);
}

void about(HWND hwndParent)
{
	DialogBox(mod.hDllInstance, MAKEINTRESOURCE(IDD_ABOUT),
		hwndParent, about_dialog);
}


void init()
{
	Wasabi_Init();
}

void quit()
{
	Wasabi_Quit();
}


void getfileinfo(const wchar_t *file, wchar_t *title, int *length_in_ms)
{

	wchar_t null_char[] = L"";
	title = null_char;

	if (!file || !*file)
	{
		// invalid filename may be playing file
		if (nullptr != decoder_tta && decoder_tta->isValid() && decoder_tta->isDecodable())
		{
			*length_in_ms = decoder_tta->getLengthbymsec();
		}
		else
		{
			*length_in_ms = 0;
		}
	}
	else
	{
		TagLib::FileName fn(file);
		TagLib::TrueAudio::File f(fn);
		if (f.isValid() == true)
		{
			*length_in_ms = f.audioProperties()->lengthInMilliseconds();
		}
		else
		{
			// cannot get fileinfo
			*length_in_ms = 0;
		}
	}
}

int infodlg(const wchar_t *filename, HWND parent)
{
	return 0;
}

int isourfile(const wchar_t *filename)
{
	return 0;
}

int play(const wchar_t *filename)
{
	int maxlatency;
	unsigned long decoder_thread_id;
	int return_number;

	if (nullptr == decoder_tta)
	{
		decoder_tta = new TTADecoder;
	}
	else
	{
		// Do nothing
	}

	if (!decoder_tta->isValid())
	{
		return 1;
	}
	else
	{
		// Do nothing
	}

	try
	{
		return_number = decoder_tta->initDecoder(filename);
	}

	catch (TTADecoder_exception& ex)
	{
		tta_error_message(ex.code(), filename);
		return -1;
	}

	maxlatency = mod.outMod->Open(decoder_tta->getSampleRate(),
		decoder_tta->getNumberofChannel(), static_cast<int>(decoder_tta->getOutputBPS()), -1, -1);
	if (maxlatency < 0)
	{
		stop();
		return 1;
	}
	else
	{
		// Do nothing
	}

	// setup information display
	mod.SetInfo(decoder_tta->getBitrate(), decoder_tta->getSampleRate() / 1000, decoder_tta->getNumberofChannel(), 1);

	// initialize vis stuff
	mod.SAVSAInit(maxlatency, decoder_tta->getSampleRate());
	mod.VSASetInfo(decoder_tta->getNumberofChannel(), decoder_tta->getSampleRate());

	// set the output plug-ins default volume
	mod.outMod->SetVolume(-666);

	killDecoderThread = 0;

	decoder_handle = CreateThread(nullptr, 0, DecoderThread, nullptr, 0, &decoder_thread_id);
	if (!decoder_handle)
	{
		stop();
		return 1;
	}
	else
	{
		// Do nothing
	}

	return 0;
}

void pause()
{
	if (nullptr != decoder_tta && decoder_tta->isValid() && decoder_tta->isDecodable())
	{
		decoder_tta->setPaused(1);
	}
	else
	{
		// Do nothing
	}

	mod.outMod->Pause(1);
}

void unpause()
{
	if (nullptr != decoder_tta && decoder_tta->isValid() && decoder_tta->isDecodable())
	{
		decoder_tta->setPaused(0);
	}
	else
	{
		// Do nothing
	}

	mod.outMod->Pause(0);
}

int ispaused()
{
	if (nullptr != decoder_tta && decoder_tta->isValid() && decoder_tta->isDecodable())
	{
		return decoder_tta->getPaused();
	}
	else
	{
		return 0;
	}

}

void stop()
{
	if (INVALID_HANDLE_VALUE != decoder_handle)
	{
		killDecoderThread = 1;
		WaitForSingleObject(decoder_handle, INFINITE);
		CloseHandle(decoder_handle);
		decoder_handle = INVALID_HANDLE_VALUE;
	}
	else
	{
		// Do nothing
	}

	if (nullptr != decoder_tta)
	{
		delete decoder_tta;
		decoder_tta = nullptr;
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
	if (nullptr != decoder_tta && decoder_tta->isValid() && decoder_tta->isDecodable())
	{
		return decoder_tta->getLengthbymsec();
	}
	else
	{
		return 0;
	}
}

int getoutputtime()
{
	if (nullptr != decoder_tta && decoder_tta->isValid() && decoder_tta->isDecodable())
	{
		return (int)(decoder_tta->getDecodePosMs())
			+ mod.outMod->GetOutputTime() - mod.outMod->GetWrittenTime();
	}
	else
	{
		return 0;
	}
}

void setoutputtime(int time_in_ms)
{
	if (nullptr != decoder_tta && decoder_tta->isValid() && decoder_tta->isDecodable())
	{
		decoder_tta->setSeekNeeded(time_in_ms);
	}
	else
	{
		// Do nothing
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
	// Do nothing
}

static void do_vis(unsigned char *data, int count, int bps, long double position)
{

	if (nullptr != decoder_tta && decoder_tta->isValid() && decoder_tta->isDecodable())
	{
		mod.SAAddPCMData(data, decoder_tta->getNumberofChannel(), bps, static_cast<int>(position));
		mod.VSAAddPCMData(data, decoder_tta->getNumberofChannel(), bps, static_cast<int>(position));
	}
	else
	{
		// Do nothing
	}
}


DWORD WINAPI __stdcall DecoderThread(void *p)
{

	int done = 0;
	int decoded_samples;
	static const __int32 PLAYING_BUFFER_SIZE = TTA_FIFO_BUFFER_SIZE;
	static BYTE pcm_buffer[PLAYING_BUFFER_SIZE];

	if (nullptr == decoder_tta || !decoder_tta->isValid() || !decoder_tta->isDecodable())
	{
		tta_error_message(-1, L"");
		done = 1;
		return 0;
	}
	else
	{
		// Do nothing
	}

	int bitrate = decoder_tta->getBitrate();

	while (!killDecoderThread)
	{
		if (!decoder_tta->isDecodable())
		{
			tta_error_message(-1, L"");
			PostMessage(mod.hMainWindow, WM_WA_MPEG_EOF, 0, 0);
			return 0;
		}
		else
		{
			// Do nothing
		}

		if (decoder_tta->getSeekNeeded() != -1)
		{
			mod.outMod->Flush((int)decoder_tta->seekPosition(&done));
		}
		else
		{
			// Do nothing
		}

		if (done)
		{
			if (!mod.outMod->IsPlaying())
			{
				PostMessage(mod.hMainWindow, WM_WA_MPEG_EOF, 0, 0);
				return 0;
			}
			else
			{
				mod.SetInfo(bitrate, decoder_tta->getSampleRate() / 1000, decoder_tta->getNumberofChannel(), 1);
			}
		}
		else if (mod.outMod->CanWrite() >=
			((PLAYING_BUFFER_LENGTH * decoder_tta->getNumberofChannel() *
				decoder_tta->getByteSize()) << (mod.dsp_isactive() ? 1 : 0)))
		{
			try
			{
				decoded_samples = decoder_tta->getSamples(reinterpret_cast<std::byte *>(pcm_buffer), PLAYING_BUFFER_SIZE, &bitrate);
			}
			catch (TTADecoder_exception &ex)
			{
				tta_error_message(ex.code(), decoder_tta->getFileName());
				PostMessage(mod.hMainWindow, WM_WA_MPEG_EOF, 0, 0);
				mod.SetInfo(0, 0, 0, 1);
				mod.outMod->Close();
				mod.SAVSADeInit();
				if (nullptr != decoder_tta && decoder_tta->isValid())
				{
					delete decoder_tta;
					decoder_tta = nullptr;
				}
				else
				{
					// Do nothing
				}
				return 0;
			}

			if (decoded_samples == 0)
			{
				done = 1;
			}
			else
			{
				do_vis(pcm_buffer, decoded_samples, static_cast<int>(decoder_tta->getOutputBPS()), decoder_tta->getDecodePosMs());
				if (mod.dsp_isactive())
				{
					decoded_samples = mod.dsp_dosamples(reinterpret_cast<short*>(pcm_buffer), decoded_samples, static_cast<int>(decoder_tta->getOutputBPS()),
						decoder_tta->getNumberofChannel(), decoder_tta->getSampleRate());
				}
				else
				{
					// Do nothing
				}
				mod.outMod->Write(reinterpret_cast<char *>(pcm_buffer), decoded_samples * decoder_tta->getNumberofChannel()
					* decoder_tta->getByteSize());
			}

			mod.SetInfo(bitrate, decoder_tta->getSampleRate() / 1000, decoder_tta->getNumberofChannel(), 1);
		}
		else
		{
			mod.SetInfo(bitrate, decoder_tta->getSampleRate() / 1000, decoder_tta->getNumberofChannel(), 1);

			Sleep(1);
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


	__declspec(dllexport) int __cdecl
		winampGetExtendedFileInfoW(const wchar_t *fn, const char *data, wchar_t *dest, size_t destlen)
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
		winampSetExtendedFileInfoW(const wchar_t *fn, const char *data, const wchar_t *val)
	{
		return m_WriteTag.SetExtendedFileInfo(fn, data, val);
	}

	__declspec(dllexport) int __cdecl winampWriteExtendedFileInfo()
	{
		m_ReadTag.FlushCache();
		return m_WriteTag.WriteExtendedFileInfo();
	}

	__declspec(dllexport) intptr_t __cdecl
		winampGetExtendedRead_openW(const wchar_t *filename, int *size, int *bps, int *nch, int *srate)
	{

		TTADecoder *dec = new TTADecoder;

		if (!dec->isValid())
		{
			return static_cast<intptr_t>(0);
		}
		else
		{
			// Do nothing
		}

		try
		{
			dec->initDecoder(filename);
		}

		catch (TTADecoder_exception &ex)
		{
			tta_error_message(ex.code(), filename);
			return static_cast<intptr_t>(0);
		}

		*bps = dec->getBitsperSample();
		*nch = dec->getNumberofChannel();
		*srate = dec->getSampleRate();
		*size = dec->getDataLength() * (*bps / 8) * (*nch);

		return reinterpret_cast<intptr_t>(dec);
	}

	__declspec(dllexport) intptr_t __cdecl winampGetExtendedRead_getData(intptr_t handle, char *dest, int len, int *killswitch)
	{
		TTADecoder *dec = reinterpret_cast<TTADecoder *>(handle);

		int dest_used = 0;
		int bitrate;
		int32_t decoded_samples = 0;
		int32_t decoded_bytes = 0;

		if (!dec->isDecodable())
		{
			return static_cast<intptr_t>(-1);
		}
		else
		{
			// Do nothing
		}

		try
		{
			decoded_samples = dec->getSamples(reinterpret_cast<std::byte *>(dest), static_cast<size_t>(len), &bitrate);
		}
		catch (TTADecoder_exception &ex)
		{
			tta_error_message(ex.code(), dec->getFileName());
			dest_used = -1;
		}

		if (0 != decoded_samples)
		{
			decoded_bytes = decoded_samples * dec->getBitsperSample() / 8 * dec->getNumberofChannel();
		}
		else
		{
			// Do nothing
		}


		return static_cast<intptr_t>(decoded_bytes);
	}

	// return nonzero on success, zero on failure
	__declspec(dllexport) int __cdecl winampGetExtendedRead_setTime(intptr_t handle, int millisecs)
	{
		int done = 0;

		TTADecoder *dec = reinterpret_cast<TTADecoder *>(handle);
		if (nullptr != dec && dec->isValid() && dec->isDecodable())
		{
			dec->setSeekNeeded(millisecs);
			dec->seekPosition(&done);
		}
		else
		{
			return 0;
		}
		return 1;
	}

	__declspec(dllexport) void __cdecl winampGetExtendedRead_close(intptr_t handle)
	{
		if (reinterpret_cast<TTADecoder *>(handle) != nullptr && reinterpret_cast<TTADecoder*>(handle)->isValid())
		{
			delete reinterpret_cast<TTADecoder *>(handle);
		}
		else
		{
			// Do nothing
		}
	}
}
