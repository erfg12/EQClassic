/*  EQEMu:  Everquest Server Emulator
	Copyright (C) 2001-2003  EQEMu Development Team (http://eqemulator.net)

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; version 2 of the License.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY except by those people which sell it, which
	are required to give you total support for your newly bought product;
	without even the implied warranty of MERCHANTABILITY or FITNESS FOR
	A PARTICULAR PURPOSE.  See the GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#ifdef WIN32
	// VS6 doesn't like the length of STL generated names: disabling
	#pragma warning(disable:4786)
	// Quagmire: Dont know why the one in debug.h doesnt work, but it doesnt.
#endif
#include "logger.h"
/*#ifdef _CRTDBG_MAP_ALLOC
	#undef new
	#define new new(_NORMAL_BLOCK, __FILE__, __LINE__)
#endif
*/
#include <sstream>
#include <iostream>
#include <limits.h>
#include "Item.h"
#include "inventory.h"
#include "database.h"
//#include "misc.h"
#include "races.h"
#include "classes.h"
using namespace std;

sint32 NextItemInstSerialNumber = 1;

static inline sint32 GetNextItemInstSerialNumber() {

	// The Bazaar relies on each item a client has up for Trade having a unique 
	// identifier. This 'SerialNumber' is sent in Serialized item packets and
	// is used in Bazaar packets to identify the item a player is buying or inspecting.
	//
	// E.g. A trader may have 3 Five dose cloudy potions, each with a different number of remaining charges
	// up for sale with different prices.
	//
	// NextItemInstSerialNumber is the next one to hand out.
	//
	// It is very unlikely to reach 2,147,483,647. Maybe we should call abort(), rather than wrapping back to 1.
	if(NextItemInstSerialNumber >= INT_MAX)
		NextItemInstSerialNumber = 1;
	else
		NextItemInstSerialNumber++;

	return NextItemInstSerialNumber;
}

ItemInst::ItemInst(const Item_Struct* item, sint16 charges) {
	m_use_type = ItemUseNormal;
	m_item = item;
	m_charges = charges;
	m_price = 0;
	m_instnodrop = false;
	m_merchantslot = 0;
	if(m_item &&m_item->type == ItemClassCommon)
		m_color = m_item->common.color;
	else
		m_color = 0;
	m_merchantcount = 1;
	m_SerialNumber = GetNextItemInstSerialNumber();
}

ItemInst::ItemInst(uint32 item_id, sint16 charges) {
	m_use_type = ItemUseNormal;
	m_item = Database::Instance()->GetItem(item_id);
	m_charges = charges;
	m_price = 0;
	m_merchantslot = 0;
	m_instnodrop=false;
	if(m_item &&m_item->type == ItemClassCommon)
		m_color = m_item->common.color;
	else
		m_color = 0;
	m_merchantcount = 1;
	m_SerialNumber = GetNextItemInstSerialNumber();
}

ItemInstQueue::~ItemInstQueue() {
	iter_queue cur,end;
	cur = m_list.begin();
	end = m_list.end();
	for(; cur != end; cur++) {
		ItemInst *tmp = * cur;
		safe_delete(tmp);
	}
	m_list.clear();
}

Inventory::~Inventory() {
	map<sint16, ItemInst*>::iterator cur,end;
	
	
	cur = m_worn.begin();
	end = m_worn.end();
	for(; cur != end; cur++) {
		ItemInst *tmp = cur->second;
		safe_delete(tmp);
	}
	m_worn.clear();
	
	cur = m_inv.begin();
	end = m_inv.end();
	for(; cur != end; cur++) {
		ItemInst *tmp = cur->second;
		safe_delete(tmp);
	}
	m_inv.clear();
	
	cur = m_bank.begin();
	end = m_bank.end();
	for(; cur != end; cur++) {
		ItemInst *tmp = cur->second;
		safe_delete(tmp);
	}
	m_bank.clear();
	
	cur = m_shbank.begin();
	end = m_shbank.end();
	for(; cur != end; cur++) {
		ItemInst *tmp = cur->second;
		safe_delete(tmp);
	}
	m_shbank.clear();
	
	cur = m_trade.begin();
	end = m_trade.end();
	for(; cur != end; cur++) {
		ItemInst *tmp = cur->second;
		safe_delete(tmp);
	}
	m_trade.clear();
}

// Make a copy of an ItemInst object
ItemInst::ItemInst(const ItemInst& copy)
{
	m_use_type=copy.m_use_type;
	m_item=copy.m_item;
	m_charges=copy.m_charges;
	m_price=copy.m_price;
	m_color=copy.m_color;
	m_merchantslot=copy.m_merchantslot;
	m_currentslot=copy.m_currentslot;
	m_instnodrop=copy.m_instnodrop;
	m_merchantcount=copy.m_merchantcount;
	// Copy container contents
	iter_contents it;
	for (it=copy.m_contents.begin(); it!=copy.m_contents.end(); it++) {
		ItemInst* inst_old = it->second;
		ItemInst* inst_new = NULL;
		
		if (inst_old) {
			inst_new = inst_old->Clone();
		}
		
		if (inst_new != NULL) {
			m_contents[it->first] = inst_new;
		}
	}
	m_SerialNumber = copy.m_SerialNumber;
}

// Clean up container contents
ItemInst::~ItemInst()
{
	Clear();
}

// Clone a type of ItemInst object
// c++ doesn't allow a polymorphic copy constructor,
// so we have to resort to a polymorphic Clone()
ItemInst* ItemInst::Clone() const
{
	// Pseudo-polymorphic copy constructor
	return new ItemInst(*this);
}

