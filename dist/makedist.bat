::------------------------------------------------------------------------
:: Fritzpart - Generates Fritzing parts from a part description script.
:: Copyright (C) 2021, Jason Cipriani <jason.cipriani.dev@gmail.com>
:: Not affiliated with Fritzing.
::
:: This file is part of Fritzpart.
::
:: Fritzpart is free software: you can redistribute it and/or modify
:: it under the terms of the GNU General Public License as published by
:: the Free Software Foundation, either version 3 of the License, or
:: (at your option) any later version.
::
:: This program is distributed in the hope that it will be useful,
:: but WITHOUT ANY WARRANTY; without even the implied warranty of
:: MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
:: GNU General Public License for more details.
::
:: You should have received a copy of the GNU General Public License
:: along with this program.  If not, see <https://www.gnu.org/licenses/>.
::
:: https://github.com/JC3/fritzpart
::------------------------------------------------------------------------

cp ..\release\fritzpart.exe .
cp ..\contrib\minizip.exe .
cp ..\LICENSE .
cp ..\README.md .
mkdir examples
cp ..\examples\*.txt examples\
windeployqt fritzpart.exe --no-quick-import --no-system-d3d-compiler --no-virtualkeyboard --no-webkit2 --no-angle --no-opengl-sw
