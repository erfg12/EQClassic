/////////////////////////////////////////////////////////////
// -Written by Cofruben-
/////////////////////////////////////////////////////////////
// New way to access to spells:
// 1. Spell* spell = spells_handler.GetSpellPtr(spell_id);
// 2. spell->GetSpellName(), etc..
/////////////////////////////////////////////////////////////
// Before we could access spells array structure directly,
//which is dangerous and uncontrolable. Now, we can access to
//many helping functions and we can debug it properly in case
//of problems.
/////////////////////////////////////////////////////////////

#ifndef SH_HPP
#define SH_HPP
#include <cstring>
#include "client.h"
#include "types.h"
#include "spdat.h"
#include "classes.h"
#include "Spell.hpp"

using namespace std;


class SpellsHandler {
public:
	//////////////////////////////
	//////////////////////////////
	SpellsHandler();
	~SpellsHandler();
	//////////////////////////////
	bool			LoadSpells(char* filename);
	bool			LoadSpellResistMods();
	bool			LoadSpellAdjustments();
	bool			LoadCustomSpells();
	bool			SpellsLoaded() { return this->pSpellsLoaded; }
	TSpellID		GetMaxSpell()  { return SPDAT_RECORDS + NUMBER_OF_CUSTOM_SPELLS; }
	//////////////////////////////
	Spell*			GetSpellPtr(TSpellID spell_id);
	Spell*			GetSpellByName(const char* spell_name);
	//////////////////////////////
	bool			WillSpellLand(Mob* caster, Mob* target, Spell* spell);
	sint32			CalcSpellValue(Spell* spell, int8 effect_number, int8 caster_level, int8 bardMod = 0);
	bool			HandleBuffSpellEffects(Mob* caster, Mob* target, Spell* spell);
	static sint32	AproximateSpellAC(float server_ac);
	int16			CalcResistValue(Spell* spell, Mob* victim, Mob* caster); 
	sint32			CalcSpellBuffTics(Mob* caster, Mob* target, Spell* spell);

private:
	void	SetSpellsLoaded(bool l) { this->pSpellsLoaded = l; }
	sint32	GetFreeBuffSlot(Mob* target, Spell* spell_target);

	Spell	pSpells[SPDAT_RECORDS + NUMBER_OF_CUSTOM_SPELLS];
	bool	pSpellsLoaded;
	sint16	spellResistModArray[SPDAT_RECORDS];
};








#endif