// Query item type
bool ItemInst::IsType(ItemClass item_class) const
{
	// Check usage type
	if ((m_use_type == ItemUseWorldContainer) && (item_class == ItemClassContainer))
		
		return true;
	if (!m_item)
		return false;
	
	return (m_item->type == item_class);
}

// Is item stackable?
bool ItemInst::IsStackable() const
{
	if(IsType(ItemClassCommon)) {
		return m_item->common.stackable;
	}

	return false;
}

// Harakiri Is item consumable, used for removeonecharge?
bool ItemInst::IsConsumable() const
{
	if(IsType(ItemClassCommon)) {
		if((m_item->common.itemType == ItemTypeFood) ||
	   (m_item->common.itemType == ItemTypeDrink) ||
	   (m_item->common.itemType == ItemTypeStackable) ||
	   (m_item->common.itemType == ItemTypeBandage) ||
	   (m_item->common.itemType == ItemTypeThrowingv2) ||
	   (m_item->common.itemType == ItemTypeUnknownConsumable) ||
	   (m_item->common.itemType == ItemTypeFishingBait) ||
	   (m_item->common.itemType == ItemTypeAlcohol) ||	
	   (m_item->common.itemType == ItemTypeSpell) ||
	   (m_item->common.itemType == ItemTypePotion) ||	   
	   (m_item->common.itemType == ItemTypePoison) ||	
	   (m_item->common.itemType == ItemTypeArrow)) {
		   return true;
		}
	}

	return false;
}

// Can item be equipped?

bool ItemInst::IsEquipable(int16 race, int16 class_) const
{
	if (!m_item)
		return false;
	
	bool israce = false;
	bool isclass = false;
	
	if (m_item->equipableSlots == 0) {
		return false;
	}
	
	uint32 classes_ = m_item->common.classes;
	uint32 races_ = m_item->common.normal.races;

	// Harakiri FIXME?
	if(race == IKSAR) {
		// Array_Race_IKSAR
		race = 13;
	}
	int32 race_ = race;

	for (int cur_class = 1; cur_class<=PLAYER_CLASS_COUNT; cur_class++) {
		if (classes_ % 2 == 1) {
    		if (cur_class == class_) {
    			isclass = true;
				break;
			}
		}
		classes_ >>= 1;
	}

	race_ = (race_==18? 16 : race_);
	// @merth: can this be optimized?  i.e., will (race & common->Races) suffice?

	for (unsigned int cur_race = 1; cur_race <= PLAYER_RACE_COUNT; cur_race++) {
		
		if (races_ % 2 == 1) {
    		if (cur_race == race_) {
    			israce = true;
				break;
   			}
		}
		races_ >>= 1;
	}
	return (israce && isclass);
}

// Can equip at this slot?
bool ItemInst::IsEquipable(sint16 slot_id) const
{
	if (!m_item)
		return false;
	
	if(slot_id == 9999) {
		slot_id = 22;
		uint32 slot_mask = (1 << slot_id);
		if (slot_mask & m_item->equipableSlots)
			return true;
	}
	
	if (slot_id < 22) {
		uint32 slot_mask = (1 << slot_id);
		if (slot_mask & m_item->equipableSlots)
			return true;
	}

	return false;
}


uint32 ItemInst::GetItemID(uint8 slot) const
{
const ItemInst *item;
uint32 id=0;
	if ((item=GetItem(slot))!=NULL)
		id= item->GetItem()->item_nr;

	return id;
}


// Has attack/delay?
bool ItemInst::IsWeapon() const
{
	if (!m_item || m_item->type != ItemClassCommon)
		return false;
	if(m_item->common.itemType==ItemTypeArrow && m_item->common.damage != 0)
		return true;
	else
		return ((m_item->common.damage != 0) && (m_item->common.delay != 0));
}

bool ItemInst::IsAmmo() const {

	if(!m_item) return false;

	if((m_item->common.itemType == ItemTypeArrow) ||
	   (m_item->common.itemType == ItemTypeThrowing) ||
	   (m_item->common.itemType == ItemTypeThrowingv2))
	   	return true;

	return false;
	
}

// Retrieve item inside container
ItemInst* ItemInst::GetItem(uint8 index) const
{
	iter_contents it = m_contents.find(index);
	if (it != m_contents.end()) {
		ItemInst* inst = it->second;
		return inst;
	}
	
	return NULL;
}

void ItemInst::PutItem(uint8 index, const ItemInst& inst)
{
	// Clean up item already in slot (if exists)
	DeleteItem(index);
	
	
	// Delegate to internal method
	_PutItem(index, inst.Clone());
}

// Remove item inside container
void ItemInst::DeleteItem(uint8 index)
{
	ItemInst* inst = PopItem(index);
	safe_delete(inst);
}

// Remove all items from container
void ItemInst::Clear()
{
	// Destroy container contents
	iter_contents cur, end;
	cur = m_contents.begin();
	end = m_contents.end();
	for (; cur != end; cur++) {
		ItemInst* inst = cur->second;
		safe_delete(inst);
	}
	m_contents.clear();
}

// Remove all items from container
void ItemInst::ClearByFlags(byFlagSetting is_nodrop, byFlagSetting is_norent)
{
	// Destroy container contents
	iter_contents cur, end, del;
	cur = m_contents.begin();
	end = m_contents.end();
	for (; cur != end;) {
		ItemInst* inst = cur->second;
		const Item_Struct* item = inst->GetItem();
		del = cur;
		cur++;
		
		switch(is_nodrop) {
		case byFlagSet:
			if (item->nodrop == 0) {
				safe_delete(inst);
				m_contents.erase(del->first);
				continue;
			}
		case byFlagNotSet:
			if (item->nodrop != 0) {
				safe_delete(inst);
				m_contents.erase(del->first);
				continue;
			}
		default:
			break;
		}
		
		switch(is_norent) {
		case byFlagSet:
			if (item->norent == 0) {
				safe_delete(inst);
				m_contents.erase(del->first);
				continue;
			}
		case byFlagNotSet:
			if (item->norent != 0) {
				safe_delete(inst);
				m_contents.erase(del->first);
				continue;
			}
		default:
			break;
		}
	}
}

