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

#include "PyBoundObject.h"
#include "PyServiceCD.h"
#include "station/JumpCloneService.h"

class JumpCloneBound
: public PyBoundObject
{
public:
    PyCallable_Make_Dispatcher(JumpCloneBound)

    JumpCloneBound()
    : PyBoundObject(new Dispatcher(this))
    {
        m_strBoundObjectName = "JumpCloneBound";

        PyCallable_REG_CALL(JumpCloneBound, GetCloneState)
        PyCallable_REG_CALL(JumpCloneBound, InstallCloneInStation)
        PyCallable_REG_CALL(JumpCloneBound, GetPriceForClone)
    }
    virtual ~JumpCloneBound() { }
    virtual void Release() {
        //I hate this statement
        delete this;
    }

    PyCallable_DECL_CALL(GetCloneState)
    PyCallable_DECL_CALL(InstallCloneInStation)
    PyCallable_DECL_CALL(GetPriceForClone)

protected:
       //we own this
};

PyCallable_Make_InnerDispatcher(JumpCloneService)

JumpCloneService::JumpCloneService()
: PyService("jumpCloneSvc", new Dispatcher(this))
{
    PyCallable_REG_CALL(JumpCloneService, GetShipCloneState)
}

JumpCloneService::~JumpCloneService() {
}

PyBoundObject* JumpCloneService::_CreateBoundObject( Client* c, const PyRep* bind_args )
{
    _log( CLIENT__MESSAGE, "JumpCloneService bind request for:" );
    bind_args->Dump( CLIENT__MESSAGE, "    " );

    return new JumpCloneBound();
}

PyResult JumpCloneBound::Handle_InstallCloneInStation( PyCallArgs& call )
{
    //takes no arguments, returns no arguments

    SysLog::Debug( "JumpCloneBound", "Called InstallCloneInStation stub." );

    return new PyNone;
}

PyResult JumpCloneBound::Handle_GetCloneState(PyCallArgs &call) {

    //returns (clones, implants, timeLastJump)
    //where jumpClones is a rowset? with at least columns: jumpCloneID, locationID

    SysLog::Debug( "JumpCloneBound", "Called GetCloneState stub." );

    PyDict* d = new PyDict;
    d->SetItemString( "clones", new PyNone );
    d->SetItemString( "implants", new PyNone );
    d->SetItemString( "timeLastJump", new PyNone );

    return new PyObject( "utillib.KeyVal", d );
}

PyResult JumpCloneBound::Handle_GetPriceForClone(PyCallArgs &call) {
    PyRep *result = NULL;
    return result;
}

PyResult JumpCloneService::Handle_GetShipCloneState(PyCallArgs &call) {
    PyRep *result = NULL;
    return result;
}
