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

#include "ID3v2TagExtension.h"

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
			ID3v2TagExtension *Tag_ex = static_cast<ID3v2TagExtension *>(TTAFile.ID3v2Tag());
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
		else if (_stricmp(Metadata, "tracks") == 0)
		{
			size_t slash_pos = m_TagDataW.Track.find_first_of(L'/');
			if (slash_pos != std::wstring::npos)
			{
				wcsncpy_s(dest, destlen, m_TagDataW.Track.substr(slash_pos + 1).c_str(), _TRUNCATE);
			}
			else
			{
				// Do nothing
			}
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
		else if (_stricmp(Metadata, "discs") == 0)
		{
			size_t slash_pos = m_TagDataW.Disc.find_first_of(L'/');
			if (slash_pos != std::wstring::npos)
			{
				wcsncpy_s(dest, destlen, m_TagDataW.Disc.substr(slash_pos + 1).c_str(), _TRUNCATE);
			}
			else
			{
				// Do nothing
			}
			RetCode = 1;
		}
		else if (_stricmp(Metadata, "bpm") == 0)
		{
			wcsncpy_s(dest, destlen, m_TagDataW.BPM.c_str(), _TRUNCATE);
			RetCode = 1;
		}
		else if (_stricmp(Metadata, "bitrate") == 0)
		{
			wcsncpy_s(dest, destlen, m_TagDataW.bitrate.c_str(), _TRUNCATE);
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

		if (_stricmp(Metadata, "title") == 0)
		{
			m_TagDataW.Title = val;
			RetCode = 1;
		}
		else if (_stricmp(Metadata, "artist") == 0)
		{
			m_TagDataW.Artist = val;
			RetCode = 1;
		}
		else if (_stricmp(Metadata, "albumartist") == 0)
		{
			m_TagDataW.AlbumArtist = val;
			RetCode = 1;
		}
		else if (_stricmp(Metadata, "comment") == 0)
		{
			m_TagDataW.Comment = val;
			RetCode = 1;
		}
		else if (_stricmp(Metadata, "album") == 0)
		{
			m_TagDataW.Album = val;
			RetCode = 1;
		}
		else if (_stricmp(Metadata, "year") == 0)
		{
			m_TagDataW.Year = val;
			RetCode = 1;
		}
		else if (_stricmp(Metadata, "genre") == 0)
		{
			m_TagDataW.Genre = val;
			RetCode = 1;
		}
		else if (_stricmp(Metadata, "track") == 0)
		{
			m_TagDataW.Track = val;
			RetCode = 1;
		}
		else if (_stricmp(Metadata, "composer") == 0)
		{
			m_TagDataW.Composer = val;
			RetCode = 1;
		}
		else if (_stricmp(Metadata, "publisher") == 0)
		{
			m_TagDataW.Publisher = val;
			RetCode = 1;
		}
		else if (_stricmp(Metadata, "disc") == 0)
		{
			m_TagDataW.Disc = val;
			RetCode = 1;
		}
		else if (_stricmp(Metadata, "bpm") == 0)
		{
			m_TagDataW.BPM = val;
			RetCode = 1;
		}
		else
		{
			RetCode = 0;
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
			Tag_ex->setTitle(m_TagDataW.Title);
			Tag_ex->setArtist(m_TagDataW.Artist);
			Tag_ex->setAlbum(m_TagDataW.Album);
			Tag_ex->setComment(m_TagDataW.Comment);
			Tag_ex->setGenre(m_TagDataW.Genre);
			Tag_ex->setStringYear(m_TagDataW.Year);
			Tag_ex->setStringTrack(m_TagDataW.Track);
			Tag_ex->setAlbumArtist(m_TagDataW.AlbumArtist);
			Tag_ex->setComposers(m_TagDataW.Composer);
			Tag_ex->setPublisher(m_TagDataW.Publisher);
			Tag_ex->setDisc(m_TagDataW.Disc);
			Tag_ex->setBPM(m_TagDataW.BPM);

		}
		else if (nullptr != TTAFile.ID3v1Tag(true))
		{
			TTAFile.ID3v1Tag()->setTitle(m_TagDataW.Title);
			TTAFile.ID3v1Tag()->setArtist(m_TagDataW.Artist);
			TTAFile.ID3v1Tag()->setAlbum(m_TagDataW.Album);
			TTAFile.ID3v1Tag()->setComment(m_TagDataW.Comment);
			TTAFile.ID3v1Tag()->setYear(static_cast<unsigned int>(_wtoi(m_TagDataW.Year.c_str())));
			TTAFile.ID3v1Tag()->setTrack(static_cast<unsigned int>(_wtoi(m_TagDataW.Track.c_str())));
			TTAFile.ID3v1Tag()->setGenre(m_TagDataW.Genre);
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