// Remove item from container without memory delete
// Hands over memory ownership to client of this function call
ItemInst* ItemInst::PopItem(uint8 index)
{
	iter_contents it = m_contents.find(index);
	if (it != m_contents.end()) {
		ItemInst* inst = it->second;
		m_contents.erase(index);
		return inst;
	}
	
	// Return pointer that needs to be deleted (or otherwise managed)
	return NULL;
}

// Put item onto back of queue
void ItemInstQueue::push(ItemInst* inst)
{
	m_list.push_back(inst);
}

// Put item onto front of queue
void ItemInstQueue::push_front(ItemInst* inst)
{
	m_list.push_front(inst);
}

// Remove item from front of queue
ItemInst* ItemInstQueue::pop()
{
	if (m_list.size() == 0)
		return NULL;
	
	ItemInst* inst = m_list.front();
	m_list.pop_front();
	return inst;
}

// Look at item at front of queue
ItemInst* ItemInstQueue::peek_front() const
{
	return (m_list.size()==0) ? NULL : m_list.front();
}

// Retrieve item at specified slot; returns false if item not found
ItemInst* Inventory::GetItem(sint16 slot_id) const
{
	_CP(Inventory_GetItem);
	ItemInst* result = NULL;
	
	// Cursor
	if (slot_id == SLOT_CURSOR) {
		// Cursor slot
		result = m_cursor.peek_front();
	}
	
	// Non bag slots
	else if (slot_id>=3000 && slot_id<=3007) {
		// Trade slots
		result = _GetItem(m_trade, slot_id);
	}
	else if (slot_id>=2500 && slot_id<=2501) {
		// Shared Bank slots
		result = _GetItem(m_shbank, slot_id);
	}
	else if (slot_id>=2000 && slot_id<=2023) {
		// Bank slots
		result = _GetItem(m_bank, slot_id);
	}
	else if ((slot_id>=22 && slot_id<=29)) {
		// Personal inventory slots
		result = _GetItem(m_inv, slot_id);
	}
	else if ((slot_id>=0 && slot_id<=21) || (slot_id >= 400 && slot_id<=404) || (slot_id == 9999)) {
		// Equippable slots (on body)
		result = _GetItem(m_worn, slot_id);
	}
	
	// Inner bag slots
	else if (slot_id>=3031 && slot_id<=3110) {
		// Trade bag slots
		ItemInst* inst = _GetItem(m_trade, Inventory::CalcSlotId(slot_id));
		if (inst && inst->IsType(ItemClassContainer)) {
			result = inst->GetItem(Inventory::CalcBagIdx(slot_id));
		}
	}
	else if (slot_id>=2531 && slot_id<=2550) {
		// Shared Bank bag slots
		ItemInst* inst = _GetItem(m_shbank, Inventory::CalcSlotId(slot_id));
		if (inst && inst->IsType(ItemClassContainer)) {
			result = inst->GetItem(Inventory::CalcBagIdx(slot_id));
		}
	}
	else if (slot_id>=2031 && slot_id<=2270) {
		// Bank bag slots
		ItemInst* inst = _GetItem(m_bank, Inventory::CalcSlotId(slot_id));
		if (inst && inst->IsType(ItemClassContainer)) {
			result = inst->GetItem(Inventory::CalcBagIdx(slot_id));
		}
	}
	else if (slot_id>=331 && slot_id<=340) {
		// Cursor bag slots
		ItemInst* inst = m_cursor.peek_front();
		if (inst && inst->IsType(ItemClassContainer)) {
			result = inst->GetItem(Inventory::CalcBagIdx(slot_id));
		}
	}
	else if (slot_id>=251 && slot_id<=330) {
		// Personal inventory bag slots
		ItemInst* inst = _GetItem(m_inv, Inventory::CalcSlotId(slot_id));
		if (inst && inst->IsType(ItemClassContainer)) {
			result = inst->GetItem(Inventory::CalcBagIdx(slot_id));
		}
	}
	
	return result;
}

// Retrieve item at specified position within bag
ItemInst* Inventory::GetItem(sint16 slot_id, uint8 bagidx) const
{
	return GetItem(Inventory::CalcSlotId(slot_id, bagidx));
}

sint16 Inventory::PushCursor(const ItemInst& inst)
{
	m_cursor.push(inst.Clone());
	return SLOT_CURSOR;
}

// Put an item snto specified slot
sint16 Inventory::PutItem(sint16 slot_id, const ItemInst& inst)
{
	// Clean up item already in slot (if exists)
	DeleteItem(slot_id);
	
	if (!inst) {
		// User is effectively deleting the item
		// in the slot, why hold a null ptr in map<>?
		return slot_id;
	}
	
	// Delegate to internal method
	return _PutItem(slot_id, inst.Clone());
}

// Swap items in inventory
void Inventory::SwapItem(sint16 slot_a, sint16 slot_b)
{
	// Temp holding area for a

	ItemInst* inst_a = GetItem(slot_a);
	if(!inst_a->IsSlotAllowed(slot_b))
		return;
	// Copy b->a
	_PutItem(slot_a, GetItem(slot_b));
	
	// Copy a->b
	_PutItem(slot_b, inst_a);
}

// Checks that user has at least 'quantity' number of items in a given inventory slot
// Returns first slot it was found in, or SLOT_INVALID if not found

