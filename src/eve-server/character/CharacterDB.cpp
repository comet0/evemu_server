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
    Author:        Zhur, Cometo
*/

#include "eve-server.h"

#include "EVEServerConfig.h"
#include "character/Character.h"
#include "character/CharacterDB.h"

CharacterDB::CharValidationSet CharacterDB::mNameValidation;
CharacterDB::CharIdNameMap CharacterDB::mIdNameContainer;
//to-do: this should probably have mutex protection on static member access.

void CharacterDB::Init()
{
    load_name_validation_set();
}

bool CharacterDB::ReportRespec(uint32 characterId)
{
    DBerror error;
    if (!DBcore::RunQuery(error, "UPDATE srvCharacter SET freeRespecs = freeRespecs - 1, lastRespecDateTime = %" PRIu64 ", nextRespecDateTime = %" PRIu64 " WHERE characterId = %u AND freeRespecs > 0",
        Win32TimeNow(), Win32TimeNow() + Win32Time_Year, characterId))
        return false;
    return true;
}

bool CharacterDB::GetRespecInfo(uint32 characterId, uint32& out_freeRespecs, uint64& out_lastRespec, uint64& out_nextRespec)
{
    DBQueryResult res;
    if (!DBcore::RunQuery(res, "SELECT freeRespecs, lastRespecDateTime, nextRespecDateTime FROM srvCharacter WHERE characterID = %u", characterId))
        return false;
    if (res.GetRowCount() < 1)
        return false;
    DBResultRow row;
    res.GetRow(row);
    out_freeRespecs = row.GetUInt(0);
    out_lastRespec = row.GetUInt64(1);
    out_nextRespec = row.GetUInt64(2);

    // can't have more than two
    if (out_freeRespecs == 2)
        out_nextRespec = 0;
    else if (out_freeRespecs < 2 && out_nextRespec < Win32TimeNow())
    {
        // you may get another
        out_freeRespecs++;
        if (out_freeRespecs == 1)
            out_nextRespec = Win32TimeNow() + Win32Time_Year;
        else
            out_nextRespec = 0;

        // reflect this in the database, too
        DBerror err;
        DBcore::RunQuery(err, "UPDATE srvCharacter SET freeRespecs = %u, nextRespecDateTime = %" PRIu64 " WHERE characterId = %u",
            out_freeRespecs, out_nextRespec, characterId);
    }

    return true;
}

uint64 CharacterDB::PrepareCharacterForDelete(uint32 accountID, uint32 charID)
{
    // calculate the point in time from which this character may be deleted
    uint64 deleteTime = Win32TimeNow() + (Win32Time_Second * EVEServerConfig::character.terminationDelay);

    // note: the queries relating to character deletion have been specifically designed to avoid wreaking havoc when used by a malicious client
    // the client can't lie to us about accountID, only charID

    DBerror error;
    uint32 affectedRows;
    DBcore::RunQuery(error, affectedRows, "UPDATE srvCharacter SET deletePrepareDateTime = %" PRIu64 " WHERE accountID = %u AND characterID = %u", deleteTime, accountID, charID);
    if (affectedRows != 1)
        return 0;

    return deleteTime;
}

void CharacterDB::CancelCharacterDeletePrepare(uint32 accountID, uint32 charID)
{
    DBerror error;
    uint32 affectedRows;
    DBcore::RunQuery(error, affectedRows, "UPDATE srvCharacter SET deletePrepareDateTime = 0 WHERE accountID = %u AND characterID = %u", accountID, charID);
    if (affectedRows != 1)
        codelog(CLIENT__ERROR, "Failed to cancel character deletion, affected rows: %u", affectedRows);
}

PyRep* CharacterDB::DeleteCharacter(uint32 accountID, uint32 charID)
{
    DBerror error;
    uint32 affectedRows;
    DBcore::RunQuery(error, affectedRows, "DELETE FROM srvCharacter WHERE deletePrepareDateTime > 0 AND deletePrepareDateTime <= %" PRIu64 " AND accountID = %u AND characterID = %u", Win32TimeNow(), accountID, charID);

    if (affectedRows == 1)
    {
        // valid request; this means we may use charID safely here
        DBcore::RunQuery(error, "DELETE FROM srvEntity WHERE ownerID = %u", charID);
        // indicates 'no error' to the client
        return NULL;
    }
    else
        return new PyString("Invalid delete request");
}

