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

// ttaTag.cpp: Implementation of ttaTag class
//
//////////////////////////////////////////////////////////////////////
#include "ttaTag.h"
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
#include <stdlib.h>

#include "ID3v2TagExtension.h"

std::wstring tagName[] =
{
	L"length",
	L"formatinformation",
	L"type",
	L"family",
	L"lossless",
	L"title",
	L"artist",
	L"albumartist",
	L"comment",
	L"album",
	L"year",
	L"genre",
	L"track",
	L"composer",
	L"publisher",
	L"disc",
	L"bpm",
	L"bitrate",
};

const __int32 METATAG_TAGTYPE_MAX = 18;

static const __int32 MAX_MUSICTEXT = 512;
static const __int32 MAX_YEAR = 10;


//////////////////////////////////////////////////////////////////////
// Create / Destroy
//////////////////////////////////////////////////////////////////////

ttaTag::ttaTag()
{
	::InitializeCriticalSection(&m_CriticalSection);

	FlushCache();
}

ttaTag::~ttaTag()
{
	FlushCache();

	::DeleteCriticalSection(&m_CriticalSection);

}

void ttaTag::FlushCache()
{
	::EnterCriticalSection(&m_CriticalSection);

	m_GetTagTime = 0;

	m_Length = 0;
	m_Tag.clear();

	m_FileName = L"";

	m_isValidFile = false;

	::LeaveCriticalSection(&m_CriticalSection);
}

bool ttaTag::SetFileName(const std::wstring fn)
{
	return GetTagInfo(fn);
}

size_t ttaTag::GetTagLength(const char* Metadata)
{
	wchar_t temp[MAX_MUSICTEXT];
	size_t size;
	mbstowcs_s(&size, temp, MAX_MUSICTEXT, Metadata, MAX_MUSICTEXT);
	if (size == -1 || size == MAX_MUSICTEXT)
	{
		return 0;
	}
	else
	{
		if (m_Tag.contains(temp))
		{
			return static_cast<size_t>(m_Tag.at(temp).length() * sizeof(wchar_t));
		}
		else
		{
			return 0;
		}

	}
	return 0;

}
bool ttaTag::GetTagInfo(const std::wstring fn)
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

int ttaTag::GetExtendedFileInfo(const wchar_t *fn, const char *Metadata, wchar_t *dest, size_t destlen)
{

	bool FindTag= false;
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
	else if (m_isValidFile)
	{
		FindTag = true;
	}
	else
	{
		// Do nothing
	}

	if (FindTag) {
		wchar_t	Buff[MAX_MUSICTEXT] = {};

		wchar_t temp[MAX_MUSICTEXT];
		size_t size;
		mbstowcs_s(&size, temp, MAX_MUSICTEXT, Metadata, MAX_MUSICTEXT);
		if (size == -1 || size == MAX_MUSICTEXT)
		{
			// Do nothing
		}
		else
		{
			if (_wcsicmp(temp, tagName[METATAG_LENGTH].c_str()) == 0)
			{
				_ultow_s(m_Length, dest, destlen, 10);
				RetCode = 1;
			}
			else if (_wcsicmp(temp, tagName[METATAG_TYPE].c_str()) == 0)
			{
				Buff[0] = '0';
				Buff[1] = 0;
				wcsncpy_s(dest, destlen, Buff, _TRUNCATE);
				RetCode = 1;
			}
			else if (_wcsicmp(temp, tagName[METATAG_FAMILY].c_str()) == 0)
			{
				wcsncpy_s(dest, destlen, L"The True Audio File", _TRUNCATE);
				RetCode = 1;
			}
			else if (_wcsicmp(temp, tagName[METATAG_LOSSLESS].c_str()) == 0)
			{
				Buff[0] = '1';
				wcsncpy_s(dest, destlen, Buff, _TRUNCATE);
				RetCode = 1;
			}
			else if (m_Tag.contains(temp))
			{
				wcsncpy_s(dest, destlen, m_Tag.at(temp).c_str(), _TRUNCATE);
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
		m_FileName = L"";
		RetCode = 0;
	}

	::LeaveCriticalSection(&m_CriticalSection);
	return RetCode;
}

int ttaTag::SetExtendedFileInfo(const wchar_t *fn, const char *Metadata, const wchar_t *val)
{

	bool FindTag = false;
	int RetCode = 0;

	::EnterCriticalSection(&m_CriticalSection);

	if (std::wstring(fn) != m_FileName)
	{
		FlushCache();

		FindTag = GetTagInfo(fn);
		if (FindTag && m_isValidFile)
		{
			m_FileName = std::wstring(fn);
		}
		else
		{
			// Do nothing
		}
	}
	else if (m_isValidFile)
	{
		FindTag = true;
	}
	else
	{
		// Do nothing
	}

	if (FindTag)
	{
		wchar_t temp[MAX_MUSICTEXT];
		size_t size;
		mbstowcs_s(&size, temp, MAX_MUSICTEXT, Metadata, MAX_MUSICTEXT);

		if (size == -1 || size == MAX_MUSICTEXT)
		{
			// Do nothing
		}
		else
		{
			if (*val != L'\0')
			{
				if (_wcsicmp(temp, tagName[METATAG_LENGTH].c_str()) == 0)
				{
					// Do nothig
				}
				else if (_wcsicmp(temp, tagName[METATAG_TYPE].c_str()) == 0)
				{
					// Do nothig
				}
				else if (_wcsicmp(temp, tagName[METATAG_FAMILY].c_str()) == 0)
				{
					// Do nothig
				}
				else if (_wcsicmp(temp, tagName[METATAG_LOSSLESS].c_str()) == 0)
				{
					// Do nothig
				}
				else
				{
					m_Tag[temp] = val;
					RetCode = 1;
				}
			}
			else
			{
				auto it = m_Tag.find(temp);
				if (it != m_Tag.end())
				{
					m_Tag.erase(it);
					RetCode = 1;
				}
				else
				{
					RetCode = 0;
				}
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

int ttaTag::WriteExtendedFileInfo()
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
