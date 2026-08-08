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

#include <Agave/Config/api_config.h>
#include <Agave/AlbumArt/svc_albumArtProvider.h>
#include <Wasabi/api/service/api_service.h>
#include <Wasabi/api/service/waservicefactory.h>
#include <Wasabi/api/memmgr/api_memmgr.h>
#include <Winamp/in2.h>
#include <Winamp/wa_ipc.h>

#include "AlbumArt.h"
#include "MetaTag.h"
#include "MetaTagFactory.h"

extern In_Module mod; // TODO: change if you called yours something else

#define WASABI_API_MEMMGR memmgr

static api_config* AGAVE_API_CONFIG = nullptr;
static api_service* WASABI_API_SVC = nullptr;
static api_memmgr* WASABI_API_MEMMGR = nullptr;

static AlbumArtFactory albumArtFactory;
static TTA_metaTagFactory metaTagFactory;

void Wasabi_Init()
{
	WASABI_API_SVC = reinterpret_cast<api_service*>(SendMessage(mod.hMainWindow, WM_WA_IPC, 0, IPC_GET_API_SERVICE));

	if (WASABI_API_SVC == nullptr || WASABI_API_SVC == reinterpret_cast<api_service*>(1))
	{
		WASABI_API_SVC = nullptr;
		return;
	}
	else
	{
		// Do nothing
	}

	WASABI_API_SVC->service_register(&albumArtFactory);
	WASABI_API_SVC->service_register(&metaTagFactory);

	waServiceFactory* sf = WASABI_API_SVC->service_getServiceByGuid(AgaveConfigGUID);

	if (sf)
	{
		AGAVE_API_CONFIG = (api_config*)sf->getInterface();
	}
	else
	{
		// Do nothing
	}

	sf = WASABI_API_SVC->service_getServiceByGuid(memMgrApiServiceGuid);

	if (sf)
	{
		WASABI_API_MEMMGR = (api_memmgr*)sf->getInterface();
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
		waServiceFactory* sf = WASABI_API_SVC->service_getServiceByGuid(AgaveConfigGUID);
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
		WASABI_API_SVC->service_deregister(&metaTagFactory);
	}
}

void* Wasabi_Malloc(size_t size_in_bytes)
{
	return WASABI_API_MEMMGR->sysMalloc(size_in_bytes);
}

void Wasabi_Free(void* memory_block)
{
	WASABI_API_MEMMGR->sysFree(memory_block);
}

