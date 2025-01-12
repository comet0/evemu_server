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

#include "inventory/AttributeEnum.h"
#include "ship/DestinyManager.h"
#include "station/Station.h"

/*
 * StationData
 */
StationData::StationData(
    uint32 _security,
    double _dockingCostPerVolume,
    double _maxShipVolumeDockable,
    uint32 _officeRentalCost,
    uint32 _operationID,
    double _reprocessingEfficiency,
    double _reprocessingStationsTake,
    EVEItemFlags _reprocessingHangarFlag)
: security(_security),
  dockingCostPerVolume(_dockingCostPerVolume),
  maxShipVolumeDockable(_maxShipVolumeDockable),
  officeRentalCost(_officeRentalCost),
  operationID(_operationID),
  reprocessingEfficiency(_reprocessingEfficiency),
  reprocessingStationsTake(_reprocessingStationsTake),
  reprocessingHangarFlag(_reprocessingHangarFlag)
{
}

/*
 * Station
 */
Station::Station(
    uint32 _stationID,
    // InventoryItem stuff:
                 const InvTypeRef _type,
    const ItemData &_data,
    // CelestialObject stuff:
    const CelestialObjectData &_cData,
    // Station stuff:
    const StationData &_stData)
: CelestialObject(_stationID, _type, _data, _cData),
m_stationType(StaStationType::getType(_type->typeID)),
  m_security(_stData.security),
  m_dockingCostPerVolume(_stData.dockingCostPerVolume),
  m_maxShipVolumeDockable(_stData.maxShipVolumeDockable),
  m_officeRentalCost(_stData.officeRentalCost),
  m_operationID(_stData.operationID),
  m_reprocessingEfficiency(_stData.reprocessingEfficiency),
  m_reprocessingStationsTake(_stData.reprocessingStationsTake),
  m_reprocessingHangarFlag(_stData.reprocessingHangarFlag)
{
}

StationRef Station::Load(uint32 stationID)
{
    return InventoryItem::Load<Station>( stationID );
}

template<class _Ty>
RefPtr<_Ty> Station::_LoadStation(uint32 stationID,
                                  // InventoryItem stuff:
                                  const InvTypeRef type, const ItemData &data,
                                  // CelestialObject stuff:
                                  const CelestialObjectData &cData,
                                  // Station stuff:
                                  const StationData &stData)
{
    // ready to create
    return StationRef(new Station(stationID, type, data, cData, stData));
}

bool Station::loadState()
{
    // load contents
    if(!LoadContents())
    {
        return false;
    }

    return CelestialObject::loadState();
}

uint32 Station::_Spawn(
    // InventoryItem stuff:
    ItemData &data
) {
    // make sure it's a Station
    const InvTypeRef item = InvType::getType(data.typeID);
    if (!(item->getCategoryID() == EVEDB::invCategories::Station))
    {
        return 0;
    }

    // store item data
    uint32 stationID = InventoryItem::_Spawn(data);
    if(stationID == 0)
    {
        return 0;
    }

    return stationID;
}

using namespace Destiny;

StationEntity::StationEntity(
    StationRef station,
    SystemManager *system,
    const Vector3D &position)
: DynamicSystemEntity(new DestinyManager(this, system), station),
  m_system(system)
{
    _stationRef = station;
    m_destiny->SetPosition(position, false);
}

void StationEntity::Process() {
    SystemEntity::Process();
}

void StationEntity::ForcedSetPosition(const Vector3D &pt) {
    m_destiny->SetPosition(pt, false);
}

void StationEntity::EncodeDestiny( Buffer& into ) const
{

    const Vector3D& position = GetPosition();
    const std::string itemName( GetName() );
/*
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
    head.sub_type = HasMiniBalls | IsGlobal;
    into.Append( head );

    DSTBALL_RIGID_Struct main;
    main.formationID = 0xFF;
    into.Append( main );

    const uint16 miniballsCount = 1;
    into.Append( miniballsCount );

    MiniBall miniball;
    miniball.x = -7701.181;
    miniball.y = 8060.06;
    miniball.z = 27878.900;
    miniball.radius = 1639.241;
    into.Append( miniball );
}

void StationEntity::MakeDamageState(DoDestinyDamageState &into) const
{
    double shields = 1, armor = 1, hull = 1;
    if (!m_self->fetchAttribute(AttrShieldCapacity, shields))
    {
        m_self->type()->fetchAttribute(AttrShieldCapacity, shields);
    }
    if (!m_self->fetchAttribute(AttrArmorHP, armor))
    {
        m_self->type()->fetchAttribute(AttrArmorHP, armor);
    }
    if (!m_self->fetchAttribute(AttrHp, hull))
    {
        m_self->type()->fetchAttribute(AttrHp, hull);
    }
    if (armor == 0)
    {
        armor = 1;
    }
    into.shield = (m_self->getAttribute(AttrShieldCharge).get_float() / shields);
    into.tau = 100000;    //no freaking clue.
    into.timestamp = Win32TimeNow();
//    armor damage isn't working...
    into.armor = 1.0 - (m_self->getAttribute(AttrArmorDamage).get_float() / armor);
    into.structure = 1.0 - (m_self->getAttribute(AttrDamage).get_float() / hull);
}

