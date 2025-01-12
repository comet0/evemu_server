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
    Author:        Captnoord
*/

#include "eve-server.h"

#include "PyServiceCD.h"
#include "account/BrowserLockdownSvc.h"

// crap
PyCallable_Make_InnerDispatcher(BrowserLockdownService)

BrowserLockdownService::BrowserLockdownService()
: PyService("browserLockdownSvc", new Dispatcher(this))
{
    PyCallable_REG_CALL(BrowserLockdownService, GetFlaggedSitesHash)
    PyCallable_REG_CALL(BrowserLockdownService, GetFlaggedSitesList)
    PyCallable_REG_CALL(BrowserLockdownService, GetDefaultHomePage)
    PyCallable_REG_CALL(BrowserLockdownService, IsBrowserInLockdown)
}

BrowserLockdownService::~BrowserLockdownService() {
}

PyObject* GenerateLockdownCachedObject()
{
/*
PyString:"util.CachedObject"
    PyTuple:3
    itr[0]:PyTuple:3
      itr[0]:"Method Call"
      itr[1]:"server"
      itr[2]:PyTuple:2
        itr[0]:"browserLockdownSvc"
        itr[1]:"GetFlaggedSitesHash"
    itr[1]:0xACECD
    itr[2]:PyTuple:2
      itr[0]:0x1CC26986D4B75A0
      itr[1]:0xC017
        */
    PyTuple * arg_tuple = new PyTuple(3);

    arg_tuple->SetItem(0, new_tuple("Method Call", "server", new_tuple("browserLockdownSvc", "GetFlaggedSitesHash")));
    arg_tuple->SetItem(1, new PyInt(0xACECD));
    arg_tuple->SetItem(2, new_tuple(new PyLong(0x1CC26986D4B75A0LL), new PyInt(0xC017) ) );

    return new PyObject( "util.CachedObject" , arg_tuple );
}

PyResult BrowserLockdownService::Handle_GetFlaggedSitesHash(PyCallArgs &call)
{
    // Future updates should be the md5 sum of the cache/browser/flaggedsites.dat file
    // from user/appdata/Local/CCP/EVE
    // Send md5 sum for an the current pickled flaggedsites.dat file.
    // To get the hash use the following python
    //import hashlib
    //with Open(flaggedSitesFile) as f:
    //  flaggedSites = cPickle.loads(f.Read())
    //m = hashlib.md5()
    //m.update(str(flaggedSites))
    //print m.hexdigest()
    return new PyString("8a78e7a9ef5770c71fe133aa0c7c4b2f");
}

PyResult BrowserLockdownService::Handle_GetFlaggedSitesList(PyCallArgs &call)
{
    //PyDict* args = new PyDict;
    //return new PyObject( "objectCaching.CachedMethodCallResult", args );
    PyTuple* arg_tuple = new PyTuple(3);

    PyDict* itr_1 = new PyDict();
    itr_1->SetItem("versionCheck", new_tuple("run", "run", "run"));

    arg_tuple->SetItem(0, itr_1);
    arg_tuple->SetItem(1, GenerateLockdownCachedObject());
    arg_tuple->SetItem(2, new PyNone());

    return new PyObject("carbon.common.script.net.objectCaching.CachedMethodCallResult", arg_tuple);
}


PyResult BrowserLockdownService::Handle_GetDefaultHomePage(PyCallArgs &call) {
    return NULL;
}

PyResult BrowserLockdownService::Handle_IsBrowserInLockdown(PyCallArgs &call) {
  /*  Empty Call  */
    return new PyNone;
}
