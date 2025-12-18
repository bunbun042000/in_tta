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

#include <agave/Metadata/svc_metatag.h>
#include <taglib/tag.h>
#include <taglib/trueaudiofile.h>
#include <taglib/id3v2tag.h>
#include <taglib/id3v1tag.h>
#include <taglib/id3v2frame.h>

#include <sstream>
#include <iomanip>
#include <strsafe.h>

#include "agaveCommon.h"
#include "ID3v2TagExtension.h"

struct TagInfo
{
	unsigned long	Length;
	std::wstring	Format;
	std::wstring	Title;
	std::wstring	Artist;
	std::wstring	AlbumArtist;
	std::wstring	Comment;
	std::wstring	Album;
	std::wstring	Year;
	std::wstring	Genre;
	std::wstring	Track;
	std::wstring	Composer;
	std::wstring	Publisher;
	std::wstring	Disc;
	std::wstring	BPM;
	std::wstring    bitrate;
};

class TTA_MetaData : public svc_metaTag
{
	public:
	TTA_MetaData();
	virtual ~TTA_MetaData();

	static FOURCC getServiceType() { return svc_metaTag::SERVICETYPE; }
	const wchar_t* getName();	// i.e. "ID3v2" or something
	GUID getGUID(); // this needs to be the same GUID that you use when registering your service factory
	int getFlags(); // how this service gets its info
	int isOurFile(const wchar_t* filename);
	int metaTag_open(const wchar_t* filename);
	void metaTag_close(); // self-destructs when this is called (you don't need to call serviceFactory->releaseInterface)

	/* user API starts here */
	const wchar_t* enumSupportedTag(int n, int* datatype = NULL);	// returns a list of understood tags. might not be complete (see note [1])
	int getTagSize(const wchar_t* tag, size_t* sizeBytes); // always gives you BYTES, not characters (be careful with your strings)
	int getMetaData(const wchar_t* tag, uint8_t* buf, int buflenBytes, int datatype = METATYPE_STRING); // buflen is BYTES, not characters (be careful with your strings)
	int setMetaData(const wchar_t* tag, const uint8_t* buf, int buflenBytes, int datatype = METATYPE_STRING);

private:
	void FlushCache();

private:
	CRITICAL_SECTION	m_CriticalSection;
	TagInfo				m_TagDataW{};
	DWORD				m_GetTagTime;
	std::wstring		m_FileName;
	bool				m_isValidFile;

};

// {50846701-71A9-40CF-9165-587D3A7DB325}
static const GUID TTA_MetaData_GUID =
{ 0x50846701, 0x71a9, 0x40cf, { 0x91, 0x65, 0x58, 0x7d, 0x3a, 0x7d, 0xb3, 0x25 } };

TTA_MetaData::TTA_MetaData() : svc_metaTag()
{
	::InitializeCriticalSection(&m_CriticalSection);

	FlushCache();
}

TTA_MetaData::~TTA_MetaData()
{
	FlushCache();

	::DeleteCriticalSection(&m_CriticalSection);
}

void TTA_MetaData::FlushCache()
{
	::EnterCriticalSection(&m_CriticalSection);

	m_GetTagTime = 0;

	m_TagDataW.Length = 0;
	m_TagDataW.Format = L"";
	m_TagDataW.Title = L"";
	m_TagDataW.Artist = L"";
	m_TagDataW.Comment = L"";
	m_TagDataW.Album = L"";
	m_TagDataW.AlbumArtist = L"";
	m_TagDataW.Year = L"";
	m_TagDataW.Genre = L"";
	m_TagDataW.Track = L"";
	m_TagDataW.Composer = L"";
	m_TagDataW.Publisher = L"";
	m_TagDataW.Disc = L"";
	m_TagDataW.BPM = L"";

	m_FileName = L"";

	m_isValidFile = false;

	::LeaveCriticalSection(&m_CriticalSection);
}

const wchar_t* TTA_MetaData::getName()
{
	return L"ID3v2";
}

GUID TTA_MetaData::getGUID()
{
	return TTA_MetaData_GUID;
}

int TTA_MetaData::getFlags()
{
	return METATAG_FILE_INFO;
}

