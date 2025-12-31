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

#ifndef ALBUMART_H_INCLUDED
#define ALBUMART_H_INCLUDED

#include <Wasabi/api/service/waservicefactory.h>
#include <Agave/AlbumArt/svc_albumArtProvider.h>

#include <taglib/trueaudiofile.h>
#include <taglib/id3v2tag.h>
#include <taglib/attachedpictureframe.h>
#include <taglib/tag.h>

#include <string>

class AlbumArtFactory : public waServiceFactory
{
public:
	virtual ~AlbumArtFactory();

	FOURCC GetServiceType();
	const char* GetServiceName();
	GUID GetGUID();
	void* GetInterface(int global_lock);
	int SupportNonLockingInterface();
	int ReleaseInterface(void* ifc);
	const char* GetTestString();
	int ServiceNotify(int msg, int param1, int param2);

protected:
	RECVS_DISPATCH;
};

class TTA_AlbumArtProvider : public svc_albumArtProvider
{
public:
	TTA_AlbumArtProvider();
	virtual ~TTA_AlbumArtProvider();
	bool IsMine(const wchar_t* filename);
	int ProviderType();
	// implementation note: use WASABI_API_MEMMGR to alloc bits and mimetype, so that the recipient can free through that
	int GetAlbumArtData(const wchar_t* filename, const wchar_t* type, void** bits, size_t* len, wchar_t** mimeType);
	int SetAlbumArtData(const wchar_t* filename, const wchar_t* type, void* bits, size_t len, const wchar_t* mimeType);
	int DeleteAlbumArt(const wchar_t* filename, const wchar_t* type);
protected:
	RECVS_DISPATCH;
	CRITICAL_SECTION	m_CriticalSection;
	std::wstring			m_FileName;
	bool					m_isSucceed;
	TagLib::ByteVector		m_AlbumArt;
	TagLib::String			m_extension;
};

#endif // #ifndef ALBUMART_H_INCLUDED
