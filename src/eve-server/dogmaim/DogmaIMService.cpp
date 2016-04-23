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
#include "cache/ObjCacheService.h"
#include "dogmaim/DogmaIMService.h"
#include "ship/modules/Modules.h"
#include "system/SystemManager.h"
#include "PyServiceMgr.h"

class DogmaIMBound
: public PyBoundObject
{
public:
    PyCallable_Make_Dispatcher(DogmaIMBound)

    DogmaIMBound()
    : PyBoundObject(new Dispatcher(this))
    {
        m_strBoundObjectName = "DogmaIMBound";

        PyCallable_REG_CALL(DogmaIMBound, ShipGetInfo)
        PyCallable_REG_CALL(DogmaIMBound, CharGetInfo)
        PyCallable_REG_CALL(DogmaIMBound, ItemGetInfo)
		PyCallable_REG_CALL(DogmaIMBound, GetAllInfo)
		PyCallable_REG_CALL(DogmaIMBound, GetLocationInfo)
        PyCallable_REG_CALL(DogmaIMBound, GetTargets)
        PyCallable_REG_CALL(DogmaIMBound, GetTargeters)
		PyCallable_REG_CALL(DogmaIMBound, GetWeaponBankInfoForShip)
        PyCallable_REG_CALL(DogmaIMBound, GetCharacterBaseAttributes)
		PyCallable_REG_CALL(DogmaIMBound, CheckSendLocationInfo)
        PyCallable_REG_CALL(DogmaIMBound, Activate)
        PyCallable_REG_CALL(DogmaIMBound, Deactivate)
		PyCallable_REG_CALL(DogmaIMBound, Overload)
        PyCallable_REG_CALL(DogmaIMBound, CancelOverloading)
		PyCallable_REG_CALL(DogmaIMBound, SetModuleOnline)
		PyCallable_REG_CALL(DogmaIMBound, TakeModuleOffline)
		PyCallable_REG_CALL(DogmaIMBound, LoadAmmoToBank)
        PyCallable_REG_CALL(DogmaIMBound, AddTarget)
        PyCallable_REG_CALL(DogmaIMBound, RemoveTarget)
        PyCallable_REG_CALL(DogmaIMBound, ClearTargets)
    }
    virtual ~DogmaIMBound() {}
    virtual void Release() {
        //I hate this statement
        delete this;
    }

    PyCallable_DECL_CALL(ShipGetInfo)
    PyCallable_DECL_CALL(CharGetInfo)
    PyCallable_DECL_CALL(ItemGetInfo)
	PyCallable_DECL_CALL(GetAllInfo)
	PyCallable_DECL_CALL(GetWeaponBankInfoForShip)
    PyCallable_DECL_CALL(GetLocationInfo)
    PyCallable_DECL_CALL(GetCharacterBaseAttributes)
    PyCallable_DECL_CALL(GetTargets)
    PyCallable_DECL_CALL(GetTargeters)
	PyCallable_DECL_CALL(CheckSendLocationInfo)
    PyCallable_DECL_CALL(Activate)
    PyCallable_DECL_CALL(Deactivate)
	PyCallable_DECL_CALL(Overload)
    PyCallable_DECL_CALL(CancelOverloading)
	PyCallable_DECL_CALL(SetModuleOnline)
	PyCallable_DECL_CALL(TakeModuleOffline)
	PyCallable_DECL_CALL(LoadAmmoToBank)
    PyCallable_DECL_CALL(AddTarget)
    PyCallable_DECL_CALL(RemoveTarget)
    PyCallable_DECL_CALL(ClearTargets)



protected:

};

PyCallable_Make_InnerDispatcher(DogmaIMService)

DogmaIMService::DogmaIMService()
: PyService("dogmaIM", new Dispatcher(this))
{
    PyCallable_REG_CALL(DogmaIMService, GetAttributeTypes)
}

DogmaIMService::~DogmaIMService() {
}

