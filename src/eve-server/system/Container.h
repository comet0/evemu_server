/*
    ------------------------------------------------------------------------------------
    LICENSE:
    ------------------------------------------------------------------------------------
    This file is part of EVEmu: EVE Online Server Emulator
    Copyright 2006 - 2016 The EVEmu Team
    For the latest information visit http://evemu.mmoforge.org
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
    Author:        Aknor Jaden
*/

#ifndef __CONTAINER__H__INCL__
#define __CONTAINER__H__INCL__

#include "inventory/Inventory.h"
#include "inventory/InventoryItem.h"
#include "system/SystemEntity.h"

// TODO: We may need to create CargoContainerTypeData and CargoContainerType classes just as Ship.h/Ship.cpp
// has in order to load up type data specific to cargo containers.  For now, the generic ItemType class is used.

/**
 * InventoryItem which represents cargo container.
 */
class CargoContainer
: public InventoryItem,
  public InventoryEx
{
    friend class InventoryItem;    // to let it construct us
public:
    /**
     * Loads CargoContainer from DB.
     *
     * @param[in] containerID ID of container to load.
     * @return Pointer to CargoContainer object; NULL if failed.
     */
    static CargoContainerRef Load(uint32 containerID);
    /**
     * Spawns new CargoContainer.
     *
     * @param[in] data Item data for CargoContainer.
     * @return Pointer to new CargoContainer object; NULL if failed.
     */
    static CargoContainerRef Spawn(ItemData &data);

    /*
     * Primary public interface:
     */
    void Delete();

    double GetCapacity(EVEItemFlags flag) const;
    /*
     * _ExecAdd validation interface:
     */
    virtual bool ValidateAddItem(EVEItemFlags flag, InventoryItemRef item) const;

    /*
     * Public fields:
     */

protected:
    CargoContainer(
        uint32 _containerID,
        // InventoryItem stuff:
        const InvTypeRef _containerType,
        const ItemData &_data
    );

    /*
     * Member functions
     */
    // Template loader:
    template<class _Ty>
    static RefPtr<_Ty> _LoadItem(uint32 containerID,
        // InventoryItem stuff:
        const InvTypeRef type, const ItemData &data)
    {
        // check if it's a cargo container
        if ((type->groupID != EVEDB::invGroups::Cargo_Container)
            && (type->groupID != EVEDB::invGroups::Audit_Log_Secure_Container)
            && (type->groupID != EVEDB::invGroups::Freight_Container)
            && (type->groupID != EVEDB::invGroups::Secure_Cargo_Container)
            && (type->groupID != EVEDB::invGroups::Spawn_Container)
            && (type->groupID != EVEDB::invGroups::Wreck))
        {
            _log(ITEM__ERROR, "Trying to load category=%s, group=%s as CargoContainer.", type->getCategory()->categoryName.c_str(), type->getGroup()->groupName.c_str());
            return RefPtr<_Ty>();
        }

        // we don't need any additional stuff
        return CargoContainerRef(new CargoContainer(containerID, type, data));
    }

    virtual bool loadState();

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

    void AddItem(InventoryItemRef item);
    void RemoveItem(InventoryItemRef item);
};


/**
 * DynamicSystemEntity which represents structure object in space
 */
class InventoryItem;
class DestinyManager;
class SystemManager;

class ContainerEntity
: public DynamicSystemEntity
{
public:
    ContainerEntity(
        CargoContainerRef container,
        SystemManager *system,
        const Vector3D &position);

    /*
     * Primary public interface:
     */
    CargoContainerRef GetContainerObject() { return _containerRef; }
    DestinyManager * GetDestiny() { return m_destiny; }
    SystemManager * GetSystem() { return m_system; }

    /*
     * Public fields:
     */

    inline double x() const { return(GetPosition().x); }
    inline double y() const { return(GetPosition().y); }
    inline double z() const { return(GetPosition().z); }

    //SystemEntity interface:
    virtual EntityClass GetClass() const { return(ecContainerEntity); }
    virtual bool IsContainerEntity() const { return true; }
    virtual ContainerEntity *CastToContainerEntity() { return(this); }
    virtual const ContainerEntity *CastToContainerEntity() const { return(this); }
    virtual void Process();
    virtual void EncodeDestiny( Buffer& into ) const;
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
    CargoContainerRef _containerRef;   // We don't own this

    /* Used to calculate the damages on NPCs
     * I don't know why, npc->Set_shieldCharge does not work
     * calling npc->shieldCharge() return the complete shield
     * So we get the values on creation and use then instead.
    */
    double m_shieldCharge;
    double m_armorDamage;
    double m_hullDamage;
};

#endif /* !__CONTAINER__H__INCL__ */


