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

#ifndef AGAVECOMMON_H_INCLUDED
#define AGAVECOMMON_H_INCLUDED

const wchar_t* GetLastCharacterW(const wchar_t* string);
const wchar_t* scanstr_backW(const wchar_t* str, const wchar_t* toscan, const wchar_t* defval);
const wchar_t* extensionW(const wchar_t* fn);

#endif //#ifndef AGAVECOMMON_H_INCLUDED
