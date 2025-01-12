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
    Author:     Aknor Jaden
*/

#include "eve-server.h"

#include "Client.h"
#include "inventory/AttributeEnum.h"
#include "ship/DestinyManager.h"
#include "system/Container.h"

/*
 * CargoContainer
 */
CargoContainer::CargoContainer(
    uint32 _containerID,
    // InventoryItem stuff:
                               const InvTypeRef _containerType,
    const ItemData &_data)
: InventoryItem(_containerID, _containerType, _data) {}

CargoContainerRef CargoContainer::Load(uint32 containerID)
{
    return InventoryItem::Load<CargoContainer>( containerID );
}

CargoContainerRef CargoContainer::Spawn(ItemData &data)
{
    InvTypeRef type = InvType::getType(data.typeID);
    // Create default dynamic attributes in the AttributeMap:
    data.attributes[AttrIsOnline] = EvilNumber(1); // Is Online
    data.attributes[AttrDamage] = EvilNumber(0.0); // Structure Damage
    //data.attributes[AttrShieldCharge,  type->getAttribute(AttrShieldCapacity));  // Shield Charge
    //data.attributes[AttrArmorDamage] = EvilNumber(0.0); // Armor Damage
    data.attributes[AttrMass] = type->mass; // Mass
    data.attributes[AttrVolume] = type->volume; // Volume
    data.attributes[AttrCapacity] = type->capacity; // Capacity
    data.attributes[AttrRadius] = type->getAttribute(AttrRadius); // Radius
    data.attributes[AttrDamage] = EvilNumber(0);

    // Set type attributes
    for(auto attr : type->m_attributes)
    {
        data.attributes[attr.first] = attr.second;
    }

    uint32 containerID = CargoContainer::_Spawn(data);
    if(containerID == 0)
    {
        return CargoContainerRef();
    }
    return CargoContainer::Load(containerID);
}

uint32 CargoContainer::_Spawn(ItemData &data)
{
    // make sure it's a cargo container
    const InvTypeRef st = InvType::getType(data.typeID);
    if (st.get() == nullptr)
    {
        return 0;
    }

    // store item data
    uint32 containerID = InventoryItem::_Spawn(data);
    if(containerID == 0)
    {
        return 0;
    }

    // nothing additional

    return containerID;
}

bool CargoContainer::loadState()
{
    // load contents
    if(!LoadContents())
    {
        return false;
    }

    return InventoryItem::loadState();
}

void CargoContainer::Delete()
{
    // delete contents first
    DeleteContents(  );

    InventoryItem::Delete();
}

double CargoContainer::GetCapacity(EVEItemFlags flag) const
{
    switch( flag ) {
        case flagAutoFit:
        case flagCargoHold:     return getAttribute(AttrCapacity).get_float();
        default:
            return 0.0;
    }
}

bool CargoContainer::ValidateAddItem(EVEItemFlags flag, InventoryItemRef item) const
{
    if( flag == flagCargoHold )
    {
        //get all items in cargohold
        EvilNumber capacityUsed(0);
        std::vector<InventoryItemRef> items;
        FindByFlag(flag, items);
        for (uint32 i = 0; i < items.size(); i++)
        {
            capacityUsed += items[i]->getAttribute(AttrVolume);
        }
        if (capacityUsed + (item->getAttribute(AttrVolume) * item->quantity()) > getAttribute(AttrCapacity))
        {
            throw PyException(MakeCustomError("Not enough cargo space!"));
            return false;
        }
        return true;
    }
    return false;
}

void CargoContainer::AddItem(InventoryItemRef item)
{
    InventoryEx::AddItem( item );
}

void CargoContainer::RemoveItem(InventoryItemRef item)
{
    SysLog::Error( "CargoContainer::RemoveItem()", "Empty status of cargo containers is not checked to ensure proper despawn of jet-cans." );
    InventoryEx::RemoveItem( item );
}


using namespace Destiny;

ContainerEntity::ContainerEntity(
    CargoContainerRef container,
    SystemManager *system,
    const Vector3D &position)
: DynamicSystemEntity(new DestinyManager(this, system), container),
  m_system(system)
{
    _containerRef = container;
    m_destiny->SetPosition(position, false);
}

void ContainerEntity::Process() {
    SystemEntity::Process();
}

void ContainerEntity::ForcedSetPosition(const Vector3D &pt) {
    m_destiny->SetPosition(pt, false);
}