PyRep *CharacterDB::GetCharacterList(uint32 accountID) {
    DBQueryResult res;

    if(!DBcore::RunQuery(res,
        "SELECT"
        " characterID,"
        " itemName AS characterName,"
        " deletePrepareDateTime,"
        " gender,"
        " typeID"
        " FROM srvCharacter "
        "    LEFT JOIN srvEntity ON characterID = itemID"
        " WHERE accountID=%u", accountID))
    {
        codelog(SERVICE__ERROR, "Error in query: %s", res.error.c_str());
        return NULL;
    }

    return DBResultToCRowset(res);
}

bool CharacterDB::ValidateCharName(const char *name)
{
    if (name == NULL || *name == '\0')
        return false;

    /* hash the name */
    uint32 hash = djb2_hash(name);

    /* check if its in our std::set */
    CharValidationSetItr itr = mNameValidation.find(hash);

    /* if itr is not equal to the end of the set it means that the same hash has been found */
    if (itr != mNameValidation.end())
        return false;

    /* if we got here the name is "new" */
    return true;
}

PyRep *CharacterDB::GetCharSelectInfo(uint32 characterID) {
    DBQueryResult res;

    uint32 worldSpaceID = 0;

    std::string shipName = "My Ship";
    uint32 shipTypeID = 606;

    DBQueryResult res2;
    if(!DBcore::RunQuery(res2, "SELECT itemName, typeID FROM srvEntity WHERE itemID = (SELECT shipID FROM srvCharacter WHERE characterID = %u)", characterID))
    {
        codelog(SERVICE__WARNING, "Unable to get current ship: %s", res.error.c_str());
    }
    else
    {
        DBResultRow row;
        while(res2.GetRow(row))
        {
            DBcore::DoEscapeString(shipName, row.GetText(0));
            shipTypeID = row.GetUInt(1);
        }
    }

    uint32 unreadMailCount = 0;
    uint32 upcomingEventCount = 0;
    uint32 unprocessedNotifications = 0;
    uint32 daysLeft = 14;
    uint32 userType = 23;
    uint64 allianceMemberStartDate = Win32TimeNow() - 15*Win32Time_Day;
    uint64 startDate = Win32TimeNow() - 24*Win32Time_Day;

    if(!DBcore::RunQuery(res,
        "SELECT "
        " itemName AS shortName,bloodlineID,gender,bounty,srvCharacter.corporationID,allianceID,title,startDateTime,createDateTime,"
        " securityRating,srvCharacter.balance, 0 As aurBalance,srvCharacter.stationID,solarSystemID,constellationID,regionID,"
        " petitionMessage,logonMinutes,tickerName, %u AS worldSpaceID, '%s' AS shipName, %u AS shipTypeID, %u AS unreadMailCount,"
        " %u AS upcomingEventCount, %u AS unprocessedNotifications, %u AS daysLeft, %u AS userType, 0 AS paperDollState, 0 AS newPaperdollState,"
        " 0 AS oldPaperdollState, skillPoints, skillQueueEndTime, %" PRIu64 " AS allianceMemberStartDate, %" PRIu64 " AS startDate,"
        " 0 AS locationSecurity"
        " FROM srvCharacter "
        "    LEFT JOIN srvEntity ON characterID = itemID"
        "    LEFT JOIN srvCorporation USING (corporationID)"
        "    LEFT JOIN blkBloodlineTypes USING (typeID)"
        " WHERE characterID=%u", worldSpaceID, shipName.c_str(), shipTypeID, unreadMailCount, upcomingEventCount, unprocessedNotifications, daysLeft, userType, allianceMemberStartDate, startDate, characterID))
    {
        codelog(SERVICE__ERROR, "Error in query: %s", res.error.c_str());
        return NULL;
    }

    return DBResultToCRowset(res);
}