int TTA_MetaData::isOurFile(const wchar_t* filename)
{
	if (extensionW(filename))
	{
		return ((_wcsicmp(extensionW(filename), L"tta") == 0) || (_wcsicmp(extensionW(filename), L"TTA") == 0)) ? 1 : 0;
	}
	else
	{
		// Do nothing
	}
	return 0;
}

int TTA_MetaData::metaTag_open(const wchar_t* filename)
{
	std::wstring fn(filename);

	if (!fn.compare(m_FileName))
	{
		TagLib::TrueAudio::File TTAFile(fn.c_str());

		if (!TTAFile.isValid())
		{
			return 0;
		}
		else
		{
			m_isValidFile = true;
		}

		m_TagDataW.Length = static_cast<unsigned long>(TTAFile.audioProperties()->lengthInMilliseconds());

		int Lengthbysec = TTAFile.audioProperties()->lengthInSeconds();
		int hour = Lengthbysec / 3600;
		int min = Lengthbysec / 60;
		int sec = Lengthbysec % 60;

		std::wstringstream second;

		if (hour > 0)
		{
			second << std::setw(2) << std::setfill(L'0') << hour << L":" << std::setw(2)
				<< std::setfill(L'0') << min << L":" << std::setw(2) << std::setfill(L'0') << sec;
		}
		else if (min > 0)
		{
			second << std::setw(2) << std::setfill(L'0') << min << L":" << std::setw(2)
				<< std::setfill(L'0') << sec;
		}
		else
		{
			second << std::setw(2) << std::setfill(L'0') << sec;
		}

		std::wstring channel_designation = (TTAFile.audioProperties()->channels() == 2) ? L"Stereo" : L"Monoral";

		std::wstringstream ttainfo_temp;

		ttainfo_temp << L"Format\t\t: TTA" << TTAFile.audioProperties()->ttaVersion()
			<< L"\nSample\t\t: " << (int)TTAFile.audioProperties()->bitsPerSample()
			<< L"bit\nSample Rate\t: " << TTAFile.audioProperties()->sampleRate()
			<< L"Hz\nBit Rate\t\t: " << TTAFile.audioProperties()->bitrate()
			<< L"kbit/s\nNum. of Chan.\t: " << TTAFile.audioProperties()->channels()
			<< L"(" << channel_designation
			<< L")\nLength\t\t: " << second.str();
		m_TagDataW.Format = ttainfo_temp.str();

		m_TagDataW.bitrate = std::to_wstring(static_cast<long long>(TTAFile.audioProperties()->bitrate()));

		if (nullptr != TTAFile.ID3v2Tag())
		{
			ID3v2TagExtension* Tag_ex = static_cast<ID3v2TagExtension*>(TTAFile.ID3v2Tag());
			m_TagDataW.Title = Tag_ex->title().toCWString();
			m_TagDataW.Artist = Tag_ex->artist().toCWString();
			m_TagDataW.Album = Tag_ex->album().toCWString();
			m_TagDataW.Comment = Tag_ex->comment().toCWString();
			m_TagDataW.Genre = Tag_ex->genre().toCWString();
			m_TagDataW.Year = Tag_ex->stringYear().toCWString();
			m_TagDataW.Track = Tag_ex->stringTrack().toCWString();
			m_TagDataW.AlbumArtist = Tag_ex->albumArtist().toCWString();
			m_TagDataW.Composer = Tag_ex->composers().toCWString();
			m_TagDataW.Publisher = Tag_ex->publisher().toCWString();
			m_TagDataW.Disc = Tag_ex->disc().toCWString();
			m_TagDataW.BPM = Tag_ex->BPM().toCWString();

		}
		else if (nullptr != TTAFile.ID3v1Tag())
		{
			std::wstringstream temp_year;
			std::wstringstream temp_track;
			m_TagDataW.Title = TTAFile.ID3v1Tag()->title().toCWString();
			m_TagDataW.Artist = TTAFile.ID3v1Tag()->artist().toCWString();
			m_TagDataW.Comment = TTAFile.ID3v1Tag()->comment().toCWString();
			m_TagDataW.Album = TTAFile.ID3v1Tag()->album().toCWString();
			temp_year << TTAFile.ID3v1Tag()->year();
			m_TagDataW.Year = temp_year.str();
			m_TagDataW.Genre = TTAFile.ID3v1Tag()->genre().toCWString();
			temp_track << TTAFile.ID3v1Tag()->track();
			m_TagDataW.Track = temp_track.str();

		}
		else
		{
			// Do nothing.
		}
		m_FileName = filename;
	}
	else
	{
		// Do nothing
	}

	return 1;

}

