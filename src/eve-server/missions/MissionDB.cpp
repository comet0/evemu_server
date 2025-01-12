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

#include "missions/MissionDB.h"

PyObjectEx *MissionDB::GetAgents()
{
    DBQueryResult res;

    if(!DBcore::RunQuery(res,
                         "SELECT"
                         " agt.agentID,"
                         " agt.agentTypeID,"
                         " agt.divisionID,"
                         " agt.level,"
                         " IFNULL(chr.stationID, 0) AS stationID,"
                         " bl.bloodlineID,"
                         " agt.quality,"
                         " agt.corporationID,"
                         " IFNULL(chr.gender, 0) AS gender,"
                         " agt.isLocator AS isLocatorAgent"
                         " FROM agtAgents AS agt"
                         " LEFT JOIN blkCharacterStatic AS chr ON chr.characterID = agt.agentID"
                         " LEFT JOIN blkBloodlineTypes AS bl ON bl.bloodlineID = agt.agentTypeID"
                         ))
    {
        codelog(SERVICE__ERROR, "Error in query: %s", res.error.c_str());
        return NULL;
    }

    return (DBResultToCRowset(res));
}

PyDict *MissionDB::GetAgentsInSpace()
{
    DBQueryResult res;

    if(!DBcore::RunQuery(res,
                         "SELECT agentID, locationID FROM agtAgents WHERE locationID < 33000000"
                         ))
    {
        codelog(SERVICE__ERROR, "Error in query: %s", res.error.c_str());
        return NULL;
    }

    PyDict *dict = new PyDict();
    DBResultRow row;
    while(res.GetRow(row))
    {
        uint32 agentID = row.GetInt(0);
        uint32 locationID = row.GetInt(1);
        dict->SetItem(new PyInt(agentID), new PyInt(locationID));
    }

    return dict;
}

#ifdef NOT_DONE
AgentLevel *MissionDB::LoadAgentLevel(uint8 level) {
    AgentLevel *result = new AgentLevel;

    DBQueryResult res;

    if(!DBcore::RunQuery(res,
        "SELECT missionID,missionName,missionLevel,"
        "    blkAgtMissions.missionTypeID,missionTypeName,"
        "    importantMission"
        " FROM blkAgtMissions"
        "    NATURAL JOIN blkAgtMissionTypes"
        " WHERE missionLevel=%d",
        level
    ))
    {
        codelog(SERVICE__ERROR, "Error in query: %s", res.error.c_str());
        delete result;
        return NULL;
    }

    std::vector<uint32> IDs;
    DBResultRow row;

    IDs.clear();
    while(res.GetRow(row)) {
        AgentMissionSpec *spec = new AgentMissionSpec;
        spec->missionID = row.GetUInt(0);
        spec->missionName = row.GetText(1);
        spec->missionLevel = row.GetUInt(2);
        spec->missionTypeID = row.GetUInt(3);
        spec->missionTypeName = row.GetText(1);
        spec->importantMission = (row.GetUInt(2)==0)?false:true;
        result->missions.push_back(spec);
    }

}
#endif
























