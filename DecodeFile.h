/*
The ttaplugins-winamp project.
Copyright (C) 2005-2025 Yamagata Fumihiro

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
	TTA_io_callback iocb {};
	HANDLE handle;
};

class alignas(16) DecodeFile
{
private:
	std::wstring			m_FileName;

	int						m_paused;
	__int32					m_seek_needed;
	__int32					m_decode_pos_ms;

	long					m_bitrate;			// kbps
	long					m_Filesize;			// total file size (in bytes)

	unsigned long			m_st_state;			// seek table status

	HANDLE					m_decoderFileHANDLE;

	alignas(16) TTA_io_callback_wrapper m_iocb_wrapper;
	alignas(tta::tta_decoder) std::byte m_ttadec_mem[sizeof(tta::tta_decoder)];
	tta::tta_decoder	   *m_TTA;
	alignas(16) TTA_info	m_tta_info;
	__int64					m_signature;
	static const __int64	m_sig_number = 7792625911880894;

	CRITICAL_SECTION		m_CriticalSection;

public:

	DecodeFile();
	virtual ~DecodeFile();

	bool			isValid() const { return m_sig_number == m_signature ? true : false; }
	bool			isDecodable() const { return m_decoderFileHANDLE != INVALID_HANDLE_VALUE ? true : false; }

	int				SetFileName(const wchar_t *filename);
	const wchar_t  *GetFileName() { return m_FileName.c_str(); }
	int				GetSamples(BYTE *buffer, size_t buffersize, int *current_bitrate);

	int				GetPaused() const { return m_paused; }
	void			SetPaused(int p) { m_paused = p; }
	double			GetDecodePosMs() const { return m_decode_pos_ms; }
	long double		SeekPosition(int *done);
	void			SetSeekNeeded(int sn) { m_seek_needed = sn; }
	int				GetSeekNeeded() const { return m_seek_needed; }
	int				GetSampleRate() const { return static_cast<int>(m_tta_info.sps); }
	int				GetBitrate() { return static_cast<int>(m_bitrate); }
	__int32			GetNumberofChannel() const { return static_cast<__int32>(m_tta_info.nch); }
	__int32			GetLengthbymsec() const { return static_cast<__int32>(m_tta_info.samples / m_tta_info.sps * 1000); }
	int				GetDataLength() const { return static_cast<int>(m_tta_info.samples); }
	TTAuint8		GetByteSize() const { return static_cast<TTAuint8>(m_tta_info.bps / 8); }
	unsigned __int32	GetOutputBPS() { return m_tta_info.bps; }
	void			SetOutputBPS(unsigned long bps);
	__int32			GetBitsperSample() const { return static_cast<__int32>(m_tta_info.bps); }

};

class DecodeFile_exception : public std::exception
{
private:
	tta_error m_err_code;

public:
	DecodeFile_exception(tta_error code) : m_err_code(code) {}
	tta_error code() const { return m_err_code; }
}; // class tta_exception

#endif
