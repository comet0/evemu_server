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
    Author:        Bloody.Rabbit
*/

#include "eve-server.h"

#include "PyServiceCD.h"
#include "account/UserService.h"
#include "cache/ObjCacheService.h"
#include "PyServiceMgr.h"

PyCallable_Make_InnerDispatcher(UserService)

UserService::UserService()
: PyService("userSvc", new Dispatcher(this))
{
    PyCallable_REG_CALL(UserService, GetRedeemTokens)
    PyCallable_REG_CALL(UserService, GetCreateDate)
    PyCallable_REG_CALL(UserService, ReportISKSpammer)
    PyCallable_REG_CALL(UserService, GetMultiCharactersTrainingSlots)
}

UserService::~UserService() {
}

PyResult UserService::Handle_GetRedeemTokens( PyCallArgs& call)
{
    uint32 accountID = call.client->GetAccountID();
    DBQueryResult res;
    if(!DBcore::RunQuery(res, "SELECT"
                         " tokenID,"
                         " massTokenID,"
                         " typeID,"
                         " quantity,"
                         " blueprintRuns,"
                         " blueprintMaterialLevel,"
                         " blueprintProductivityLevel,"
                         " label,"
                         " description,"
                         " dateTime,"
                         " expireDateTime,"
                         " availableDateTime,"
                         " stationID"
                         " FROM srvRedeemTokens"
                         " WHERE accountID = %u", accountID))
    {
        codelog(SERVICE__ERROR, "Error in query: %s", res.error.c_str());
        return NULL;
    }

    return DBResultToCRowset(res);
}

PyResult UserService::Handle_GetCreateDate( PyCallArgs& call )
{
    SysLog::Debug( "UserService", "Called GetCreateDate stub." );

    return new PyLong((long) call.client->GetChar()->createDateTime());
}

PyResult UserService::Handle_ReportISKSpammer( PyCallArgs& call )
{
    //  will add this completed code at a later date  -allan 25Jul14
    return NULL;
}

PyResult UserService::Handle_GetMultiCharactersTrainingSlots( PyCallArgs& call )
{
    PyDict *slots = new PyDict();
    // Construct results.
    // TO-DO: create table for training slots and times.
    //uint32 trainingID = 0;
    //uint64 trainingExpiry = Win32TimeNow() + Win32Time_Month;
    //slots->SetItem(new PyInt(trainingID), new PyLong(expiryTime));

    PySubStream *ret = new PySubStream(slots);
    return PyService::_BuildCachedReturn(&ret, CacheCheckTime::check_Run);;
}