PyObject *CharacterDB::GetCharPublicInfo(uint32 characterID) {
    DBQueryResult res;

    if(!DBcore::RunQuery(res,
        "SELECT "
        " srvEntity.typeID,"
        " srvCharacter.corporationID,"
        " chrBloodlines.raceID,"
        " blkBloodlineTypes.bloodlineID,"
        " srvCharacter.ancestryID,"
        " srvCharacter.careerID,"
        " srvCharacter.schoolID,"
        " srvCharacter.careerSpecialityID,"
        " srvEntity.itemName AS characterName,"
        " 0 as age,"    //hack
        " srvCharacter.createDateTime,"
        " srvCharacter.gender,"
        " srvCharacter.characterID,"
        " srvCharacter.description,"
        " srvCharacter.corporationDateTime"
        " FROM srvCharacter "
        "    LEFT JOIN srvEntity ON characterID = itemID"
        "    LEFT JOIN blkBloodlineTypes USING (typeID)"
        "    LEFT JOIN chrBloodlines USING (bloodlineID)"
        " WHERE characterID=%u", characterID))
    {
        codelog(SERVICE__ERROR, "Error in query: %s", res.error.c_str());
        return NULL;
    }

    DBResultRow row;
    if(!res.GetRow(row)) {
        _log(SERVICE__ERROR, "Error in GetCharPublicInfo query: no data for char %d", characterID);
        return NULL;
    }
    return(DBRowToKeyVal(row));

}

//void CharacterDB::GetCharacterData(uint32 characterID, std::vector<uint32> &characterDataVector) {
void CharacterDB::GetCharacterData(uint32 characterID, std::map<std::string, uint32> &characterDataMap) {

    DBQueryResult res;
    DBResultRow row;

    if(!DBcore::RunQuery(res,
        "SELECT "
        "  srvCharacter.corporationID, "
        "  srvCharacter.stationID, "
        "  srvCharacter.solarSystemID, "
        "  srvCharacter.constellationID, "
        "  srvCharacter.regionID, "
        "  srvCorporation.stationID, "
        "  srvCharacter.corpRole, "
        "  srvCharacter.rolesAtAll, "
        "  srvCharacter.rolesAtBase, "
        "  srvCharacter.rolesAtHQ, "
        "  srvCharacter.rolesAtOther, "
        "  srvCharacter.shipID, "
		"  srvCharacter.gender, "
        "  srvEntity.locationID, "
        "  chrBloodlines.raceID, "
        "  blkBloodlineTypes.bloodlineID "
        " FROM srvCharacter "
        "  LEFT JOIN srvCorporation USING (corporationID) "
        "  LEFT JOIN srvEntity ON srvEntity.itemID = srvCharacter.characterID "
        "  LEFT JOIN blkBloodlineTypes USING (typeID)"
        "  LEFT JOIN chrBloodlines USING (bloodlineID)"
        " WHERE characterID = %u",
        characterID))
    {
        SysLog::Error("CharacterDB::GetCharPublicInfo2()", "Failed to query HQ of character's %u corporation: %s.", characterID, res.error.c_str());
    }

    if(!res.GetRow(row))
    {
        SysLog::Error("CharacterDB::GetCharacterData()", "No valid rows were returned by the database query.");
    }

//    std::map<std::string,uint32> characterDataMap;
//    for( uint32 i=0; i<=characterDataVector.size(); i++ )
//        characterDataVector.push_back( row.GetUInt(i) );

    characterDataMap["corporationID"] = row.GetUInt(0);
    characterDataMap["stationID"] = row.GetUInt(1);
    characterDataMap["solarSystemID"] = row.GetUInt(2);
    characterDataMap["constellationID"] = row.GetUInt(3);
    characterDataMap["regionID"] = row.GetUInt(4);
    characterDataMap["corporationHQ"] = row.GetUInt(5);
    characterDataMap["corpRole"] = row.GetUInt(6);
    characterDataMap["rolesAtAll"] = row.GetUInt(7);
    characterDataMap["rolesAtBase"] = row.GetUInt(8);
    characterDataMap["rolesAtHQ"] = row.GetUInt(9);
    characterDataMap["rolesAtOther"] = row.GetUInt(10);
    characterDataMap["shipID"] = row.GetUInt(11);
	characterDataMap["gender"] = row.GetUInt(12);
    characterDataMap["locationID"] = row.GetUInt(13);
    characterDataMap["raceID"] = row.GetUInt(14);
    characterDataMap["bloodlineID"] = row.GetUInt(15);
}

PyObject *CharacterDB::GetCharPublicInfo3(uint32 characterID) {

    DBQueryResult res;

    if(!DBcore::RunQuery(res,
        "SELECT "
        " srvCharacter.bounty,"
        " srvCharacter.title,"
        " srvCharacter.startDateTime,"
        " srvCharacter.description,"
        " srvCharacter.corporationID"
        " FROM srvCharacter"
        " WHERE characterID=%u", characterID))
    {
        codelog(SERVICE__ERROR, "Error in query: %s", res.error.c_str());
        return NULL;
    }

    return DBResultToRowset(res);
}

