/*
The ttaplugins-winamp project.
Copyright (C) 2005-2026 Yamagata Fumihiro

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

#ifndef DECODEFILE_H
#define DECODEFILE_H

#include "libtta.h"
#include "in_tta.h"
#include <stdexcept>
#include <type_traits>
#include <cstddef>

struct TTA_io_callback_wrapper
{
	TTA_io_callback iocb{};
	HANDLE handle{};
} ;

class alignas(16) DecodeFile
{
private:
	std::wstring			FileName;

	int						paused;
	__int32					seek_needed;
	__int32					decode_pos_ms;
	TTAuint64				pos;

	long					bitrate;			// kbps
	long					Filesize;			// total file size (in bytes)

	unsigned long			st_state;			// seek table status

	HANDLE					decoderFileHANDLE;

	alignas(16) TTA_io_callback_wrapper iocb_wrapper;
	alignas(tta::tta_decoder) std::byte ttadec_mem[sizeof(tta::tta_decoder)];
	tta::tta_decoder	   *TTA;
	alignas(16) TTA_info	tta_info;
	__int64					signature;
	static const __int64	sig_number = 7792625911880894;

	CRITICAL_SECTION		CriticalSection;

public:
	DecodeFile(void);
	virtual ~DecodeFile(void);

	bool			isValid() { return sig_number == signature ? true : false; }
	bool			isDecodable() const { return decoderFileHANDLE != INVALID_HANDLE_VALUE ? true : false; }

	int				SetFileName(const wchar_t *filename);
	const wchar_t  *GetFileName() { return FileName.c_str(); }
	int				GetSamples(BYTE *buffer, size_t buffersize, int *current_bitrate);

	int				GetPaused() const{ return paused; }
	void			SetPaused(int p) { paused = p; }
	double			GetDecodePosMs() const{ return decode_pos_ms; }
	long double		SeekPosition(int *done);
	void			SetSeekNeeded(int sn) { seek_needed = sn; }
	int				GetSeekNeeded() const{ return seek_needed; }
	int				GetSampleRate() const{ return (int)tta_info.sps; }
	int				GetBitrate() const{ return (int)(bitrate); }
	__int32			GetNumberofChannel() const{ return (__int32)tta_info.nch; }
	__int32			GetLengthbymsec() const{ return (__int32)(tta_info.samples / tta_info.sps * 1000); }
	int				GetDataLength() const{ return (int)tta_info.samples; }
	TTAuint8		GetByteSize() const{ return TTAuint8(tta_info.bps / 8); }
	unsigned __int32	GetOutputBPS() const{ return tta_info.bps; } 
	void			SetOutputBPS(unsigned long bps);
	__int32			GetBitsperSample() const{ return (__int32)tta_info.bps; }

};

class DecodeFile_exception : public std::exception
{
private:
	tta_error err_code;

public:
	DecodeFile_exception(tta_error code) : err_code(code) {}
	tta_error code() const { return err_code; }
}; // class tta_exception

#endif
