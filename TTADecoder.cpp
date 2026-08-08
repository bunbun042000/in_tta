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

#include "TTADecoder.h"
#include <libtta.h>

TTAint32 CALLBACK buffer_read_callback(TTA_io_callback* io, TTAuint8* buffer, TTAuint32 size)
{
	TTA_io_callback_wrapper* iocb = reinterpret_cast<TTA_io_callback_wrapper*>(io);
	TTAint32 result = 0;

	if (iocb->remain_data_buffer.current_end_pos > iocb->remain_data_buffer.current_pos)
	{
		if (iocb->remain_data_buffer.current_end_pos - iocb->remain_data_buffer.current_pos > size)
		{
			result = size;
			memcpy_s(buffer, size, iocb->remain_data_buffer.buffer + iocb->remain_data_buffer.current_pos, result);
			iocb->remain_data_buffer.current_pos += result;
		}
		else
		{
			result = iocb->remain_data_buffer.current_end_pos - iocb->remain_data_buffer.current_pos;
			memcpy_s(buffer,size, iocb->remain_data_buffer.buffer + iocb->remain_data_buffer.current_pos, result);
			iocb->remain_data_buffer.current_pos = iocb->remain_data_buffer.current_end_pos;
		}
	}
	else 
	{
		if (iocb->remain_data_buffer.data_length - iocb->remain_data_buffer.current_pos > size)
		{
			result = size;
			memcpy_s(buffer, size, iocb->remain_data_buffer.buffer + iocb->remain_data_buffer.current_pos, result);
			iocb->remain_data_buffer.current_pos += result;
		}
		else
		{
			auto divided_size = iocb->remain_data_buffer.data_length - iocb->remain_data_buffer.current_pos;
			memcpy_s(buffer, size, iocb->remain_data_buffer.buffer + iocb->remain_data_buffer.current_pos, divided_size);
			result = size - divided_size;
			if (iocb->remain_data_buffer.current_end_pos > static_cast<size_t>(result))
			{
				memcpy_s(buffer + divided_size, size - divided_size, iocb->remain_data_buffer.buffer, result);
				iocb->remain_data_buffer.current_pos = result;
				result += divided_size;
			}
			else
			{
				memcpy_s(buffer + divided_size, size - divided_size, iocb->remain_data_buffer.buffer, iocb->remain_data_buffer.current_end_pos);
				iocb->remain_data_buffer.current_pos = iocb->remain_data_buffer.current_end_pos;
				result = divided_size + iocb->remain_data_buffer.current_pos;
			}

		}
	}


	if (iocb->remain_data_buffer.current_end_pos == iocb->remain_data_buffer.current_pos)
	{
		iocb->remain_data_buffer.current_pos = 0;
		iocb->remain_data_buffer.current_end_pos = 0;
	}
	else
	{
		// Do nothing
	}

	return result;
} // read_callback

TTAint32 CALLBACK buffer_write_callback(TTA_io_callback* io, TTAuint8* buffer, TTAuint32 size)
{
	TTA_io_callback_wrapper* iocb = reinterpret_cast<TTA_io_callback_wrapper*>(io);

	if (iocb->remain_data_buffer.data_length > iocb->remain_data_buffer.current_end_pos + size)
	{
		memcpy_s(iocb->remain_data_buffer.buffer + iocb->remain_data_buffer.current_end_pos,
			iocb->remain_data_buffer.data_length - iocb->remain_data_buffer.current_end_pos, buffer, size);
		iocb->remain_data_buffer.current_end_pos += size;
		return static_cast<TTAint32>(size);
	}
	else
	{
		// Do nothing
	}
	return 0;
} // write_callback

TTAint64 CALLBACK buffer_seek_callback(TTA_io_callback* io, TTAint64 offset) {
	TTA_io_callback_wrapper* iocb = reinterpret_cast<TTA_io_callback_wrapper*>(io);

	if (iocb->remain_data_buffer.current_end_pos > offset)
	{
		iocb->remain_data_buffer.current_pos = static_cast<size_t>(offset);
		return offset;
	}
	else
	{
		// Do nothing
	}
	return 0;
} // seek_callback

TTAint32 CALLBACK file_read_callback(TTA_io_callback* io, TTAuint8* buffer, TTAuint32 size)
{
	TTA_io_callback_wrapper* iocb = reinterpret_cast<TTA_io_callback_wrapper*>(io);
	TTAint32 result = 1;

	if (::ReadFile(iocb->handle, buffer, size, reinterpret_cast<LPDWORD>(&result), nullptr))
	{
		return result;
	}
	else
	{
		// Do nothing
	}

	return 0;
} // read_callback


TTAint64 CALLBACK file_seek_callback(TTA_io_callback* io, TTAint64 offset)
{
	TTA_io_callback_wrapper* iocb = reinterpret_cast<TTA_io_callback_wrapper*>(io);
	return ::SetFilePointer(iocb->handle, static_cast<LONG>(offset), nullptr, FILE_BEGIN);
} // seek_callback


