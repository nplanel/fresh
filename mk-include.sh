#!/bin/bash
# /****************************************************************************
# **
# **      Created using Monkey Studio IDE v1.8.4.0 (1.8.4.0)
# ** Authors   : Filipe AZEVEDO aka Nox P@sNox <pasnox@gmail.com>
# ** Project   : Fresh Library
# ** FileName  : mk-include.sh
# ** Date      : 2011-02-20T00:44:25
# ** License   : LGPL v3
# ** Home Page : https://github.com/pasnox/fresh
# ** Comment   : Fresh Library is a Qt 4 extension library providing set of new core & gui classes.
# **
# ** This program is free software: you can redistribute it and/or modify
# ** it under the terms of the GNU Leser General Public License as published by
# ** the Free Software Foundation, either version 3 of the License, or
# ** (at your option) any later version.
# **
# ** This package is distributed in the hope that it will be useful,
# ** but WITHOUT ANY WARRANTY; without even the implied warranty of
# ** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# ** GNU Lesser General Public License for more details.
# **
# ** You should have received a copy of the GNU Lesser General Public License
# ** along with this program. If not, see <http://www.gnu.org/licenses/>.
# **
# ****************************************************************************/

CORE_HEADERS=`find src/core -type f -name '*.h'`
GUI_HEADERS=`find src/gui -type f -name '*.h'`

rm -fr include
mkdir -p include/FreshCore
mkdir -p include/FreshGui

for header in ${CORE_HEADERS};
do
    headerFileName=`basename "${header}"`
    
    if [ "${headerFileName}" == "Core.h" ]; then
        continue;
    fi
    
    headerBaseName=`basename "${header}" .h`
    
    echo "#include \"${headerFileName}\"" >> "include/FreshCore/${headerBaseName}"
done

for header in ${GUI_HEADERS};
do
    headerFileName=`basename "${header}"`
    
    if [ "${headerFileName}" == "Gui.h" ]; then
        continue;
    fi
    
    headerBaseName=`basename "${header}" .h`
    
    echo "#include \"${headerFileName}\"" >> "include/FreshGui/${headerBaseName}"
done