PyBoundObject *DogmaIMService::_CreateBoundObject(Client *c, const PyRep *bind_args) {
    _log(CLIENT__MESSAGE, "DogmaIMService bind request for:");
    bind_args->Dump(CLIENT__MESSAGE, "    ");

    return(new DogmaIMBound());
}

PyResult DogmaIMService::Handle_GetAttributeTypes(PyCallArgs &call) {
    PyString* str = new PyString( "dogmaIM.attributesByName" );
    PyRep* result = PyServiceMgr::cache_service->GetCacheHint(str);
    PyDecRef( str );

    return result;
}

PyResult DogmaIMBound::Handle_GetCharacterBaseAttributes(PyCallArgs &call)
{
    CharacterRef cref = call.client->GetChar();

    PyDict* result = new PyDict();
    result->SetItem(new PyInt(AttrIntelligence), new PyInt(static_cast<int32>(cref->getAttribute(AttrIntelligence).get_int())));
    result->SetItem(new PyInt(AttrPerception), new PyInt(static_cast<int32>(cref->getAttribute(AttrPerception).get_int())));
    result->SetItem(new PyInt(AttrCharisma), new PyInt(static_cast<int32>(cref->getAttribute(AttrCharisma).get_int())));
    result->SetItem(new PyInt(AttrWillpower), new PyInt(static_cast<int32>(cref->getAttribute(AttrWillpower).get_int())));
    result->SetItem(new PyInt(AttrMemory), new PyInt(static_cast<int32>(cref->getAttribute(AttrMemory).get_int())));

    return result;
}

PyResult DogmaIMBound::Handle_GetLocationInfo(PyCallArgs &call)
{
    // no arguments

    // dummy right now, don't have any meaningful packet logs
    return new PyDict();
}

PyResult DogmaIMBound::Handle_ShipGetInfo(PyCallArgs &call) {
    //takes no arguments

    /*PyObject *result = call.client->GetShip()->ShipGetInfo();
    if(result == NULL) {
        codelog(SERVICE__ERROR, "Unable to build ship info for ship %u", call.client->GetShipID());
        return NULL;
    }

    return result;*/
    return new PyNone;
}

PyResult DogmaIMBound::Handle_ItemGetInfo(PyCallArgs &call) {
    Call_SingleIntegerArg args;
    if(!args.Decode(&call.tuple)) {
        codelog(SERVICE__ERROR, "Failed to decode arguments");
        return NULL;
    }

    InventoryItemRef item = ItemFactory::GetItem(args.arg);
    if( !item ) {
        codelog(SERVICE__ERROR, "Unable to load item %u", args.arg);
        return NULL;
    }

    return item->ItemGetInfo();
}

PyResult DogmaIMBound::Handle_CharGetInfo(PyCallArgs &call) {
    //no arguments

    /*PyObject *result = call.client->GetChar()->CharGetInfo();
    if(result == NULL) {
        codelog(SERVICE__ERROR, "Unable to build char info for char %u", call.client->GetCharacterID());
        return NULL;
    }

    return result;
    */
    return new PyNone;
}

PyResult DogmaIMBound::Handle_CheckSendLocationInfo( PyCallArgs& call )
{
    //no arguments

    SysLog::Debug( "DogmaIMBound", "Called CheckSendLocationInfo stub." );

    return new PyNone;
}

PyResult DogmaIMBound::Handle_GetTargets(PyCallArgs &call) {
    //no arguments
    return call.client->targets.getTargets();
}

PyResult DogmaIMBound::Handle_GetTargeters(PyCallArgs &call) {
    //no arguments
    return call.client->targets.getTargeters();
}


PyResult DogmaIMBound::Handle_SetModuleOnline( PyCallArgs& call ) {
	Call_TwoIntegerArgs args; //locationID, moduleID

	if( !args.Decode( &call.tuple ) )
    {
        codelog( SERVICE__ERROR, "Unable to decode arguments from '%s'", call.client->GetName() );
        return NULL;
    }

	call.client->GetShip()->Online(args.arg2);

	return NULL;
}