//just return all itemIDs which has ownerID set to characterID
bool CharacterDB::GetCharItems(uint32 characterID, std::vector<uint32> &into) {
    DBQueryResult res;

    if(!DBcore::RunQuery(res,
        "SELECT"
        "  itemID"
        " FROM srvEntity"
        " WHERE ownerID = %u",
        characterID))
    {
        _log(DATABASE__ERROR, "Failed to query items of char %u: %s.", characterID, res.error.c_str());
        return false;
    }

    DBResultRow row;
    while(res.GetRow(row))
        into.push_back(row.GetUInt(0));

    return true;
}

//returns a list of the itemID for all the clones belonging to the character
bool CharacterDB::GetCharClones(uint32 characterID, std::vector<uint32> &into) {
    DBQueryResult res;

    if(!DBcore::RunQuery(res,
        "SELECT"
        "  itemID"
        " FROM srvEntity"
        " WHERE ownerID = %u"
        " and flag='400'",
        characterID))
    {
        _log(DATABASE__ERROR, "Failed to query clones of char %u: %s.", characterID, res.error.c_str());
        return false;
    }

    DBResultRow row;
    while(res.GetRow(row))
        into.push_back(row.GetUInt(0));

    return true;
}

//returns the itemID of the active clone
//if you want to get the typeID of the clone, please use GetActiveCloneType
bool CharacterDB::GetActiveClone(uint32 characterID, uint32 &itemID) {
    DBQueryResult res;

    if(!DBcore::RunQuery(res,
        "SELECT"
        "  itemID"
        " FROM srvEntity"
        " WHERE ownerID = %u"
        " and flag='400'"
        " and customInfo='active'",
        characterID))
    {
        _log(DATABASE__ERROR, "Failed to query active clone of char %u: %s.", characterID, res.error.c_str());
        return false;
    }

    DBResultRow row;
    res.GetRow(row);
    itemID=row.GetUInt(0);

    return true;
}

//we use this function because, when we change the clone type,
//the cached item type doesn't change, so we need to read it
//directly from the db
bool CharacterDB::GetActiveCloneType(uint32 characterID, uint32 &typeID) {
    DBQueryResult res;

    if(!DBcore::RunQuery(res,
        "SELECT"
        "  typeID"
        " FROM srvEntity"
        " WHERE ownerID = %u"
        " and flag='400'"
        " and customInfo='active'",
        characterID))
    {
        _log(DATABASE__ERROR, "Failed to query active clone of char %u: %s.", characterID, res.error.c_str());
        return false;
    }

    DBResultRow row;
    res.GetRow(row);
    typeID=row.GetUInt(0);

    return true;
}

// Return the Home station of the char based on the active clone
bool CharacterDB::GetCharHomeStation(uint32 characterID, uint32 &stationID) {
	uint32 activeCloneID;
	if( !GetActiveClone(characterID, activeCloneID) )
	{
		_log( DATABASE__ERROR, "Could't get the active clone for char %u", characterID );
		return false;
	}

	DBQueryResult res;
	if( !DBcore::RunQuery(res,
		"SELECT locationID "
		"FROM srvEntity "
		"WHERE itemID = %u",
		activeCloneID ))
	{
		_log(DATABASE__ERROR, "Could't get the location of the clone for char %u", characterID );
		return false;
	}

	DBResultRow row;
    res.GetRow(row);
    stationID = row.GetUInt(0);
	return true;
}

bool CharacterDB::GetCareerBySchool(uint32 schoolID, uint32 &careerID) {
    DBQueryResult res;
    if (!DBcore::RunQuery(res,
     "SELECT "
     " careerID, "
     " schoolID, "
     " raceID "
     " FROM blkChrCareers"
     " WHERE schoolID = %u", schoolID))
    {
        codelog(SERVICE__ERROR, "Error in query: %s", res.error.c_str());
        return (false);
    }


    DBResultRow row;
    if(!res.GetRow(row)) {
        codelog(SERVICE__ERROR, "Failed to find matching career for school %u", schoolID);
        return false;
    }

    careerID = row.GetInt(0);

    return (true);
}