//This function has a flaw in that it only returns the last stack that it looked at
//when quantity is greater than 1 and not all of quantity can be found in 1 stack.

sint16 Inventory::HasItem(uint32 item_id, uint8 quantity, uint8 where)
{
	_CP(Inventory_HasItem);
	sint16 slot_id = SLOT_INVALID;
	
	//Altered by Father Nitwit to support a specification of
	//where to search, with a default value to maintain compatibility
	
	// Check each inventory bucket
	if(where & invWhereWorn) {
		slot_id = _HasItem(m_worn, item_id, quantity);
		if (slot_id != SLOT_INVALID)
			return slot_id;
	}
	
	if(where & invWherePersonal) {
		slot_id = _HasItem(m_inv, item_id, quantity);
		if (slot_id != SLOT_INVALID)
			return slot_id;
	}
		
	if(where & invWhereBank) {
		slot_id = _HasItem(m_bank, item_id, quantity);
		if (slot_id != SLOT_INVALID)
			return slot_id;
	}
		
	if(where & invWhereSharedBank) {
		slot_id = _HasItem(m_shbank, item_id, quantity);
		if (slot_id != SLOT_INVALID)
			return slot_id;
	}
		
	if(where & invWhereTrading) {
		slot_id = _HasItem(m_trade, item_id, quantity);
		if (slot_id != SLOT_INVALID)
			return slot_id;
	}
		
	if(where & invWhereCursor) {
		// Check cursor queue
		slot_id = _HasItem(m_cursor, item_id, quantity);
		if (slot_id != SLOT_INVALID)
			return slot_id;
	}
	
	return slot_id;
}

//this function has the same quantity flaw mentioned above in HasItem()

sint16 Inventory::HasItemByUse(uint8 use, uint8 quantity, uint8 where)
{
	sint16 slot_id = SLOT_INVALID;
	
	// Check each inventory bucket
	if(where & invWhereWorn) {
		slot_id = _HasItemByUse(m_worn, use, quantity);
		if (slot_id != SLOT_INVALID)
			return slot_id;
	}
	
	if(where & invWherePersonal) {
		slot_id = _HasItemByUse(m_inv, use, quantity);
		if (slot_id != SLOT_INVALID)
			return slot_id;
	}
		
	if(where & invWhereBank) {
		slot_id = _HasItemByUse(m_bank, use, quantity);
		if (slot_id != SLOT_INVALID)
			return slot_id;
	}
		
	if(where & invWhereSharedBank) {
		slot_id = _HasItemByUse(m_shbank, use, quantity);
		if (slot_id != SLOT_INVALID)
			return slot_id;
	}
		
	if(where & invWhereTrading) {
		slot_id = _HasItemByUse(m_trade, use, quantity);
		if (slot_id != SLOT_INVALID)
			return slot_id;
	}
		
	if(where & invWhereCursor) {
		// Check cursor queue
		slot_id = _HasItemByUse(m_cursor, use, quantity);
		if (slot_id != SLOT_INVALID)
			return slot_id;
	}
	
	return slot_id;
}


bool Inventory::HasSpaceForItem(const Item_Struct *ItemToTry, uint8 Quantity) {

	if(ItemToTry->common.stackable) {

		for(sint16 i = 22; i <= 29; i++) {
	
			ItemInst* InvItem = GetItem(i);

			if(InvItem && (InvItem->GetItem()->item_nr == ItemToTry->item_nr) && (InvItem->GetCharges() < StackSize)) {

				int ChargeSlotsLeft = StackSize - InvItem->GetCharges();
		
				if(Quantity <= ChargeSlotsLeft)
					return true;

				Quantity -= ChargeSlotsLeft;

			}
			if (InvItem && InvItem->IsType(ItemClassContainer)) {

				sint16 BaseSlotID = Inventory::CalcSlotId(i, 0);
				int8 BagSize=InvItem->GetItem()->common.container.BagSlots;
				for (uint8 BagSlot = 0; BagSlot < BagSize; BagSlot++) {

					InvItem = GetItem(BaseSlotID + BagSlot);

					if(InvItem && (InvItem->GetItem()->item_nr == ItemToTry->item_nr) &&
					   (InvItem->GetCharges() < StackSize)) {

						int ChargeSlotsLeft = StackSize - InvItem->GetCharges();
			
						if(Quantity <= ChargeSlotsLeft)
							return true;

						Quantity -= ChargeSlotsLeft;
					}
				}
			}
		}
	}

	for (sint16 i = 22; i <= 29; i++) {

		ItemInst* InvItem = GetItem(i);

		if (!InvItem) {

			if(!ItemToTry->common.stackable) {

				if(Quantity == 1)
					return true;
				else
					Quantity--;
			}
			else {
				if(Quantity <= StackSize)
					return true;
				else
					Quantity -= StackSize;
			}

		}
		else if(InvItem->IsType(ItemClassContainer) && CanItemFitInContainer(ItemToTry, InvItem->GetItem())) {

			sint16 BaseSlotID = Inventory::CalcSlotId(i, 0);

			int8 BagSize=InvItem->GetItem()->common.container.BagSlots;

			for (uint8 BagSlot=0; BagSlot<BagSize; BagSlot++) {

				InvItem = GetItem(BaseSlotID + BagSlot);

				if(!InvItem) {
					if(!ItemToTry->common.stackable) {

						if(Quantity == 1)
							return true;
						else
							Quantity--;
					}
					else {
						if(Quantity <= StackSize)
							return true;
						else
							Quantity -= StackSize;
					}
				}
			}
		}
	}

	return false;

}

