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

#include "PyServiceCD.h"
#include "inventory/InvBrokerService.h"
#include "inventory/InventoryBound.h"
#include "PyServiceMgr.h"
#include "database/EVEDBUtils.h"

class InvBrokerBound
: public PyBoundObject
{
public:

    PyCallable_Make_Dispatcher(InvBrokerBound)

    InvBrokerBound(uint32 entityID)
    : PyBoundObject(new Dispatcher(this)),
      m_entityID(entityID)
    {
        m_strBoundObjectName = "InvBrokerBound";

		PyCallable_REG_CALL(InvBrokerBound, GetContainerContents);
        PyCallable_REG_CALL(InvBrokerBound, GetInventoryFromId)
        PyCallable_REG_CALL(InvBrokerBound, GetInventory)
        PyCallable_REG_CALL(InvBrokerBound, SetLabel)
        PyCallable_REG_CALL(InvBrokerBound, TrashItems)
        PyCallable_REG_CALL(InvBrokerBound, GetSelfInvItem)
    }
    virtual ~InvBrokerBound()
    {
    }

    virtual void Release() {
        //I hate this statement
        delete this;
    }

	PyCallable_DECL_CALL(GetContainerContents)
    PyCallable_DECL_CALL(GetInventoryFromId)
    PyCallable_DECL_CALL(GetInventory)
    PyCallable_DECL_CALL(SetLabel)
    PyCallable_DECL_CALL(TrashItems)
    PyCallable_DECL_CALL(GetSelfInvItem)


protected:
    uint32 m_entityID;
};

PyCallable_Make_InnerDispatcher(InvBrokerService)

InvBrokerService::InvBrokerService()
: PyService("invbroker", new Dispatcher(this))
{
    PyCallable_REG_CALL(InvBrokerService, GetItemDescriptor)
}

InvBrokerService::~InvBrokerService()
{
}

PyResult InvBrokerService::Handle_GetItemDescriptor(PyCallArgs &call)
{
    // not really clear on the use of this one? just a general header update?!
    // from Inventory::List

    PyList *keywords = new PyList();
    keywords->AddItem(new_tuple(new PyString("stacksize"), new PyToken("eve.common.script.sys.eveCfg.StackSize")));
    keywords->AddItem(new_tuple(new PyString("singleton"), new PyToken("eve.common.script.sys.eveCfg.Singleton")));

    DBRowDescriptor* header = new DBRowDescriptor(keywords);
    header->AddColumn( "itemID",     DBTYPE_I8 );
    header->AddColumn( "typeID",     DBTYPE_I4 );
    header->AddColumn( "ownerID",    DBTYPE_I4 );
    header->AddColumn( "locationID", DBTYPE_I8 );
    header->AddColumn( "flagID",     DBTYPE_I2 );
    header->AddColumn( "quantity",   DBTYPE_I4 );
    header->AddColumn( "groupID",    DBTYPE_I2 );
    header->AddColumn( "categoryID", DBTYPE_I4 );
    header->AddColumn( "customInfo", DBTYPE_STR );

    //header->AddColumn( "singleton",  DBTYPE_BOOL );
    return header;
}


PyBoundObject *InvBrokerService::_CreateBoundObject(Client *c, const PyRep *bind_args) {
    InvBroker_BindArgs args;
    //temp crap until I rework _CreateBoundObject's signature
    PyRep *t = bind_args->Clone();
    if(!args.Decode(&t)) {
        codelog(SERVICE__ERROR, "Failed to decode bind args from '%s'", c->GetName());
        return NULL;
    }
    _log(CLIENT__MESSAGE, "InvBrokerService bind request for:");
    args.Dump(CLIENT__MESSAGE, "    ");

    return new InvBrokerBound(args.entityID);
}

PyResult InvBrokerBound::Handle_GetContainerContents(PyCallArgs &call)
{
	uint32 itemID = pyAs(Int, call.tuple->GetItem(0))->value();

	// TODO: check if container is allowed to be examined

	// get list of items in container and return it to client
    DBQueryResult res;
	if(!DBcore::RunQuery(res,
		"SELECT "
		"  e.itemID, "
		"  e.flag as flagID, "
		"  e.typeID, "
		"  e.quantity as stacksize "
		"FROM "
		"  srvEntity e "
		"WHERE "
		"  locationID=%d AND "
		"  flag=5", itemID))
	{
        codelog(SERVICE__ERROR, "Error in query: %s", res.error.c_str());
		return NULL;
	}
	return(DBResultToCRowset(res));
}

//this is a view into the entire inventory item.
PyResult InvBrokerBound::Handle_GetInventoryFromId(PyCallArgs &call) {
    Call_TwoIntegerArgs args;
    if (!args.Decode(&call.tuple)) {
        codelog(SERVICE__ERROR, "%s: Bad arguments", call.client->GetName());
        return (NULL);
    }
    //bool passive = (args.arg2 != 0);  //no idea what this is for.

    ItemFactory::SetUsingClient(call.client);
    // TODO: this line is insufficient for some object types, like containers in space, so expand it
    // by having a switch that acts differently based on either categoryID or groupID or both:
    Inventory *inventory = ItemFactory::GetInventory(args.arg1);
    if(inventory == NULL) {
        codelog(SERVICE__ERROR, "%s: Unable to load inventory %u", call.client->GetName(), args.arg1);
        return (NULL);
    }

    //we just bind up a new inventory object and give it back to them.
    InventoryBound *ib = new InventoryBound(*inventory, flagAutoFit);
    PyRep *result = PyServiceMgr::BindObject(call.client, ib);

    // Release the item factory now that the ItemFactory is finished being used:
    ItemFactory::UnsetUsingClient();

    return result;
}

