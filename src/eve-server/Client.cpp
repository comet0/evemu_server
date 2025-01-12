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

#include "eve-server.h"

#include "Client.h"
#include "LiveUpdateDB.h"
#include "PyBoundObject.h"
#include "services/lscProxy/LscProxyService.h"
#include "imageserver/ImageServer.h"
#include "npc/NPC.h"
#include "ship/DestinyManager.h"
#include "ship/ShipOperatorInterface.h"
#include "system/SystemManager.h"
#include "character/CharUnboundMgrService.h"
#include "PyServiceMgr.h"

static const uint32 PING_INTERVAL_US = 60000;

Client::Client(EVETCPConnection** con, EVEServerConfig::EVEConfigNet &net)
: DynamicSystemEntity(NULL),
EVEClientSession(con),
  m_pingTimer(PING_INTERVAL_US),
  m_system(NULL),
//  m_destinyTimer(1000, true), //accurate timing is essential
//  m_lastDestinyTime(Timer::GetTimeSeconds()),
  m_moveState(msIdle),
  m_moveTimer(500),
  m_movePoint(0, 0, 0),
  m_timeEndTrain(0),
  m_destinyEventQueue( new PyList ),
  m_destinyUpdateQueue( new PyList ),
m_nextNotifySequence(1),
m_networkConfig(net)
//  m_nextDestinyUpdate(46751)
{
    m_moveTimer.Disable();
    m_pingTimer.Start();

    m_dockStationID = 0;
    m_justUndocked = false;
    m_justUndockedCount = 0;
    m_needToDock = false;

    bKennyfied = false;     // by default, we do NOT want chat messages kennyfied, LOL

    // Start handshake
    reset();
}

Client::~Client() {
    if( GetChar() ) {
        // we have valid character

        // LSC logout
        PyServiceMgr::lscProxy_service->CharacterLogout(GetCharacterID(), LSCChannel::_MakeSenderInfo(this));

        //before we remove ourself from the system, store our last location.
        SavePosition();

        // Save character info including attributes, save current ship's attributes, current ship's fitted mModulesMgr,
        // and save all skill attributes to the Database:
		if( GetShip() )
			GetShip()->SaveShip();                              // Save Ship's and Modules' attributes and info to DB
		if( GetChar() )
		{
	        GetChar()->SaveFullCharacter();                     // Save Character info to DB
			GetChar()->SaveSkillQueue();                        // Save Skill Queue to DB
		}

        // remove ourselves from system
        if(m_system)
            m_system->RemoveClient(this);   //handles removing us from bubbles and sending RemoveBall events.

        //johnsus - characterOnline mod
        // switch character online flag to 0
        ServiceDB::SetCharacterOnlineStatus(GetCharacterID(), false);
    }

    if(GetAccountID() != 0) { // this is not very good ....
        ServiceDB::SetAccountOnlineStatus(GetAccountID(), false);
    }

    PyServiceMgr::ClearBoundObjects(this);

    targets.doDestruction();

    PyDecRef( m_destinyEventQueue );
    PyDecRef( m_destinyUpdateQueue );
}

bool Client::ProcessNet()
{
    if(getState() != TCPConnection::STATE_CONNECTED)
        return false;

    if(m_pingTimer.Check()) {
        //_log(CLIENT__TRACE, "%s: Sending ping request.", GetName());
        _SendPingRequest();
    }

    PyPacket *p;
    while((p = popPacket()))
    {
        if(is_log_enabled(CLIENT__IN_ALL))
        {
            _log(CLIENT__IN_ALL, "Received packet:");
            std::string pfx = getLogPrefix(CLIENT__IN_ALL);
            std::ostringstream ss;
            p->dump(ss, pfx);
            outputLogMsg(CLIENT__IN_ALL, ss.str().c_str());
        }

        try
        {
            if( !DispatchPacket( p ) )
                SysLog::Error( "Client", "%s: Failed to dispatch packet of type %s (%d).", GetName(), MACHONETMSG_TYPE_NAMES[ p->type ], (int)p->type );
        }
        catch( PyException& e )
        {
            _SendException( p->dest, p->source.callID, p->type, WRAPPEDEXCEPTION, &e.ssException );
        }

        SafeDelete( p );
    }

    CharacterRef charRef = GetChar();
    if (charRef.get() != nullptr)
    {
        GetChar()->updateSkillQueue();
    }

    // send queued updates
    _SendQueuedUpdates();

    return true;
}

void Client::Process() {
    if(m_moveTimer.Check(false)) {
        m_moveTimer.Disable();
        _MoveState s = m_moveState;
        m_moveState = msIdle;
        switch(s) {
        case msIdle:
            SysLog::Error("Client","%s: Move timer expired when no move is pending.", GetName());
            break;
        //used to delay stargate animation
        case msJump:
            _ExecuteJump();
            break;
        }
    }

    // Check Character Save Timer Expiry:
    if( GetChar()->CheckSaveTimer() )
        GetChar()->SaveCharacter();			// Should this perhaps be invoking GetChar()->SaveFullCharacter() or is saving basic character info enough here?

    // Check Ship Save Timer Expiry:
    if( GetShip()->CheckSaveTimer() )
        GetShip()->SaveShip();

    // Check Module Manager Save Timer Expiry:
    //if( mModulesMgr.CheckSaveTimer() )
    //    mModulesMgr.SaveModules();

    GetShip()->Process();

    SystemEntity::Process();
}

//this displays a modal error dialog on the client side.
void Client::SendErrorMsg( const char* fmt, ... )
{
    va_list args;
    va_start( args, fmt );

    char* str = NULL;
    vasprintf( &str, fmt, args );
    assert( str );

    SysLog::Error("Client","Sending Error Message to %s:", GetName() );
    log_messageVA( CLIENT__ERROR, fmt, args );
    va_end( args );

    //want to send some sort of notify with a "ServerMessage" message ID maybe?
    //else maybe a "ChatTxt"??
    Notify_OnRemoteMessage n;
    n.msgType = "CustomError";
    n.args[ "error" ] = new PyString( str );

    PyTuple* tmp = n.Encode();
    SendNotification( "OnRemoteMessage", "charid", &tmp );

    SafeFree( str );
}

void Client::SendErrorMsg( const char* fmt, va_list args )
{
    char* str = NULL;
    vasprintf( &str, fmt, args );
    assert( str );

    SysLog::Error("Client","Sending Error Message to %s:", GetName() );
    log_messageVA( CLIENT__ERROR, fmt, args );

    //want to send some sort of notify with a "ServerMessage" message ID maybe?
    //else maybe a "ChatTxt"??
    Notify_OnRemoteMessage n;
    n.msgType = "CustomError";
    n.args[ "error" ] = new PyString( str );

    PyTuple* tmp = n.Encode();
    SendNotification( "OnRemoteMessage", "charid", &tmp );

    SafeFree( str );

}

//this displays a modal info dialog on the client side.
void Client::SendInfoModalMsg( const char* fmt, ... )
{
    va_list args;
    va_start( args, fmt );

    char* str = NULL;
    vasprintf( &str, fmt, args );
    assert( str );

    SysLog::Log("Client","Info Modal to %s:", GetName() );
    log_messageVA( CLIENT__MESSAGE, fmt, args );
    va_end( args );

    //want to send some sort of notify with a "ServerMessage" message ID maybe?
    //else maybe a "ChatTxt"??
    Notify_OnRemoteMessage n;
    n.msgType = "ServerMessage";
    n.args[ "msg" ] = new PyString( str );

    PyTuple* tmp = n.Encode();
    SendNotification( "OnRemoteMessage", "charid", &tmp );

    SafeFree( str );
}

