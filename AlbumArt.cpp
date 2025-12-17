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

#include <Wasabi/api/service/api_service.h>
#include <Agave/Config/api_config.h>
#include <Wasabi/api/memmgr/api_memmgr.h>
#include <Winamp/in2.h>
#include <Winamp/wa_ipc.h>
#include <Wasabi/api/service/waservicefactory.h>
#include <Agave/AlbumArt/svc_albumArtProvider.h>
#include "AlbumArt.h"
#include <taglib/trueaudiofile.h>
#include <taglib/id3v2tag.h>
#include <taglib/attachedpictureframe.h>
#include <taglib/tag.h>

#include "MediaLibrary.h"


class AlbumArtFactory : public waServiceFactory
{
public:
	FOURCC GetServiceType();
	const char *GetServiceName();
	GUID GetGUID();
	void *GetInterface(int global_lock);
	int SupportNonLockingInterface();
	int ReleaseInterface(void *ifc);
	const char *GetTestString();
	int ServiceNotify(int msg, int param1, int param2);

protected:
	RECVS_DISPATCH;
};

extern In_Module mod; // TODO: change if you called yours something else

#define WASABI_API_MEMMGR memmgr

static api_config *AGAVE_API_CONFIG = 0;
static api_service *WASABI_API_SVC = 0;
static api_memmgr *WASABI_API_MEMMGR = 0;

static AlbumArtFactory albumArtFactory;

void Wasabi_Init()
{
	WASABI_API_SVC = reinterpret_cast<api_service *>(SendMessage(mod.hMainWindow, WM_WA_IPC, 0, IPC_GET_API_SERVICE));

	if (WASABI_API_SVC == 0 || WASABI_API_SVC == reinterpret_cast<api_service *>(1))
	{
		WASABI_API_SVC = 0;
		return;
	}
	else
	{
		// Do nothing
	}

	WASABI_API_SVC->service_register(&albumArtFactory);

	waServiceFactory *sf = WASABI_API_SVC->service_getServiceByGuid(AgaveConfigGUID);

	if (sf)
	{
		AGAVE_API_CONFIG = (api_config *)sf->getInterface();
	}
	else
	{
		// Do nothing
	}

	sf = WASABI_API_SVC->service_getServiceByGuid(memMgrApiServiceGuid);

	if (sf)
	{
		WASABI_API_MEMMGR = (api_memmgr *)sf->getInterface();
	}
	else
	{
		// Do nothing
	}
}

void Wasabi_Quit()
{
	if (WASABI_API_SVC)
	{
		waServiceFactory *sf = WASABI_API_SVC->service_getServiceByGuid(AgaveConfigGUID);
		if (sf)
		{
			sf->releaseInterface(AGAVE_API_CONFIG);
		}
		else
		{
			// Do nothing
		}

		sf = WASABI_API_SVC->service_getServiceByGuid(memMgrApiServiceGuid);
		if (sf)
		{
			sf->releaseInterface(WASABI_API_MEMMGR);
		}
		else
		{
			// Do nothing
		}

		WASABI_API_SVC->service_deregister(&albumArtFactory);
	}
}

void *Wasabi_Malloc(size_t size_in_bytes)
{
	return WASABI_API_MEMMGR->sysMalloc(size_in_bytes);
}

void Wasabi_Free(void *memory_block)
{
	WASABI_API_MEMMGR->sysFree(memory_block);
}

class TTA_AlbumArtProvider : public svc_albumArtProvider
{
public:
	TTA_AlbumArtProvider();
	virtual ~TTA_AlbumArtProvider();
	bool IsMine(const wchar_t *filename);
	int ProviderType();
	// implementation note: use WASABI_API_MEMMGR to alloc bits and mimetype, so that the recipient can free through that
	int GetAlbumArtData(const wchar_t *filename, const wchar_t *type, void **bits, size_t *len, wchar_t **mimeType);
	int SetAlbumArtData(const wchar_t *filename, const wchar_t *type, void *bits, size_t len, const wchar_t *mimeType);
	int DeleteAlbumArt(const wchar_t *filename, const wchar_t *type);
protected:
	RECVS_DISPATCH;
	CRITICAL_SECTION	m_CriticalSection;
	std::wstring			m_FileName;
	bool					m_isSucceed;
	TagLib::ByteVector		m_AlbumArt;
	TagLib::String			m_extension;
};

TTA_AlbumArtProvider::TTA_AlbumArtProvider() : svc_albumArtProvider()
{
	::InitializeCriticalSection(&m_CriticalSection);
	m_isSucceed = false;
	m_FileName = L"";
	m_AlbumArt = TagLib::ByteVector();
	m_extension = TagLib::String();
}