void ContainerEntity::EncodeDestiny( Buffer& into ) const
{
    const Vector3D& position = GetPosition();
    const std::string itemName( GetName() );

    /*if(m_orbitingID != 0) {
        #pragma pack(1)
        struct AddBall_Orbit {
            BallHeader head;
            MassSector mass;
            ShipSector ship;
            DSTBALL_ORBIT_Struct main;
            NameStruct name;
        };
        #pragma pack()

        into.resize(start
            + sizeof(AddBall_Orbit)
            + slen*sizeof(uint16) );
        uint8 *ptr = &into[start];
        AddBall_Orbit *item = (AddBall_Orbit *) ptr;
        ptr += sizeof(AddBall_Orbit);

        item->head.entityID = GetID();
        item->head.mode = Destiny::DSTBALL_ORBIT;
        item->head.radius = m_self->radius();
        item->head.x = x();
        item->head.y = y();
        item->head.z = z();
        item->head.sub_type = IsMassive | IsFree;

        item->mass.mass = m_self->mass();
        item->mass.unknown51 = 0;
        item->mass.unknown52 = 0xFFFFFFFFFFFFFFFFLL;
        item->mass.corpID = GetCorporationID();
        item->mass.unknown64 = 0xFFFFFFFF;

        item->ship.max_speed = m_self->maxVelocity();
        item->ship.velocity_x = m_self->maxVelocity();    //hacky hacky
        item->ship.velocity_y = 0.0;
        item->ship.velocity_z = 0.0;
        item->ship.agility = 1.0;    //hacky
        item->ship.speed_fraction = 0.133f;    //just strolling around. TODO: put in speed fraction!

        item->main.unknown116 = 0xFF;
        item->main.followID = m_orbitingID;
        item->main.followRange = 6000.0f;

        item->name.name_len = slen;    // in number of unicode chars
        //strcpy_fake_unicode(item->name.name, GetName());
    } else {
        BallHeader head;
        head.entityID = GetID();
        head.mode = Destiny::DSTBALL_STOP;
        head.radius = GetRadius();
        head.x = position.x;
        head.y = position.y;
        head.z = position.z;
        head.sub_type = IsMassive | IsFree;
        into.Append( head );

        MassSector mass;
        mass.mass = GetMass();
        mass.cloak = 0;
        mass.unknown52 = 0xFFFFFFFFFFFFFFFFLL;
        mass.corpID = GetCorporationID();
        mass.allianceID = GetAllianceID();
        into.Append( mass );

        ShipSector ship;
        ship.max_speed = GetMaxVelocity();
        ship.velocity_x = 0.0;
        ship.velocity_y = 0.0;
        ship.velocity_z = 0.0;
        ship.unknown_x = 0.0;
        ship.unknown_y = 0.0;
        ship.unknown_z = 0.0;
        ship.agility = GetAgility();
        ship.speed_fraction = 0.0;
        into.Append( ship );

        DSTBALL_STOP_Struct main;
        main.formationID = 0xFF;
        into.Append( main );

    }
*/
    BallHeader head;
    head.entityID = GetID();
    head.mode = Destiny::DSTBALL_RIGID;
    head.radius = GetRadius();
    head.x = position.x;
    head.y = position.y;
    head.z = position.z;
    head.sub_type = /*HasMiniBalls | */IsGlobal;
    into.Append( head );

    DSTBALL_RIGID_Struct main;
    main.formationID = 0xFF;
    into.Append( main );

/*
    const uint16 miniballsCount = 1;
    into.Append( miniballsCount );

    MiniBall miniball;
    miniball.x = -7701.181;
    miniball.y = 8060.06;
    miniball.z = 27878.900;
    miniball.radius = 1639.241;
    into.Append( miniball );
*/
}

void ContainerEntity::MakeDamageState(DoDestinyDamageState &into) const
{
    into.shield = 0.0; //(m_self->getAttribute(AttrShieldCharge).get_float() / m_self->getAttribute(AttrShieldCapacity).get_float());
    into.tau = 100000;    //no freaking clue.
    into.timestamp = Win32TimeNow();
//    armor damage isn't working...
    into.armor = 0.0; //1.0 - (m_self->getAttribute(AttrArmorDamage).get_float() / m_self->getAttribute(AttrArmorHP).get_float());
    into.structure = 1.0 - (m_self->getAttribute(AttrDamage).get_float() / m_self->getAttribute(AttrHp).get_float());
}

