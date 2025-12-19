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

// MediaLibrary.cpp: Implementation of MediaLibrary class
//
//////////////////////////////////////////////////////////////////////
#include "MediaLibrary.h"
#include "resource.h"
#include <taglib/trueaudiofile.h>
#include <taglib/tag.h>
#include <taglib/id3v2tag.h>
#include <taglib/id3v1tag.h>
#include <taglib/attachedpictureframe.h>
#include <taglib/id3v2frame.h>
#include <sstream>
#include <iomanip>
#include <strsafe.h>
#include <map>

#include "ID3v2TagExtension.h"

enum
{
	METATAG_LENGTH,
	METATAG_FORMATINFORMATION,
	METATAG_TYPE,
	METATAG_FAMILY,
	METATAG_LOSSLESS,
	METATAG_TITLE,
	METATAG_ARTIST,
	METATAG_ALBUMARTIST,
	METATAG_COMMENT,
	METATAG_ALBUM,
	METATAG_YEAR,
	METATAG_GENRE,
	METATAG_TRACK,
	METATAG_COMPOSER,
	METATAG_PUBLISHER,
	METATAG_DISC,
	METATAG_BPM,
	METATAG_BITRATE,
};

static std::string tagName[] =
{
	"length",
	"formatinformation",
	"type",
	"family",
	"lossless",
	"title",
	"artist",
	"albumartist",
	"comment",
	"album",
	"year",
	"genre",
	"track",
	"composer",
	"publisher",
	"disc",
	"bpm",
	"bitrate",
};


//////////////////////////////////////////////////////////////////////
// Create / Destroy
//////////////////////////////////////////////////////////////////////

MediaLibrary::MediaLibrary()
{
	::InitializeCriticalSection(&m_CriticalSection);

	FlushCache();
}

MediaLibrary::~MediaLibrary()
{
	FlushCache();

	::DeleteCriticalSection(&m_CriticalSection);

}

void MediaLibrary::FlushCache()
{
	::EnterCriticalSection(&m_CriticalSection);

	m_GetTagTime = 0;

	m_Length = 0;
	m_Tag.clear();

	m_FileName = L"";

	m_isValidFile = false;

	::LeaveCriticalSection(&m_CriticalSection);
}

bool MediaLibrary::GetTagInfo(const std::wstring fn)
{
	if (m_FileName != fn)
	{
		TagLib::TrueAudio::File TTAFile(fn.c_str());

		if (!TTAFile.isValid())
		{
			return false;
		}
		else
		{
			m_isValidFile = true;
		}

		m_Length = static_cast<unsigned long>(TTAFile.audioProperties()->lengthInMilliseconds());

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
		m_Tag.insert(std::make_pair(tagName[METATAG_FORMATINFORMATION], ttainfo_temp.str()));

		m_Tag.insert(std::make_pair(tagName[METATAG_BITRATE], std::to_wstring(static_cast<long long>(TTAFile.audioProperties()->bitrate()))));

		if (nullptr != TTAFile.ID3v2Tag())
		{
			ID3v2TagExtension *Tag_ex = static_cast<ID3v2TagExtension *>(TTAFile.ID3v2Tag());
			m_Tag.insert(std::make_pair(tagName[METATAG_TITLE], Tag_ex->title().toCWString()));
			m_Tag.insert(std::make_pair(tagName[METATAG_ARTIST], Tag_ex->artist().toCWString()));
			m_Tag.insert(std::make_pair(tagName[METATAG_ALBUM], Tag_ex->album().toCWString()));
			m_Tag.insert(std::make_pair(tagName[METATAG_COMMENT], Tag_ex->comment().toCWString()));
			m_Tag.insert(std::make_pair(tagName[METATAG_GENRE], Tag_ex->genre().toCWString()));
			m_Tag.insert(std::make_pair(tagName[METATAG_YEAR], Tag_ex->stringYear().toCWString()));
			m_Tag.insert(std::make_pair(tagName[METATAG_TRACK], Tag_ex->stringTrack().toCWString()));
			m_Tag.insert(std::make_pair(tagName[METATAG_ALBUMARTIST], Tag_ex->albumArtist().toCWString()));
			m_Tag.insert(std::make_pair(tagName[METATAG_COMPOSER], Tag_ex->composers().toCWString()));
			m_Tag.insert(std::make_pair(tagName[METATAG_PUBLISHER], Tag_ex->publisher().toCWString()));
			m_Tag.insert(std::make_pair(tagName[METATAG_DISC], Tag_ex->disc().toCWString()));
			m_Tag.insert(std::make_pair(tagName[METATAG_BPM], Tag_ex->BPM().toCWString()));

		}
		else if (nullptr != TTAFile.ID3v1Tag())
		{
			std::wstringstream temp_year;
			std::wstringstream temp_track;
			m_Tag.insert(std::make_pair(tagName[METATAG_TITLE], TTAFile.ID3v1Tag()->title().toCWString()));
			m_Tag.insert(std::make_pair(tagName[METATAG_ARTIST], TTAFile.ID3v1Tag()->artist().toCWString()));
			m_Tag.insert(std::make_pair(tagName[METATAG_COMMENT], TTAFile.ID3v1Tag()->comment().toCWString()));
			m_Tag.insert(std::make_pair(tagName[METATAG_ALBUM], TTAFile.ID3v1Tag()->album().toCWString()));
			temp_year << TTAFile.ID3v1Tag()->year();
			m_Tag.insert(std::make_pair(tagName[METATAG_YEAR], temp_year.str()));
			m_Tag.insert(std::make_pair(tagName[METATAG_GENRE], TTAFile.ID3v1Tag()->genre().toCWString()));
			temp_track << TTAFile.ID3v1Tag()->track();
			m_Tag.insert(std::make_pair(tagName[METATAG_TRACK], temp_track.str()));

		}
		else
		{
			// Do nothing.
		}
		m_FileName = fn;
	}
	else
	{
		// Do nothing
	}

	return true;
}