void TTA_MetaData::metaTag_close()
{
	// Do nothing
}

const wchar_t* TTA_MetaData::enumSupportedTag(int n, int* datatype = NULL)
{
	if (_stricmp(Metadata, "length") == 0)
	{
		_ultow_s(m_TagDataW.Length, dest, destlen, 10);
		RetCode = 1;
	}
	else if (_stricmp(Metadata, "formatinformation") == 0)
	{
		wcsncpy_s(dest, destlen, m_TagDataW.Format.c_str(), _TRUNCATE);
		RetCode = 1;
	}
	else if (_stricmp(Metadata, "type") == 0)
	{
		Buff[0] = '0';
		Buff[1] = 0;
		wcsncpy_s(dest, destlen, Buff, _TRUNCATE);
		RetCode = 1;
	}
	else if (_stricmp(Metadata, "family") == 0)
	{
		wcsncpy_s(dest, destlen, L"The True Audio File", _TRUNCATE);
		RetCode = 1;
	}
	else if (_stricmp(Metadata, "lossless") == 0)
	{
		Buff[0] = '1';
		wcsncpy_s(dest, destlen, Buff, _TRUNCATE);
		RetCode = 1;
	}
	else if (_stricmp(Metadata, "title") == 0)
	{
		wcsncpy_s(dest, destlen, m_TagDataW.Title.c_str(), _TRUNCATE);
		RetCode = 1;
	}
	else if (_stricmp(Metadata, "artist") == 0)
	{
		wcsncpy_s(dest, destlen, m_TagDataW.Artist.c_str(), _TRUNCATE);
		RetCode = 1;
	}
	else if (_stricmp(Metadata, "albumartist") == 0)
	{
		wcsncpy_s(dest, destlen, m_TagDataW.AlbumArtist.c_str(), _TRUNCATE);
		RetCode = 1;
	}
	else if (_stricmp(Metadata, "comment") == 0)
	{
		wcsncpy_s(dest, destlen, m_TagDataW.Comment.c_str(), _TRUNCATE);
		RetCode = 1;
	}
	else if (_stricmp(Metadata, "album") == 0)
	{
		wcsncpy_s(dest, destlen, m_TagDataW.Album.c_str(), _TRUNCATE);
		RetCode = 1;
	}
	else if (_stricmp(Metadata, "year") == 0)
	{
		wcsncpy_s(dest, destlen, m_TagDataW.Year.c_str(), _TRUNCATE);
		RetCode = 1;
	}
	else if (_stricmp(Metadata, "genre") == 0)
	{
		wcsncpy_s(dest, destlen, m_TagDataW.Genre.c_str(), _TRUNCATE);
		RetCode = 1;
	}
	else if (_stricmp(Metadata, "track") == 0)
	{
		wcsncpy_s(dest, destlen, m_TagDataW.Track.c_str(), _TRUNCATE);
		RetCode = 1;
	}
	else if (_stricmp(Metadata, "composer") == 0)
	{
		wcsncpy_s(dest, destlen, m_TagDataW.Composer.c_str(), _TRUNCATE);
		RetCode = 1;
	}
	else if (_stricmp(Metadata, "publisher") == 0)
	{
		wcsncpy_s(dest, destlen, m_TagDataW.Publisher.c_str(), _TRUNCATE);
		RetCode = 1;
	}
	else if (_stricmp(Metadata, "disc") == 0)
	{
		wcsncpy_s(dest, destlen, m_TagDataW.Disc.c_str(), _TRUNCATE);
		RetCode = 1;
	}
	else if (_stricmp(Metadata, "bpm") == 0)
	{
		wcsncpy_s(dest, destlen, m_TagDataW.BPM.c_str(), _TRUNCATE);
		RetCode = 1;
	}
	else if (_stricmp(Metadata, "bitrate") == 0)

}
