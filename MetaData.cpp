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
#include <Agave/Metadata/api_metadata.h>
#include <Wasabi/bfc/dispatch.h>
#include <Wasabi/api/service/api_service.h>
#include <Wasabi/api/service/waservicefactory.h>
#include <winamp/wa_ipc.h>

#include <taglib/tag.h>
#include <taglib/trueaudiofile.h>
#include <taglib/id3v2tag.h>
#include <taglib/id3v1tag.h>
#include <taglib/id3v2frame.h>

#include <sstream>
#include <iomanip>
#include <strsafe.h>
#include <map>
#include <vector>

#include "agaveCommon.h"
#include "ID3v2TagExtension.h"
#include "MetaData.h"


// {50846701-71A9-40CF-9165-587D3A7DB325}
static const GUID TTA_metaTag_GUID =
{ 0x50846701, 0x71a9, 0x40cf, { 0x91, 0x65, 0x58, 0x7d, 0x3a, 0x7d, 0xb3, 0x25 } };

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

static const std::wstring tagName[] =
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

TTA_metaTag::TTA_metaTag() : svc_metaTag()
{
	::InitializeCriticalSection(&m_CriticalSection);

	FlushCache();
}

TTA_metaTag::~TTA_metaTag()
{
	FlushCache();

	::DeleteCriticalSection(&m_CriticalSection);
}

void TTA_metaTag::FlushCache()
{
	::EnterCriticalSection(&m_CriticalSection);

	m_GetTagTime = 0;

	m_FileName = L"";

	m_isValidFile = false;
	m_isChanged = false;

	m_Tag.clear();

	::LeaveCriticalSection(&m_CriticalSection);
}

const wchar_t* TTA_metaTag::getName()
{
	return L"ID3v2";
}

GUID TTA_metaTag::getGUID()
{
	return TTA_metaTag_GUID;
}

int TTA_metaTag::getFlags()
{
	return static_cast<int>(METATAG_FILE_INFO);
}

int TTA_metaTag::isOurFile(const wchar_t* filename)
{
	if (extensionW(filename))
	{
		return ((_wcsicmp(extensionW(filename), L"tta") == 0) || (_wcsicmp(extensionW(filename), L"TTA") == 0)) ? METATAG_SUCCESS : METATAG_FAILED;
	}
	else
	{
		// Do nothing
	}
	return METATAG_FAILED;
}