//this displays a little notice (like combat messages)
void Client::SendNotifyMsg( const char* fmt, ... )
{
    va_list args;
    va_start( args, fmt );

    char* str = NULL;
    vasprintf( &str, fmt, args );
    assert( str );

    SysLog::Log("Client","Notify to %s:", GetName() );
    log_messageVA( CLIENT__MESSAGE, fmt, args );
    va_end( args );

    //want to send some sort of notify with a "ServerMessage" message ID maybe?
    //else maybe a "ChatTxt"??
    Notify_OnRemoteMessage n;
    n.msgType = "CustomNotify";
    n.args[ "notify" ] = new PyString( str );

    PyTuple* tmp = n.Encode();
    SendNotification( "OnRemoteMessage", "charid", &tmp );

    SafeFree( str );
}

void Client::SendNotifyMsg( const char* fmt, va_list args )
{
    char* str = NULL;
    vasprintf( &str, fmt, args );
    assert( str );

    SysLog::Log("Client","Notify to %s:", GetName() );
    log_messageVA( CLIENT__MESSAGE, fmt, args );

    //want to send some sort of notify with a "ServerMessage" message ID maybe?
    //else maybe a "ChatTxt"??
    Notify_OnRemoteMessage n;
    n.msgType = "CustomNotify";
    n.args[ "notify" ] = new PyString( str );

    PyTuple* tmp = n.Encode();
    SendNotification( "OnRemoteMessage", "charid", &tmp );

    SafeFree( str );
}

//there may be a less hackish way to do this.
void Client::SelfChatMessage( const char* fmt, ... )
{
    va_list args;
    va_start( args, fmt );

    char* str = NULL;
    vasprintf( &str, fmt, args );
    assert( str );

    va_end( args );

    if( m_channels.empty() )
    {
        SysLog::Error("Client", "%s: Tried to send self chat, but we are not joined to any channels: %s", GetName(), str );
        free( str );
        return;
    }

    SysLog::Log("Client","%s: Self message on all channels: %s", GetName(), str );

    //this is such a pile of crap, but im not sure whats better.
    //maybe a private message...
    std::set<LSCChannel*>::iterator cur, end;
    cur = m_channels.begin();
    end = m_channels.end();
    for(; cur != end; ++cur)
        (*cur)->SendMessage( this, str, true );

    //m_channels[

    //just send it to the first channel we are in..
    /*LSCChannel *chan = *(m_channels.begin());
    char self_id[24];   //such crap..
    snprintf(self_id, sizeof(self_id), "%u", GetCharacterID());
    if(chan->GetName() == self_id) {
        if(m_channels.size() > 1) {
            chan = *(++m_channels.begin());
        }
    }*/

    SafeFree( str );
}

void Client::ChannelJoined(LSCChannel *chan) {
    m_channels.insert(chan);
}

void Client::ChannelLeft(LSCChannel *chan) {
    m_channels.erase(chan);
}

bool Client::EnterSystem(bool login) {

    if(m_system && m_system->GetID() != GetSystemID()) {
        //we have different m_system
        m_system->RemoveClient(this);
        m_system = NULL;

        delete m_destiny;
        m_destiny = NULL;
    }

    if(m_system == NULL) {
        //m_system is NULL, we need new system
        //find our system manager and register ourself with it.
        m_system = EntityList::FindOrBootSystem(GetSystemID());
        if(m_system == NULL) {
            SysLog::Error("Client", "Failed to boot system %u for char %s (%u)", GetSystemID(), GetName(), GetCharacterID());
            SendErrorMsg("Unable to boot system %u", GetSystemID());
            return false;
        }
        m_system->AddClient(this);
    }

    return true;
}

bool Client::UpdateLocation() {
    if(IsStation(GetLocationID())) {
        //we entered station, delete m_destiny
        delete m_destiny;
        m_destiny = NULL;

        //remove ourselves from any bubble
        //m_system->bubbles.Remove(this, false);
		m_system->RemoveClient(this);
		m_system = NULL;

        OnCharNowInStation();
    } else if(IsSolarSystem(GetLocationID())) {
        //we are in a system, so we need a destiny manager
        m_destiny = new DestinyManager(this, m_system);
        //ship should never be NULL.
        m_destiny->SetShipCapabilities( GetShip() );

        /*if( login )
        {
            // We are just logging in, so we need to warp to our last position from a
            // random vector 15.0AU away:
            Vector3D warpToPoint( GetShip()->position() );
            Vector3D warpFromPoint( GetShip()->position() );
            warpFromPoint.MakeRandomPointOnSphere( 15.0*ONE_AU_IN_METERS );
            m_destiny->SetPosition( warpFromPoint, true );
            WarpTo( warpToPoint, 0.0 );        // Warp ship from the random login point to the position saved on last disconnect
        }
        else */
        {

            // This is NOT a login, so we always enter a system stopped.
            m_destiny->Halt(false);
            //set position.
            m_destiny->SetPosition(GetShip()->position(), false);
        }
    }

    return true;
}

void Client::MoveToLocation( uint32 location, const Vector3D& pt )
{
    if( GetLocationID() == location )
    {
        // This is a warp or simple movement
        MoveToPosition( pt );
        return;
    }

    if( IsStation( GetLocationID() ) )
        OnCharNoLongerInStation();

    uint32 stationID, solarSystemID, constellationID, regionID;
    if( IsStation( location ) )
    {
        // Entering station
        stationID = location;

        ServiceDB::GetStationInfo(
            stationID,
            &solarSystemID, &constellationID, &regionID,
            NULL, NULL, NULL
        );

        GetShip()->Move( stationID, flagHangar );
    }
    else if( IsSolarSystem( location ) )
    {
        // Entering a solarsystem
        // source is GetLocation()
        // destination is location
        stationID = 0;
        solarSystemID = location;

        ServiceDB::GetSystemInfo(
            solarSystemID,
            &constellationID, &regionID,
            NULL, NULL
        );

        GetShip()->Move( solarSystemID, flagAutoFit );
        GetShip()->Relocate( pt );
    }
    else
    {
        SendErrorMsg( "Move requested to unsupported location %u", location );
        return;
    }

    //move the srvCharacter record... we really should derive the char's location from the entity table...
    GetChar()->SetLocation( stationID, solarSystemID, constellationID, regionID );
    //update session with new values
    _UpdateSession( GetChar() );

    EnterSystem( false );
    UpdateLocation();

    _SendSessionChange();
}

void Client::MoveToPosition(const Vector3D &pt) {
    if(m_destiny == NULL)
        return;
    m_destiny->Halt(true);
    m_destiny->SetPosition(pt, true);
    GetShip()->Relocate(pt);
}

void Client::MoveItem(uint32 itemID, uint32 location, EVEItemFlags flag)
{
    ItemFactory::SetUsingClient(this);
    InventoryItemRef item = ItemFactory::GetItem(itemID);
    if( !item ) {
        SysLog::Error("Client","%s: Unable to load item %u", GetName(), itemID);
        return;
    }

    bool was_module = ( item->flag() >= flagSlotFirst && item->flag() <= flagSlotLast);

    //do the move. This will update the DB and send the notification.
    item->Move(location, flag);

    if(was_module || (item->flag() >= flagSlotFirst && item->flag() <= flagSlotLast)) {
        //it was equipped, or is now. so mModulesMgr need to know.
        GetShip()->UpdateModules();
    }

    // Release the item factory now that the ItemFactory is finished being used:
    ItemFactory::UnsetUsingClient();
}