int MediaLibrary::GetExtendedFileInfo(const wchar_t *fn, const char *Metadata, wchar_t *dest, size_t destlen)
{

	bool FindTag;
	int RetCode;

	::EnterCriticalSection(&m_CriticalSection);

	if (std::wstring(fn) != m_FileName)
	{
		FlushCache();

		FindTag = GetTagInfo(fn);
		if (FindTag)
		{
			m_FileName = std::wstring(fn);
		}
		else
		{
			// Do nothing
		}
	}
	else
	{
		FindTag = true;
	}

	if (FindTag) {
		wchar_t	Buff[MAX_MUSICTEXT] = {};

		if (_stricmp(Metadata, tagName[METATAG_LENGTH].c_str()) == 0)
		{
			_ultow_s(m_Length, dest, destlen, 10);
			RetCode = 1;
		}
		else if (_stricmp(Metadata, tagName[METATAG_TYPE].c_str()) == 0)
		{
			Buff[0] = '0';
			Buff[1] = 0;
			wcsncpy_s(dest, destlen, Buff, _TRUNCATE);
			RetCode = 1;
		}
		else if (_stricmp(Metadata, tagName[METATAG_FAMILY].c_str()) == 0)
		{
			wcsncpy_s(dest, destlen, L"The True Audio File", _TRUNCATE);
			RetCode = 1;
		}
		else if (_stricmp(Metadata, tagName[METATAG_LOSSLESS].c_str()) == 0)
		{
			Buff[0] = '1';
			wcsncpy_s(dest, destlen, Buff, _TRUNCATE);
			RetCode = 1;
		}
		else if (m_Tag.contains(Metadata))
		{
			wcsncpy_s(dest, destlen, m_Tag.at(Metadata).c_str(), _TRUNCATE);
			RetCode = 1;
		}
		else
		{
			RetCode = 0;
		}

	}
	else
	{
		m_FileName = L"";
		RetCode = 0;
	}

	::LeaveCriticalSection(&m_CriticalSection);
	return RetCode;
}

