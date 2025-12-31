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

#include "agaveCommon.h"
#include <Winamp/wa_ipc.h>

const wchar_t* GetLastCharacterW(const wchar_t* string)
{
	if (!string || !*string)
	{
		return string;
	}
	else
	{
		// Do nothing
	}

	return CharPrevW(string, string + lstrlenW(string));
}

const wchar_t* scanstr_backW(const wchar_t* str, const wchar_t* toscan, const wchar_t* defval)
{
	const wchar_t* s = GetLastCharacterW(str);

	if (!str[0])
	{
		return defval;
	}
	else
	{
		// Do nothing
	}

	if (!toscan || !toscan[0])
	{
		return defval;
	}
	else
	{
		// Do nothing
	}

	while (1)
	{
		const wchar_t* t = toscan;

		while (*t)
		{
			if (*t == *s)
			{
				return s;
			}
			else
			{
				// Do nothing
			}
			t = CharNextW(t);
		}

		t = CharPrevW(str, s);

		if (t == s)
		{
			return defval;
		}
		else
		{
			// Do nothing
		}
		s = t;
	}
}

const wchar_t* extensionW(const wchar_t* fn)
{
	const wchar_t* end = scanstr_backW(fn, L"./\\", 0);
	if (!end)
	{
		return (fn + lstrlenW(fn));
	}
	else
	{
		// Do nothing
	}

	if (*end == L'.')
	{
		return end + 1;
	}
	else
	{
		// Do nothing
	}

	return (fn + lstrlenW(fn));
}
