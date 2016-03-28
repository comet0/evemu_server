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
    Author:     caytchen
*/

#include "eve-server.h"

#include "PyServiceCD.h"
#include "character/PaperDollService.h"
#include "character/PaperDollDB.h"

PyCallable_Make_InnerDispatcher(PaperDollService)

PaperDollService::PaperDollService()
: PyService("paperDollServer", new Dispatcher(this))
{
    PyCallable_REG_CALL(PaperDollService, GetPaperDollData)
    PyCallable_REG_CALL(PaperDollService, ConvertAndSavePaperDoll)
    PyCallable_REG_CALL(PaperDollService, UpdateExistingCharacterFull)
    PyCallable_REG_CALL(PaperDollService, UpdateExistingCharacterLimited)
    PyCallable_REG_CALL(PaperDollService, GetPaperDollPortraitDataFor)
    PyCallable_REG_CALL(PaperDollService, GetMyPaperDollData)
}

PaperDollService::~PaperDollService() {
}

PyResult PaperDollService::Handle_GetPaperDollData(PyCallArgs &call) {
    return new PyList;
}

PyResult PaperDollService::Handle_ConvertAndSavePaperDoll(PyCallArgs &call) {
    return NULL;
}

PyResult PaperDollService::Handle_UpdateExistingCharacterFull(PyCallArgs &call) {
    return NULL;
}

PyResult PaperDollService::Handle_UpdateExistingCharacterLimited(PyCallArgs &call) {
    return NULL;
}

PyResult PaperDollService::Handle_GetPaperDollPortraitDataFor(PyCallArgs &call) {
    return NULL;
}

PyResult PaperDollService::Handle_GetMyPaperDollData(PyCallArgs &call)
{

	PyDict* args = new PyDict;

    args->SetItemString("colors", PaperDollDB::GetPaperDollAvatarColors(call.client->GetCharacterID()));
    args->SetItemString("modifiers", PaperDollDB::GetPaperDollAvatarModifiers(call.client->GetCharacterID()));
    args->SetItemString("appearance", PaperDollDB::GetPaperDollAvatar(call.client->GetCharacterID()));
    args->SetItemString("sculpts", PaperDollDB::GetPaperDollAvatarSculpts(call.client->GetCharacterID()));

    return new PyObject("utillib.KeyVal", args);
}