bool CharacterDB::GetCorporationBySchool(uint32 schoolID, uint32 &corporationID)
{
    DBQueryResult res;

    if(!DBcore::RunQuery(res, "SELECT corporationID FROM blkChrSchools WHERE schoolID = %u", schoolID))
    {
        codelog(SERVICE__ERROR, "Error in query: %S", res.error.c_str());
        return false;
    }

    DBResultRow row;
    if(!res.GetRow(row))
    {
        codelog(SERVICE__ERROR, "Failed to find matching corporation for school %u", schoolID);
        return false;
    }

    corporationID = row.GetInt(0);

    return true;
}

/**
  * @todo Here should come a call to Corp??::CharacterJoinToCorp or what the heck... for now we only put it there
  */
bool CharacterDB::GetLocationCorporationByCareer(CharacterData &cdata) {
    DBQueryResult res;
    if (!DBcore::RunQuery(res,
                          "SELECT "
                          "  blkChrSchools.corporationID, "
                          "  blkChrSchools.schoolID, "
                          "  allianceID, "
                          "  stationID, "
                          "  solarSystemID, "
                          "  constellationID, "
                          "  regionID "
                          " FROM staStations"
                          "  LEFT JOIN srvCorporation USING(stationID)"
                          "  LEFT JOIN blkChrSchools ON srvCorporation.corporationID=blkChrSchools.corporationID"
                          "  LEFT JOIN blkChrCareers ON blkChrSchools.schoolID=blkChrCareers.schoolID"
                          " WHERE blkChrCareers.careerID = %u", cdata.careerID))
    {
        codelog(SERVICE__ERROR, "Error in query: %s", res.error.c_str());
        return (false);
    }

    DBResultRow row;
    if(!res.GetRow(row)) {
        codelog(SERVICE__ERROR, "Failed to find career %u", cdata.careerID);
        return false;
    }

    cdata.corporationID = row.GetUInt(0);
    cdata.schoolID = row.GetUInt(1);
    cdata.allianceID = row.GetUInt(2);

    cdata.stationID = row.GetUInt(3);
    cdata.solarSystemID = row.GetUInt(4);
    cdata.constellationID = row.GetUInt(5);
    cdata.regionID = row.GetUInt(6);

    return (true);
}

bool CharacterDB::GetLocationByStation(uint32 staID, CharacterData &cdata) {
    DBQueryResult res;
    if (!DBcore::RunQuery(res,
     "SELECT "
     "  stationID, "
     "  solarSystemID, "
     "  constellationID, "
     "  regionID "
     " FROM staStations"
     " WHERE stationID = %u", staID))
    {
        codelog(SERVICE__ERROR, "Error in query: %s", res.error.c_str());
        return (false);
    }

    DBResultRow row;
    if(!res.GetRow(row)) {
        codelog(SERVICE__ERROR, "Failed to find station %u", staID);
        return false;
    }

    cdata.stationID = staID;
    cdata.solarSystemID = row.GetUInt(1);
    cdata.constellationID = row.GetUInt(2);
    cdata.regionID = row.GetUInt(3);

    return (true);

}

bool CharacterDB::GetCareerStationByCorporation(uint32 corporationID, uint32 &stationID)
{
    DBQueryResult res;
    if(!DBcore::RunQuery(res, "SELECT stationID FROM srvCorporation WHERE corporationID = %u", corporationID))
    {
        codelog(SERVICE__ERROR, "Error in query: %s", res.error.c_str());
        return false;
    }

    DBResultRow row;
    if(!res.GetRow(row))
    {
        codelog(SERVICE__ERROR, "Failed to find corporation %u", corporationID);
        return false;
    }

    stationID = row.GetUInt(0);
    return true;
}

bool CharacterDB::DoesCorporationExist(uint32 corpID) {
    DBQueryResult res;
    if (!DBcore::RunQuery(res,
     "SELECT "
     "  corporationID, "
     "  corporationName "
     " FROM srvCorporation"
     " WHERE corporationID = %u", corpID))
    {
        codelog(SERVICE__ERROR, "Error in query: %s", res.error.c_str());
        return false;
    }

    DBResultRow row;
    if(!res.GetRow(row)) {
        codelog(SERVICE__ERROR, "Failed to find corporation %u", corpID);
        return false;
    }
    return true;
}