PyResult DogmaIMBound::Handle_TakeModuleOffline( PyCallArgs& call ) {
	Call_TwoIntegerArgs args; //locationID, moduleID

	if( !args.Decode( &call.tuple ) )
    {
        codelog( SERVICE__ERROR, "Unable to decode arguments from '%s'", call.client->GetName() );
        return NULL;
    }

	call.client->GetShip()->Offline(args.arg2);

	return NULL;
}

PyResult DogmaIMBound::Handle_LoadAmmoToBank( PyCallArgs& call ) {
	Call_Dogma_LoadAmmoToBank args;

	if( !args.Decode( &call.tuple ) )
    {
        codelog( SERVICE__ERROR, "Unable to decode arguments from '%s'", call.client->GetName() );
        return NULL;
    }

	// NOTES:
	// args.chargeList will contain one or more entries, each of which is an itemID of a charge or stack of charges
	// presumably, this list allows the player to select more than one stack of exact same ammo and drag whole selection
	// onto the module to be loaded into it; then what can be loaded is, and a single stack of the remainder quantity is
	// created and returned to the inventory the original selection of charges was pulled from.
	// -- this still must be fully confirmed by testing on hybrid or projectile turrets or missile batteries

	// WARNING!  Initial Implementation ONLY handles the FIRST entry in the args.chargeList,
	// which is basically supporting only single charge stacks applied to module!

	// Get Reference to Ship, Module, and Charge
	ShipRef shipRef = call.client->GetShip();
	InventoryItemRef moduleRef = shipRef->GetModule(args.moduleItemID);
	if( moduleRef.get() == NULL )
	{
		SysLog::Error("DogmaIMBound::Handle_LoadAmmoToBank()", "ERROR: cannot find module into which charge should be loaded!  How did we get here!?!?!" );
		return NULL;
	}
	EVEItemFlags moduleFlag = moduleRef->flag();
	InventoryItemRef chargeRef;

	if( !(args.chargeList.empty()))
        chargeRef = ItemFactory::GetItem(args.chargeList.at(0));

	// Move Charge into Ship's Inventory and change the Charge's flag to match flag of Module
	uint32 loadedChargeID = shipRef->AddItem( moduleFlag, chargeRef );
	//call.client->MoveItem(chargeRef->itemID(), call.client->GetShipID(), moduleFlag);

    //Create new item id return result
	if( loadedChargeID )
	{
		Call_SingleIntegerArg result;
		result.arg = loadedChargeID;	//chargeRef->itemID();
		//Return new item result
		return result.Encode();
	}

	return NULL;
}