void Client::BoardShip(ShipRef new_ship) {

    if(!new_ship->singleton()) {
        SysLog::Error("Client","%s: tried to board ship %u, which is not assembled.", GetName(), new_ship->itemID());
        SendErrorMsg("You cannot board a ship which is not assembled!");
        return;
    }

    if((m_system) && (IsInSpace()))
        m_system->RemoveClient(this);

    // Set dynamic system entity item reference.
    m_self = new_ship;
  //  m_char->MoveInto( *new_ship, flagPilot, true );

    new_ship->GetOperator()->SetOperatorObject(this);

    m_shipId = new_ship->itemID();
    m_char->SetActiveShip(m_shipId);
    if (IsInSpace())
        mSession.SetInt( "shipid", new_ship->itemID() );

    GetShip()->UpdateModules();

    if((m_system) && (IsInSpace()))
        m_system->AddClient(this);

    if(m_destiny)
        m_destiny->SetShipCapabilities( GetShip() );

}

void Client::UpdateCorpSession(const CharacterConstRef& character)
{
    if (!character) return;

    mSession.SetInt("corpid", character->corporationID());
    mSession.SetInt("hqID", character->corporationHQ());
    mSession.SetInt("corpAccountKey", character->corpAccountKey());
    mSession.SetLong("corpRole", character->corpRole());
    mSession.SetLong("rolesAtAll", character->rolesAtAll());
    mSession.SetLong("rolesAtBase", character->rolesAtBase());
    mSession.SetLong("rolesAtHQ", character->rolesAtHQ());
    mSession.SetLong("rolesAtOther", character->rolesAtOther());

    _SendSessionChange();
}

void Client::UpdateFleetSession(const CharacterConstRef& character)
{
    if (!character) return;

    mSession.SetLong("fleetid", character->fleetID());
    mSession.SetInt("fleetrole", character->fleetRole());
    mSession.SetInt("fleetbooster", character->fleetBooster());
    mSession.SetInt("wingid", character->wingID());
    mSession.SetInt("squadid", character->squadID());

    _SendSessionChange();
}

void Client::_UpdateSession( const CharacterConstRef& character )
{
    if( !character )
        return;

    mSession.SetInt("constellationid", GetConstellationID());
    mSession.SetInt("corpid",          GetCorporationID());
    mSession.SetInt("regionid",        GetRegionID());
    mSession.SetInt("locationid",      GetLocationID());
    mSession.SetInt("hqID",            GetCorpHQ());
    mSession.SetInt("solarsystemid2",  character->solarSystemID());
    mSession.SetInt("shipid",          GetShipID());
    mSession.SetInt("charid",          GetCharacterID());

    if(IsInSpace())
    {
        mSession.SetInt("solarsystemid", character->solarSystemID());
    }
    else
    {
        mSession.SetInt("stationid2",   character->stationID());
        mSession.SetInt("worldspaceid", character->stationID());
        mSession.SetInt("stationid",    character->stationID());
    }
}

/* Session change notes
 * First session change sent to client after character creation (logging into space)
 *      genderID        bool-
 *      constellationid int-
 *      raceID          int-
 *      corpid          int-
 *      regionid        int-
 *      bloodlineID     int-
 *      locationid      int-
 *      hqID            int-
 *      solarsystemid2  int-
 *      solarsystemid   int
 *      shipid          int-
 *      charid          int-
 *
 * First session change sent to client while docked in station
 *      genderID        bool-
 *      shipid          int-
 *      constellationid int-
 *      bloodlineID     int-
 *      stationid2      int
 *      regionid        int-
 *      worldspaceid    int
 *      stationid       int
 *      locationid      int-
 *      hqID            int-
 *      raceID          int-
 *      solarsystemid2  int-
 *      corpid          int-
 *      charid          int-
 */

void Client::_UpdateSession2( uint32 characterID )
{
    std::vector<uint32> characterDataVector;
    std::map<std::string, uint32> characterDataMap;

    if( characterID == 0 )
    {
        SysLog::Error( "Client::_UpdateSession2()", "characterID == 0, which is illegal" );
        return;
    }

	uint16 gender = 0;
    uint32 corporationID = 0;
    uint32 stationID = 0;
    uint32 solarSystemID = 0;
    uint32 constellationID = 0;
    uint32 regionID = 0;
    uint32 corporationHQ = 0;
    uint32 corpRole = 0;
    uint32 rolesAtAll = 0;
    uint32 rolesAtBase = 0;
    uint32 rolesAtHQ = 0;
    uint32 rolesAtOther = 0;
    uint32 locationID = 0;
    uint32 shipID = 0;
    uint32 raceID = 0;
    uint32 bloodlineID = 0;

    ((CharUnboundMgrService *) (PyServiceMgr::LookupService("charUnboundMgr")))->GetCharacterData(characterID, characterDataMap);

    if( characterDataMap.size() == 0 )
    {
        SysLog::Error( "Client::_UpdateSession2()", "characterDataMap.size() returned zero." );
        return;
    }

	gender = characterDataMap["gender"];
    corporationID = characterDataMap["corporationID"];
    stationID = characterDataMap["stationID"];
    solarSystemID = characterDataMap["solarSystemID"];
    constellationID = characterDataMap["constellationID"];
    regionID = characterDataMap["regionID"];
    corporationHQ = characterDataMap["corporationHQ"];
    corpRole = characterDataMap["corpRole"];
    rolesAtAll = characterDataMap["rolesAtAll"];
    rolesAtBase = characterDataMap["rolesAtBase"];
    rolesAtHQ = characterDataMap["rolesAtHQ"];
    rolesAtOther = characterDataMap["rolesAtOther"];
    locationID = characterDataMap["locationID"];
    shipID = characterDataMap["shipID"];
    raceID = characterDataMap["raceID"];
    bloodlineID = characterDataMap["bloodlineID"];

    m_shipId = shipID;
    if( m_char )
    {
        m_char->SetActiveShip(m_shipId);
    }

    // Always sent
    mSession.SetInt("genderID",        gender);
    mSession.SetInt("constellationid", constellationID);
    mSession.SetInt("raceID",          raceID);
    mSession.SetInt("corpid",          corporationID);
    mSession.SetInt("regionid",        regionID);
    mSession.SetInt("bloodlineID",     bloodlineID);
    mSession.SetInt("locationid",      locationID);
    mSession.SetInt("hqID",            corporationHQ);
    mSession.SetInt("solarsystemid2",  solarSystemID);
    mSession.SetInt("shipid",          shipID);
    mSession.SetInt("charid",          characterID);

    if(stationID == 0)
    {
        mSession.SetInt("solarsystemid", solarSystemID);
    }
    else
    {
        mSession.SetInt("stationid2",   stationID);
        mSession.SetInt("worldspaceid", stationID);
        mSession.SetInt("stationid",    stationID);
    }
}

void Client::_SendCallReturn( const PyAddress& source, uint64 callID, PyResult &return_value )
{
    //build the packet:
    PyPacket* p = new PyPacket;
    p->type_string = "carbon.common.script.net.machoNetPacket.CallRsp";
    p->type = CALL_RSP;

    p->source = source;

    p->dest.type = PyAddress::Client;
    p->dest.typeID = GetAccountID();
    p->dest.callID = callID;

    p->userid = GetAccountID();

    p->payload = new PyTuple(1);
    p->payload->SetItem( 0, new PySubStream( return_value.ssResult ) );
    return_value.ssResult = nullptr;   //consumed

    p->named_payload = return_value.ssNamedResult;
    return_value.ssNamedResult = nullptr;

    fastQueuePacket(&p, true);
}

void Client::_SendException( const PyAddress& source, uint64 callID, MACHONETMSG_TYPE in_response_to, MACHONETERR_TYPE exception_type, PyRep** payload )
{
    //build the packet:
    PyPacket* p = new PyPacket;
    p->type_string = "carbon.common.script.net.machoNetPacket.ErrorResponse";
    p->type = ERRORRESPONSE;

    p->source = source;

    p->dest.type = PyAddress::Client;
    p->dest.typeID = GetAccountID();
    p->dest.callID = callID;

    p->userid = GetAccountID();

    macho_MachoException e;
    e.in_response_to = in_response_to;
    e.exception_type = exception_type;
    e.payload = *payload;
    *payload = NULL;    //consumed

    p->payload = e.Encode();
    fastQueuePacket(&p);
}

