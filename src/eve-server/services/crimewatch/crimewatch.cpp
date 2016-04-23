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
    Author:        Cometo
*/


#include "eve-server.h"
#include "crimewatch.h"
#include "PyBoundObject.h"
#include "PyServiceCD.h"
#include "PyServiceMgr.h"

class CrimewatchBound
: public PyBoundObject
{
public:
    PyCallable_Make_Dispatcher(CrimewatchBound)

    CrimewatchBound()
    : PyBoundObject(new Dispatcher(this))
    {
        m_strBoundObjectName = "crimewatchBound";

        PyCallable_REG_CALL(CrimewatchBound, GetClientStates)
    }
    virtual ~CrimewatchBound() { }
    virtual void Release() {
        delete this;
    }
    PyCallable_DECL_CALL(GetClientStates)

protected:
};

PyCallable_Make_InnerDispatcher(CrimewatchService)

CrimewatchService::CrimewatchService()
: PyService("crimewatch", new Dispatcher(this))
{
}

CrimewatchService::~CrimewatchService() {
}

PyBoundObject *CrimewatchService::_CreateBoundObject(Client *c, const PyRep *bind_args) {
    if(bind_args->IsTuple())
    {
        SysLog::Debug("CrimewatchSvc", "arg1: solarsystemid/stationid/worldspaceid: %u", bind_args->AsTuple()->GetItem(0)->AsInt()->value());
        SysLog::Debug("CrimewatchSvc", "arg2: groupSolarSystem/groupStation/groupWorldSpace %u", bind_args->AsTuple()->GetItem(1)->AsInt()->value());
    }
    return new CrimewatchBound();
}


PyResult CrimewatchBound::Handle_GetClientStates(PyCallArgs &call) {
    SysLog::Log("CrimewatchBound", "Called GetClientStates semi-stub.");
    PyTuple *rtn = new PyTuple(4);
    rtn->SetItem(0, new_tuple(new_tuple(new PyInt(100), new PyNone()), new_tuple(new PyInt(200), new PyNone()), new_tuple(new PyInt(400), new PyNone()), new_tuple(new PyInt(300), new PyNone()))); //myCombatTimers
    rtn->SetItem(1, new PyDict()); //myEngagements
    rtn->SetItem(2, new_tuple(new PyObjectEx_Type1(new PyToken("__builtin__.set"), new_tuple(new PyList(0))), new PyObjectEx_Type1(new PyToken("__builtin__.set"), new_tuple(new PyList(0))))); //flaggedCharacters
    rtn->SetItem(3, new PyInt(2)); //safetyLevel

    return rtn;
}