TTA_AlbumArtProvider::~TTA_AlbumArtProvider()
{
	::DeleteCriticalSection(&m_CriticalSection);
	m_isSucceed = false;
	m_FileName = L"";
	m_AlbumArt = TagLib::ByteVector();
	m_extension = TagLib::String();
}

static const wchar_t *GetLastCharactercW(const wchar_t *string)
{
	if (!string || !*string)
	{
		return string;
	}
	else
	{
		// Do nothing
	}

	return CharPrevW(string, string + lstrlenW(string));
}

static const wchar_t *scanstr_backW(const wchar_t *str, const wchar_t *toscan, const wchar_t *defval)
{
	const wchar_t *s = GetLastCharactercW(str);

	if (!str[0])
	{
		return defval;
	}
	else
	{
		// Do nothing
	}

	if (!toscan || !toscan[0])
	{
		return defval;
	}
	else
	{
		// Do nothing
	}

	while (1)
	{
		const wchar_t *t = toscan;

		while (*t)
		{
			if (*t == *s)
			{
				return s;
			}
			else
			{
				// Do nothing
			}
			t = CharNextW(t);
		}

		t = CharPrevW(str, s);

		if (t == s)
		{
			return defval;
		}
		else
		{
			// Do nothing
		}
		s = t;
	}
}

static const wchar_t *extensionW(const wchar_t *fn)
{
	const wchar_t *end = scanstr_backW(fn, L"./\\", 0);
	if (!end)
	{
		return (fn + lstrlenW(fn));
	}
	else
	{
		// Do nothing
	}

	if (*end == L'.')
	{
		return end + 1;
	}
	else
	{
		// Do nothing
	}

	return (fn + lstrlenW(fn));
}

bool TTA_AlbumArtProvider::IsMine(const wchar_t *filename)
{

	if (extensionW(filename))
	{
		return ((_wcsicmp(extensionW(filename), L"tta") == 0) || (_wcsicmp(extensionW(filename), L"TTA") == 0)) ? true : false;
	}
	else
	{
		// Do nothing
	}
	return false;
}

int TTA_AlbumArtProvider::ProviderType()
{
	return ALBUMARTPROVIDER_TYPE_EMBEDDED;
}

int TTA_AlbumArtProvider::GetAlbumArtData(const wchar_t *filename, const wchar_t *type, void **bits, size_t *len, wchar_t **mime_type)
{

	int retval = ALBUMARTPROVIDER_FAILURE;
	size_t string_len = 0;
	TagLib::String mimeType;

	::EnterCriticalSection(&m_CriticalSection);

	if (_wcsicmp(type, L"cover"))
	{
		::LeaveCriticalSection(&m_CriticalSection);
		return retval;
	}
	else
	{
		// Do nothing
	}

	if (!bits || !len || !mime_type)
	{
		::LeaveCriticalSection(&m_CriticalSection);
		return retval;
	}
	else
	{
		// Do nothing
	}

	if (!m_isSucceed || _wcsicmp(m_FileName.c_str(), filename))
	{
		m_FileName = filename;

		TagLib::TrueAudio::File TagFile(m_FileName.c_str());

		if (!TagFile.isValid())
		{
			m_isSucceed = false;
			::LeaveCriticalSection(&m_CriticalSection);
			return retval;
		}
		else
		{
			m_isSucceed = true;
		}

		// read Album Art
		m_AlbumArt =
			TagFile.ID3v2Tag()->albumArt(TagLib::ID3v2::AttachedPictureFrame::FrontCover, mimeType);
		m_extension = mimeType.substr(mimeType.find("/") + 1);
	}
	else
	{
		// Do nothing
	}

	if (!m_AlbumArt.isEmpty())
	{
		*len = m_AlbumArt.size();
		*bits = static_cast<char *>(Wasabi_Malloc(*len));
		if (nullptr == *bits)
		{
			::LeaveCriticalSection(&m_CriticalSection);
			return retval;
		}
		else
		{
			// Do nothing
		}

		errno_t err = memcpy_s(*bits, m_AlbumArt.size(), m_AlbumArt.data(), m_AlbumArt.size());
		if (err)
		{
			::LeaveCriticalSection(&m_CriticalSection);
			return retval;
		}
		else
		{
			// Do nothing
		}

		*mime_type = static_cast<wchar_t *>(Wasabi_Malloc(m_extension.size() * 2 + 2));
		if (nullptr == *mime_type)
		{
			if (nullptr != *bits)
			{
				Wasabi_Free(*bits);
			}
			else
			{
				// Do nothing
			}
			::LeaveCriticalSection(&m_CriticalSection);
			return retval;
		}
		else
		{
			mbstowcs_s(&string_len, *mime_type, m_extension.size() + 1, m_extension.toCString(), _TRUNCATE);
			retval = ALBUMARTPROVIDER_SUCCESS;
		}

		if (retval)
		{
			if (nullptr != *bits)
			{
				Wasabi_Free(*bits);
			}
			else
			{
				// Do nothing
			}

			if (nullptr != *mime_type)
			{
				Wasabi_Free(*mime_type);
			}
			else
			{
				// Do nothing
			}
		}
		else
		{
			// Do nothing
		}
	}
	else
	{
		// Do nothing
	}

	::LeaveCriticalSection(&m_CriticalSection);
	return retval;
}