void Client::_SendSessionChange()
{
    if( !mSession.isDirty() )
        return;

    SessionChangeNotification scn;
    scn.changes = new PyDict;
    // TO-DO: This should be a unique value for each login.
    scn.sessionID = GetSessionID();

    mSession.EncodeChanges( scn.changes );
    if( scn.changes->empty() )
        return;

    SysLog::Log("Client","Session updated, sending session change");
    scn.changes->Dump(CLIENT__SESSION, "  Changes: ");

    //this is probably not necessary...
    scn.nodesOfInterest.push_back(PyServiceMgr::GetNodeID());

    //build the packet:
    PyPacket* p = new PyPacket;
    p->type_string = "carbon.common.script.net.machoNetPacket.SessionChangeNotification";
    p->type = SESSIONCHANGENOTIFICATION;

    p->source.type = PyAddress::Node;
    p->source.typeID = PyServiceMgr::GetNodeID();
    p->source.callID = 0;

    p->dest.type = PyAddress::Client;
    p->dest.typeID = GetAccountID();
    p->dest.callID = 0;

    p->userid = GetAccountID();

    p->payload = scn.Encode();

    p->named_payload = NULL;
    //p->named_payload = new PyDict();
    //p->named_payload->SetItemString( "channel", new PyString( "sessionchange" ) );

    fastQueuePacket(&p);
}

void Client::_SendPingRequest()
{
    PyPacket *ping_req = new PyPacket();

    ping_req->type = PING_REQ;
    ping_req->type_string = "carbon.common.script.net.machoNetPacket.PingReq";

    ping_req->source.type = PyAddress::Node;
    ping_req->source.typeID = PyServiceMgr::GetNodeID();
    ping_req->source.service = "ping";
    ping_req->source.callID = 0;

    ping_req->dest.type = PyAddress::Client;
    ping_req->dest.typeID = GetAccountID();
    ping_req->dest.callID = 0;

    ping_req->userid = GetAccountID();

    ping_req->payload = new_tuple( new PyList() ); //times
    ping_req->named_payload = new PyDict();

    fastQueuePacket(&ping_req);
}

void Client::_SendPingResponse( const PyAddress& source, uint64 callID )
{
    PyPacket* ret = new PyPacket;
    ret->type = PING_RSP;
    ret->type_string = "carbon.common.script.net.machoNetPacket.PingRsp";

    ret->source = source;

    ret->dest.type = PyAddress::Client;
    ret->dest.typeID = GetAccountID();
    ret->dest.callID = callID;

    ret->userid = GetAccountID();

    /*  Here the hacking begins, the ping packet handles the timestamps of various packet handling steps.
        To really simulate/emulate that we need the various packet handlers which in fact we don't have ( :P ).
        So the next piece of code "fake's" it, with a slight delay on the received packet time.
    */
    PyList* pingList = new PyList;
    PyTuple* pingTuple;

    pingTuple = new PyTuple(3);
    pingTuple->SetItem(0, new PyLong(Win32TimeNow() - 20));
    pingTuple->SetItem(1, new PyLong(Win32TimeNow()));
    pingTuple->SetItem(2, new PyString("client::start"));
    pingList->AddItem( pingTuple );

    pingTuple = new PyTuple(3);
    pingTuple->SetItem(0, new PyLong(Win32TimeNow() - 20));        // this should be the time the packet was received (we cheat here a bit)
    pingTuple->SetItem(1, new PyLong(Win32TimeNow()));             // this is the time the packet is (handled/writen) by the (proxy/server) so we're cheating a bit again.
    pingTuple->SetItem(2, new PyString("proxy::handle_message"));
    pingList->AddItem( pingTuple );

    pingTuple = new PyTuple(3);
    pingTuple->SetItem(0, new PyLong(Win32TimeNow() - 20));
    pingTuple->SetItem(1, new PyLong(Win32TimeNow()));
    pingTuple->SetItem(2, new PyString("proxy::writing"));
    pingList->AddItem( pingTuple );

    pingTuple = new PyTuple(3);
    pingTuple->SetItem(0, new PyLong(Win32TimeNow() - 20));
    pingTuple->SetItem(1, new PyLong(Win32TimeNow()));
    pingTuple->SetItem(2, new PyString("server::handle_message"));
    pingList->AddItem( pingTuple );

    pingTuple = new PyTuple(3);
    pingTuple->SetItem(0, new PyLong(Win32TimeNow() - 20));
    pingTuple->SetItem(1, new PyLong(Win32TimeNow()));
    pingTuple->SetItem(2, new PyString("server::turnaround"));
    pingList->AddItem( pingTuple );

    pingTuple = new PyTuple(3);
    pingTuple->SetItem(0, new PyLong(Win32TimeNow() - 20));
    pingTuple->SetItem(1, new PyLong(Win32TimeNow()));
    pingTuple->SetItem(2, new PyString("proxy::handle_message"));
    pingList->AddItem( pingTuple );

    pingTuple = new PyTuple(3);
    pingTuple->SetItem(0, new PyLong(Win32TimeNow() - 20));
    pingTuple->SetItem(1, new PyLong(Win32TimeNow()));
    pingTuple->SetItem(2, new PyString("proxy::writing"));
    pingList->AddItem( pingTuple );

    // Set payload
    ret->payload = new PyTuple( 1 );
    ret->payload->SetItem( 0, pingList );

    // Don't clone so it eats the ret object upon sending.
    fastQueuePacket(&ret);
}

//these are specialized Queue functions when our caller can
//easily provide us with our own copy of the data.
void Client::QueueDestinyUpdate(PyTuple **du)
{
    DoDestinyAction act;
    act.update_id = DestinyManager::GetStamp();
    act.update = *du;
    *du = NULL;

    m_destinyUpdateQueue->AddItem( act.Encode() );
}

void Client::QueueDestinyEvent(PyTuple** multiEvent)
{
    m_destinyEventQueue->AddItem( *multiEvent );
    *multiEvent = NULL;
}

void Client::_SendQueuedUpdates() {
    if( !m_destinyUpdateQueue->empty() )
    {
        DoDestinyUpdateMain dum;

        //first insert the destiny updates.
        dum.updates = m_destinyUpdateQueue;
        PyIncRef( m_destinyUpdateQueue );

        //encode any multi-events which go along with it.
        dum.events = m_destinyEventQueue;
        PyIncRef( m_destinyEventQueue );

        //right now, we never wait. I am sure they do this for a reason, but
        //I haven't found it yet
        dum.waitForBubble = false;

        //now send it
        PyTuple* t = dum.Encode();
        t->Dump(DESTINY__UPDATES, "");
        SendNotification( "DoDestinyUpdate", "clientID", &t );
    }
    else if( !m_destinyEventQueue->empty() )
    {
        Notify_OnMultiEvent nom;

        //insert updates, clear our queue
        nom.events = m_destinyEventQueue;
        PyIncRef( m_destinyEventQueue );

        //send it
        PyTuple* t = nom.Encode();   //this is consumed below
        t->Dump(DESTINY__UPDATES, "");
        SendNotification( "OnMultiEvent", "charid", &t );
    } //else nothing to be sent ...

    // clear the queues now, after the packets have been sent
    m_destinyEventQueue->clear();
    m_destinyUpdateQueue->clear();
}

void Client::SendNotification(const char *notifyType, const char *idType, PyTuple **payload, bool seq) {

    //build a little notification out of it.
    EVENotificationStream notify;
    notify.remoteObject = 1;
    notify.args = *payload;
    *payload = NULL;    //consumed

    PyAddress dest;
    dest.type = PyAddress::Broadcast;
    dest.service = notifyType;
    dest.bcast_idtype = idType;

    //now send it to the client
    SendNotification(dest, notify, seq);
}