PyResult DogmaIMBound::Handle_Activate( PyCallArgs& call )
{
    Call_Dogma_Activate args;
    uint32 callTupleSize = call.tuple->size();
    uint32 itemID = 0;
    uint32 effect = 0;

    if( callTupleSize == 2 )
    {
        // This call is for Anchor/Unanchor a POS structure or Cargo Container,
        // so get the new flag value and change the item referenced:
        if( call.tuple->items.at( 0 )->IsInt() )
        {
            itemID = call.tuple->items.at( 0 )->AsInt()->value();

            if( call.tuple->items.at( 1 )->IsInt() )
            {
                effect = call.tuple->items.at( 1 )->AsInt()->value();

                SystemEntity * se = call.client->System()->get( itemID );

                if( se == NULL )
                {
                    SysLog::Error( "DogmaIMBound::Handle_Activate()", "Item ID = %u is not a valid SystemEntity found in this system.", itemID );
                    return NULL;
                }

                // TODO: somehow notify client with one of these effects:
                // 1) effectAnchorDrop for effect = 649
                // 2) effectAnchorDropForStructures = 1022
                // 3) effectAnchorLift = 650
                // 4) effectAnchorLiftForStructures = 1023

                // Send notification SFX appropriate effect for the value effect value supplied:
                switch( effect )
                {
                    case 649:
                        //call.client->Destiny()->SendContainerUnanchor( ItemFactory::GetCargoContainer( itemID ) );
                        break;
                    case 1022:
                        //call.client->Destiny()->SendStructureUnanchor( ItemFactory::GetStructure( itemID ) );
                        break;
                    case 650:
                        //call.client->Destiny()->SendContainerAnchor( ItemFactory::GetCargoContainer( itemID ) );
                        break;
                    case 1023:
                        //call.client->Destiny()->SendStructureAnchor( ItemFactory::GetStructure( itemID ) );
                        break;
                    default:
                        break;
                }

                return new PyInt( 1 );
            }
            else
            {
                SysLog::Error( "DogmaIMBound::Handle_Activate()", "call.tuple->items.at( 1 ) was not PyInt expected type." );
                return NULL;
            }
        }
        else
        {
            SysLog::Error( "DogmaIMBound::Handle_Activate()", "call.tuple->items.at( 0 ) was not PyInt expected type." );
            return NULL;
        }
    }
    else if( callTupleSize == 4 )
    {
        if( !args.Decode( &call.tuple ) )
        {
            codelog( SERVICE__ERROR, "Unable to decode arguments from '%s'", call.client->GetName() );
            return NULL;
        }

        //TODO: make sure we are allowed to do this.
        return new PyInt( call.client->GetShip()->Activate( args.itemID, args.effectName, args.target, args.repeat ) );
    }

    return NULL;
}

PyResult DogmaIMBound::Handle_Deactivate( PyCallArgs& call )
{
    Call_Dogma_Deactivate args;
    if( !args.Decode( &call.tuple ) )
    {
        codelog( SERVICE__ERROR, "Unable to decode arguments from '%s'", call.client->GetName() );
        return NULL;
    }

    //TODO: make sure we are allowed to do this.
    call.client->GetShip()->Deactivate( args.itemID, args.effectName );

    return NULL;
}

PyResult DogmaIMBound::Handle_Overload( PyCallArgs& call ) {

	SysLog::Warning("Server", "Called Overload stub");
	return NULL;
}

PyResult DogmaIMBound::Handle_CancelOverloading( PyCallArgs& call ) {

	SysLog::Warning("Server", "Called CancelOverloading stub");
	return NULL;
}

PyResult DogmaIMBound::Handle_AddTarget(PyCallArgs &call) {
    Call_SingleIntegerArg args;
    if(!args.Decode(&call.tuple)) {
        codelog(SERVICE__ERROR, "Unable to decode arguments from '%s'", call.client->GetName());
        return NULL;
    }

    SystemManager *smgr = call.client->System();
    if(smgr == NULL) {
        codelog(SERVICE__ERROR, "Unable to find system manager from '%s'", call.client->GetName());
        return NULL;
    }
    SystemEntity *target = smgr->get(args.arg);
    if(target == NULL) {
        codelog(SERVICE__ERROR, "Unable to find entity %u in system %u from '%s'", args.arg, smgr->GetID(), call.client->GetName());
        return NULL;
    }

    ShipRef ship = call.client->GetShip();

    if( !call.client->targets.startTargeting(target, ship) )
        return NULL;

    // For Debugging purposes, put a message in the log to print out the range to the target:
    Vector3D vectorToTarget(target->GetPosition() - call.client->GetPosition());
    double rangeToTarget = vectorToTarget.length();
    // TODO: calculate double distance = SQRT(x^2 + y^2 + z^), where x,y,z are to.x-from.x, etc
    SysLog::Warning( "DogmaIMBound::Handle_AddTarget()", "TARGET ADDED - Range to Target = %f meters.", rangeToTarget );

    Rsp_Dogma_AddTarget rsp;
    rsp.success = true;
    rsp.targets.push_back(target->GetID());

    return(rsp.Encode());
}