void CharacterDB::SetAvatar(uint32 charID, PyRep* hairDarkness) {
	//populate the DB wiht avatar information
	DBerror err;
	if(!DBcore::RunQuery(err,
		"INSERT INTO srvAvatars ("
		"charID, hairDarkness)"
		"VALUES (%u, %f)",
		charID, pyAs(Float, hairDarkness)->value()))
	{
		codelog(SERVICE__ERROR, "Error in query: %s", err.c_str());
	}
}

void CharacterDB::SetAvatarColors(uint32 charID, uint32 colorID, uint32 colorNameA, uint32 colorNameBC, double weight, double gloss) {
	//add avatar colors to the DB
	DBerror err;
	if(!DBcore::RunQuery(err,
		"INSERT INTO srvAvatarColors ("
		"charID, colorID, colorNameA, colorNameBC, weight, gloss)"
		"VALUES (%u, %u, %u, %u, %f, %f)",
		charID, colorID, colorNameA, colorNameBC, weight, gloss))
	{
		codelog(SERVICE__ERROR, "Error in query: %s", err.c_str());
	}
}

void CharacterDB::SetAvatarModifiers(uint32 charID, PyRep* modifierLocationID,  PyRep* paperdollResourceID, PyRep* paperdollResourceVariation) {
	//add avatar modifiers to the DB
	DBerror err;
	if(!DBcore::RunQuery(err,
		"INSERT INTO srvAvatarModifiers ("
		"charID, modifierLocationID, paperdollResourceID, paperdollResourceVariation)"
		"VALUES (%u, %u, %u, %u)",
		charID,
		pyAs(Int, modifierLocationID)->value(),
		pyAs(Int, paperdollResourceID)->value(),
		pyIs(Int, paperdollResourceVariation) ? pyAs(Int, paperdollResourceVariation)->value() : 0 ))
	{
		codelog(SERVICE__ERROR, "Error in query: %s", err.c_str());
	}
}

void CharacterDB::SetAvatarSculpts(uint32 charID, PyRep* sculptLocationID, PyRep* weightUpDown, PyRep* weightLeftRight, PyRep* weightForwardBack) {
	//add avatar sculpts to the DB
	DBerror err;
	if(!DBcore::RunQuery(err,
		"INSERT INTO srvAvatarSculpts ("
		"charID, sculptLocationID, weightUpDown, weightLeftRight, weightForwardBack)"
		"VALUES (%u, %u, %f, %f, %f)",
		charID,
		pyAs(Int, sculptLocationID)->value(),
		pyIs(Float, weightUpDown) ? pyAs(Float, weightUpDown)->value() : 0.0f,
		pyIs(Float, weightLeftRight) ? pyAs(Float, weightLeftRight)->value() : 0.0f,
		pyIs(Float, weightForwardBack) ? pyAs(Float, weightForwardBack)->value() : 0.0f))
	{
		codelog(SERVICE__ERROR, "Error in query: %s", err.c_str());
	}
}

bool CharacterDB::GetSkillsByRace(uint32 raceID, std::map<uint32, uint32> &into) {
    DBQueryResult res;

    if (!DBcore::RunQuery(res,
        "SELECT "
        "        skillTypeID, levels"
        " FROM blkChrRaceSkills "
        " WHERE raceID = %u ", raceID))
    {
        _log(SERVICE__ERROR, "Error in query: %s", res.error.c_str());
        return false;
    }

    DBResultRow row;
    while(res.GetRow(row)) {
        if(into.find(row.GetUInt(0)) == into.end())
            into[row.GetUInt(0)] = row.GetUInt(1);
        else
            into[row.GetUInt(0)] += row.GetUInt(1);
        //check to avoid more than 5 levels by skill
        if(into[row.GetUInt(0)] > 5)
            into[row.GetUInt(0)] = 5;
    }

    return true;
}

bool CharacterDB::GetSkillsByCareer(uint32 careerID, std::map<uint32, uint32> &into) {
    DBQueryResult res;

    if (!DBcore::RunQuery(res,
        "SELECT "
        "        skillTypeID, levels"
        " FROM blkChrCareerSkills"
        " WHERE careerID = %u", careerID))
    {
        _log(SERVICE__ERROR, "Error in query: %s", res.error.c_str());
        return false;
    }

    DBResultRow row;
    while(res.GetRow(row)) {
        if(into.find(row.GetUInt(0)) == into.end())
            into[row.GetUInt(0)] = row.GetUInt(1);
        else
            into[row.GetUInt(0)] += row.GetUInt(1);
        //check to avoid more than 5 levels by skill
        if(into[row.GetUInt(0)] > 5)
            into[row.GetUInt(0)] = 5;
    }

    return true;
}

