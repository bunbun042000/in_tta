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

#ifndef METADATA_H_INCLUDED
#define METADATA_H_INCLUDED

#include <agave/Metadata/svc_metatag.h>


class MetaTagFactory : public waServiceFactory
{
public:
	virtual ~MetaTagFactory();

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

class TTA_metaTag : public svc_metaTag
{
public:
	TTA_metaTag();
	virtual ~TTA_metaTag();

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
	void FlushCache();

private:
	CRITICAL_SECTION	m_CriticalSection;
	std::map<const std::wstring, std::wstring>m_Tag;
	DWORD				m_GetTagTime;
	std::wstring		m_FileName;
	bool				m_isValidFile;
	unsigned long		m_Length;
	bool				m_isChanged;

protected:
	RECVS_DISPATCH;
};

#endif // #ifndef METADATA_H_INCLUDED