PyResult DogmaIMBound::Handle_RemoveTarget(PyCallArgs &call) {
    Call_SingleIntegerArg args;
    if(!args.Decode(&call.tuple)) {
        codelog(SERVICE__ERROR, "Unable to decode arguments from '%s'", call.client->GetName());
        return NULL;
    }

    SystemManager *smgr = call.client->System();
    if(smgr == NULL) {
        codelog(SERVICE__ERROR, "Unable to find system manager from '%s'", call.client->GetName());
        return NULL;
    }
    SystemEntity *target = smgr->get(args.arg);
    if(target == NULL) {
        codelog(SERVICE__ERROR, "Unable to find entity %u in system %u from '%s'", args.arg, smgr->GetID(), call.client->GetName());
        return NULL;
    }

    // For Debugging purposes, put a message in the log to print out the range to the target:
    Vector3D vectorToTarget(target->GetPosition() - call.client->GetPosition());
    double rangeToTarget = vectorToTarget.length();
    SysLog::Warning( "DogmaIMBound::Handle_AddTarget()", "TARGET REMOVED - Range to Target = %f meters.", rangeToTarget );

    call.client->targets.clearTarget(target);

    return NULL;
}

PyResult DogmaIMBound::Handle_ClearTargets(PyCallArgs &call) {
    //no arguments.

    call.client->targets.clearTargets();

    return NULL;
}

PyResult DogmaIMBound::Handle_GetWeaponBankInfoForShip( PyCallArgs& call )
{
    SysLog::Debug( "DogmaIMBound", "Called GetWeaponBankInfoForShip stub." );

    return new PyDict;
}


class BuiltinSet : public PyObjectEx_Type1
{
public:
    BuiltinSet() : PyObjectEx_Type1( new PyToken("collections.defaultdict"), new_tuple(new PyToken("__builtin__.set")) ) {}
};

