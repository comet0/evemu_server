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

#ifndef __CELESTIAL__H__INCL__
#define __CELESTIAL__H__INCL__

#include "inventory/InventoryItem.h"
#include "system/SystemEntity.h"
#include "map/MapSolarSystem.h"

/**
 * Data container for celestial object.
 */
class CelestialObjectData {
public:
    CelestialObjectData(
        double _radius = 0.0,
        double _security = 0.0,
        uint8 _celestialIndex = 0,
        uint8 _orbitIndex = 0,
        uint8 _solarSystemID = 0
    );

    // Data members:
    double radius;
    double security;
    uint8 celestialIndex;
    uint8 orbitIndex;
    uint32 solarSystemID;
};

/**
 * InventoryItem for generic celestial object.
 */
class CelestialObject
: public InventoryItem
{
    friend class InventoryItem; // to let it construct us
public:
    CelestialObject(
        uint32 _celestialID,
                    const InvTypeRef _type,
        const ItemData &_data);

    CelestialObject(
        uint32 _celestialID,
        // InventoryItem stuff:
                    const InvTypeRef _type,
        const ItemData &_data,
        // CelestialObject stuff:
        const CelestialObjectData &_cData
    );

    /**
     * Loads celestial object.
     *
     * @param[in] celestialID ID of celestial object to load.
     * @return Pointer to new CelestialObject; NULL if fails.
     */
    static CelestialObjectRef Load(uint32 celestialID);
    /**
     * Spawns new celestial object.
     *
     * @param[in] data Item data for celestial object.
     * @return Pointer to new celestial object; NULL if failed.
     */
    static CelestialObjectRef Spawn(ItemData &data);

    /*
     * Primary public interface:
     */
    void Delete();

    /*
     * Access methods:
     */
    double      radius() const { return m_radius; }
    double      security() const { return m_security; }
    uint8       celestialIndex() const { return m_celestialIndex; }
    uint8       orbitIndex() const { return m_orbitIndex; }
    uint32      solarSystemID() const { return m_solarSystemID; }
    MapSolarSystemRef getSolarSystem() { return MapSolarSystem::getSystem(m_solarSystemID); }

protected:
    /*
     * Member functions:
     */
    // Template loader:
    template<class _Ty>
    static RefPtr<_Ty> _LoadItem(uint32 celestialID,
        // InventoryItem stuff:
                                 const InvTypeRef type, const ItemData &data)
    {
        // make sure it's celestial object, entity object or station
        if (type->getCategoryID() != EVEDB::invCategories::Celestial
            && type->getCategoryID() != EVEDB::invCategories::Entity
            && type->groupID != EVEDB::invGroups::Station)
        {
            _log(ITEM__ERROR, "Trying to load %s as Celestial.", type->getCategory()->categoryName.c_str());
            return RefPtr<_Ty>();
        }

        // load celestial data
        CelestialObjectData cData;
        if(!InventoryDB::GetCelestialObject(celestialID, cData))
        {
            return RefPtr<_Ty>();
        }

        return _Ty::template _LoadCelestialObject<_Ty>( celestialID, type, data, cData );
    }

    // Actual loading stuff:
    template<class _Ty>
    static RefPtr<_Ty> _LoadCelestialObject(uint32 celestialID,
        // InventoryItem stuff:
                                            const InvTypeRef type, const ItemData &data,
        // CelestialObject stuff:
        const CelestialObjectData &cData
    );

    static uint32 _Spawn(
        // InventoryItem stuff:
        ItemData &data
    );

    uint32 inventoryID() const { return itemID();
    }

    PyRep *GetItem() 
    {
        return getPackedRow();
    }

    //void AddItem(InventoryItemRef item);

    /*
     * Data members:
     */
    double m_radius;
    double m_security;
    uint8 m_celestialIndex;
    uint8 m_orbitIndex;
    uint32 m_solarSystemID;
};


/**
 * DynamicSystemEntity which represents celestial object in space
 */
class InventoryItem;
class DestinyManager;
class SystemManager;

class CelestialEntity
: public CelestialDynamicSystemEntity
{
public:
    CelestialEntity(
        CelestialObjectRef celestial,
        //InventoryItemRef celestial,
        SystemManager *system,
        const Vector3D &position);

    /*
     * Primary public interface:
     */
    CelestialObjectRef GetCelestialObject() { return _celestialRef; }
//    InventoryItemRef GetCelestialObject() { return _celestialRef; }
    DestinyManager * GetDestiny() { return m_destiny; }
    SystemManager * GetSystem() { return m_system; }

    /*
     * Public fields:
     */

    inline double x() const { return(GetPosition().x); }
    inline double y() const { return(GetPosition().y); }
    inline double z() const { return(GetPosition().z); }

    //SystemEntity interface:
    virtual EntityClass GetClass() const { return(ecCelestial); }
    virtual bool IsCelestialEntity() const { return true; }
    virtual CelestialEntity *CastToCelestialEntity() { return(this); }
    virtual const CelestialEntity *CastToCelestialEntity() const { return(this); }
    virtual void Process();
    //virtual void EncodeDestiny( Buffer& into ) const;
    virtual void QueueDestinyUpdate(PyTuple **du) {/* not required to consume */}
    virtual void QueueDestinyEvent(PyTuple **multiEvent) {/* not required to consume */}
    virtual uint32 GetCorporationID() const { return(1); }
    virtual uint32 GetAllianceID() const { return(0); }
    virtual void Killed(Damage &fatal_blow);
    virtual SystemManager *System() const { return(m_system); }

    void ForcedSetPosition(const Vector3D &pt);

    virtual bool ApplyDamage(Damage &d);
    virtual void MakeDamageState(DoDestinyDamageState &into) const;

    void SendNotification(const PyAddress &dest, EVENotificationStream &noti, bool seq=true);
    void SendNotification(const char *notifyType, const char *idType, PyTuple **payload, bool seq=true);

protected:
    /*
     * Member functions
     */
    void _ReduceDamage(Damage &d);
    void ApplyDamageModifiers(Damage &d, SystemEntity *target);

    /*
     * Member fields:
     */
    SystemManager *const m_system;    //we do not own this
    CelestialObjectRef _celestialRef;   // We don't own this
//    InventoryItemRef _celestialRef;     // We don't own this

    /* Used to calculate the damages on NPCs
     * I don't know why, npc->Set_shieldCharge does not work
     * calling npc->shieldCharge() return the complete shield
     * So we get the values on creation and use then instead.
    */
    double m_shieldCharge;
    double m_armorDamage;
    double m_hullDamage;
};

#endif /* !__CELESTIAL__H__INCL__ */


