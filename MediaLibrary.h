/*
The ttaplugins-winamp project.
Copyright (C) 2005-2015 Yamagata Fumihiro

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

#if !defined(AFX_MediaLibrary_H__997DC726_50DB_46B4_A156_DB5E92EC2BE8__INCLUDED_)
#define AFX_MediaLibrary_H__997DC726_50DB_46B4_A156_DB5E92EC2BE8__INCLUDED_

#include "..\Winamp SDK\Winamp\wa_ipc.h"
#include "libtta.h"

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
};

struct AlbumArtInfo
{
	TagLib::ByteVector	Albumart;
	TagLib::ID3v2::AttachedPictureFrame::Type arttype;
	TagLib::String mimetype;
};

class CMediaLibrary  
{
public:
	CMediaLibrary();
	virtual ~CMediaLibrary();
	__int32  GetExtendedFileInfo(const wchar_t *fn, const wchar_t *Metadata, wchar_t *dest, size_t destlen);
	__int32  SetExtendedFileInfo(const wchar_t *fn, const wchar_t *Metadata, const wchar_t *val);
	__int32  WriteExtendedFileInfo();
	std::wstring GetCurrentFileName() { return FileName; };
	bool	isValid() { return isValidFile; };
	TagLib::ByteVector	GetAlbumArt(TagLib::ID3v2::AttachedPictureFrame::Type arttype, TagLib::String &mimetype);
	void				SetAlbumArt(const TagLib::ByteVector &v, TagLib::ID3v2::AttachedPictureFrame::Type arttype, TagLib::String &mimetype);

private:

	CRITICAL_SECTION	CriticalSection;
	TagInfo				TagDataW;
	DWORD				GetTagTime;
	std::wstring		FileName;
	bool				isValidFile;
	AlbumArtInfo		albumArtInfo;

	void FlushCache(void);
	bool GetTagInfo(const std::wstring fn);

};


#endif // !defined(AFX_MediaLibrary_H__997DC726_50DB_46B4_A156_DB5E92EC2BE8__INCLUDED_)