// Remove item from inventory (with memory delete)
bool Inventory::DeleteItem(sint16 slot_id, uint8 quantity)
{
	// Pop item out of inventory map (or queue)
	ItemInst* item_to_delete = PopItem(slot_id);
	
	// Determine if object should be fully deleted, or
	// just a quantity of charges of the item can be deleted
	if (item_to_delete && (quantity > 0)) {

		item_to_delete->SetCharges(item_to_delete->GetCharges() - quantity);

		// If there are no charges left on the item,
		if(item_to_delete->GetCharges() <= 0) {
			// If the item is stackable (e.g arrows), or
			//    the item is not stackable, and is not a charged item, or is expendable, delete it
			if(item_to_delete->IsStackable() ||
			   (!item_to_delete->IsStackable() && 
			    ((item_to_delete->GetItem()->common.charges == 0)  || (item_to_delete->GetItem()->common.effecttype == ET_Expendable || item_to_delete->GetItem()->type == ItemTypePotion)))) {
				// Item can now be destroyed
				safe_delete(item_to_delete);
				return true;
			}
		}

		// Charges still exist, or it is a charged item that is not expendable.  Put back into inventory
		_PutItem(slot_id, item_to_delete);
		return false;
	}
	
	safe_delete(item_to_delete);

	return true;
	
}

// Checks All items in a bag for No Drop
bool Inventory::CheckNoDrop(sint16 slot_id) {
    ItemInst* inst = GetItem(slot_id);
	if (!inst) return false;
	if (!inst->GetItem()->nodrop) return true;
	if (inst->GetItem()->type == ItemClassContainer) {
		for (int16 i=0; i<10; i++) {
			ItemInst* bagitem = GetItem(Inventory::CalcSlotId(slot_id, i));
			if (bagitem && !bagitem->GetItem()->nodrop) return true;
		}
	}
	return false;
}

// Remove item from bucket without memory delete
// Returns item pointer if full delete was successful
ItemInst* Inventory::PopItem(sint16 slot_id)
{
	ItemInst* p = NULL;
	
	if (slot_id==SLOT_CURSOR) { // Cursor
		p = m_cursor.pop();
	}
	else if ((slot_id>=0 && slot_id<=21) || (slot_id >= 400 && slot_id<=404) || (slot_id == 9999)) { // Worn slots
		p = m_worn[slot_id];
		m_worn.erase(slot_id);
	}
	else if ((slot_id>=22 && slot_id<=29)) {
		p = m_inv[slot_id];
		m_inv.erase(slot_id);
	}
	else if (slot_id>=2000 && slot_id<=2023) { // Bank slots
		p = m_bank[slot_id];
		m_bank.erase(slot_id);
	}
	else if (slot_id>=2500 && slot_id<=2501) { // Shared bank slots
		p = m_shbank[slot_id];
		m_shbank.erase(slot_id);
	}
	else if (slot_id>=3000 && slot_id<=3007) { // Trade window slots
		p = m_trade[slot_id];
		m_trade.erase(slot_id);
	}
	else {
		// Is slot inside bag?
		ItemInst* baginst = GetItem(Inventory::CalcSlotId(slot_id));
		if (baginst != NULL && baginst->IsType(ItemClassContainer)) {
			p = baginst->PopItem(Inventory::CalcBagIdx(slot_id));
		}
	}
	
	// Return pointer that needs to be deleted (or otherwise managed)
	return p;
}

// Locate an available inventory slot
// Returns slot_id when there's one available, else SLOT_INVALID
sint16 Inventory::FindFreeSlot(bool for_bag, bool try_cursor, int8 min_size)
{
	// Check basic inventory
	for (sint16 i=22; i<=29; i++) {
		if (!GetItem(i))
			// Found available slot in personal inventory
			return i;
	}
	
	if (!for_bag) {
		for (sint16 i=22; i<=29; i++) {
			const ItemInst* inst = GetItem(i);
			if (inst && inst->IsType(ItemClassContainer) 
				&& inst->GetItem()->common.container.BagSize >= min_size
				) {
				sint16 base_slot_id = Inventory::CalcSlotId(i, 0);

				int8 slots=inst->GetItem()->common.container.BagSlots;
				uint8 j;
				for (j=0; j<slots; j++) {
					if (!GetItem(base_slot_id + j))
						// Found available slot within bag
						return (base_slot_id + j);
				}
			}
		}
	}
	
	if (try_cursor)
		// Always room on cursor (it's a queue)
		// (we may wish to cap this in the future)
		return SLOT_CURSOR;
	
	// No available slots
	return SLOT_INVALID;
}

