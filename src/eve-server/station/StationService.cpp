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
    Author:        Zhur
*/

#include "eve-server.h"

#include "EntityList.h"
#include "PyServiceCD.h"
#include "station/StationDB.h"
#include "station/StationService.h"

PyCallable_Make_InnerDispatcher(StationService)

StationService::StationService()
: PyService("station", new Dispatcher(this))
{
    PyCallable_REG_CALL(StationService, GetGuests)
    PyCallable_REG_CALL(StationService, GetSolarSystem)
}

StationService::~StationService() {
}

PyResult StationService::Handle_GetSolarSystem(PyCallArgs &call) {
    Call_SingleIntegerArg arg;
    if (!arg.Decode(&call.tuple))
    {
        codelog(CLIENT__ERROR, "%s: Failed to decode GetSolarSystem arguments.", call.client->GetName());
        return NULL;
    }

    int system = arg.arg;

    // this needs to return some cache status?
    return new PyObject("util.CachedObject", new PyInt(system));
}

PyResult StationService::Handle_GetGuests(PyCallArgs &call) {
    PyList *res = new PyList();

    std::vector<Client *> clients;
    EntityList::FindByStationID(call.client->GetStationID(), clients);
    for(Client *client : clients)
    {
        PyList *list = new PyList();
        list->AddItem(new PyInt(client->GetCharacterID()));
        uint32 corpID = client->GetCorporationID();
        list->AddItem(corpID == 0 ? (PyRep*)new PyNone() : (PyRep*)new PyInt(corpID));
        uint32 allianceID = client->GetAllianceID();
        list->AddItem(allianceID == 0 ? (PyRep*)new PyNone() : (PyRep*)new PyInt(allianceID));
        uint32 warID = client->GetWarFactionID();
        list->AddItem(warID == 0 ? (PyRep*)new PyNone() : (PyRep*)new PyInt(warID));
        res->AddItem(list);
    }

    return res;

    //Log::Debug("StationService", "Called GetGuests stub.");
    //return NULL;
}