void Client::SendNotification(const PyAddress &dest, EVENotificationStream &noti, bool seq) {

    //build the packet:
    PyPacket *p = new PyPacket();
    p->type_string = "carbon.common.script.net.machoNetPacket.Notification";
    p->type = NOTIFICATION;

    p->source.type = PyAddress::Node;
    p->source.typeID = PyServiceMgr::GetNodeID();

    p->dest = dest;

    p->userid = GetAccountID();

    p->payload = noti.Encode();

    if(seq) {
        p->named_payload = new PyDict();
        p->named_payload->SetItemString("sn", new PyInt(m_nextNotifySequence++));
    }

    SysLog::Log("Client","Sending notify of type %s with ID type %s", dest.service.c_str(), dest.bcast_idtype.c_str());

    fastQueuePacket(&p);
}

PyDict *Client::MakeSlimItem() const {
    PyDict *slim = DynamicSystemEntity::MakeSlimItem();

    slim->SetItemString("charID", new PyInt(GetCharacterID()));
    slim->SetItemString("corpID", new PyInt(GetCorporationID()));
    slim->SetItemString("allianceID", new PyNone);
    slim->SetItemString("warFactionID", new PyNone);

    //encode the mModulesMgr list, if we have any visible mModulesMgr
    std::vector<InventoryItemRef> items;
    GetShip()->FindByFlagRange( flagHiSlot0, flagHiSlot7, items );
    if( !items.empty() )
    {
        PyList *l = new PyList();

        std::vector<InventoryItemRef>::iterator cur, end;
        cur = items.begin();
        end = items.end();
        for(; cur != end; cur++) {

            PyTuple* t = new_tuple( (*cur)->itemID(), (*cur)->typeID());
            l->AddItem(t);
        }

        slim->SetItemString("mModulesMgr", l);
    }

    slim->SetItemString("color", new PyFloat(0.0));
    slim->SetItemString("bounty", new PyFloat(GetBounty()));
    slim->SetItemString("securityStatus", new PyFloat(GetSecurityRating()));

    return(slim);
}

void Client::WarpTo(const Vector3D &to, double distance) {
    if(m_moveState != msIdle || m_moveTimer.Enabled()) {
        SysLog::Log("Client","%s: WarpTo called when a move is already pending. Ignoring.", GetName());
        return;
    }

    m_destiny->WarpTo(to, distance);
    //TODO: OnModuleAttributeChange with attribute 18 for capacitory charge
}

void Client::StargateJump(uint32 fromGate, uint32 toGate) {
    if(m_moveState != msIdle || m_moveTimer.Enabled()) {
        SysLog::Log("Client","%s: StargateJump called when a move is already pending. Ignoring.", GetName());
        return;
    }

    //TODO: verify that they are actually close to 'fromGate'
    //TODO: verify that 'fromGate' actually jumps to 'toGate'

    uint32 solarSystemID, constellationID, regionID;
    Vector3D position;
    if (!ServiceDB::GetStaticItemInfo(
        toGate,
        &solarSystemID, &constellationID, &regionID, &position
    )) {
        SysLog::Error("Client","%s: Failed to query information for stargate %u", GetName(), toGate);
        return;
    }

    GetShip()->DeactivateAllModules();

    m_moveSystemID = solarSystemID;
    m_movePoint = position;
    m_movePoint.MakeRandomPointOnSphere( 15000 );   // Make Jump-In point a random spot on a 10km radius sphere about the stargate

    m_destiny->SendJumpOut(fromGate);
    //TODO: send 'effects.GateActivity' on 'toGate' at the same time

    //delay the move so they can see the JumpOut animation
    _postMove(msJump, 5000);
}

void Client::SetDockinVector3D(Vector3D &dockPoint)
{
    m_movePoint.x = dockPoint.x;
    m_movePoint.y = dockPoint.y;
    m_movePoint.z = dockPoint.z;
}

void Client::GetDockinVector3D(Vector3D &dockPoint)
{
    dockPoint.x = m_movePoint.x;
    dockPoint.y = m_movePoint.y;
    dockPoint.z = m_movePoint.z;
}

// THESE FUNCTIONS ARE HACKS AS WE DONT KNOW WHY THE CLIENT CALLS STOP AT UNDOCK
// *SetJustUndocking (only in Client.h)
// *GetJustUndocking
// *SetUndockAlignToPoint
// *GetUndockAlignToPoint
void Client::SetUndockAlignToPoint(Vector3D &dest)
{
    m_undockAlignToPoint.x = dest.x;
    m_undockAlignToPoint.y = dest.y;
    m_undockAlignToPoint.z = dest.z;
}

void Client::GetUndockAlignToPoint(Vector3D &dest)
{
    dest.x = m_undockAlignToPoint.x;
    dest.y = m_undockAlignToPoint.y;
    dest.z = m_undockAlignToPoint.z;
}
// --- END HACK FUNCTIONS FOR UNDOCK ---

void Client::_postMove(_MoveState type, uint32 wait_ms) {
    m_moveState = type;
    m_moveTimer.Start(wait_ms);
}

void Client::_ExecuteJump() {
    if(m_destiny == NULL)
        return;

    MoveToLocation(m_moveSystemID, m_movePoint);
}

bool Client::AddBalance(double amount) {
    if(!GetChar()->AlterBalance(amount))
        return false;

    //send notification of change
    OnAccountChange ac;
    ac.accountKey = "cash";
    ac.ownerid = GetCharacterID();
    ac.balance = GetBalance();
    PyTuple *answer = ac.Encode();
    SendNotification("OnAccountChange", "cash", &answer, false);

    return true;
}



bool Client::SelectCharacter( uint32 char_id )
{
    ItemFactory::SetUsingClient(this);

    _UpdateSession2( char_id );

//    if( !EnterSystem( true ) )
//        return false;

    m_char = ItemFactory::GetCharacter(char_id);
    if( !GetChar() )
    {
        // Release the item factory now that the ItemFactory is finished being used:
        ItemFactory::UnsetUsingClient();
        return false;
    }

    ShipRef ship = ItemFactory::GetShip(GetShipID());
   if( !ship )
   {
        // Release the item factory now that the ItemFactory is finished being used:
        ItemFactory::UnsetUsingClient();
        return false;
    }

    ship->Load(GetShipID());

   BoardShip( ship );

    if( !EnterSystem( true ) )
    {
        // Release the item factory now that the ItemFactory is finished being used:
        ItemFactory::UnsetUsingClient();
        return false;
    }

    UpdateLocation();

    // update skill queue
    GetChar()->updateSkillQueue();

    //johnsus - characterOnline mod
    ServiceDB::SetCharacterOnlineStatus(GetCharacterID(), true);

    _SendSessionChange();

    // Release the item factory now that the ItemFactory is finished being used:
    ItemFactory::UnsetUsingClient();
    return true;
}

void Client::UpdateSkillTraining()
{
    if( GetChar() )
        m_timeEndTrain = GetChar()->GetEndOfTraining();
    else
        m_timeEndTrain = 0;
}

