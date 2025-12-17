/*
The ttaplugin-winamp project.
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

#include "DecodeFile.h"
#include "libtta.h"

TTAint32 CALLBACK read_callback(TTA_io_callback *io, TTAuint8 *buffer, TTAuint32 size)
{
	TTA_io_callback_wrapper *iocb = reinterpret_cast<TTA_io_callback_wrapper *>(io);
	TTAint32 result = 1;

	if (::ReadFile(iocb->handle, buffer, size, (LPDWORD)&result, nullptr))
	{
		return result;
	}
	else
	{
		// Do nothing
	}

	return 0;
} // read_callback

TTAint32 CALLBACK write_callback(TTA_io_callback *io, TTAuint8 *buffer, TTAuint32 size)
{
	TTA_io_callback_wrapper *iocb = reinterpret_cast<TTA_io_callback_wrapper *>(io);
	TTAint32 result = 1;

	if (::WriteFile(iocb->handle, buffer, size, (LPDWORD)&result, nullptr))
	{
		return result;
	}
	else
	{
		// Do nothing
	}

	return 0;
} // write_callback

TTAint64 CALLBACK seek_callback(TTA_io_callback *io, TTAint64 offset)
{
	TTA_io_callback_wrapper *iocb = reinterpret_cast<TTA_io_callback_wrapper *>(io);
	return ::SetFilePointer(iocb->handle, (LONG)offset, nullptr, FILE_BEGIN);
} // seek_callback

DecodeFile::DecodeFile() : m_FileName(L""), m_paused(0), m_seek_needed(1), m_decode_pos_ms(0), m_bitrate(0), m_Filesize(0),
m_st_state(0), m_decoderFileHANDLE(INVALID_HANDLE_VALUE), m_iocb_wrapper{}, m_ttadec_mem{}, m_TTA(nullptr), m_tta_info{}, m_signature(m_sig_number)
{
	m_iocb_wrapper.handle = INVALID_HANDLE_VALUE;
	m_iocb_wrapper.iocb.read = nullptr;
	m_iocb_wrapper.iocb.seek = nullptr;
	m_iocb_wrapper.iocb.write = nullptr;

	::InitializeCriticalSection(&m_CriticalSection);
}


DecodeFile::~DecodeFile()
{
	::EnterCriticalSection(&m_CriticalSection);

	if (INVALID_HANDLE_VALUE != m_decoderFileHANDLE)
	{
		::CloseHandle(m_decoderFileHANDLE);
		m_decoderFileHANDLE = INVALID_HANDLE_VALUE;
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

	::LeaveCriticalSection(&m_CriticalSection);

	::DeleteCriticalSection(&m_CriticalSection);

}

int DecodeFile::SetFileName(const wchar_t *filename)
{
	::EnterCriticalSection(&m_CriticalSection);

	// check for required data presented
	if (!filename)
	{
		throw DecodeFile_exception(TTA_OPEN_ERROR);
	}
	else
	{
		// Do nothing
	}

	m_FileName = filename;
	m_decoderFileHANDLE = CreateFileW(m_FileName.c_str(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE,
		nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);

	if (m_decoderFileHANDLE == INVALID_HANDLE_VALUE || m_decoderFileHANDLE == nullptr)
	{
		::LeaveCriticalSection(&m_CriticalSection);
		throw DecodeFile_exception(TTA_OPEN_ERROR);
	}
	else
	{
		// Do nothing
	}

	m_Filesize = ::GetFileSize(m_decoderFileHANDLE, nullptr);

	m_iocb_wrapper.handle = m_decoderFileHANDLE;
	m_iocb_wrapper.iocb.read = &read_callback;
	m_iocb_wrapper.iocb.seek = &seek_callback;

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
		m_TTA = new (&m_ttadec_mem) tta::tta_decoder(reinterpret_cast<TTA_io_callback *>(&m_iocb_wrapper));
		m_TTA->init_get_info(&m_tta_info, 0);
	}

	catch (tta::tta_exception &ex)
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

		::CloseHandle(m_decoderFileHANDLE);
		m_decoderFileHANDLE = INVALID_HANDLE_VALUE;
		::LeaveCriticalSection(&m_CriticalSection);
		throw DecodeFile_exception(ex.code());
	}

	m_paused = 0;
	m_decode_pos_ms = 0;
	m_seek_needed = -1;

	// m_Filesize / (total samples * number of channel) = datasize per sample [byte/sample]
	// datasize per sample * 8 * samples per sec = m_bitrate [bit/sec]
	m_bitrate = (long)(m_Filesize / (m_tta_info.samples * m_tta_info.nch) * 8 * m_tta_info.sps / 1000);

	if (m_TTA->seek_allowed)
	{
		m_st_state = 1;
	}
	else
	{
		m_st_state = 0;
	}

	::LeaveCriticalSection(&m_CriticalSection);

	return TTA_NO_ERROR;
}

long double DecodeFile::SeekPosition(int *done)
{

	::EnterCriticalSection(&m_CriticalSection);

	TTAuint32 new_pos;

	if (m_seek_needed >= GetLengthbymsec())
	{
		m_decode_pos_ms = GetLengthbymsec();
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
		return (double)0;
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
		throw DecodeFile_exception(ex.code());
	}

	::LeaveCriticalSection(&m_CriticalSection);

	return m_decode_pos_ms;
}

int  DecodeFile::GetSamples(BYTE *buffer, size_t buffersize, int *current_bitrate)
{
	int skip_len = 0;
	int len = 0;


	if (INVALID_HANDLE_VALUE == m_decoderFileHANDLE || nullptr == buffer || 0 == buffersize)
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
		throw DecodeFile_exception(TTA_MEMORY_ERROR);
	}
	else
	{
		// Do nothing
	}

	try 
	{
		len = m_TTA->process_stream(buffer, buffersize);
	}

	catch (tta::tta_exception &ex)
	{
		throw DecodeFile_exception(ex.code());
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

void DecodeFile::SetOutputBPS(unsigned long bps)
{
	::EnterCriticalSection(&m_CriticalSection);

	if (nullptr == m_TTA)
	{
		throw DecodeFile_exception(TTA_MEMORY_ERROR);
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
		throw DecodeFile_exception(ex.code());
	}

	::LeaveCriticalSection(&m_CriticalSection);

}
