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

You should have received a copy of the GNU General Public License along with in_tta.
If not, see <https://www.gnu.org/licenses/>.
*/

#ifndef TTADECODER_H
#define TTADECODER_H

#include <libtta.h>
#include "in_tta.h"
#include <stdexcept>
#include <type_traits>
#include <cstddef>

static const int PCM_BUFFER_LENGTH = 5210;

struct data_buf
{
	size_t	data_length;
	size_t	current_pos;
	size_t	current_end_pos;
	TTAuint8* buffer;
};

struct TTA_io_callback_wrapper
{
	TTA_io_callback iocb{};
	HANDLE handle{};
	data_buf remain_data_buffer{};
};

class alignas(16) TTADecoder
{
private:
	std::wstring			m_FileName;

	int						m_paused;
	__int32					m_seek_needed;
	__int32					m_decode_pos_ms;

	long					m_bitrate;			// kbps
	long					m_Filesize;			// total file size (in bytes)

	unsigned long			m_st_state;			// seek table status

	alignas(16) TTA_io_callback_wrapper m_iocb_wrapper;
	alignas(tta::tta_decoder) std::byte m_ttadec_mem[sizeof(tta::tta_decoder)];
	tta::tta_decoder	   *m_TTA;
	alignas(16) TTA_info	m_tta_info;
	__int64					m_signature;
	static const __int64	m_sig_number = 7792625911880894;

	CRITICAL_SECTION		m_CriticalSection;
	bool					m_isFile = false;
	bool					m_isDecodable = false;

private:
	int				init();

public:
	TTADecoder();
	virtual ~TTADecoder();

	bool			isValid() const { return m_sig_number == m_signature ? true : false; }
	bool			isDecodable() const { return m_isDecodable; }


	int				initDecoder(const wchar_t *filename);
	int				initDecoder(int32_t bps, int32_t nch);
	const wchar_t  *getFileName() { return m_FileName.c_str(); }
	int				getSamples(std::byte *buffer, size_t buffersize, int *current_bitrate);

	int				getPaused() const{ return m_paused; }
	void			setPaused(int p) { m_paused = p; }
	double			getDecodePosMs() const{ return m_decode_pos_ms; }
	long double		seekPosition(int *done);
	void			setSeekNeeded(int sn) { m_seek_needed = sn; }
	int				getSeekNeeded() const{ return m_seek_needed; }
	int				getSampleRate() const{ return static_cast<int>(m_tta_info.sps); }
	int				getBitrate() const{ return static_cast<int>(m_bitrate); }
	__int32			getNumberofChannel() const{ return static_cast<__int32>(m_tta_info.nch); }
	__int32			getLengthbymsec() const{ return static_cast<__int32>(m_tta_info.samples / m_tta_info.sps * 1000); }
	int				getDataLength() const{ return static_cast<int>(m_tta_info.samples); }
	TTAuint8		getByteSize() const{ return static_cast<TTAuint8>(m_tta_info.bps / 8); }
	unsigned __int32	getOutputBPS() const{ return m_tta_info.bps; } 
	void			setOutputBPS(unsigned long bps);
	__int32			getBitsperSample() const{ return static_cast<__int32>(m_tta_info.bps); }

};

class TTADecoder_exception : public std::exception
{
private:
	tta_error m_err_code;

public:
	TTADecoder_exception(tta_error code) : m_err_code(code) {}
	tta_error code() const { return m_err_code; }
}; // class tta_exception

#endif // #ifndef TTADECODER_H
