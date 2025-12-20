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
#include <string.h>

#include "agaveCommon.h"
#include "ID3v2TagExtension.h"
#include "MetaTag.h"
#include "MediaLibrary.h"


TTA_metaTag::TTA_metaTag() : svc_metaTag()
{
}

TTA_metaTag::~TTA_metaTag()
{
	m_FileName = L"";
}

const wchar_t* TTA_metaTag::getName()
{
	return L"TTA Metadata";
}

GUID TTA_metaTag::getGUID()
{
	return TTA_metaTag::getServiceGUID();
}

int TTA_metaTag::getFlags()
{
	return METATAG_FILE_INFO;
}

int TTA_metaTag::isOurFile(const wchar_t* filename)
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

int TTA_metaTag::metaTag_open(const wchar_t* filename)
{
	if (m_MediaLibrary.SetFileName(filename))
	{
		return METATAG_SUCCESS;
	}
	else
	{
		return METATAG_FAILED;
	}

	return METATAG_FAILED;

}

void TTA_metaTag::metaTag_close()
{
	return;
}

const wchar_t* TTA_metaTag::enumSupportedTag(int n, int* datatype)
{
//	return tagName[n].c_str();
	return 0;
}

int TTA_metaTag::getTagSize(const wchar_t* tag, size_t* sizeBytes)
{
	auto i = 0;
	bool isExist = false;
	for (i = 0; i < METATAG_TAGTYPE_MAX; i++)
	{
		if (wcscmp(tag, tagName[i].wstr.c_str()) == 0)
		{
			isExist = true;
			break;
		}
		else
		{
			// Do nothing
		}
	}

	if (isExist)
	{
		*sizeBytes = m_MediaLibrary.GetTagLength(tagName[i].str.c_str());
		return METATAG_SUCCESS;
	}
	else
	{
		*sizeBytes = 0;
		return METATAG_UNKNOWN_TAG;
	}
	return METATAG_SUCCESS;
}

int TTA_metaTag::getMetaData(const wchar_t* tag, uint8_t* buf, int buflenBytes, int datatype)
{
	auto i = 0;
	bool isExist = false;
	for (i = 0; i < METATAG_TAGTYPE_MAX; i++)
	{
		if (wcscmp(tag, tagName[i].wstr.c_str()) == 0)
		{
			isExist = true;
			break;
		}
		else
		{
			// Do nothing
		}
	}

	if (isExist)
	{
		m_MediaLibrary.GetExtendedFileInfo(m_FileName.c_str(), tagName[i].str.c_str(), reinterpret_cast<wchar_t *>(buf), buflenBytes);
	}
	else
	{
		return METATAG_FAILED;
	}
	return METATAG_SUCCESS;
}

int TTA_metaTag::setMetaData(const wchar_t* tag, const uint8_t* buf, int buflenBytes, int datatype)
{
	m_FileName =L"";
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