int TTA_metaTag::metaTag_open(const wchar_t* filename)
{
	::EnterCriticalSection(&m_CriticalSection);

	std::wstring fn(filename);

	if (!fn.compare(m_FileName))
	{
		TagLib::TrueAudio::File TTAFile(fn.c_str());

		if (!TTAFile.isValid())
		{
			::LeaveCriticalSection(&m_CriticalSection);
			return METATAG_FAILED;
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
		m_Tag.insert(std::make_pair(tagName[METATAG_TYPE], std::wstring(L"0")));
		m_Tag.insert(std::make_pair(tagName[METATAG_FAMILY], std::wstring(L"The True Audio File")));
		m_Tag.insert(std::make_pair(tagName[METATAG_LOSSLESS], std::wstring(L"1")));

		if (nullptr != TTAFile.ID3v2Tag())
		{
			ID3v2TagExtension* Tag_ex = static_cast<ID3v2TagExtension*>(TTAFile.ID3v2Tag());
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
		m_FileName = filename;
	}
	else
	{
		// Do nothing
	}

	::LeaveCriticalSection(&m_CriticalSection);

	return METATAG_SUCCESS;

}

void TTA_metaTag::metaTag_close()
{
	if (m_isChanged)
	{
		::EnterCriticalSection(&m_CriticalSection);

		if (m_FileName.empty())
		{
			::LeaveCriticalSection(&m_CriticalSection);
			return;
		}
		else
		{
			TagLib::TrueAudio::File TTAFile(m_FileName.c_str());

			if (!TTAFile.isValid())
			{
				::LeaveCriticalSection(&m_CriticalSection);
				return;
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
				TTAFile.ID3v1Tag()->setGenre(m_Tag.at(tagName[METATAG_GENRE]));
			}
			else
			{
				// Do nothing.
			}
			TTAFile.save();
		}
		::LeaveCriticalSection(&m_CriticalSection);
	}
	else
	{
		// Do nothing
	}

	FlushCache();

	::DeleteCriticalSection(&m_CriticalSection);

	return;
}

const wchar_t* TTA_metaTag::enumSupportedTag(int n, int* datatype)
{
	return tagName[n].c_str();
}

int TTA_metaTag::getTagSize(const wchar_t* tag, size_t* sizeBytes)
{
	if (m_Tag.contains(tag))
	{
		*sizeBytes = static_cast<size_t>(m_Tag.at(tag).length() * sizeof(wchar_t));
	}
	else
	{
		*sizeBytes = 0;
		return 0;
	}
	return 1;
}

int TTA_metaTag::getMetaData(const wchar_t* tag, uint8_t* buf, int buflenBytes, int datatype)
{
	if (m_Tag.contains(tag))
	{
		memcpy_s(buf, buflenBytes, reinterpret_cast<const uint8_t *>(m_Tag[tag].c_str()), static_cast<rsize_t>(m_Tag.at(tag).length() * sizeof(wchar_t)));
	}
	else
	{
		return METATAG_FAILED;
	}
	return METATAG_SUCCESS;
}

int TTA_metaTag::setMetaData(const wchar_t* tag, const uint8_t* buf, int buflenBytes, int datatype)
{
	m_isChanged = true;
	m_Tag[tag] = reinterpret_cast<const wchar_t *>(buf);
	return METATAG_SUCCESS;
}

#define CBCLASS TTA_metaTag
START_DISPATCH;
CB(SVC_METATAG_GETNAME, getName);
CB(SVC_METATAG_GETGUID, getGUID);
CB(SVC_METATAG_GETFLAGS, getFlags);
CB(SVC_METATAG_ISOURFILE, isOurFile);
CB(SVC_METATAG_OPEN, metaTag_open);
VCB(SVC_METATAG_CLOSE, metaTag_close);
CB(SVC_METATAG_ENUMTAGS, enumSupportedTag);
CB(SVC_METATAG_GETTAGSIZE, getTagSize);
CB(SVC_METATAG_GETMETADATA, getMetaData);
CB(SVC_METATAG_SETMETADATA, setMetaData);
END_DISPATCH;
#undef CBCLASS

static TTA_metaTag metatag;

metaTagFactory::~metaTagFactory()
{
}

FOURCC metaTagFactory::GetServiceType()
{
	return metatag.getServiceType();
}

const char* metaTagFactory::GetServiceName()
{
	return "TTA Meta Tag Provider";
}

GUID metaTagFactory::GetGUID()
{
	return metatag.getGUID();
}

void* metaTagFactory::GetInterface(int global_lock)
{
	return &metatag;
}

int metaTagFactory::SupportNonLockingInterface()
{
	return 1;
}

int metaTagFactory::ReleaseInterface(void* ifc)
{
	return 1;
}

const char* metaTagFactory::GetTestString()
{
	return 0;
}

int metaTagFactory::ServiceNotify(int msg, int param1, int param2)
{
	return 1;
}

#define CBCLASS metaTagFactory
START_DISPATCH;
CB(WASERVICEFACTORY_GETSERVICETYPE, GetServiceType)
CB(WASERVICEFACTORY_GETSERVICENAME, GetServiceName)
CB(WASERVICEFACTORY_GETGUID, GetGUID)
CB(WASERVICEFACTORY_GETINTERFACE, GetInterface)
CB(WASERVICEFACTORY_SUPPORTNONLOCKINGGETINTERFACE, SupportNonLockingInterface)
CB(WASERVICEFACTORY_RELEASEINTERFACE, ReleaseInterface)
CB(WASERVICEFACTORY_GETTESTSTRING, GetTestString)
CB(WASERVICEFACTORY_SERVICENOTIFY, ServiceNotify)
END_DISPATCH;
#undef CBCLASS
