/*
The ttaplugin-winamp project.
Copyright (C) 2005-2026 Yamagata Fumihiro

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

#pragma once

#include <taglib/tag.h>
#include <taglib/id3v2tag.h>
#include <taglib/tstring.h>
#include <taglib/trueaudiofile.h>
#include <taglib/attachedpictureframe.h>

class ID3v2TagExtension : public TagLib::ID3v2::Tag
{
public:
	using TagLib::ID3v2::Tag::Tag;

    ID3v2TagExtension(const ID3v2TagExtension &tag) = delete;
    ID3v2TagExtension& operator=(const ID3v2TagExtension&) = delete;

    virtual ~ID3v2TagExtension();

    TagLib::String stringYear() const;
    TagLib::String stringTrack() const;
    TagLib::String albumArtist() const;
    TagLib::String copyright() const;
    TagLib::String URI() const;
    TagLib::String words() const;
    TagLib::String composers() const;
    TagLib::String arrangements() const;
    TagLib::String origArtist() const;
    TagLib::String encEngineer() const;
    TagLib::String publisher() const;
    TagLib::String disc() const;
    TagLib::String BPM() const;
    TagLib::ByteVector albumArt(TagLib::ID3v2::AttachedPictureFrame::Type arttype, TagLib::String& mimetype);

    void setAlbumArtist(const TagLib::String& s);
    void setStringYear(const TagLib::String& s);
    void setStringTrack(const TagLib::String& s);
    void setCopyright(const TagLib::String& s);
    void setURI(const TagLib::String& s);
    void setWords(const TagLib::String& s);
    void setComposers(const TagLib::String& s);
    void setArrangements(const TagLib::String& s);
    void setOrigArtist(const TagLib::String& s);
    void setEncEngineer(const TagLib::String& s);
    void setPublisher(const TagLib::String& s);
    void setDisc(const TagLib::String& s);
    void setBPM(const TagLib::String& s);
    void setAlbumArt(const TagLib::ByteVector& v, TagLib::ID3v2::AttachedPictureFrame::Type arttype, TagLib::String& mimetype);

private:
    TagLib::String getTextFrame(const TagLib::ByteVector& id) const;

};