void Inventory::dumpInventory() {
	iter_inst it;
	iter_contents itb;
	ItemInst* inst = NULL;
	
	// Check item: After failed checks, check bag contents (if bag)
	printf("Worn items:\n");
	for (it=m_worn.begin(); it!=m_worn.end(); it++) {
		inst = it->second;
		it->first;
		if(!inst || !inst->GetItem())
			continue;
		
		printf("Slot %d: %s (%d)\n", it->first, it->second->GetItem()->name, (inst->GetCharges()<=0) ? 1 : inst->GetCharges());
		
		// Go through bag, if bag
		if (inst && inst->IsType(ItemClassContainer)) {
			for (itb=inst->_begin(); itb!=inst->_end(); itb++) {
				ItemInst* baginst = itb->second;
				if(!baginst || !baginst->GetItem())
					continue;
				printf("	Slot %d: %s (%d)\n", Inventory::CalcSlotId(it->first, itb->first),
						baginst->GetItem()->name, (baginst->GetCharges()<=0) ? 1 : baginst->GetCharges());
			}
		}
	}
	
	printf("Inventory items:\n");
	for (it=m_inv.begin(); it!=m_inv.end(); it++) {
		inst = it->second;
		it->first;
		if(!inst || !inst->GetItem())
			continue;
		
		printf("Slot %d: %s (%d)\n", it->first, it->second->GetItem()->name, (inst->GetCharges()<=0) ? 1 : inst->GetCharges());
		
		// Go through bag, if bag
		if (inst && inst->IsType(ItemClassContainer)) {
			for (itb=inst->_begin(); itb!=inst->_end(); itb++) {
				ItemInst* baginst = itb->second;
				if(!baginst || !baginst->GetItem())
					continue;
				printf("	Slot %d: %s (%d)\n", Inventory::CalcSlotId(it->first, itb->first),
							baginst->GetItem()->name, (baginst->GetCharges()<=0) ? 1 : baginst->GetCharges());
				
			}
		}
	}
	
	printf("Bank items:\n");
	for (it=m_bank.begin(); it!=m_bank.end(); it++) {
		inst = it->second;
		it->first;
		if(!inst || !inst->GetItem())
			continue;
		
		printf("Slot %d: %s (%d)\n", it->first, it->second->GetItem()->name, (inst->GetCharges()<=0) ? 1 : inst->GetCharges());
		
		// Go through bag, if bag
		if (inst && inst->IsType(ItemClassContainer)) {
			
			for (itb=inst->_begin(); itb!=inst->_end(); itb++) {
				ItemInst* baginst = itb->second;
				if(!baginst || !baginst->GetItem())
					continue;
				printf("	Slot %d: %s (%d)\n", Inventory::CalcSlotId(it->first, itb->first),
						baginst->GetItem()->name, (baginst->GetCharges()<=0) ? 1 : baginst->GetCharges());
				
			}
		}
	}
	
	printf("Shared Bank items:\n");
	for (it=m_shbank.begin(); it!=m_shbank.end(); it++) {
		inst = it->second;
		it->first;
		if(!inst || !inst->GetItem())
			continue;
		
		printf("Slot %d: %s (%d)\n", it->first, it->second->GetItem()->name, (inst->GetCharges()<=0) ? 1 : inst->GetCharges());
		
		// Go through bag, if bag
		if (inst && inst->IsType(ItemClassContainer)) {
			
			for (itb=inst->_begin(); itb!=inst->_end(); itb++) {
				ItemInst* baginst = itb->second;
				if(!baginst || !baginst->GetItem())
					continue;
				printf("	Slot %d: %s (%d)\n", Inventory::CalcSlotId(it->first, itb->first),
						baginst->GetItem()->name, (baginst->GetCharges()<=0) ? 1 : baginst->GetCharges());
				
			}
		}
	}
	
	printf("\n");
	fflush(stdout);
}

// Internal Method: Retrieves item within an inventory bucket
ItemInst* Inventory::_GetItem(const map<sint16, ItemInst*>& bucket, sint16 slot_id) const
{
	iter_inst it = bucket.find(slot_id);
	if (it != bucket.end()) {
		return it->second;
	}
	
	// Not found!
	return NULL;
}

// Internal Method: "put" item into bucket, without regard for what is currently in bucket
// Assumes item has already been allocated
sint16 Inventory::_PutItem(sint16 slot_id, ItemInst* inst)
{
	// If putting a NULL into slot, we need to remove slot without memory delete
	if (inst == NULL) {
		//Why do we not delete the poped item here????
		PopItem(slot_id);
		return slot_id;
	}
	
	sint16 result = SLOT_INVALID;
	
	if (slot_id==SLOT_CURSOR) { // Cursor
		// Replace current item on cursor, if exists
		m_cursor.pop(); // no memory delete, clients of this function know what they are doing
		m_cursor.push_front(inst);
		result = slot_id;
	}
	else if ((slot_id>=0 && slot_id<=21) || (slot_id >= 400 && slot_id<=404) || (slot_id == 9999)) { // Worn slots
		m_worn[slot_id] = inst;
		result = slot_id;
	}
	else if ((slot_id>=22 && slot_id<=29)) {
		m_inv[slot_id] = inst;
		result = slot_id;
	}
	else if (slot_id>=2000 && slot_id<=2023) { // Bank slots
		m_bank[slot_id] = inst;
		result = slot_id;
	}
	else if (slot_id>=2500 && slot_id<=2501) { // Shared bank slots
		m_shbank[slot_id] = inst;
		result = slot_id;
	}
	else if (slot_id>=3000 && slot_id<=3007) { // Trade window slots
		m_trade[slot_id] = inst;
		result = slot_id;
	}
	else {
		// Slot must be within a bag
		ItemInst* baginst = GetItem(Inventory::CalcSlotId(slot_id)); // Get parent bag
		if (baginst && baginst->IsType(ItemClassContainer)) {
			baginst->_PutItem(Inventory::CalcBagIdx(slot_id), inst);
			result = slot_id;
		}
	}
	
	if (result == SLOT_INVALID) {
		EQC::Common::Log(EQCLog::Error,CP_CLIENT, "Inventory::_PutItem: Invalid slot_id specified (%i)", slot_id);
		safe_delete(inst); // Slot not found, clean up
	}
	
	return result;
}

// Internal Method: Checks an inventory bucket for a particular item
sint16 Inventory::_HasItem(map<sint16, ItemInst*>& bucket, uint32 item_id, uint8 quantity)
{
	iter_inst it;
	iter_contents itb;
	ItemInst* inst = NULL;
	uint8 quantity_found = 0;
	
	// Check item: After failed checks, check bag contents (if bag)
	for (it=bucket.begin(); it!=bucket.end(); it++) {
		inst = it->second;
		if (inst) {
			if (inst->GetID() == item_id) {
				quantity_found += (inst->GetCharges()<=0) ? 1 : inst->GetCharges();
				if (quantity_found >= quantity)
					return it->first;
			}
		 		
		}
		// Go through bag, if bag
		if (inst && inst->IsType(ItemClassContainer)) {
			
			for (itb=inst->_begin(); itb!=inst->_end(); itb++) {
				ItemInst* baginst = itb->second;
				if (baginst->GetID() == item_id) {
					quantity_found += (baginst->GetCharges()<=0) ? 1 : baginst->GetCharges();
					if (quantity_found >= quantity)
						return Inventory::CalcSlotId(it->first, itb->first);
				}			
			}
		}
	}
	
	// Not found
	return SLOT_INVALID;
}

