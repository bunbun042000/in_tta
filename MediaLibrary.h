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

#if !defined(AFX_MediaLibrary_H__997DC726_50DB_46B4_A156_DB5E92EC2BE8__INCLUDED_)
#define AFX_MediaLibrary_H__997DC726_50DB_46B4_A156_DB5E92EC2BE8__INCLUDED_

#include <Winamp/wa_ipc.h>

#include <taglib/tstring.h>
#include <taglib/trueaudiofile.h>
#include <taglib/attachedpictureframe.h>

#include "in_tta.h"

static const __int32 MAX_MUSICTEXT = 512;
static const __int32 MAX_YEAR = 10;

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

class MediaLibrary
{
public:
	MediaLibrary();
	virtual ~MediaLibrary();
	__int32  GetExtendedFileInfo(const wchar_t *fn, const char *Metadata, wchar_t *dest, size_t destlen);
	__int32  SetExtendedFileInfo(const wchar_t *fn, const char *Metadata, const wchar_t *val);
	__int32  WriteExtendedFileInfo();
	void FlushCache();
	std::wstring GetCurrentFileName() { return m_FileName; };
	bool	isValid() const { return m_isValidFile; };

private:
	CRITICAL_SECTION	m_CriticalSection;
	TagInfo				m_TagDataW {};
	DWORD				m_GetTagTime;
	std::wstring		m_FileName;
	bool				m_isValidFile;

	bool GetTagInfo(const std::wstring fn);

};

#endif // !defined(AFX_MediaLibrary_H__997DC726_50DB_46B4_A156_DB5E92EC2BE8__INCLUDED_)