int MediaLibrary::SetExtendedFileInfo(const wchar_t *fn, const char *Metadata, const wchar_t *val)
{

	bool FindTag = false;
	int RetCode = 0;

	::EnterCriticalSection(&m_CriticalSection);

	if (std::wstring(fn) != m_FileName)
	{
		FlushCache();

		FindTag = GetTagInfo(fn);
		if (FindTag)
		{
			m_FileName = std::wstring(fn);
		}
		else
		{
			// Do nothing
		}
	}
	else
	{
		FindTag = true;
	}

	if (FindTag)
	{
		if (*val != L'\0')
		{
			if (_stricmp(Metadata, tagName[METATAG_LENGTH].c_str()) == 0)
			{
				// Do nothig
			}
			else if (_stricmp(Metadata, tagName[METATAG_TYPE].c_str()) == 0)
			{
				// Do nothig
			}
			else if (_stricmp(Metadata, tagName[METATAG_FAMILY].c_str()) == 0)
			{
				// Do nothig
			}
			else if (_stricmp(Metadata, tagName[METATAG_LOSSLESS].c_str()) == 0)
			{
				// Do nothig
			}
			else
			{
				m_Tag[Metadata] = val;
				RetCode = 1;
			}
		}
		else
		{
			auto it = m_Tag.find(Metadata);
			if (it != m_Tag.end())
			{
				m_Tag.erase(m_Tag.find(Metadata));
				RetCode = 1;
			}
			else
			{
				RetCode = 0;
			}
		}
	}
	else
	{
		RetCode = 0;
	}

	::LeaveCriticalSection(&m_CriticalSection);
	return RetCode;
}

int MediaLibrary::WriteExtendedFileInfo()
{

	::EnterCriticalSection(&m_CriticalSection);

	if (m_FileName.empty())
	{
		::LeaveCriticalSection(&m_CriticalSection);
		return 0;
	}
	else
	{
		TagLib::TrueAudio::File TTAFile(m_FileName.c_str());

		if (!TTAFile.isValid())
		{
			::LeaveCriticalSection(&m_CriticalSection);
			return 0;
		}
		else
		{
			// Do nothing
		}

		if (nullptr != TTAFile.ID3v2Tag(true))
		{
			ID3v2TagExtension* Tag_ex = static_cast<ID3v2TagExtension*>(TTAFile.ID3v2Tag());
			Tag_ex->setTitle(m_Tag[tagName[METATAG_TITLE]]);
			Tag_ex->setArtist(m_Tag[tagName[METATAG_ARTIST]]);
			Tag_ex->setAlbum(m_Tag[tagName[METATAG_ALBUM]]);
			Tag_ex->setComment(m_Tag[tagName[METATAG_COMMENT]]);
			Tag_ex->setGenre(m_Tag[tagName[METATAG_GENRE]]);
			Tag_ex->setStringYear(m_Tag[tagName[METATAG_YEAR]]);
			Tag_ex->setStringTrack(m_Tag[tagName[METATAG_TRACK]]);
			Tag_ex->setAlbumArtist(m_Tag[tagName[METATAG_ALBUMARTIST]]);
			Tag_ex->setComposers(m_Tag[tagName[METATAG_COMPOSER]]);
			Tag_ex->setPublisher(m_Tag[tagName[METATAG_PUBLISHER]]);
			Tag_ex->setDisc(m_Tag[tagName[METATAG_DISC]]);
			Tag_ex->setBPM(m_Tag[tagName[METATAG_BPM]]);

		}
		else if (nullptr != TTAFile.ID3v1Tag(true))
		{
			TTAFile.ID3v1Tag()->setTitle(m_Tag[tagName[METATAG_TITLE]]);
			TTAFile.ID3v1Tag()->setArtist(m_Tag[tagName[METATAG_ARTIST]]);
			TTAFile.ID3v1Tag()->setAlbum(m_Tag[tagName[METATAG_ALBUM]]);
			TTAFile.ID3v1Tag()->setComment(m_Tag[tagName[METATAG_COMMENT]]);
			TTAFile.ID3v1Tag()->setYear(static_cast<unsigned int>(_wtoi(m_Tag[tagName[METATAG_YEAR]].c_str())));
			TTAFile.ID3v1Tag()->setTrack(static_cast<unsigned int>(_wtoi(m_Tag[tagName[METATAG_TRACK]].c_str())));
			TTAFile.ID3v1Tag()->setGenre(m_Tag[tagName[METATAG_GENRE]]);
		}
		else
		{
			// Do nothing.
		}
		TTAFile.save();
	}


	::LeaveCriticalSection(&m_CriticalSection);

	return 1;
}