// Internal Method: Checks an inventory queue type bucket for a particular item
sint16 Inventory::_HasItem(ItemInstQueue& iqueue, uint32 item_id, uint8 quantity)
{
	iter_queue it;
	iter_contents itb;
	uint8 quantity_found = 0;
	
	// Read-only iteration of queue
	for (it=iqueue.begin(); it!=iqueue.end(); it++) {
		ItemInst* inst = *it;
		if (inst)
		{
			if (inst->GetID() == item_id) {
				quantity_found += (inst->GetCharges()<=0) ? 1 : inst->GetCharges();
				if (quantity_found >= quantity)
					return SLOT_CURSOR;
			}		
		}
		// Go through bag, if bag
		if (inst && inst->IsType(ItemClassContainer)) {
			
			for (itb=inst->_begin(); itb!=inst->_end(); itb++) {
				ItemInst* baginst = itb->second;
				if (baginst->GetID() == item_id) {
					quantity_found += (baginst->GetCharges()<=0) ? 1 : baginst->GetCharges();
					if (quantity_found >= quantity)
						return Inventory::CalcSlotId(SLOT_CURSOR, itb->first);
				}			
			}
		}
	}
	
	// Not found
	return SLOT_INVALID;
}

// Internal Method: Checks an inventory bucket for a particular item
sint16 Inventory::_HasItemByUse(map<sint16, ItemInst*>& bucket, uint8 use, uint8 quantity)
{
	iter_inst it;
	iter_contents itb;
	ItemInst* inst = NULL;
	uint8 quantity_found = 0;
	
	// Check item: After failed checks, check bag contents (if bag)
	for (it=bucket.begin(); it!=bucket.end(); it++) {
		inst = it->second;
		if (inst && inst->IsType(ItemClassCommon) && inst->GetItem()->type == use) {
			quantity_found += (inst->GetCharges()<=0) ? 1 : inst->GetCharges();
			if (quantity_found >= quantity)
				return it->first;
		}
		
		// Go through bag, if bag
		if (inst && inst->IsType(ItemClassContainer)) {
			
			for (itb=inst->_begin(); itb!=inst->_end(); itb++) {
				ItemInst* baginst = itb->second;
				if (baginst && baginst->IsType(ItemClassCommon) && baginst->GetItem()->type == use) {
					quantity_found += (baginst->GetCharges()<=0) ? 1 : baginst->GetCharges();
					if (quantity_found >= quantity)
						return Inventory::CalcSlotId(it->first, itb->first);
				}
			}
		}
	}
	
	// Not found
	return SLOT_INVALID;
}

// Internal Method: Checks an inventory queue type bucket for a particular item
sint16 Inventory::_HasItemByUse(ItemInstQueue& iqueue, uint8 use, uint8 quantity)
{
	iter_queue it;
	iter_contents itb;
	uint8 quantity_found = 0;
	
	// Read-only iteration of queue
	for (it=iqueue.begin(); it!=iqueue.end(); it++) {
		ItemInst* inst = *it;
		if (inst && inst->IsType(ItemClassCommon) && inst->GetItem()->type == use) {
			quantity_found += (inst->GetCharges()<=0) ? 1 : inst->GetCharges();
			if (quantity_found >= quantity)
				return SLOT_CURSOR;
		}
		
		// Go through bag, if bag
		if (inst && inst->IsType(ItemClassContainer)) {
			
			for (itb=inst->_begin(); itb!=inst->_end(); itb++) {
				ItemInst* baginst = itb->second;
				if (baginst && baginst->IsType(ItemClassCommon) && baginst->GetItem()->type == use) {
					quantity_found += (baginst->GetCharges()<=0) ? 1 : baginst->GetCharges();
					if (quantity_found >= quantity)
						return Inventory::CalcSlotId(SLOT_CURSOR, itb->first);
				}
			}
		}
	}
	
	// Not found
	return SLOT_INVALID;
}



bool ItemInst::IsSlotAllowed(sint16 slot_id) const
{
	if(slot_id == 9999)
		slot_id = 22;
	if(!m_item)
		return false;
	else if(Inventory::SupportsContainers(slot_id))
		return true;
	else if(m_item->equipSlot & (1 << slot_id))
		return true;
	else if(slot_id > 21)
		return true;
	else
		return false;
}

uint8 ItemInst::FirstOpenSlot() const
{
	int8 slots=m_item->common.container.BagSlots ,i;
	for(i=0;i<slots;i++) {
		if (!GetItem(i))
			break;
	}

	return (i<slots) ? i : 0xff;
}

