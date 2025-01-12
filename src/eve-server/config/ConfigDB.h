/*
    ------------------------------------------------------------------------------------
    LICENSE:
    ------------------------------------------------------------------------------------
    This file is part of EVEmu: EVE Online Server Emulator
    Copyright 2006 - 2016 The EVEmu Team
    For the latest information visit http://evemu.org
    ------------------------------------------------------------------------------------
    This program is free software; you can redistribute it and/or modify it under
    the terms of the GNU Lesser General Public License as published by the Free Software
    Foundation; either version 2 of the License, or (at your option) any later
    version.

    This program is distributed in the hope that it will be useful, but WITHOUT
    ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
    FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public License along with
    this program; if not, write to the Free Software Foundation, Inc., 59 Temple
    Place - Suite 330, Boston, MA 02111-1307, USA, or go to
    http://www.gnu.org/copyleft/lesser.txt.
    ------------------------------------------------------------------------------------
    Author:     Zhur
*/


#ifndef __CONFIGDB_H_INCL__
#define __CONFIGDB_H_INCL__

#include "ServiceDB.h"

class PyRep;

class ConfigDB
: public ServiceDB
{
public:
    static PyRep *GetMultiOwnersEx(const std::vector<int32> &entityIDs);
    static PyRep *GetMultiLocationsEx(const std::vector<int32> &entityIDs);
    static PyRep *GetMultiAllianceShortNamesEx(const std::vector<int32> &entityIDs);
    static PyRep *GetMultiCorpTickerNamesEx(const std::vector<int32> &entityIDs);
    static PyRep *GetMultiGraphicsEx(const std::vector<int32> &entityIDs);
    static PyRep *GetMultiInvTypesEx(const std::vector<int32> &typeIDs);
    static PyObject *GetUnits();
    static PyObjectEx *GetMapObjects(uint32 entityID, bool wantRegions, bool wantConstellations, bool wantSystems, bool wantStations);
    static PyObject *GetMap(uint32 solarSystemID);
    static PyObject *ListLanguages();
    static PyRep *GetStationSolarSystemsByOwner(uint32 ownerID);
    static PyRep *GetCelestialStatistic(uint32 celestialID);
    /**
     * @brief Retrieves dynamic, celestial objects for a given solar system
     *
     * @param[in] solarSystemID  ID of the solar system whose objects are being retrieved
     */
    static PyRep *GetDynamicCelestials(uint32 solarSystemID);
    static PyRep *GetTextsForGroup(const std::string & langID, uint32 textgroup);
    static PyRep *getAveragePrices();

protected:
};

#endif



