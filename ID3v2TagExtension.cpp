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

You should have received a copy of the GNU General Public License along with in_tta.
If not, see <https://www.gnu.org/licenses/>.
*/

#include "ID3v2TagExtension.h"
#include <taglib/tstring.h>
#include <taglib/id3v2tag.h>
#include <taglib/id3v2.h>
#include <taglib/urllinkframe.h>
#include <taglib/textidentificationframe.h>

ID3v2TagExtension::~ID3v2TagExtension()
{
}

TagLib::String ID3v2TagExtension::stringYear() const
{
	return getTextFrame("TDRC");
}

TagLib::String ID3v2TagExtension::stringTrack() const
{
	return getTextFrame("TRCK");
}

TagLib::String ID3v2TagExtension::albumArtist() const
{
	return getTextFrame("TPE2");
}

TagLib::String ID3v2TagExtension::copyright() const
{
	return getTextFrame("TCOP");
}

TagLib::String ID3v2TagExtension::URI() const
{
	if (!frameList("WXXX").isEmpty())
	{
		TagLib::ID3v2::UrlLinkFrame* f = dynamic_cast<TagLib::ID3v2::UrlLinkFrame*>(frameList("WXXX").front());
		return f->url();
	}
	else
	{
		// Do nothing
	}
	return TagLib::String();
}

TagLib::String ID3v2TagExtension::words() const
{
	return getTextFrame("TEXT");
}

TagLib::String ID3v2TagExtension::composers() const
{
	return getTextFrame("TCOM");
}

TagLib::String ID3v2TagExtension::arrangements() const
{
	return getTextFrame("TPE4");
}

TagLib::String ID3v2TagExtension::origArtist() const
{
	return getTextFrame("TOPE");
}

TagLib::String ID3v2TagExtension::encEngineer() const
{
	return getTextFrame("TENC");
}

TagLib::String ID3v2TagExtension::publisher() const
{
	return getTextFrame("TPUB");
}

TagLib::String ID3v2TagExtension::disc() const
{
	return getTextFrame("TPOS");
}

TagLib::String ID3v2TagExtension::BPM() const
{
	return getTextFrame("TBPM");
}

TagLib::ByteVector ID3v2TagExtension::albumArt(TagLib::ID3v2::AttachedPictureFrame::Type arttype, TagLib::String& mimetype)
{
	if (!frameList("APIC").isEmpty())
	{
		TagLib::ID3v2::AttachedPictureFrame* f = dynamic_cast<TagLib::ID3v2::AttachedPictureFrame*>(frameList("APIC").front());
		mimetype = f->mimeType();
		if (f->type() == arttype)
		{
			return f->picture();
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
	return TagLib::ByteVector();
}

void ID3v2TagExtension::setAlbumArtist(const TagLib::String& s)
{
	setTextFrame("TPE2", s);
	return;
}

void ID3v2TagExtension::setStringYear(const TagLib::String& s)
{
	setTextFrame("TDRC", s);
	return;
}

void ID3v2TagExtension::setStringTrack(const TagLib::String& s)
{
	setTextFrame("TRCK", s);
	return;
}

void ID3v2TagExtension::setCopyright(const TagLib::String& s)
{
	setTextFrame("TCOP", s);
	return;
}

void ID3v2TagExtension::setURI(const TagLib::String& s)
{
	if (s.isEmpty())
	{
		removeFrames("WXXX");
		return;
	}
	else
	{
		if (!frameList("WXXX").isEmpty())
		{
			TagLib::String temp = TagLib::String(" ") + s;
		}
		else
		{
			const TagLib::String::Type encoding = TagLib::String::UTF8;
			TagLib::ID3v2::UserUrlLinkFrame* f = new TagLib::ID3v2::UserUrlLinkFrame(encoding);
			f->setTextEncoding(encoding);
			addFrame(f);
			TagLib::String temp = TagLib::String(" ") + s;
			f->setText(temp);
		}
	}
	return;
}

void ID3v2TagExtension::setWords(const TagLib::String& s)
{
	setTextFrame("TEXT", s);
	return;
}

void ID3v2TagExtension::setComposers(const TagLib::String& s)
{
	setTextFrame("TCOM", s);
	return;
}

void ID3v2TagExtension::setArrangements(const TagLib::String& s)
{
	setTextFrame("TPE4", s);
	return;
}

void ID3v2TagExtension::setOrigArtist(const TagLib::String& s)
{
	setTextFrame("TOPE", s);
	return;
}

void ID3v2TagExtension::setEncEngineer(const TagLib::String& s)
{
	setTextFrame("TENC", s);
	return;
}

void ID3v2TagExtension::setPublisher(const TagLib::String& s)
{
	setTextFrame("TPUB", s);
	return;
}

void ID3v2TagExtension::setDisc(const TagLib::String& s)
{
	setTextFrame("TPOS", s);
	return;
}

void ID3v2TagExtension::setBPM(const TagLib::String& s)
{
	setTextFrame("TBPM", s);
	return;
}

void ID3v2TagExtension::setAlbumArt(const TagLib::ByteVector& v, TagLib::ID3v2::AttachedPictureFrame::Type arttype, TagLib::String& mimetype)
{
	if (v.isEmpty())
	{
		removeFrames("APIC");
		return;
	}
	else
	{
		if (!frameList("APIC").isEmpty())
		{
			removeFrames("APIC");
		}
		else
		{
			// Do nothing
		}

		TagLib::ID3v2::AttachedPictureFrame* f = new TagLib::ID3v2::AttachedPictureFrame("APIC");
		f->setMimeType(mimetype);
		f->setType(arttype);
		f->setPicture(v);
		addFrame(f);
	}
	return;
}

TagLib::String ID3v2TagExtension::getTextFrame(const TagLib::ByteVector& id) const
{
	auto temp = TagLib::String();

	if (!frameList(id).isEmpty())
	{
		temp = frameList(id).front()->toString();
	}
	else
	{
		// Do nothing
	}

	return temp;
}