bool CharacterDB::GetSkillsByCareerSpeciality(uint32 careerSpecialityID, std::map<uint32, uint32> &into) {
    DBQueryResult res;

    if (!DBcore::RunQuery(res,
        "SELECT "
        "        skillTypeID, levels"
        " FROM blkChrSpecialitySkills"
        " WHERE specialityID = %u", careerSpecialityID))
    {
        _log(SERVICE__ERROR, "Error in query: %s", res.error.c_str());
        return false;
    }

    DBResultRow row;
    while(res.GetRow(row)) {
        if(into.find(row.GetUInt(0)) == into.end())
            into[row.GetUInt(0)] = row.GetUInt(1);
        else
            into[row.GetUInt(0)] += row.GetUInt(1);
        //check to avoid more than 5 levels by skill
        if(into[row.GetUInt(0)] > 5)
            into[row.GetUInt(0)] = 5;
    }

    return true;
}

PyString *CharacterDB::GetNote(uint32 ownerID, uint32 itemID) {
    DBQueryResult res;

    if (!DBcore::RunQuery(res,
            "SELECT `note` FROM `srvChrNotes` WHERE ownerID = %u AND itemID = %u",
            ownerID, itemID)
        )
    {
        codelog(SERVICE__ERROR, "Error on query: %s", res.error.c_str());
        return NULL;
    }
    DBResultRow row;
    if(!res.GetRow(row))
        return NULL;

    return(new PyString(row.GetText(0)));
}

bool CharacterDB::SetNote(uint32 ownerID, uint32 itemID, const char *str) {
    DBerror err;

    if (str[0] == '\0') {
        // str is empty
        if (!DBcore::RunQuery(err,
            "DELETE FROM `srvChrNotes` "
            " WHERE itemID = %u AND ownerID = %u LIMIT 1",
            ownerID, itemID)
            )
        {
            codelog(CLIENT__ERROR, "Error on query: %s", err.c_str());
            return false;
        }
    } else {
        // escape it before insertion
        std::string escaped;
        DBcore::DoEscapeString(escaped, str);

        if (!DBcore::RunQuery(err,
            "REPLACE INTO `srvChrNotes` (itemID, ownerID, note)    "
            "VALUES (%u, %u, '%s')",
            ownerID, itemID, escaped.c_str())
            )
        {
            codelog(CLIENT__ERROR, "Error on query: %s", err.c_str());
            return false;
        }
    }

    return true;
}

uint32 CharacterDB::AddOwnerNote(uint32 charID, const std::string & label, const std::string & content) {
    DBerror err;
    uint32 id;

    std::string lblS;
    DBcore::DoEscapeString(lblS, label);

    std::string contS;
    DBcore::DoEscapeString(contS, content);

    if (!DBcore::RunQueryLID(err, id,
        "INSERT INTO srvChrOwnerNote (ownerID, label, note) VALUES (%u, '%s', '%s');",
        charID, lblS.c_str(), contS.c_str()))
    {
        codelog(SERVICE__ERROR, "Error on query: %s", err.c_str());
        return 0;
    }

    return id;
}

bool CharacterDB::EditOwnerNote(uint32 charID, uint32 noteID, const std::string & label, const std::string & content) {
    DBerror err;

    std::string contS;
    DBcore::DoEscapeString(contS, content);

    if (!DBcore::RunQuery(err,
        "UPDATE srvChrOwnerNote SET note = '%s' WHERE ownerID = %u AND noteID = %u;",
        contS.c_str(), charID, noteID))
    {
        codelog(SERVICE__ERROR, "Error on query: %s", err.c_str());
        return false;
    }

    return true;
}

PyObject *CharacterDB::GetOwnerNoteLabels(uint32 charID) {
    DBQueryResult res;

    if (!DBcore::RunQuery(res, "SELECT noteID, label FROM srvChrOwnerNote WHERE ownerID = %u", charID))
    {
        codelog(SERVICE__ERROR, "Error on query: %s", res.error.c_str());
        return (NULL);
    }

    return DBResultToRowset(res);
}