//this is a view into an inventory item using a specific flag.
PyResult InvBrokerBound::Handle_GetInventory(PyCallArgs &call) {
    Inventory_GetInventory args;
    if(!args.Decode(&call.tuple)) {
        codelog(SERVICE__ERROR, "Unable to decode arguments");
        return NULL;
    }

    EVEItemFlags flag;
    switch(args.container) {
        case containerWallet:
            flag = flagWallet;
            break;
        case containerCharacter:
            flag = flagSkill;
            break;
        case containerHangar:
            flag = flagHangar;
            break;

        case containerGlobal:
            flag = flagNone;
            break;

        case containerSolarSystem:
        case containerScrapHeap:
        case containerFactory:
        case containerBank:
        case containerRecycler:
        case containerOffices:
        case containerStationCharacters:
        case containerCorpMarket:
        default:
            codelog(SERVICE__ERROR, "Unhandled container type %u", args.container);
            return NULL;
    }

    ItemFactory::SetUsingClient(call.client);
    Inventory *inventory = ItemFactory::GetInventory(m_entityID);
    if(inventory == NULL) {
        codelog(SERVICE__ERROR, "%s: Unable to load item %u", call.client->GetName(), m_entityID);
        return (NULL);
    }

    _log(SERVICE__MESSAGE, "Binding inventory object for %s for inventory %u with flag %u", call.client->GetName(), m_entityID, flag);

    //we just bind up a new inventory object and give it back to them.
    InventoryBound *ib = new InventoryBound(*inventory, flag);
    PyRep *result = PyServiceMgr::BindObject(call.client, ib);

    // Release the item factory now that the ItemFactory is finished being used:
    ItemFactory::UnsetUsingClient();

    return result;
}

PyResult InvBrokerBound::Handle_SetLabel(PyCallArgs &call) {
    CallSetLabel args;
    if(!args.Decode(&call.tuple)) {
        codelog(SERVICE__ERROR, "Unable to decode arguments");
        return NULL;
    }

    ItemFactory::SetUsingClient(call.client);
    InventoryItemRef item = ItemFactory::GetItem(args.itemID);
    if( !item ) {
        codelog(SERVICE__ERROR, "%s: Unable to load item %u", call.client->GetName(), args.itemID);
        return (NULL);
    }

    if(item->ownerID() != call.client->GetCharacterID()) {
        _log(SERVICE__ERROR, "Character %u tried to rename item %u of character %u.", call.client->GetCharacterID(), item->itemID(), item->ownerID());
        return NULL;
    }

    item->Rename( args.itemName.c_str() );


    // This call as-is is NOT correct for any item category other than ships,
    // so until we can get the right string argument for other kinds of session updates,
    // we need to block this call so our characters don't "board" non-ship objects:
    if( item->categoryID() == EVEDB::invCategories::Ship )
        call.client->UpdateSession("shipid", item->itemID() );

    // Release the item factory now that the ItemFactory is finished being used:
    ItemFactory::UnsetUsingClient();

    return NULL;
}

PyResult InvBrokerBound::Handle_TrashItems(PyCallArgs &call)
{
    Call_TrashItems args;
    if(!args.Decode(&call.tuple)) {
        codelog(SERVICE__ERROR, "Unable to decode arguments");
        return(new PyList());
    }

    std::vector<int32>::const_iterator cur, end;
    cur = args.items.begin();
    end = args.items.end();
    ItemFactory::SetUsingClient(call.client);
    for(; cur != end; cur++)
    {
        InventoryItemRef item = ItemFactory::GetItem(*cur);
        if( !item ) {
            codelog(SERVICE__ERROR, "%s: Unable to load item %u to delete it. Skipping.", call.client->GetName(), *cur);
        }
        else if( call.client->GetCharacterID() != item->ownerID() ) {
            codelog(SERVICE__ERROR, "%s: Tried to trash item %u which is not yours. Skipping.", call.client->GetName(), *cur);
        }
        else if( item->locationID() != (uint32)args.locationID ) {
            codelog(SERVICE__ERROR, "%s: Item %u is not in location %u. Skipping.", call.client->GetName(), *cur, args.locationID);
        }
        else
            item->Delete();
    }

    // Release the item factory now that the ItemFactory is finished being used:
    ItemFactory::UnsetUsingClient();

    return(new PyList());
}

PyResult InvBrokerBound::Handle_GetSelfInvItem(PyCallArgs &call)
{
    DBQueryResult result;
    if(!DBcore::RunQuery(result,
                         "SELECT"
                         " itemID, typeID, ownerID, locationID, flag as flagID,"
                         " quantity, groupID, categoryID, customInfo"
                         " FROM srvEntity"
                         " LEFT JOIN invTypes USING(typeID)"
                         " LEFT JOIN invGroups USING(groupID)"
                         " WHERE itemID = %u",
                         call.client->GetShip()->itemID()))
    {
        _log(DATABASE__ERROR, "Failed to query ship item for %u: %s.", call.client->GetCharacterID(), result.error.c_str());
        return nullptr;
    }

    DBRowDescriptor *header = new DBRowDescriptor(result);
    DBResultRow row;
    while(result.GetRow(row))
    {
        return CreatePackedRow(row, header);
    }

    return nullptr;

}
