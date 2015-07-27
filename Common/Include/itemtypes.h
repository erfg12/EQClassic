// ***************************************************************
//  EQCException   ·  date: 18/7/2007
//  -------------------------------------------------------------
//  Copyright (C) 2007 - All Rights Reserved
// ***************************************************************
// These contain the item types for more labeled item/stack management.
// ***************************************************************

#ifndef ITEMTYPES_H
#define ITEMTYPES_H

/*
	Bag types
*/
enum {
	bagTypeSmallBag		= 0,
	bagTypeLargeBag		= 1,
	bagTypeQuiver		= 2,
	bagTypeBeltPouch	= 3,
	bagTypeBandolier	= 8
	//... there are 50 types
};

/*
** Item uses
**
*/
enum ItemTypes
{
	ItemType1HS			= 0,
	ItemType2HS			= 1,
	ItemTypePierce		= 2,
	ItemType1HB			= 3,
	ItemType2HB			= 4,
	ItemTypeBow			= 5,
	//6
	ItemTypeThrowing		= 7,
	ItemTypeShield		= 8,
	//9
	ItemTypeArmor		= 10,
	ItemTypeUnknon		= 11,	//A lot of random crap has this item use.
	ItemTypeLockPick		= 12,
	ItemTypeFood			= 14,
	ItemTypeDrink		= 15,
	ItemTypeLightSource	= 16,
	ItemTypeStackable	= 17,	//Not all stackable items are this use...
	ItemTypeBandage		= 18,
	ItemTypeThrowingv2	= 19,
	ItemTypeSpell		= 20,	//spells and tomes
	ItemTypePotion		= 21,
	ItemTypeWindInstr	= 23,
	ItemTypeStringInstr	= 24,
	ItemTypeBrassInstr	= 25,
	ItemTypeDrumInstr	= 26,
	ItemTypeArrow		= 27,
	ItemTypeUnknownConsumable = 28, // Harakiri does not exist, but client checks this type as consumable
	ItemTypeJewlery		= 29,
	ItemTypeSkull		= 30,
	ItemTypeTome			= 31,
	ItemTypeNote			= 32,
	ItemTypeKey			= 33,
	ItemTypeCoin			= 34,
	ItemType2HPierce		= 35,
	ItemTypeFishingPole	= 36,
	ItemTypeFishingBait	= 37,
	ItemTypeAlcohol		= 38,
	ItemTypeCompass		= 40,
	ItemTypePoison		= 42,	//might be wrong, but includes poisons
	ItemTypeHand2Hand	= 45,
	ItemUseSinging		= 50,
	ItemUseAllInstruments	= 51,
	ItemTypeCharm		= 52,
	ItemTypeAugment		= 54,
	ItemTypeAugmentSolvent	= 55,
	ItemTypeAugmentDistill	= 56
};

/*
** Item Effect Types
**
*/
enum {
	ET_CombatProc = 0,
	ET_ClickEffect = 1,
	ET_WornEffect = 2,
	ET_Expendable = 3,
	ET_EquipClick = 4,
	ET_ClickEffect2 = 5,	//name unknown
	ET_Focus = 6,
	ET_Scroll = 7
};


#endif