PyObject *CharacterDB::GetOwnerNote(uint32 charID, uint32 noteID) {
    DBQueryResult res;

    if (!DBcore::RunQuery(res, "SELECT note FROM srvChrOwnerNote WHERE ownerID = %u AND noteID = %u", charID, noteID))
    {
        codelog(SERVICE__ERROR, "Error on query: %s", res.error.c_str());
        return (NULL);
    }

    return DBResultToRowset(res);
}

uint32 CharacterDB::djb2_hash( const char* str )
{
    uint32 hash = 5381;
    int c;

    while ((c = *str++))
        hash = ((hash << 5) + hash) + c; /* hash * 33 + c */

    return hash;
}

void CharacterDB::load_name_validation_set()
{
    DBQueryResult res;
    if(!DBcore::RunQuery(res,
        "SELECT"
        " characterID, itemName AS characterName"
        " FROM srvCharacter"
        "    JOIN srvEntity ON characterID = itemID"
        ))
    {
        codelog(SERVICE__ERROR, "Error in query for %s", res.error.c_str());
        return;
    }

    DBResultRow row;
    while(res.GetRow(row) == true)
    {
        uint32 characterID = row.GetUInt(0);
        const char* name = row.GetText(1);

        //printf("initializing name validation: %s\n", name);
        uint32 hash = djb2_hash(name);

        mNameValidation.insert(hash);
        mIdNameContainer.insert(std::make_pair(characterID, name));
    }
}

bool CharacterDB::add_name_validation_set( const char* name, uint32 characterID )
{
    if (name == NULL || *name == '\0')
        return false;

    uint32 hash = djb2_hash(name);

    /* check if the name is already present ( this should not be possible but we all know how hackers are ) */
    if (mNameValidation.find(hash) != mNameValidation.end())
    {
        printf("CharacterDB::add_name_validation_set: unable to add: %s as its a dupe", name);
        return false;
    }

    mNameValidation.insert(hash);
    mIdNameContainer.insert(std::make_pair(characterID, name));
    return true;
}

bool CharacterDB::del_name_validation_set( uint32 characterID )
{
    CharIdNameMapItr helper_itr = mIdNameContainer.find(characterID);

    /* if we are unable to find the entry... return.
     * @note we do risk keeping the name in the name validation.
     * which I am willing to take.
     */
    if (helper_itr == mIdNameContainer.end())
        return false;

    const char* name = helper_itr->second.c_str();
    if (name == NULL || *name == '\0')
        return false;

    uint32 hash = djb2_hash(name);

    CharValidationSetItr name_itr = mNameValidation.find(hash);
    if (name_itr != mNameValidation.end())
    {
        // we found the name hash... deleting
        mNameValidation.erase(name_itr);
        mIdNameContainer.erase(helper_itr);
        return true;
    }
    else
    {
        /* normally this should never happen... */
        printf("CharacterDB::del_name_validation_set: unable to remove: %s as its not in the set", name);
        return false;
    }
}

PyObject *CharacterDB::GetTopBounties() {
    DBQueryResult res;
    if(!DBcore::RunQuery(res, "SELECT `characterID`, `itemName` as `ownerName`, `bounty`, `online`  FROM srvCharacter  LEFT JOIN `srvEntity` ON `characterID` = `itemID` WHERE `characterID` >= %u AND `bounty` > 0 ORDER BY `bounty` DESC LIMIT 0,100" , EVEMU_MINIMUM_ID)) {
        SysLog::Error("CharacterDB", "Error in GetTopBounties query: %s", res.error.c_str());
        return NULL;
    }
    return DBResultToRowset(res);
}

uint32 CharacterDB::GetBounty(uint32 charID) {
    DBQueryResult res;
    if(!DBcore::RunQuery(res, "SELECT `bounty` FROM srvCharacter WHERE `characterID` = %u", charID)) {
        SysLog::Error("CharacterDB", "Error in GetBounty query: %s", res.error.c_str());
        return 0;
    }
    DBResultRow row;
    if(!res.GetRow(row))
        return 0;
    else
        return row.GetUInt(0);
}

bool CharacterDB::AddBounty(uint32 charID, uint32 ammount) {
    DBerror err;
    uint32 total = GetBounty(charID) + ammount;
    if(!DBcore::RunQuery(err, "UPDATE srvCharacter SET `bounty` = %u WHERE `characterID` = %u", total, charID))
        return false;
    else
        return true;
}