double Client::GetPropulsionStrength() const {

    /**
     * if we don't have a ship return bogus propulsion strength
     * @note we should report a error for this
     */
    if( !GetShip() )
        return 3.0f;

    /**
     * Old comments:
     * just making shit up, I think skills modify this, as newbies
     * tend to end up with 3.038 instead of the base 3.0 on their ship..
     */
    EvilNumber res;
//    res =  GetShip()->getAttribute( AttrPropulsionFusionStrength );
//    res += GetShip()->getAttribute( AttrPropulsionIonStrength );
//    res += GetShip()->getAttribute( AttrPropulsionMagpulseStrength );
//    res += GetShip()->getAttribute( AttrPropulsionPlasmaStrength );
    res += GetShip()->getAttribute( AttrPropulsionFusionStrengthBonus );
    res += GetShip()->getAttribute( AttrPropulsionIonStrengthBonus );
    res += GetShip()->getAttribute( AttrPropulsionMagpulseStrengthBonus );
    res += GetShip()->getAttribute( AttrPropulsionPlasmaStrengthBonus );

    res += 0.038f;

    /**
     * we should watch out here, because we know for a fact that this function returns a floating point.
     * the only reason we know for sure is because we do the "res += 0.038f;" at the end of the bogus calculation.
     * @note this function isn't even used... lolz
     */
    return res.get_float();
}

void Client::TargetAdded( SystemEntity* who )
{
    PyTuple* up = NULL;

    DoDestiny_OnDamageStateChange odsc;
    odsc.entityID = who->GetID();
    odsc.state = who->MakeDamageState();

    up = odsc.Encode();
    QueueDestinyUpdate( &up );
    PySafeDecRef( up );

    Notify_OnTarget te;
    te.mode = "add";
    te.targetID = who->GetID();

    up = te.Encode();
    QueueDestinyEvent( &up );
    PySafeDecRef( up );
}

void Client::TargetLost(SystemEntity *who)
{
    //OnMultiEvent: OnTarget lost
    Notify_OnTarget te;
    te.mode = "lost";
    te.targetID = who->GetID();

    Notify_OnMultiEvent multi;
    multi.events = new PyList;
    multi.events->AddItem( te.Encode() );

    PyTuple* tmp = multi.Encode();   //this is consumed below
    SendNotification("OnMultiEvent", "clientID", &tmp);
}

void Client::TargetedAdd(SystemEntity *who) {
    //OnMultiEvent: OnTarget otheradd
    Notify_OnTarget te;
    te.mode = "otheradd";
    te.targetID = who->GetID();

    Notify_OnMultiEvent multi;
    multi.events = new PyList;
    multi.events->AddItem( te.Encode() );

    PyTuple* tmp = multi.Encode();   //this is consumed below
    SendNotification("OnMultiEvent", "clientID", &tmp);
}

void Client::TargetedLost(SystemEntity *who)
{
    //OnMultiEvent: OnTarget otherlost
    Notify_OnTarget te;
    te.mode = "otherlost";
    te.targetID = who->GetID();

    Notify_OnMultiEvent multi;
    multi.events = new PyList;
    multi.events->AddItem( te.Encode() );

    PyTuple* tmp = multi.Encode();   //this is consumed below
    SendNotification("OnMultiEvent", "clientID", &tmp);
}

void Client::TargetsCleared()
{
    //OnMultiEvent: OnTarget clear
    Notify_OnTarget te;
    te.mode = "clear";
    te.targetID = 0;

    Notify_OnMultiEvent multi;
    multi.events = new PyList;
    multi.events->AddItem( te.Encode() );

    PyTuple* tmp = multi.Encode();   //this is consumed below
    SendNotification("OnMultiEvent", "clientID", &tmp);
}

void Client::SavePosition() {
    if( !GetShip() || m_destiny == NULL ) {
        SysLog::Debug("Client","%s: Unable to save position. We are probably not in space.", GetName());
        return;
    }
    GetShip()->Relocate( m_destiny->GetPosition() );
}

void Client::SaveAllToDatabase()
{
    SavePosition();
    GetChar()->SaveSkillQueue();
    GetShip()->SaveShip();
    GetChar()->SaveCharacter();
}