int TTA_AlbumArtProvider::SetAlbumArtData(const wchar_t *filename, const wchar_t *type, void *bits, size_t len, const wchar_t *mime_type)
{

	int retval = ALBUMARTPROVIDER_FAILURE;
	TagLib::String mimeType(L"");
	unsigned int size = 0;
	TagLib::ID3v2::AttachedPictureFrame::Type artType = TagLib::ID3v2::AttachedPictureFrame::Other;

	::EnterCriticalSection(&m_CriticalSection);

	if (std::wstring(filename) == L"")
	{
		::LeaveCriticalSection(&m_CriticalSection);
		return retval;
	}

	if (!bits)
	{
		//delete m_AlbumArt
		m_AlbumArt.setData(NULL, 0);

	}
	else if (len == 0 || wcscmp(mime_type, L"") == 0)
	{
		::LeaveCriticalSection(&m_CriticalSection);
		return retval;
	}
	else
	{
		mimeType = L"image/";
		mimeType += mime_type;
		size = len;
		artType = TagLib::ID3v2::AttachedPictureFrame::FrontCover;
		m_AlbumArt.setData((const char *)bits, size);
	}

	TagLib::TrueAudio::File TTAFile(filename);

	if (TTAFile.isValid())
	{
		TTAFile.ID3v2Tag()->setAlbumArt(m_AlbumArt, artType, mimeType);
		TTAFile.save();
		m_isSucceed = false;
		retval = ALBUMARTPROVIDER_SUCCESS;
	}
	else
	{
		// Do nothing
	}
	::LeaveCriticalSection(&m_CriticalSection);

	return retval;
}

int TTA_AlbumArtProvider::DeleteAlbumArt(const wchar_t *filename, const wchar_t *type)
{
	return SetAlbumArtData(filename, type, nullptr, 0, L"jpeg");
}

#define CBCLASS TTA_AlbumArtProvider
START_DISPATCH;
CB(SVC_ALBUMARTPROVIDER_PROVIDERTYPE, ProviderType);
CB(SVC_ALBUMARTPROVIDER_GETALBUMARTDATA, GetAlbumArtData);
CB(SVC_ALBUMARTPROVIDER_ISMINE, IsMine);
CB(SVC_ALBUMARTPROVIDER_SETALBUMARTDATA, SetAlbumArtData);
CB(SVC_ALBUMARTPROVIDER_DELETEALBUMART, DeleteAlbumArt);
END_DISPATCH;
#undef CBCLASS

static TTA_AlbumArtProvider albumArtProvider;

// {bb653840-6dab-4867-9f42-A772E4068C81}
static const GUID TTA_albumArtproviderGUID =
{ 0xbb653840, 0x6dab, 0x4867, { 0x9f, 0x42, 0xa7, 0x72, 0xe4, 0x05, 0x8c, 0x81 } };


FOURCC AlbumArtFactory::GetServiceType()
{
	return svc_albumArtProvider::SERVICETYPE;
}

const char *AlbumArtFactory::GetServiceName()
{
	return "TTA Album Art Provider";
}

GUID AlbumArtFactory::GetGUID()
{
	return TTA_albumArtproviderGUID;
}

void *AlbumArtFactory::GetInterface(int global_lock)
{
	return &albumArtProvider;
}

int AlbumArtFactory::SupportNonLockingInterface()
{
	return 1;
}

int AlbumArtFactory::ReleaseInterface(void *ifc)
{
	return 1;
}

const char *AlbumArtFactory::GetTestString()
{
	return 0;
}

int AlbumArtFactory::ServiceNotify(int msg, int param1, int param2)
{
	return 1;
}

#define CBCLASS AlbumArtFactory
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