TTADecoder::TTADecoder() : m_FileName(L""), m_paused(0), m_seek_needed(1), m_decode_pos_ms(0), m_bitrate(0), m_Filesize(0),
m_st_state(0), m_ttadec_mem{}, m_TTA(nullptr), m_tta_info{}, m_signature(m_sig_number)
{
	m_iocb_wrapper.handle = INVALID_HANDLE_VALUE;
	m_iocb_wrapper.iocb.read = nullptr;
	m_iocb_wrapper.iocb.seek = nullptr;
	m_iocb_wrapper.iocb.write = nullptr;
	m_iocb_wrapper.remain_data_buffer.buffer = nullptr;
	m_iocb_wrapper.remain_data_buffer.current_end_pos = 0;
	m_iocb_wrapper.remain_data_buffer.current_pos = 0;
	m_iocb_wrapper.remain_data_buffer.data_length = 0;

	::InitializeCriticalSection(&m_CriticalSection);
}

TTADecoder::~TTADecoder()
{
	::EnterCriticalSection(&m_CriticalSection);

	if (m_isFile && INVALID_HANDLE_VALUE != m_iocb_wrapper.handle)
	{
		::CloseHandle(m_iocb_wrapper.handle);
		m_iocb_wrapper.handle = INVALID_HANDLE_VALUE;
	}
	else
	{
		// Do nothing
	}

	if (m_iocb_wrapper.remain_data_buffer.buffer != nullptr)
	{
		_aligned_free(m_iocb_wrapper.remain_data_buffer.buffer);
	}
	else
	{
		// Do nothing
	}

	m_paused = 0;
	m_seek_needed = -1;
	m_decode_pos_ms = 0;
	m_bitrate = 0;
	m_Filesize = 0;
	m_st_state = 0;

	m_iocb_wrapper.handle = INVALID_HANDLE_VALUE;
	m_iocb_wrapper.iocb.read = nullptr;
	m_iocb_wrapper.iocb.seek = nullptr;
	m_iocb_wrapper.iocb.write = nullptr;
	m_iocb_wrapper.remain_data_buffer.current_end_pos = 0;
	m_iocb_wrapper.remain_data_buffer.current_pos = 0;
	m_iocb_wrapper.remain_data_buffer.data_length = 0;

	m_signature = -1;

	if (nullptr != m_TTA)
	{
		reinterpret_cast<tta::tta_decoder*>(&m_ttadec_mem)->~tta_decoder();
		m_TTA = nullptr;
	}
	else
	{
		// Do nothing
	}

	m_isDecodable = false;

	::LeaveCriticalSection(&m_CriticalSection);

	::DeleteCriticalSection(&m_CriticalSection);

}

int TTADecoder::init()
{
	::EnterCriticalSection(&m_CriticalSection);

	if (m_isFile)
	{
		m_iocb_wrapper.iocb.read = &file_read_callback;
		m_iocb_wrapper.iocb.seek = &file_seek_callback;
	}
	else
	{
		m_iocb_wrapper.iocb.read = &buffer_read_callback;
		m_iocb_wrapper.iocb.seek = &buffer_seek_callback;
	}

	if (nullptr != m_TTA)
	{
		reinterpret_cast<tta::tta_decoder*>(&m_ttadec_mem)->~tta_decoder();
		m_TTA = nullptr;
	}
	else
	{
		// Do nothing
	}

	try
	{
		m_TTA = new (&m_ttadec_mem) tta::tta_decoder(reinterpret_cast<TTA_io_callback*>(&m_iocb_wrapper));
		if (m_isFile)
		{
			m_TTA->init_get_info(&m_tta_info, 0);
		}
		else
		{
			m_TTA->init_set_info(&m_tta_info);
		}
	}

	catch (tta::tta_exception& ex)
	{
		if (nullptr != m_TTA)
		{
			reinterpret_cast<tta::tta_decoder*>(m_TTA)->~tta_decoder();
			m_TTA = nullptr;
		}
		else
		{
			// Do nothing
		}

		if (m_isFile)
		{
			::CloseHandle(m_iocb_wrapper.handle);
			m_iocb_wrapper.handle = INVALID_HANDLE_VALUE;
		}
		else
		{
			// Do nothing
		}

		::LeaveCriticalSection(&m_CriticalSection);
		throw TTADecoder_exception(ex.code());
	}


	m_paused = 0;
	m_decode_pos_ms = 0;
	m_seek_needed = -1;

	if (m_isFile)
	{
		// m_Filesize / (total samples * number of channel) = datasize per sample [byte/sample]
		// datasize per sample * 8 * samples per sec = m_bitrate [bit/sec]
		m_bitrate = static_cast<long>(m_Filesize / (m_tta_info.samples * m_tta_info.nch) * 8 * m_tta_info.sps / 1000);
	}
	else
	{
		// Do nothing
	}

	if (m_isFile && m_TTA->seek_allowed)
	{
		m_st_state = 1;
	}
	else
	{
		m_st_state = 0;
	}

	::LeaveCriticalSection(&m_CriticalSection);

	m_isDecodable = true;

	return TTA_NO_ERROR;
}

