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

#include "DecodeFile.h"
#include "libtta.h"

TTAint32 CALLBACK read_callback(TTA_io_callback *io, TTAuint8 *buffer, TTAuint32 size)
{
	TTA_io_callback_wrapper *iocb = (TTA_io_callback_wrapper *)io;
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
	TTA_io_callback_wrapper *iocb = (TTA_io_callback_wrapper *)io;
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
	TTA_io_callback_wrapper *iocb = (TTA_io_callback_wrapper *)io;
	return ::SetFilePointer(iocb->handle, (LONG)offset, nullptr, FILE_BEGIN);
} // seek_callback

DecodeFile::DecodeFile(void) : FileName(L""), paused(0), seek_needed(1), decode_pos_ms(0), bitrate(0), Filesize(0),
st_state(0), decoderFileHANDLE(INVALID_HANDLE_VALUE), iocb_wrapper{}, ttadec_mem{}, TTA(nullptr), tta_info{}, signature(sig_number)
{

	iocb_wrapper.handle = INVALID_HANDLE_VALUE;
	iocb_wrapper.iocb.read = nullptr;
	iocb_wrapper.iocb.seek = nullptr;
	iocb_wrapper.iocb.write = nullptr;

	pos = 0;

	::InitializeCriticalSection(&CriticalSection);
}

DecodeFile::~DecodeFile(void)
{
	::EnterCriticalSection(&CriticalSection);

	if (INVALID_HANDLE_VALUE != decoderFileHANDLE)
	{
		::CloseHandle(decoderFileHANDLE);
		decoderFileHANDLE = INVALID_HANDLE_VALUE;
	}
	else
	{
		// Do nothing
	}

	paused = 0;
	seek_needed = -1;
	decode_pos_ms = 0;
	bitrate = 0;
	Filesize = 0;
	st_state = 0;
	pos = 0;

	iocb_wrapper.handle = INVALID_HANDLE_VALUE;
	iocb_wrapper.iocb.read = nullptr;
	iocb_wrapper.iocb.seek = nullptr;
	iocb_wrapper.iocb.write = nullptr;

	signature = -1;

	if (nullptr != TTA)
	{
		reinterpret_cast<tta::tta_decoder*>(&ttadec_mem)->~tta_decoder();
		TTA = nullptr;
	}
	else
	{
		// Do nothing
	}

	::LeaveCriticalSection(&CriticalSection);

	::DeleteCriticalSection(&CriticalSection);

}

int DecodeFile::SetFileName(const wchar_t *filename)
{
	::EnterCriticalSection(&CriticalSection);

	// check for required data presented
	if (!filename)
	{
		throw DecodeFile_exception(TTA_OPEN_ERROR);
	}
	else
	{
		// Do nothing
	}

	FileName = filename;
	decoderFileHANDLE = CreateFileW(FileName.c_str(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE,
		nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);

	if (decoderFileHANDLE == INVALID_HANDLE_VALUE || decoderFileHANDLE == nullptr)
	{
		::LeaveCriticalSection(&CriticalSection);
		throw DecodeFile_exception(TTA_OPEN_ERROR);
	}
	else
	{
		// Do nothing
	}

	Filesize = (long)::GetFileSize(decoderFileHANDLE, nullptr);

	iocb_wrapper.handle = decoderFileHANDLE;
	iocb_wrapper.iocb.read = &read_callback;
	iocb_wrapper.iocb.seek = &seek_callback;

	if (nullptr != TTA)
	{
		reinterpret_cast<tta::tta_decoder*>(&ttadec_mem)->~tta_decoder();
		TTA = nullptr;
	}
	else
	{
		// Do nothing
	}

	try 
	{
		TTA = new (&ttadec_mem) tta::tta_decoder((TTA_io_callback *)&iocb_wrapper);
		TTA->init_get_info(&tta_info, 0);
	}

	catch (tta::tta_exception &ex)
	{
		if (nullptr != TTA)
		{
			reinterpret_cast<tta::tta_decoder*>(TTA)->~tta_decoder();
			TTA = nullptr;
		}
		else
		{
			// Do nothing
		}

		::CloseHandle(decoderFileHANDLE);
		decoderFileHANDLE = INVALID_HANDLE_VALUE;
		::LeaveCriticalSection(&CriticalSection);
		throw DecodeFile_exception(ex.code());
	}

	paused = 0;
	decode_pos_ms = 0;
	seek_needed = -1;

	// Filesize / (total samples * number of channel) = datasize per sample [byte/sample]
	// datasize per sample * 8 * samples per sec = bitrate [bit/sec]
	bitrate = (long)(Filesize / (tta_info.samples * tta_info.nch) * 8 * tta_info.sps / 1000);

	if (TTA->seek_allowed)
	{
		st_state = 1;
	}
	else
	{
		st_state = 0;
	}

	::LeaveCriticalSection(&CriticalSection);

	return TTA_NO_ERROR;
}

long double DecodeFile::SeekPosition(int *done)
{

	::EnterCriticalSection(&CriticalSection);

	TTAuint32 new_pos;

	if (seek_needed >= GetLengthbymsec())
	{
		decode_pos_ms = GetLengthbymsec();
		*done = 1;
	}
	else
	{
		decode_pos_ms = seek_needed;
		seek_needed = -1;
	}

	if (nullptr == TTA)
	{
		::LeaveCriticalSection(&CriticalSection);
		return (double)0;
	}
	else
	{
		// Do nothing
	}

	try
	{
		TTA->set_position((TTAuint32)(decode_pos_ms / 1000.), &new_pos);
	}

	catch (tta::tta_exception &ex)
	{
		::LeaveCriticalSection(&CriticalSection);
		throw DecodeFile_exception(ex.code());
	}

	::LeaveCriticalSection(&CriticalSection);

	return decode_pos_ms;
}

int  DecodeFile::GetSamples(BYTE *buffer, size_t buffersize, int *current_bitrate)
{
	int skip_len = 0;
	int len = 0;


	if (INVALID_HANDLE_VALUE == decoderFileHANDLE || nullptr == buffer || 0 == buffersize)
	{
		return 0; // no decode data
	}
	else
	{
		// Do nothing
	}

	::EnterCriticalSection(&CriticalSection);

	if (nullptr == TTA)
	{
		throw DecodeFile_exception(TTA_MEMORY_ERROR);
	}
	else
	{
		// Do nothing
	}

	try 
	{
		len = TTA->process_stream(buffer, buffersize);
	}

	catch (tta::tta_exception &ex)
	{
		throw DecodeFile_exception(ex.code());
	}

	if (len != 0)
	{
		skip_len += len;
		decode_pos_ms += (__int32)(skip_len * 1000. / tta_info.sps);
		*current_bitrate = (int) TTA->get_rate();
	}
	else
	{
		// Do nothing
	}

	::LeaveCriticalSection(&CriticalSection);

	return len;

}

void DecodeFile::SetOutputBPS(unsigned long bps)
{
	::EnterCriticalSection(&CriticalSection);

	if (nullptr == TTA)
	{
		throw DecodeFile_exception(TTA_MEMORY_ERROR);
	}
	else
	{
		// Do nothing
	}

	try
	{
		tta_info.bps = bps;
		TTA->init_set_info(&tta_info);
	}

	catch (tta::tta_exception &ex)
	{
		throw DecodeFile_exception(ex.code());
	}

	::LeaveCriticalSection(&CriticalSection);

}
