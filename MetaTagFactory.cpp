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

#include <agave/Metadata/svc_metatag.h>
#include <Wasabi/api/service/waservicefactory.h>

#include "MetaTag.h"
#include "MetaTagFactory.h"

TTA_metaTagFactory::~TTA_metaTagFactory()
{
}

FOURCC TTA_metaTagFactory::GetServiceType()
{
	return WaSvc::METATAG;
}

const char* TTA_metaTagFactory::GetServiceName()
{
	return "TTA Metadata";
}

GUID TTA_metaTagFactory::GetGUID()
{
	return TTA_metaTag::getServiceGUID();
}

void* TTA_metaTagFactory::GetInterface(int global_lock)
{
	svc_metaTag* ifc = new TTA_metaTag;
	return ifc;
}

int TTA_metaTagFactory::SupportNonLockingInterface()
{
	return 1;
}

int TTA_metaTagFactory::ReleaseInterface(void* ifc)
{
	svc_metaTag *metaTag = static_cast<svc_metaTag *>(ifc);
	TTA_metaTag *TTAMetaTag = static_cast<TTA_metaTag *>(metaTag);
	delete TTAMetaTag;
	return 1;
}

const char* TTA_metaTagFactory::GetTestString()
{
	return nullptr;
}

int TTA_metaTagFactory::ServiceNotify(int msg, int param1, int param2)
{
	return 1;
}

#define CBCLASS TTA_metaTagFactory
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