int TTADecoder::initDecoder(const wchar_t *filename)
{

	// check for required data presented
	if (!filename)
	{
		throw TTADecoder_exception(TTA_OPEN_ERROR);
	}
	else
	{
		// Do nothing
	}

	m_FileName = filename;
	m_iocb_wrapper.handle = CreateFileW(m_FileName.c_str(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE,
		nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);

	if (INVALID_HANDLE_VALUE == m_iocb_wrapper.handle || nullptr == m_iocb_wrapper.handle)
	{
		throw TTADecoder_exception(TTA_OPEN_ERROR);
	}
	else
	{
		// Do nothing
	}

	m_Filesize = static_cast<long>(::GetFileSize(m_iocb_wrapper.handle, nullptr));

	m_isFile = true;

	return init();

}

int	TTADecoder::initDecoder(int32_t bps, int32_t nch)
{
	m_isFile = false;
	m_tta_info.format = 1;
	m_tta_info.bps = bps;
	m_tta_info.nch = nch;

	m_iocb_wrapper.remain_data_buffer.data_length = (size_t)(PCM_BUFFER_LENGTH * MAX_DEPTH * MAX_NCH + 4); // +4 for READ_BUFFER macro

	// allocate memory for PCM buffer
	m_iocb_wrapper.remain_data_buffer.buffer =
		static_cast<TTAuint8*>(_aligned_malloc(m_iocb_wrapper.remain_data_buffer.data_length, 16));

	if (m_iocb_wrapper.remain_data_buffer.buffer == nullptr)
	{
		return TTA_MEMORY_ERROR;
	}
	else
	{
		memset(m_iocb_wrapper.remain_data_buffer.buffer, 0, m_iocb_wrapper.remain_data_buffer.data_length);
	}

	return init();

}

long double TTADecoder::seekPosition(int *done)
{

	::EnterCriticalSection(&m_CriticalSection);

	TTAuint32 new_pos;

	if (m_seek_needed >= getLengthbymsec())
	{
		m_decode_pos_ms = getLengthbymsec();
		*done = 1;
	}
	else
	{
		m_decode_pos_ms = m_seek_needed;
		m_seek_needed = -1;
	}

	if (nullptr == m_TTA)
	{
		::LeaveCriticalSection(&m_CriticalSection);
		return static_cast<double>(0);
	}
	else
	{
		// Do nothing
	}

	try
	{
		m_TTA->set_position(static_cast<TTAuint32>(m_decode_pos_ms / 1000.), &new_pos);
	}

	catch (tta::tta_exception &ex)
	{
		::LeaveCriticalSection(&m_CriticalSection);
		throw TTADecoder_exception(ex.code());
	}

	::LeaveCriticalSection(&m_CriticalSection);

	return m_decode_pos_ms;
}

int  TTADecoder::getSamples(std::byte *buffer, size_t buffersize, int *current_bitrate)
{
	int skip_len = 0;
	int len = 0;


	if (((m_isFile && INVALID_HANDLE_VALUE == m_iocb_wrapper.handle) &&
		(!m_isFile && nullptr == m_iocb_wrapper.remain_data_buffer.buffer))
		|| nullptr == buffer || 0 == buffersize)
	{
		return 0; // no decode data
	}
	else
	{
		// Do nothing
	}

	::EnterCriticalSection(&m_CriticalSection);

	if (nullptr == m_TTA)
	{
		throw TTADecoder_exception(TTA_MEMORY_ERROR);
	}
	else
	{
		// Do nothing
	}

	try 
	{
		len = m_TTA->process_stream(reinterpret_cast<TTAuint8 *>(buffer), buffersize);
	}

	catch (tta::tta_exception &ex)
	{
		throw TTADecoder_exception(ex.code());
	}

	if (len != 0)
	{
		skip_len += len;
		m_decode_pos_ms += static_cast<__int32>(skip_len * 1000. / m_tta_info.sps);
		*current_bitrate = static_cast<int>(m_TTA->get_rate());
	}
	else
	{
		// Do nothing
	}

	::LeaveCriticalSection(&m_CriticalSection);

	return len;

}

void TTADecoder::setOutputBPS(unsigned long bps)
{
	::EnterCriticalSection(&m_CriticalSection);

	if (nullptr == m_TTA)
	{
		throw TTADecoder_exception(TTA_MEMORY_ERROR);
	}
	else
	{
		// Do nothing
	}

	try
	{
		m_tta_info.bps = bps;
		m_TTA->init_set_info(&m_tta_info);
	}

	catch (tta::tta_exception &ex)
	{
		throw TTADecoder_exception(ex.code());
	}

	::LeaveCriticalSection(&m_CriticalSection);

}