bool Client::LaunchDrone(InventoryItemRef drone) {
#if 0
drop 166328265

OnItemChange ,*args= (Row(itemID: 166328265,typeID: 15508,
    ownerID: 105651216,locationID: 166363674,flag: 87,
    contraband: 0,singleton: 1,quantity: 1,groupID: 100,
    categoryID: 18,customInfo: (166363674, None)),
    {10: None}) ,**kw= {}

OnItemChange ,*args= (Row(itemID: 166328265,typeID: 15508,
    ownerID: 105651216,locationID: 30001369,flag: 0,
    contraband: 0,singleton: 1,quantity: 1,groupID: 100,
    categoryID: 18,customInfo: (166363674, None)),
    {3: 166363674, 4: 87}) ,**kw= {}

returns list of deployed drones.

internaly scatters:
    OnItemLaunch ,*args= ([166328265], [166328265])

DoDestinyUpdate ,*args= ([(31759,
    ('OnDroneStateChange',
        [166328265, 105651216, 166363674, 0, 15508, 105651216])
    ), (31759,
    ('OnDamageStateChange',
        (2100212375, [(1.0, 400000.0, 127997215757036940L), 1.0, 1.0]))
    ), (31759, ('AddBalls',
        ...
    True

DoDestinyUpdate ,*args= ([(31759,
    ('Orbit', (166328265, 166363674, 750.0))
    ), (31759,
    ('SetSpeedFraction', (166328265, 0.265625)))],
    False) ,**kw= {} )
#endif

    if(!IsSolarSystem(GetLocationID())) {
        SysLog::Log("Client","%s: Trying to launch drone when not in space!", GetName());
        return false;
    }

    SysLog::Log("Client","%s: Launching drone %u", GetName(), drone->itemID());

    //first, the item gets moved into space
    //TODO: set customInfo to a tuple: (shipID, None)
    drone->Move(GetSystemID(), flagAutoFit);
    //temp for testing:
    drone->Move(GetShipID(), flagDroneBay, false);

    //now we create an NPC to represent it.
    Vector3D position(GetPosition());
    position.x += 750.0f;   //total crap.

    //this adds itself into the system.
    NPC *drone_npc = new NPC(
        m_system,
        drone,
        GetCorporationID(),
        GetAllianceID(),
        position);
    m_system->AddNPC(drone_npc);

    //now we tell the client that "its ALIIIIIVE"
    //DoDestinyUpdate:

    //drone_npc->Destiny()->SendAddBall();

    /*PyList *actions = new PyList();
    DoDestinyAction act;
    act.update_id = NextDestinyUpdateID();  //update ID?
    */
    //  OnDroneStateChange
/*  {
        DoDestiny_OnDroneStateChange du;
        du.droneID = drone->itemID();
        du.ownerID = GetCharacterID();
        du.controllerID = GetShipID();
        du.activityState = 0;
        du.droneTypeID = drone->typeID();
        du.controllerOwnerID = GetShip()->ownerID();
        act.update = du.Encode();
        actions->add( act.Encode() );
    }*/

    //  AddBall
    /*{
        DoDestiny_AddBall addball;
        drone_npc->MakeAddBall(addball, act.update_id);
        act.update = addball.Encode();
        actions->add( act.Encode() );
    }*/

    //  Orbit
/*  {
        DoDestiny_Orbit du;
        du.entityID = drone->itemID();
        du.orbitEntityID = GetCharacterID();
        du.distance = 750;
        act.update = du.Encode();
        actions->add( act.Encode() );
    }

    //  SetSpeedFraction
    {
        DoDestiny_SetSpeedFraction du;
        du.entityID = drone->itemID();
        du.fraction = 0.265625f;
        act.update = du.Encode();
        actions->add( act.Encode() );
    }*/

//testing:
/*  {
        DoDestiny_SetBallInteractive du;
        du.entityID = drone->itemID();
        du.interactive = 1;
        act.update = du.Encode();
        actions->add( act.Encode() );
    }
    {
        DoDestiny_SetBallInteractive du;
        du.entityID = GetCharacterID();
        du.interactive = 1;
        act.update = du.Encode();
        actions->add( act.Encode() );
    }*/

    //NOTE: we really want to broadcast this...
    //TODO: delay this until after the return.
    //_SendDestinyUpdate(&actions, false);

    return false;
}

//assumes that the backend DB stuff was already done.
void Client::JoinCorporationUpdate(uint32 corp_id) {
    //GetChar()->JoinCorporation(corp_id);

    _UpdateSession( GetChar() );

    //logs indicate that we need to push this update out asap.
    _SendSessionChange();
}

/************************************************************************/
/* character notification messages wrapper                              */
/************************************************************************/
void Client::OnCharNoLongerInStation()
{
    PyList *list = new PyList();
    list->AddItem(new PyInt(GetCharacterID()));
    uint32 corpID = GetCorporationID();
    list->AddItem(corpID == 0 ? (PyRep*)new PyNone() : (PyRep*)new PyInt(corpID));
    uint32 allianceID = GetAllianceID();
    list->AddItem(allianceID == 0 ? (PyRep*)new PyNone() : (PyRep*)new PyInt(allianceID));
    uint32 warID = GetWarFactionID();
    list->AddItem(warID == 0 ? (PyRep*)new PyNone() : (PyRep*)new PyInt(warID));

    PyTuple *tuple = new_tuple001(list);
            // To-DO: Limit broadcast to only characters in station.
    EntityList::Broadcast("OnCharNoLongerInStation", "stationid", &tuple);
}

/* besides broadcasting the message this function should handle everything for this event */
void Client::OnCharNowInStation()
{
    PyList *list = new PyList();
    list->AddItem(new PyInt(GetCharacterID()));
    uint32 corpID = GetCorporationID();
    list->AddItem(corpID == 0 ? (PyRep*)new PyNone() : (PyRep*)new PyInt(corpID));
    uint32 allianceID = GetAllianceID();
    list->AddItem(allianceID == 0 ? (PyRep*)new PyNone() : (PyRep*)new PyInt(allianceID));
    uint32 warID = GetWarFactionID();
    list->AddItem(warID == 0 ? (PyRep*)new PyNone() : (PyRep*)new PyInt(warID));

    PyTuple *tuple = new_tuple(list);
    // To-DO: Limit broadcast to only characters in station.
    EntityList::Broadcast("OnCharNowInStation", "stationid", &tuple);
}

/************************************************************************/
/* EVEAdministration Interface                                          */
/************************************************************************/
void Client::DisconnectClient()
{
    //initiate closing the client TCP Connection
    closeClientConnection();
}
void Client::BanClient()
{
    //send message to client
    SendNotifyMsg("You have been banned from this server and will be disconnected shortly.  You will no longer be able to log in");

    //ban the client
            ServiceDB::SetAccountBanStatus(GetAccountID(), true);
}

/************************************************************************/
/* EVEClientSession interface                                           */
/************************************************************************/
void Client::_GetVersion( VersionExchangeServer& version )
{
    version.birthday = EVEBirthday;
    version.macho_version = MachoNetVersion;
    version.user_count = _GetUserCount();
    version.version_number = EVEVersionNumber;
    version.build_version = EVEBuildVersion;
    version.project_version = EVEProjectVersion;
}

uint32 Client::_GetUserCount()
{
    return EntityList::GetClientCount();
}

bool Client::_VerifyVersion( VersionExchangeClient& version )
{
    SysLog::Log("Client","%s: Received Low Level Version Exchange:", GetAddress().c_str());
    version.Dump(NET__PRES_REP, "    ");

    if( version.birthday != EVEBirthday )
        SysLog::Error("Client","%s: Client's birthday does not match ours!", GetAddress().c_str());

    if( version.macho_version != MachoNetVersion )
        SysLog::Error("Client","%s: Client's macho_version not match ours!", GetAddress().c_str());

    if( version.version_number != EVEVersionNumber )
        SysLog::Error("Client","%s: Client's version_number not match ours!", GetAddress().c_str());

    if( version.build_version != EVEBuildVersion )
        SysLog::Error("Client","%s: Client's build_version not match ours!", GetAddress().c_str());

    if( version.project_version != EVEProjectVersion )
        SysLog::Error("Client","%s: Client's project_version not match ours!", GetAddress().c_str());


    return true;
}

bool Client::_VerifyCrypto( CryptoRequestPacket& cr )
{
    if( cr.keyVersion != "placebo" )
    {
        //I'm sure cr.keyVersion can specify either CryptoAPI or PyCrypto, but its all binary so im not sure how.
        CryptoAPIRequestParams car;
        if( !car.Decode( cr.keyParams ) )
        {
            SysLog::Error("Client","%s: Received invalid CryptoAPI request!", GetAddress().c_str());
        }
        else
        {
            SysLog::Error("Client","%s: Unhandled CryptoAPI request: hashmethod=%s sessionkeylength=%d provider=%s sessionkeymethod=%s", GetAddress().c_str(), car.hashmethod.c_str(), car.sessionkeylength, car.provider.c_str(), car.sessionkeymethod.c_str());
            SysLog::Error("Client","%s: You must change your client to use Placebo crypto in common.ini to talk to this server!\n", GetAddress().c_str());
        }

        return false;
    }
    else
    {
        SysLog::Debug("Client","%s: Received Placebo crypto request, accepting.", GetAddress().c_str());

        //send out accept response
        PyRep* rsp = new PyString( "OK CC" );
                m_tcpConnecton->QueueRep(rsp);
        PyDecRef( rsp );

        return true;
    }
}

bool Client::_VerifyLogin( CryptoChallengePacket& ccp )
{
    std::string account_hash;
    std::string transport_closed_msg = "LoginAuthFailed";

    AccountInfo account_info;
    CryptoServerHandshake server_shake;

    /* send passwordVersion required: 1=plain, 2=hashed */
    PyRep* rsp = new PyInt( 2 );

    //Log::Debug("Client","%s: Received Client Challenge.", GetAddress().c_str());
    //Log::Debug("Client","Login with %s:", ccp.user_name.c_str());

    if (!ServiceDB::GetAccountInformation(
				ccp.user_name.c_str(),
				ccp.user_password_hash.c_str(),
				account_info))
	{
        goto error_login_auth_failed;
    }

    /* check wether the account has been banned and if so send the semi correct message */
    if (account_info.banned) {
        transport_closed_msg = "ACCOUNTBANNED";
        goto error_login_auth_failed;
    }

    /* if we have stored a password we need to create a hash from the username and pass and remove the pass */
    if( account_info.password.empty() )
        account_hash = account_info.hash;
    else
    {
        /* here we generate the password hash ourselves */
        std::string password_hash;
        if( !PasswordModule::GeneratePassHash(
                ccp.user_name,
                account_info.password,
                password_hash ) )
        {
            SysLog::Error("Client", "unable to generate password hash, sending LoginAuthFailed");
            goto error_login_auth_failed;
        }

        if (!ServiceDB::UpdateAccountHash(
                ccp.user_name.c_str(),
                password_hash ) )
        {
            SysLog::Error("Client", "unable to update account hash, sending LoginAuthFailed");
            goto error_login_auth_failed;
        }

        account_hash = password_hash;
    }

    /* here we check if the user successfully entered his password or if he failed */
    if (account_hash != ccp.user_password_hash) {
        goto error_login_auth_failed;
    }

    /* Check if we already have a client online and if we do disconnect it
     * @note we should send GPSTransportClosed with reason "The user's connection has been usurped on the proxy"
     */
    if (account_info.online) {
        Client* client = EntityList::FindAccount(account_info.id);
        if (client)
            client->DisconnectClient();
        }

    m_tcpConnecton->QueueRep(rsp);
    PyDecRef( rsp );

    SysLog::Log("Client","successful");

    /* update account information, increase login count, last login timestamp and mark account as online */
            ServiceDB::UpdateAccountInformation(account_info.name.c_str(), true);

    /* marshaled Python string "None" */
    static const uint8 handshakeFunc[] = { 0x74, 0x04, 0x00, 0x00, 0x00, 0x4E, 0x6F, 0x6E, 0x65 };

    /* send our handshake */

    server_shake.serverChallenge = "";
    server_shake.func_marshaled_code = new PyBuffer( handshakeFunc, handshakeFunc + sizeof( handshakeFunc ) );
    server_shake.verification = new PyBool( false );
    server_shake.cluster_usercount = _GetUserCount();
    server_shake.proxy_nodeid = 0xFFAA;
    server_shake.user_logonqueueposition = _GetQueuePosition();
    // binascii.crc_hqx of marshaled single-element tuple containing 64 zero-bytes string
    server_shake.challenge_responsehash = "55087";

    // begin config_vals
    server_shake.imageserverurl = ImageServer::getURL(m_networkConfig); // Image server used to download images
    server_shake.publicCrestUrl = "";
    server_shake.bugReporting_BugReportServer = "";
    server_shake.sessionChangeDelay = "10";       // yea yea, the client has this as a default anyway, Live sends it therefor we do too
    server_shake.experimental_scanners = "1";     // Hey they look nice.
    server_shake.experimental_map_default = "1";  // OK, this is ugly as hell, but live has forced it so do we.
    server_shake.experimental_newcam3 = "1";      // See above remark
    server_shake.isProjectDiscoveryEnabled = "0"; // Why...
    server_shake.bugReporting_ShowButton = "0";   // We do not have that service.
    server_shake.serverInfo = EVEServerConfig::serverInfo.compiledValue;


    server_shake.macho_version = MachoNetVersion;
    server_shake.boot_version = EVEVersionNumber;
    server_shake.boot_build = EVEBuildVersion;
    server_shake.boot_codename = EVEProjectCodename;
    server_shake.boot_region = EVEProjectRegion;

    rsp = server_shake.Encode();
            m_tcpConnecton->QueueRep(rsp);
    PyDecRef( rsp );

    // Setup session, but don't send the change yet.
            mSession.SetString("address", EVEClientSession::getAddress().c_str());
    mSession.SetString( "languageID", ccp.user_languageid.c_str() );

    //user type 1 is normal user, type 23 is a trial account user.
    mSession.SetInt( "userType", 20); // That was old, 1 is not defined by the client, 20 is userTypePBC //1 );
    mSession.SetInt( "userid", account_info.id );
    mSession.SetLong( "role", account_info.role );
    mSession.SetLong( "sessionID", MakeRandomInt(0, UINT64_MAX-1)); // defined in client in basesession.py:GetNewSid(): return random.getrandbits(63)

    return true;

error_login_auth_failed:

    GPSTransportClosed* except = new GPSTransportClosed( transport_closed_msg );
            m_tcpConnecton->QueueRep(except);
    PyDecRef( except );

    return false;
}

bool Client::_VerifyFuncResult( CryptoHandshakeResult& result )
{
    _log(NET__PRES_DEBUG, "%s: Handshake result received.", GetAddress().c_str());

    //send this before session change
    CryptoHandshakeAck ack;
    ack.access_token = new PyNone;
    ack.client_hash = new PyNone;
    ack.sessionID = GetSessionID();
    ack.user_clientid = GetNextClientSessionID();
    ack.live_updates = new PyList(0);       // No, we will never update the client with this method.
    ack.languageID = GetLanguageID();
    ack.userid = GetAccountID();
    ack.maxSessionTime = new PyNone;
    ack.userType = 20; // userTypePBC = 20
    ack.role = 6917529027641081856;         // (ROLE_LOGIN & ROLE_PLAYER) Live player role.
    ack.address = GetAddress();
    ack.inDetention = new PyNone;


    PyRep* r = ack.Encode();
            m_tcpConnecton->QueueRep(r);
    PyDecRef( r );

    // Send out the session change
    _SendSessionChange();

    return true;
}

/************************************************************************/
/* EVEPacketDispatcher interface                                        */
/************************************************************************/
bool Client::Handle_CallReq( PyPacket* packet, PyCallStream& req )
{
    PyCallable* dest;
    if( packet->dest.service.empty() )
    {
        //bound object
        uint32 nodeID, bindID;
        if( sscanf( req.remoteObjectStr.c_str(), "N=%u:%u", &nodeID, &bindID ) != 2 )
        {
            SysLog::Error("Client","Failed to parse bind string '%s'.", req.remoteObjectStr.c_str());
            return false;
        }

        if (nodeID != PyServiceMgr::GetNodeID())
        {
            SysLog::Error("Client", "Unknown nodeID %u received (expected %u).", nodeID, PyServiceMgr::GetNodeID());
            return false;
        }

        dest = PyServiceMgr::FindBoundObject(bindID);
        if( dest == NULL )
        {
            SysLog::Error("Client", "Failed to find bound object %u.", bindID);
            return false;
        }
    }
    else
    {
        //service
        dest = PyServiceMgr::LookupService(packet->dest.service);
        if( dest == NULL )
        {
            SysLog::Error("Client","Unable to find service to handle call to: %s", packet->dest.service.c_str());
            packet->dest.Dump(CLIENT__ERROR, "    ");

            //TODO: throw proper exception to client (exceptions.ServiceNotFound).
            throw PyException( new PyNone );
        }
    }

    //Debug code
    if( req.method == "BeanCount" )
        SysLog::Error("Client","BeanCount");
    else
        //this should be Log::Debug, but because of the number of messages, I left it as .Log for readability, and ease of finding other debug messages
        SysLog::Log("Server", "%s call made to %s",req.method.c_str(),packet->dest.service.c_str());

    //build arguments
    PyCallArgs args( this, req.arg_tuple, req.arg_dict );

    //parts of call may be consumed here
    PyResult result = dest->Call( req.method, args );

    _SendSessionChange();  //send out the session change before the return.
    _SendCallReturn( packet->dest, packet->source.callID, result );

    return true;
}

bool Client::Handle_Notify( PyPacket* packet )
{
    //turn this thing into a notify stream:
    ServerNotification notify;
    if( !notify.Decode( packet->payload ) )
    {
        SysLog::Error("Client","Failed to convert rep into a notify stream");
        return false;
    }

    if(notify.method == "ClientHasReleasedTheseObjects")
    {
        ServerNotification_ReleaseObj element;

        PyList::const_iterator cur, end;
        cur = notify.elements->begin();
        end = notify.elements->end();
        for(; cur != end; cur++)
        {
            if(!element.Decode( *cur )) {
                SysLog::Error("Client","Notification '%s' from %s: Failed to decode element. Skipping.", notify.method.c_str(), GetName());
                continue;
            }

            uint32 nodeID, bindID;
            if(sscanf(element.boundID.c_str(), "N=%u:%u", &nodeID, &bindID) != 2) {
                SysLog::Error("Client","Notification '%s' from %s: Failed to parse bind string '%s'. Skipping.",
                    notify.method.c_str(), GetName(), element.boundID.c_str());
                continue;
            }

            if (nodeID != PyServiceMgr::GetNodeID())
            {
                SysLog::Error("Client","Notification '%s' from %s: Unknown nodeID %u received (expected %u). Skipping.",
                           notify.method.c_str(), GetName(), nodeID, PyServiceMgr::GetNodeID());
                continue;
            }

            PyServiceMgr::ClearBoundObject(bindID);
        }
    }
    else
    {
        SysLog::Error("Client","Unhandled notification from %s: unknown method '%s'", GetName(), notify.method.c_str());
        return false;
    }

    _SendSessionChange();  //just for good measure...
    return true;
}

void Client::UpdateSession(const char *sessionType, int value)
{
    mSession.SetInt(sessionType, value);
}

int64 GetNextClientSessionID()
{
    return ++EntityList::clientIDOffset * 10000000000 + PyServiceMgr::GetNodeID();
}