// Calculate slot_id for an item within a bag
sint16 Inventory::CalcSlotId(sint16 bagslot_id, uint8 bagidx)
{
	if (!Inventory::SupportsContainers(bagslot_id)) {
		return SLOT_INVALID;
	}
	
	sint16 slot_id = SLOT_INVALID;
	
	if (bagslot_id==SLOT_CURSOR || bagslot_id==8000) // Cursor
		slot_id = IDX_CURSOR_BAG + bagidx;
	else if (bagslot_id>=22 && bagslot_id<=29) // Inventory slots
		slot_id = IDX_INV_BAG + (bagslot_id-22)*MAX_ITEMS_PER_BAG + bagidx;
	else if (bagslot_id>=2000 && bagslot_id<=2023) // Bank slots
		slot_id = IDX_BANK_BAG + (bagslot_id-2000)*MAX_ITEMS_PER_BAG + bagidx;
	else if (bagslot_id>=2500 && bagslot_id<=2501) // Shared bank slots
		slot_id = IDX_SHBANK_BAG + (bagslot_id-2500)*MAX_ITEMS_PER_BAG + bagidx;
	else if (bagslot_id>=3000 && bagslot_id<=3007) // Trade window slots
		slot_id = IDX_TRADE_BAG + (bagslot_id-3000)*MAX_ITEMS_PER_BAG + bagidx;
	
	return slot_id;
}

// Opposite of above: Get parent bag slot_id from a slot inside of bag
sint16 Inventory::CalcSlotId(sint16 slot_id)
{
	sint16 parent_slot_id = SLOT_INVALID;
	
	if (slot_id>=251 && slot_id<=330)
		parent_slot_id = IDX_INV + (slot_id-251) / MAX_ITEMS_PER_BAG;
	else if (slot_id>=331 && slot_id<=340)
		parent_slot_id = SLOT_CURSOR;
	else if (slot_id>=2000 && slot_id<=2023)
		parent_slot_id = IDX_BANK + (slot_id-2000) / MAX_ITEMS_PER_BAG;
	else if (slot_id>=2031 && slot_id<=2270)
		parent_slot_id = IDX_BANK + (slot_id-2031) / MAX_ITEMS_PER_BAG;
	else if (slot_id>=2531 && slot_id<=2550)
		parent_slot_id = IDX_SHBANK + (slot_id-2531) / MAX_ITEMS_PER_BAG;
	else if (slot_id>=3100 && slot_id<=3179)
		parent_slot_id = IDX_TRADE + (slot_id-3100) / MAX_ITEMS_PER_BAG;
	
	return parent_slot_id;
}

uint8 Inventory::CalcBagIdx(sint16 slot_id)
{
	uint8 index = 0;
	
	if (slot_id>=251 && slot_id<=330)
		index = (slot_id-251) % MAX_ITEMS_PER_BAG;
	else if (slot_id>=331 && slot_id<=340)
		index = (slot_id-331) % MAX_ITEMS_PER_BAG;
	else if (slot_id>=2000 && slot_id<=2023)
		index = (slot_id-2000) % MAX_ITEMS_PER_BAG;
	else if (slot_id>=2031 && slot_id<=2270)
		index = (slot_id-2031) % MAX_ITEMS_PER_BAG;
	else if (slot_id>=2531 && slot_id<=2550)
		index = (slot_id-2531) % MAX_ITEMS_PER_BAG;
	else if (slot_id>=3100 && slot_id<=3179)
		index = (slot_id-3100) % MAX_ITEMS_PER_BAG;
	else if (slot_id>=4000 && slot_id<=4009)
		index = (slot_id-4000) % MAX_ITEMS_PER_BAG;
	
	return index;
}

sint16 Inventory::CalcSlotFromMaterial(int8 material)
{
	switch(material)
	{
		case MATERIAL_HEAD:
			return SLOT_HEAD;
		case MATERIAL_CHEST:
			return SLOT_CHEST;
		case MATERIAL_ARMS:
			return SLOT_ARMS;
		case MATERIAL_BRACER:
			return SLOT_BRACER01;	// there's 2 bracers, only one bracer material
		case MATERIAL_HANDS:
			return SLOT_HANDS;
		case MATERIAL_LEGS:
			return SLOT_LEGS;
		case MATERIAL_FEET:
			return SLOT_FEET;
		case MATERIAL_PRIMARY:
			return SLOT_PRIMARY;
		case MATERIAL_SECONDARY:
			return SLOT_SECONDARY;
		default:
			return -1;
	}
}

int8 Inventory::CalcMaterialFromSlot(sint16 equipslot)
{
	switch(equipslot)
	{
		case SLOT_HEAD:
			return MATERIAL_HEAD;
		case SLOT_CHEST:
			return MATERIAL_CHEST;
		case SLOT_ARMS:
			return MATERIAL_ARMS;
		case SLOT_BRACER01:
		case SLOT_BRACER02:
			return MATERIAL_BRACER;
		case SLOT_HANDS:
			return MATERIAL_HANDS;
		case SLOT_LEGS:
			return MATERIAL_LEGS;
		case SLOT_FEET:
			return MATERIAL_FEET;
		case SLOT_PRIMARY:
			return MATERIAL_PRIMARY;
		case SLOT_SECONDARY:
			return MATERIAL_SECONDARY;
		default:
			return 0xFF;
	}
}


// Test whether a given slot can support a container item
bool Inventory::SupportsContainers(sint16 slot_id)
{
	if ((slot_id>=22 && slot_id<=30) ||		// Personal inventory slots
		(slot_id>=2000 && slot_id<=2023) ||	// Bank slots
		(slot_id>=2500 && slot_id<=2501) ||	// Shared bank slots
		(slot_id==SLOT_CURSOR) ||			// Cursor
		(slot_id>=3000 && slot_id<=3007))	// Trade window
		return true;
	return false;
}


bool Inventory::CanItemFitInContainer(const Item_Struct *ItemToTry, const Item_Struct *Container) {

	if(!ItemToTry || !Container) return false;

	if(ItemToTry->size > Container->common.container.BagSize) return false;

	if((Container->common.container.BagType == bagTypeQuiver) && (ItemToTry->type != ItemTypeArrow)) return false;

	if((Container->common.container.BagType == bagTypeBandolier) && (ItemToTry->type != ItemTypeThrowingv2)) return false;

	return true;
}
