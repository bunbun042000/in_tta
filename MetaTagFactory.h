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

#ifndef METATAGFACTORY_H_INCLUDED
#define METATAGFACTORY_H_INCLUDED

#include <Wasabi/api/service/waservicefactory.h>

class TTA_metaTagFactory : public waServiceFactory
{
public:
	virtual ~TTA_metaTagFactory();

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

#endif // #ifndef METATAGFACTORY_H_INCLUDED
