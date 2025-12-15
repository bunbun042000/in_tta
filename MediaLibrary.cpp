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
	::InitializeCriticalSection(&CriticalSection);

	FlushCache();
}

MediaLibrary::~MediaLibrary()
{
	FlushCache();

	::DeleteCriticalSection(&CriticalSection);

}

void MediaLibrary::FlushCache(void)
{
	::EnterCriticalSection(&CriticalSection);

	GetTagTime = 0;

	TagDataW.Length = 0;
	TagDataW.Format = L"";
	TagDataW.Title = L"";
	TagDataW.Artist = L"";
	TagDataW.Comment = L"";
	TagDataW.Album = L"";
	TagDataW.AlbumArtist = L"";
	TagDataW.Year = L"";
	TagDataW.Genre = L"";
	TagDataW.Track = L"";
	TagDataW.Composer = L"";
	TagDataW.Publisher = L"";
	TagDataW.Disc = L"";
	TagDataW.BPM = L"";

	FileName = L"";

	isValidFile = false;

	::LeaveCriticalSection(&CriticalSection);
}

bool MediaLibrary::GetTagInfo(const std::wstring fn)
{
	if (FileName != fn)
	{
		TagLib::TrueAudio::File TTAFile(fn.c_str());

		if (!TTAFile.isValid())
		{
			return false;
		}
		else
		{
			isValidFile = true;
		}

		TagDataW.Length = (unsigned long)(TTAFile.audioProperties()->lengthInMilliseconds());

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
		TagDataW.Format = ttainfo_temp.str();

		TagDataW.bitrate = std::to_wstring((long long)TTAFile.audioProperties()->bitrate());

		if (nullptr != TTAFile.ID3v2Tag())
		{
			ID3v2TagExtension *Tag_ex = static_cast<ID3v2TagExtension *>(TTAFile.ID3v2Tag());
			TagDataW.Title = Tag_ex->title().toCWString();
			TagDataW.Artist = Tag_ex->artist().toCWString();
			TagDataW.Album = Tag_ex->album().toCWString();
			TagDataW.Comment = Tag_ex->comment().toCWString();
			TagDataW.Genre = Tag_ex->genre().toCWString();
			TagDataW.Year = Tag_ex->stringYear().toCWString();
			TagDataW.Track = Tag_ex->stringTrack().toCWString();
			TagDataW.AlbumArtist = Tag_ex->albumArtist().toCWString();
			TagDataW.Composer = Tag_ex->composers().toCWString();
			TagDataW.Publisher = Tag_ex->publisher().toCWString();
			TagDataW.Disc = Tag_ex->disc().toCWString();
			TagDataW.BPM = Tag_ex->BPM().toCWString();

		}
		else if (nullptr != TTAFile.ID3v1Tag())
		{
			std::wstringstream temp_year;
			std::wstringstream temp_track;
			TagDataW.Title = TTAFile.ID3v1Tag()->title().toCWString();
			TagDataW.Artist = TTAFile.ID3v1Tag()->artist().toCWString();
			TagDataW.Comment = TTAFile.ID3v1Tag()->comment().toCWString();
			TagDataW.Album = TTAFile.ID3v1Tag()->album().toCWString();
			temp_year << TTAFile.ID3v1Tag()->year();
			TagDataW.Year = temp_year.str();
			TagDataW.Genre = TTAFile.ID3v1Tag()->genre().toCWString();
			temp_track << TTAFile.ID3v1Tag()->track();
			TagDataW.Track = temp_track.str();

		}
		else
		{
			// Do nothing.
		}
		FileName = fn;
	}
	else
	{
		// Do nothing
	}

	return true;
}

