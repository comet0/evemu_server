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
    Author:        ozatomic
*/

#include "eve-server.h"

#include "PyServiceCD.h"
#include "account/InfoGatheringMgr.h"

PyCallable_Make_InnerDispatcher(InfoGatheringMgr)

InfoGatheringMgr::InfoGatheringMgr()
: PyService("infoGatheringMgr", new Dispatcher(this))
{
    PyCallable_REG_CALL(InfoGatheringMgr, GetStateAndConfig);
    PyCallable_REG_CALL(InfoGatheringMgr, LogInfoEventsFromClient);

}

InfoGatheringMgr::~InfoGatheringMgr() { }

PyResult InfoGatheringMgr::Handle_GetStateAndConfig(PyCallArgs &call)
{
    // TO-DO: find the purpose of all these values.
    PyDict *rsp = new PyDict;

    rsp->SetItemString("clientWorkerInterval", new PyInt(600000)); //Default From packetlogs is 600000
    rsp->SetItemString("isEnabled", new PyInt(1)); //0 = Disabled, 1 = Enabled. Set to 0 becuase jsut gettting rid of exception.

    PyDict *aggregates = new PyDict();
    aggregates->SetItem(new PyInt(infoEventOreMined), new BuiltinSet({1, 2, 3}));
    aggregates->SetItem(new PyInt(infoEventSalvagingAttempts), new BuiltinSet({1, 2, 6}));
    aggregates->SetItem(new PyInt(infoEventHackingAttempts), new BuiltinSet({1, 2, 6}));
    aggregates->SetItem(new PyInt(infoEventArcheologyAttempts), new BuiltinSet({1, 2, 6}));
    aggregates->SetItem(new PyInt(infoEventScanningAttempts), new BuiltinSet({1, 2, 3, 4}));
    aggregates->SetItem(new PyInt(infoEventFleet), new BuiltinSet({1}));
    aggregates->SetItem(new PyInt(infoEventFleetCreated), new BuiltinSet({1, 4}));
    aggregates->SetItem(new PyInt(infoEventFleetBroadcast), new BuiltinSet({1, 4}));
    aggregates->SetItem(new PyInt(infoEventNPCKilled), new BuiltinSet({1, 2, 3}));
    aggregates->SetItem(new PyInt(infoEventRefinedTypesAmount), new BuiltinSet({1, 2, 3}));
    aggregates->SetItem(new PyInt(infoEventRefiningYieldTypesAmount), new BuiltinSet({1, 2, 3}));
    aggregates->SetItem(new PyInt(infoEventPlanetUserAccess), new BuiltinSet({1}));
    aggregates->SetItem(new PyInt(infoEventPlanetInstallProgramQuery), new BuiltinSet({1, 3}));
    aggregates->SetItem(new PyInt(infoEventPlanetUpdateNetwork), new BuiltinSet({1, 2}));
    aggregates->SetItem(new PyInt(infoEventPlanetAbandonPlanet), new BuiltinSet({1, 3}));
    aggregates->SetItem(new PyInt(infoEventPlanetEstablishColony), new BuiltinSet({1, 3}));
    aggregates->SetItem(new PyInt(infoEventEntityKillWithoutBounty), new BuiltinSet({1, 2, 3, 4, 5}));
    aggregates->SetItem(new PyInt(infoEventViewStateUsage), new BuiltinSet({1, 6}));
    aggregates->SetItem(new PyInt(infoEventDoubleclickToMove), new BuiltinSet({1}));
    aggregates->SetItem(new PyInt(infoEventCCDuration), new BuiltinSet({1, 2, 6, 7}));
    aggregates->SetItem(new PyInt(infoEvenTrackingCameraEnabled), new BuiltinSet({1, 4}));
    aggregates->SetItem(new PyInt(infoEventRadialMenuAction), new BuiltinSet({1, 3, 6}));
    aggregates->SetItem(new PyInt(infoEventISISCounters), new BuiltinSet({1}));
    aggregates->SetItem(new PyInt(infoEventCareerFunnel), new BuiltinSet({1, 2, 6, 7}));
    aggregates->SetItem(new PyInt(infoEventWndOpenedFirstTime), new BuiltinSet({1, 2, 6}));
    aggregates->SetItem(new PyInt(infoEventWndOpenedCounters), new BuiltinSet({1, 2, 6}));
    rsp->SetItemString("infoTypeAggregates", aggregates);
    rsp->SetItemString("infoTypesOncePerRun", new BuiltinSet(new PyList()));
    PyDict *params = new PyDict();
    params->SetItem(new PyInt(infoEventOreMined), new BuiltinSet({0, 1, 2, 3, 4}));
    params->SetItem(new PyInt(infoEventSalvagingAttempts), new BuiltinSet({0, 1, 2, 3, 6}));
    params->SetItem(new PyInt(infoEventHackingAttempts), new BuiltinSet({0, 1, 2, 3, 6}));
    params->SetItem(new PyInt(infoEventArcheologyAttempts), new BuiltinSet({0, 1, 2, 3, 6}));
    params->SetItem(new PyInt(infoEventScanningAttempts), new BuiltinSet({0, 1, 2, 3, 4, 5}));
    params->SetItem(new PyInt(infoEventFleet), new BuiltinSet({0, 1, 3}));
    params->SetItem(new PyInt(infoEventFleetCreated), new BuiltinSet({0, 1, 3, 4}));
    params->SetItem(new PyInt(infoEventFleetBroadcast), new BuiltinSet({0, 1, 3, 4}));
    params->SetItem(new PyInt(infoEventNPCKilled), new BuiltinSet({0, 1, 2, 3, 4}));
    params->SetItem(new PyInt(infoEventRefinedTypesAmount), new BuiltinSet({0, 1, 2, 3, 9}));
    params->SetItem(new PyInt(infoEventRefiningYieldTypesAmount), new BuiltinSet({0, 1, 2, 3, 9}));
    params->SetItem(new PyInt(infoEventPlanetResourceDepletion), new BuiltinSet({0, 1, 2, 3, 4, 5}));
    params->SetItem(new PyInt(infoEventPlanetResourceScanning), new BuiltinSet({0, 1, 2, 3, 4, 5, 9, 10}));
    params->SetItem(new PyInt(infoEventPlanetUserAccess), new BuiltinSet({0, 1, 3, 4, 5}));
    params->SetItem(new PyInt(infoEventPlanetInstallProgramQuery), new BuiltinSet({0, 1, 3, 4}));
    params->SetItem(new PyInt(infoEventPlanetUpdateNetwork), new BuiltinSet({0, 1, 2, 3, 4, 5}));
    params->SetItem(new PyInt(infoEventPlanetAbandonPlanet), new BuiltinSet({0, 1, 3, 4}));
    params->SetItem(new PyInt(infoEventPlanetEstablishColony), new BuiltinSet({0, 1, 3, 4}));
    params->SetItem(new PyInt(infoEventEntityKillWithoutBounty), new BuiltinSet({0, 1, 2, 3, 4, 5, 9}));
    params->SetItem(new PyInt(infoEventNexCloseNex), new BuiltinSet({0, 1, 2, 3}));
    params->SetItem(new PyInt(infoEventViewStateUsage), new BuiltinSet({0, 1, 3, 6, 9}));
    params->SetItem(new PyInt(infoEventDoubleclickToMove), new BuiltinSet({0, 1, 3}));
    params->SetItem(new PyInt(infoEventCCDuration), new BuiltinSet({0, 1, 2, 3, 6, 7, 9}));
    params->SetItem(new PyInt(infoEvenTrackingCameraEnabled), new BuiltinSet({0, 1, 3, 4}));
    params->SetItem(new PyInt(infoEventRadialMenuAction), new BuiltinSet({0, 1, 3, 4, 6}));
    params->SetItem(new PyInt(infoEventISISCounters), new BuiltinSet({0, 1, 3, 4, 5, 9}));
    params->SetItem(new PyInt(infoEventCareerFunnel), new BuiltinSet({0, 1, 2, 3, 6, 7}));
    params->SetItem(new PyInt(infoEventWndOpenedFirstTime), new BuiltinSet({0, 1, 2, 3, 6}));
    params->SetItem(new PyInt(infoEventWndOpenedCounters), new BuiltinSet({0, 1, 2, 3, 6}));
    params->SetItem(new PyInt(infoEventTaskCompleted), new BuiltinSet({0, 1, 2, 3, 4, 5, 9, 10, 11}));
    params->SetItem(new PyInt(infoEventCharacterCreationStep), new BuiltinSet({0, 1, 2, 3, 4, 5, 9}));
    params->SetItem(new PyInt(infoEventCharacterFinalStep), new BuiltinSet({0, 1, 2, 3, 4, 5, 9, 10}));
    params->SetItem(new PyInt(infoEventVideoPlayed), new BuiltinSet({0, 1, 2, 3, 6}));
    rsp->SetItemString("infoTypeParameters", params);

    std::vector<int32> infoTypesVals = {
        infoEventOreMined,
        infoEventSalvagingAttempts,
        infoEventHackingAttempts,
        infoEventArcheologyAttempts,
        infoEventScanningAttempts,
        infoEventFleet,
        infoEventFleetCreated,
        infoEventFleetBroadcast,
        infoEventNPCKilled,
        infoEventRefinedTypesAmount,
        infoEventRefiningYieldTypesAmount,
        infoEventPlanetResourceDepletion,
        infoEventPlanetResourceScanning,
        infoEventPlanetUserAccess,
        infoEventPlanetInstallProgramQuery,
        infoEventPlanetUpdateNetwork,
        infoEventPlanetAbandonPlanet,
        infoEventPlanetEstablishColony,
        infoEventEntityKillWithoutBounty,
        infoEventNexCloseNex,
        infoEventViewStateUsage,
        infoEventDoubleclickToMove,
        infoEventCCDuration,
        infoEvenTrackingCameraEnabled,
        infoEventRadialMenuAction,
        infoEventISISCounters,
        infoEventCareerFunnel,
        infoEventWndOpenedFirstTime,
        infoEventWndOpenedCounters,
        infoEventTaskCompleted,
        infoEventCharacterCreationStep,
        infoEventCharacterFinalStep,
        infoEventVideoPlayed
    };

    rsp->SetItemString("infoTypes", new BuiltinSet(infoTypesVals));

    return new PyObject( "utillib.KeyVal", rsp );
}

PyResult InfoGatheringMgr::Handle_LogInfoEventsFromClient(PyCallArgs &call) {
    return new PyDict();
}