PyResult DogmaIMBound::Handle_GetAllInfo( PyCallArgs& call )
{
    //arg1: getCharInfo, arg2: getShipInfo
    Call_TwoBoolArgs args;
    if(!args.Decode(&call.tuple)) {
        codelog(SERVICE__ERROR, "Unable to decode arguments from '%s'", call.client->GetName());
        return NULL;
    }
    SysLog::Debug("DogmaIMBound", "GetAllInfo: getCharInfo: %s", args.arg1 ? "true" : "false");
    SysLog::Debug("DogmaIMBound", "GetAllInfo: getShipInfo: %s", args.arg2 ? "true" : "false");

    PyDict *rtn = new PyDict();

    rtn->SetItemString("shipModifiedCharAttribs", new PyNone());                // Seen PyNone
    rtn->SetItemString("structureInfo", new PyDict());                          // Seen PyDict 0 kvp
    rtn->SetItemString("locationInfo", new PyNone());                           // Seen PyNone
    rtn->SetItemString("shipInfo", new PyNone());                               // Seen PyDict x kvp
    rtn->SetItemString("charInfo", new PyNone());                               // Seen PyTuple x items
    rtn->SetItemString("shipState", new PyNone());                              // Seen PyTuple x items
    rtn->SetItemString("activeShipID", new PyInt(call.client->GetShipID()));
    rtn->SetItemString("shipID", new PyInt(call.client->GetShipID()));



    //-------------------------------------------------------------------------
    // shipInfo
    PyDict *shipInfo = new PyDict();
    PyDict *activeShipInfo = new PyDict();
    activeShipInfo->SetItemString("itemID", new PyInt(call.client->GetShipID()));
    activeShipInfo->SetItemString("invItem", call.client->GetShip()->GetItemRow());
    activeShipInfo->SetItemString("activeEffects", new PyDict());
    activeShipInfo->SetItemString("time", new PyLong(Win32TimeNow()));
    activeShipInfo->SetItemString("attributes", new PyDict());
    activeShipInfo->SetItemString("wallclockTime", new PyLong(Win32TimeNow()));
    PyDict *shipInfoAttributes = new PyDict();
    shipInfoAttributes->SetItem(new PyInt(3), new PyInt(0));
    shipInfoAttributes->SetItem(new PyInt(4), new PyInt(1148000));
    shipInfoAttributes->SetItem(new PyInt(9), new PyFloat(258.75));
    shipInfoAttributes->SetItem(new PyInt(11), new PyInt(30));
    shipInfoAttributes->SetItem(new PyInt(12), new PyInt(2));
    shipInfoAttributes->SetItem(new PyInt(13), new PyInt(2));
    shipInfoAttributes->SetItem(new PyInt(14), new PyInt(2));
    shipInfoAttributes->SetItem(new PyInt(15), new PyInt(8));
    shipInfoAttributes->SetItem(new PyInt(18), new PyFloat(155.25));
    shipInfoAttributes->SetItem(new PyInt(37), new PyFloat(350.75));
    shipInfoAttributes->SetItem(new PyInt(38), new PyInt(135));
    shipInfoAttributes->SetItem(new PyInt(552), new PyInt(54));
    shipInfoAttributes->SetItem(new PyInt(48), new PyInt(150));
    shipInfoAttributes->SetItem(new PyInt(49), new PyFloat(68.6));
    shipInfoAttributes->SetItem(new PyInt(564), new PyInt(504));
    shipInfoAttributes->SetItem(new PyInt(55), new PyInt(57375));
    shipInfoAttributes->SetItem(new PyInt(70), new PyFloat(3.88455));
    shipInfoAttributes->SetItem(new PyInt(524), new PyFloat(0.75));
    shipInfoAttributes->SetItem(new PyInt(76), new PyInt(24675));
    shipInfoAttributes->SetItem(new PyInt(79), new PyInt(4675));
    shipInfoAttributes->SetItem(new PyInt(600), new PyInt(3));
    shipInfoAttributes->SetItem(new PyInt(101), new PyInt(1));
    shipInfoAttributes->SetItem(new PyInt(102), new PyInt(0));
    shipInfoAttributes->SetItem(new PyInt(1132), new PyInt(0));
    shipInfoAttributes->SetItem(new PyInt(109), new PyFloat(0.67));
    shipInfoAttributes->SetItem(new PyInt(110), new PyFloat(0.67));
    shipInfoAttributes->SetItem(new PyInt(111), new PyFloat(0.67));
    shipInfoAttributes->SetItem(new PyInt(113), new PyFloat(0.67));
    shipInfoAttributes->SetItem(new PyInt(633), new PyInt(0));
    shipInfoAttributes->SetItem(new PyInt(1154), new PyInt(0));
    shipInfoAttributes->SetItem(new PyInt(1175), new PyInt(0));
    shipInfoAttributes->SetItem(new PyInt(1176), new PyInt(0));
    shipInfoAttributes->SetItem(new PyInt(153), new PyFloat(7.794E-07));
    shipInfoAttributes->SetItem(new PyInt(1178), new PyInt(100));
    shipInfoAttributes->SetItem(new PyInt(1179), new PyFloat(0.01));
    shipInfoAttributes->SetItem(new PyInt(1182), new PyInt(0));
    shipInfoAttributes->SetItem(new PyInt(1183), new PyInt(0));
    shipInfoAttributes->SetItem(new PyInt(1184), new PyInt(0));
    shipInfoAttributes->SetItem(new PyInt(161), new PyInt(24500));
    shipInfoAttributes->SetItem(new PyInt(162), new PyInt(40));
    shipInfoAttributes->SetItem(new PyInt(1196), new PyFloat(0.01));
    shipInfoAttributes->SetItem(new PyInt(1198), new PyFloat(0.01));
    shipInfoAttributes->SetItem(new PyInt(1199), new PyInt(100));
    shipInfoAttributes->SetItem(new PyInt(1200), new PyInt(100));
    shipInfoAttributes->SetItem(new PyInt(182), new PyInt(3327));
    shipInfoAttributes->SetItem(new PyInt(192), new PyInt(3));
    shipInfoAttributes->SetItem(new PyInt(1224), new PyInt(1));
    shipInfoAttributes->SetItem(new PyInt(208), new PyInt(0));
    shipInfoAttributes->SetItem(new PyInt(209), new PyInt(0));
    shipInfoAttributes->SetItem(new PyInt(210), new PyInt(6));
    shipInfoAttributes->SetItem(new PyInt(211), new PyInt(0));
    shipInfoAttributes->SetItem(new PyInt(1768), new PyInt(11331));
    shipInfoAttributes->SetItem(new PyInt(1259), new PyFloat(0.25));
    shipInfoAttributes->SetItem(new PyInt(1261), new PyFloat(0.25));
    shipInfoAttributes->SetItem(new PyInt(1262), new PyFloat(0.25));
    shipInfoAttributes->SetItem(new PyInt(246), new PyInt(395));
    shipInfoAttributes->SetItem(new PyInt(1271), new PyInt(10));
    shipInfoAttributes->SetItem(new PyInt(1281), new PyInt(1));
    shipInfoAttributes->SetItem(new PyInt(263), new PyFloat(157.5));
    shipInfoAttributes->SetItem(new PyInt(264), new PyFloat(157.5));
    shipInfoAttributes->SetItem(new PyInt(265), new PyFloat(201.25));
    shipInfoAttributes->SetItem(new PyInt(267), new PyFloat(0.5));
    shipInfoAttributes->SetItem(new PyInt(268), new PyFloat(0.9));
    shipInfoAttributes->SetItem(new PyInt(269), new PyFloat(0.65));
    shipInfoAttributes->SetItem(new PyInt(270), new PyFloat(0.65));
    shipInfoAttributes->SetItem(new PyInt(271), new PyInt(1));
    shipInfoAttributes->SetItem(new PyInt(272), new PyFloat(0.5));
    shipInfoAttributes->SetItem(new PyInt(273), new PyFloat(0.6));
    shipInfoAttributes->SetItem(new PyInt(274), new PyFloat(0.8));
    shipInfoAttributes->SetItem(new PyInt(277), new PyInt(1));
    shipInfoAttributes->SetItem(new PyInt(283), new PyInt(10));
    shipInfoAttributes->SetItem(new PyInt(1177), new PyInt(0));
    shipInfoAttributes->SetItem(new PyInt(525), new PyInt(1));
    shipInfoAttributes->SetItem(new PyInt(422), new PyInt(1));
    shipInfoAttributes->SetItem(new PyInt(479), new PyInt(593750));
    shipInfoAttributes->SetItem(new PyInt(482), new PyFloat(155.25));
    shipInfoAttributes->SetItem(new PyInt(484), new PyFloat(0.8));
    activeShipInfo->SetItemString("attributes", shipInfoAttributes);

    shipInfo->SetItem(new PyInt(call.client->GetShipID()), new PyObject("utillib.KeyVal", activeShipInfo));
    rtn->SetItemString("shipInfo", shipInfo);


    //-------------------------------------------------------------------------
    // charInfo
    PyTuple *charInfo = new PyTuple(2);
    PyDict *charInfoDict = new PyDict();
    charInfoDict->SetItemString("itemID", new PyInt(call.client->GetCharacterID()));
    charInfoDict->SetItemString("wallclockTime", new PyLong(Win32TimeNow()));
    charInfoDict->SetItemString("time", new PyLong(Win32TimeNow()));
    charInfoDict->SetItemString("activeEffects", new PyDict());
    charInfoDict->SetItemString("invItem", call.client->GetChar()->GetItemRow());
    charInfoDict->SetItemString("attributes", new PyDict());

    PyDict *charInfoAttributes = new PyDict();
    charInfoAttributes->SetItem(new PyInt(4), new PyInt(0));        //attributeMass
    charInfoAttributes->SetItem(new PyInt(522), new PyInt(2));      //attributeDamageCloudChance
    charInfoAttributes->SetItem(new PyInt(38), new PyInt(0));       //attributeCapacity
    charInfoAttributes->SetItem(new PyInt(161), new PyInt(1));      //attributeVolume
    charInfoAttributes->SetItem(new PyInt(162), new PyInt(0));      //attributeRadius
    charInfoAttributes->SetItem(new PyInt(164), new PyInt(19));     //attributeCharisma
    charInfoAttributes->SetItem(new PyInt(165), new PyInt(20));     //attributeIntelligence
    charInfoAttributes->SetItem(new PyInt(166), new PyInt(20));     //attributeMemory
    charInfoAttributes->SetItem(new PyInt(167), new PyInt(20));     //attributePerception
    charInfoAttributes->SetItem(new PyInt(168), new PyInt(20));     //attributeWillpower
    charInfoAttributes->SetItem(new PyInt(187), new PyFloat(0.1));  //attributeRepairCostMultiplier
    charInfoAttributes->SetItem(new PyInt(192), new PyInt(4));      //attributeMaxLockedTargets
    charInfoAttributes->SetItem(new PyInt(196), new PyInt(1));      //attributeManufactureSlotLimit
    charInfoAttributes->SetItem(new PyInt(723), new PyFloat(0.9));  //attributeContrabandDetectionChance
    charInfoAttributes->SetItem(new PyInt(212), new PyInt(1));      //attributeMissileDamageMultiplier
    charInfoAttributes->SetItem(new PyInt(219), new PyFloat(0.96)); //attributeManufactureTimeMultiplier
    charInfoAttributes->SetItem(new PyInt(1267), new PyInt(10));    //attributeModuleRepairRate
    charInfoAttributes->SetItem(new PyInt(1277), new PyFloat(0.5)); //attributeShipBrokenModuleRepairCostMultiplier
    charInfoAttributes->SetItem(new PyInt(352), new PyInt(1));      //attributeMaxActiveDrones
    charInfoAttributes->SetItem(new PyInt(359), new PyInt(1));      //attributeFastTalkPercentage
    charInfoAttributes->SetItem(new PyInt(369), new PyInt(1));      //attributeManufactureCostMultiplier
    charInfoAttributes->SetItem(new PyInt(378), new PyInt(1));      //attributeRefiningYieldPercentage
    charInfoAttributes->SetItem(new PyInt(385), new PyInt(1));      //attributeManufacturingTimeResearchSpeed
    charInfoAttributes->SetItem(new PyInt(387), new PyFloat(0.8));  //attributeCopySpeedPercent
    charInfoAttributes->SetItem(new PyInt(398), new PyInt(1));      //attributeMineralNeedResearchSpeed
    charInfoAttributes->SetItem(new PyInt(399), new PyInt(0));      //attributeDuplicatingChance
    charInfoAttributes->SetItem(new PyInt(1959), new PyInt(1));     //attributeInventionReverseEngineeringResearchSpeed
    charInfoAttributes->SetItem(new PyInt(428), new PyInt(120));    //attributeMiningDroneAmountPercent
    charInfoAttributes->SetItem(new PyInt(435), new PyInt(1));      //attributeMaxGangModules
    charInfoAttributes->SetItem(new PyInt(445), new PyFloat(0.1));  //attributeSmugglingChance
    charInfoAttributes->SetItem(new PyInt(458), new PyInt(25000));  //attributeDroneControlDistance
    charInfoAttributes->SetItem(new PyInt(467), new PyInt(1));      //attributeMaxLaborotorySlots
    charInfoDict->SetItemString("attributes", charInfoAttributes);


    PyDict *charInfoWrapperDict = new PyDict();
    charInfoWrapperDict->SetItem(new PyInt(call.client->GetCharacterID()), new PyObject("utillib.KeyVal", charInfoDict));

    charInfo->SetItem(0, charInfoWrapperDict);
    charInfo->SetItem(1, new PyTuple(0));
    rtn->SetItem("charInfo", charInfo);

    return new PyObject("utillib.KeyVal", rtn);

}

