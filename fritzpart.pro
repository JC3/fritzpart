#------------------------------------------------------------------------
# Fritzpart - Generates Fritzing parts from a part description script.
# Copyright (C) 2021, Jason Cipriani <jason.cipriani.dev@gmail.com>
# Not affiliated with Fritzing.
#
# This file is part of Fritzpart.
#
# Fritzpart is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <https://www.gnu.org/licenses/>.
#
# https://github.com/JC3/fritzpart
#------------------------------------------------------------------------

VERSION = 0.9.1.0

QT       += core gui xml svg widgets

CONFIG += c++17

SOURCES += \
    main.cpp \
    mainwindow.cpp

HEADERS += \
    mainwindow.h

FORMS += \
    mainwindow.ui

DISTFILES += \
    README.md \
    about.html \
    dist/distclean.bat \
    dist/installer.nsi \
    dist/makedist.bat \
    distclean.bat \
    examples/test.txt

RESOURCES += \
    fritzpart.qrc

QMAKE_TARGET_DESCRIPTION = "Fritzpart"
QMAKE_TARGET_COMPANY = "Jason Cipriani"
QMAKE_TARGET_COPYRIGHT = "Copyright (C) 2021, Jason Cipriani"
QMAKE_TARGET_PRODUCT = "Fritzpart"
DEFINES += APPLICATION_VERSION='\\"$$VERSION\\"'

nsiversion.target = dist/version.nsh
nsiversion.depends = fritzpart.pro
nsiversion.commands = echo "!define PRODUCT_VERSION \"$$VERSION\"" > $$nsiversion.target
QMAKE_EXTRA_TARGETS += nsiversion
PRE_TARGETDEPS += $$nsiversion.target

win32:RC_ICONS = contrib/ic.ico

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