int MediaLibrary::GetExtendedFileInfo(const wchar_t *fn, const wchar_t *Metadata, wchar_t *dest, size_t destlen)
{

	bool FindTag;
	int RetCode;

	::EnterCriticalSection(&CriticalSection);

	if (std::wstring(fn) != FileName)
	{
		FindTag = GetTagInfo(fn);
		if (FindTag)
		{
			FileName = std::wstring(fn);
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
		const char *MetaData = reinterpret_cast<const char*>(Metadata);

		if (_stricmp(MetaData, "length") == 0)
		{
			_ultow_s(TagDataW.Length, dest, destlen, 10);
			RetCode = 1;
		}
		else if (_stricmp(MetaData, "formatinformation") == 0)
		{
			wcsncpy_s(dest, destlen, TagDataW.Format.c_str(), _TRUNCATE);
			RetCode = 1;
		}
		else if (_stricmp(MetaData, "type") == 0)
		{
			Buff[0] = '0';
			Buff[1] = 0;
			wcsncpy_s(dest, destlen, Buff, _TRUNCATE);
			RetCode = 1;
		}
		else if (_stricmp(MetaData, "family") == 0)
		{
			wcsncpy_s(dest, destlen, L"The True Audio File", _TRUNCATE);
			RetCode = 1;
		}
		else if (_stricmp(MetaData, "lossless") == 0)
		{
			Buff[0] = '1';
			wcsncpy_s(dest, destlen, Buff, _TRUNCATE);
			RetCode = 1;
		}
		else if (_stricmp(MetaData, "title") == 0)
		{
			wcsncpy_s(dest, destlen, TagDataW.Title.c_str(), _TRUNCATE);
			RetCode = 1;
		}
		else if (_stricmp(MetaData, "artist") == 0)
		{
			wcsncpy_s(dest, destlen, TagDataW.Artist.c_str(), _TRUNCATE);
			RetCode = 1;
		}
		else if (_stricmp(MetaData, "albumartist") == 0)
		{
			wcsncpy_s(dest, destlen, TagDataW.AlbumArtist.c_str(), _TRUNCATE);
			RetCode = 1;
		}
		else if (_stricmp(MetaData, "comment") == 0)
		{
			wcsncpy_s(dest, destlen, TagDataW.Comment.c_str(), _TRUNCATE);
			RetCode = 1;
		}
		else if (_stricmp(MetaData, "album") == 0)
		{
			wcsncpy_s(dest, destlen, TagDataW.Album.c_str(), _TRUNCATE);
			RetCode = 1;
		}
		else if (_stricmp(MetaData, "year") == 0)
		{
			wcsncpy_s(dest, destlen, TagDataW.Year.c_str(), _TRUNCATE);
			RetCode = 1;
		}
		else if (_stricmp(MetaData, "genre") == 0)
		{
			wcsncpy_s(dest, destlen, TagDataW.Genre.c_str(), _TRUNCATE);
			RetCode = 1;
		}
		else if (_stricmp(MetaData, "track") == 0)
		{
			wcsncpy_s(dest, destlen, TagDataW.Track.c_str(), _TRUNCATE);
			RetCode = 1;
		}
		else if (_stricmp(MetaData, "tracks") == 0)
		{
			size_t slash_pos = TagDataW.Track.find_first_of(L'/');
			if (slash_pos != std::wstring::npos)
			{
				wcsncpy_s(dest, destlen, TagDataW.Track.substr(slash_pos + 1).c_str(), _TRUNCATE);
			}
			else
			{
				// Do nothing
			}
			RetCode = 1;
		}
		else if (_stricmp(MetaData, "composer") == 0)
		{
			wcsncpy_s(dest, destlen, TagDataW.Composer.c_str(), _TRUNCATE);
			RetCode = 1;
		}
		else if (_stricmp(MetaData, "publisher") == 0)
		{
			wcsncpy_s(dest, destlen, TagDataW.Publisher.c_str(), _TRUNCATE);
			RetCode = 1;
		}
		else if (_stricmp(MetaData, "disc") == 0)
		{
			wcsncpy_s(dest, destlen, TagDataW.Disc.c_str(), _TRUNCATE);
			RetCode = 1;
		}
		else if (_stricmp(MetaData, "discs") == 0)
		{
			size_t slash_pos = TagDataW.Disc.find_first_of(L'/');
			if (slash_pos != std::wstring::npos)
			{
				wcsncpy_s(dest, destlen, TagDataW.Disc.substr(slash_pos + 1).c_str(), _TRUNCATE);
			}
			else
			{
				// Do nothing
			}
			RetCode = 1;
		}
		else if (_stricmp(MetaData, "bpm") == 0)
		{
			wcsncpy_s(dest, destlen, TagDataW.BPM.c_str(), _TRUNCATE);
			RetCode = 1;
		}
		else if (_stricmp(MetaData, "bitrate") == 0)
		{
			wcsncpy_s(dest, destlen, TagDataW.bitrate.c_str(), _TRUNCATE);
			RetCode = 1;
		}
		else
		{
			RetCode = 0;
		}

	}
	else
	{
		FileName = L"";
		RetCode = 0;
	}

	::LeaveCriticalSection(&CriticalSection);
	return RetCode;
}

int MediaLibrary::SetExtendedFileInfo(const wchar_t *fn, const wchar_t *Metadata, const wchar_t *val)
{

	bool FindTag = false;
	int RetCode = 0;

	::EnterCriticalSection(&CriticalSection);

	if (std::wstring(fn) != FileName)
	{
		FindTag = GetTagInfo(fn);
		if (FindTag)
		{
			FileName = std::wstring(fn);
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
		const char *MetaData = reinterpret_cast<const char*>(Metadata);

		if (_stricmp(MetaData, "title") == 0)
		{
			TagDataW.Title = val;
			RetCode = 1;
		}
		else if (_stricmp(MetaData, "artist") == 0)
		{
			TagDataW.Artist = val;
			RetCode = 1;
		}
		else if (_stricmp(MetaData, "albumartist") == 0)
		{
			TagDataW.AlbumArtist = val;
			RetCode = 1;
		}
		else if (_stricmp(MetaData, "comment") == 0)
		{
			TagDataW.Comment = val;
			RetCode = 1;
		}
		else if (_stricmp(MetaData, "album") == 0)
		{
			TagDataW.Album = val;
			RetCode = 1;
		}
		else if (_stricmp(MetaData, "year") == 0)
		{
			TagDataW.Year = val;
			RetCode = 1;
		}
		else if (_stricmp(MetaData, "genre") == 0)
		{
			TagDataW.Genre = val;
			RetCode = 1;
		}
		else if (_stricmp(MetaData, "track") == 0)
		{
			TagDataW.Track = val;
			RetCode = 1;
		}
		else if (_stricmp(MetaData, "composer") == 0)
		{
			TagDataW.Composer = val;
			RetCode = 1;
		}
		else if (_stricmp(MetaData, "publisher") == 0)
		{
			TagDataW.Publisher = val;
			RetCode = 1;
		}
		else if (_stricmp(MetaData, "disc") == 0)
		{
			TagDataW.Disc = val;
			RetCode = 1;
		}
		else if (_stricmp(MetaData, "bpm") == 0)
		{
			TagDataW.BPM = val;
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

	::LeaveCriticalSection(&CriticalSection);
	return RetCode;
}

int MediaLibrary::WriteExtendedFileInfo()
{

	::EnterCriticalSection(&CriticalSection);

	if (FileName.empty())
	{
		::LeaveCriticalSection(&CriticalSection);
		return 0;
	}
	else
	{
		TagLib::TrueAudio::File TTAFile(FileName.c_str());

		if (!TTAFile.isValid())
		{
			::LeaveCriticalSection(&CriticalSection);
			return 0;
		}
		else
		{
			// Do nothing
		}

		if (nullptr != TTAFile.ID3v2Tag(true))
		{
			ID3v2TagExtension* Tag_ex = static_cast<ID3v2TagExtension*>(TTAFile.ID3v2Tag());
			Tag_ex->setTitle(TagDataW.Title);
			Tag_ex->setArtist(TagDataW.Artist);
			Tag_ex->setAlbum(TagDataW.Album);
			Tag_ex->setComment(TagDataW.Comment);
			Tag_ex->setGenre(TagDataW.Genre);
			Tag_ex->setStringYear(TagDataW.Year);
			Tag_ex->setStringTrack(TagDataW.Track);
			Tag_ex->setAlbumArtist(TagDataW.AlbumArtist);
			Tag_ex->setComposers(TagDataW.Composer);
			Tag_ex->setPublisher(TagDataW.Publisher);
			Tag_ex->setDisc(TagDataW.Disc);
			Tag_ex->setBPM(TagDataW.BPM);

		}
		else if (nullptr != TTAFile.ID3v1Tag(true))
		{
			TTAFile.ID3v1Tag()->setTitle(TagDataW.Title);
			TTAFile.ID3v1Tag()->setArtist(TagDataW.Artist);
			TTAFile.ID3v1Tag()->setAlbum(TagDataW.Album);
			TTAFile.ID3v1Tag()->setComment(TagDataW.Comment);
			TTAFile.ID3v1Tag()->setYear((unsigned int)_wtoi(TagDataW.Year.c_str()));
			TTAFile.ID3v1Tag()->setTrack((unsigned int)_wtoi(TagDataW.Track.c_str()));
			TTAFile.ID3v1Tag()->setGenre(TagDataW.Genre);
		}
		else
		{
			// Do nothing.
		}
		TTAFile.save();
	}


	::LeaveCriticalSection(&CriticalSection);

	return 1;
}
