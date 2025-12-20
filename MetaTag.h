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
#include "MediaLibrary.h"

#ifndef METATAG_H_INCLUDED
#define METATAG_H_INCLUDED


// {50846701-71A9-40CF-9165-587D3A7DB325}
static const GUID TTA_metaTag_GUID =
{ 0x50846701, 0x71a9, 0x40cf, { 0x91, 0x65, 0x58, 0x7d, 0x3a, 0x7d, 0xb3, 0x25 } };

class TTA_metaTag : public svc_metaTag
{
public:
	TTA_metaTag();
	virtual ~TTA_metaTag();

	const static GUID getServiceGUID() { return TTA_metaTag_GUID; }
	static FOURCC getServiceType() { return svc_metaTag::SERVICETYPE; }
	const wchar_t* getName();	// i.e. "ID3v2" or something
	GUID getGUID(); // this needs to be the same GUID that you use when registering your service factory
	int getFlags(); // how this service gets its info
	int isOurFile(const wchar_t* filename);
	int metaTag_open(const wchar_t* filename);
	void metaTag_close(); // self-destructs when this is called (you don't need to call serviceFactory->releaseInterface)

	/* user API starts here */
	const wchar_t* enumSupportedTag(int n, int* datatype = NULL);	// returns a list of understood tags. might not be complete (see note [1])
	int getTagSize(const wchar_t* tag, size_t* sizeBytes); // always gives you BYTES, not characters (be careful with your strings)
	int getMetaData(const wchar_t* tag, uint8_t* buf, int buflenBytes, int datatype = METATYPE_STRING); // buflen is BYTES, not characters (be careful with your strings)
	int setMetaData(const wchar_t* tag, const uint8_t* buf, int buflenBytes, int datatype = METATYPE_STRING);

private:
	MediaLibrary	    m_MediaLibrary;
	std::wstring		m_FileName;

protected:
	RECVS_DISPATCH;
};

#endif // #ifndef METATAG_H_INCLUDED

