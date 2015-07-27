//TODO: Fill this file out - Dark-Pinrce - 29/01/2008
#include "client.h"
#include "skills.h"

void Client::ProcessOP_Mend(APPLAYER* pApp)
{
	int mendhp = GetMaxHP() / 4;
	int noadvance = (rand()%200);
	int currenthp = GetHP();

	if (rand()%100 <= GetSkill(MEND))
	{
		SetHP(GetHP() + mendhp);
		SendHPUpdate();
		Message(DARK_BLUE, "You mend your wounds and heal some damage");
	}
	else if (noadvance > 175 && currenthp > mendhp)
	{
		SetHP(GetHP() - mendhp);
		SendHPUpdate();
		Message(DARK_BLUE, "You fail to mend your wounds and damage yourself!");
	}
	else if (noadvance > 175 && currenthp <= mendhp)
	{
		SetHP(1);
		SendHPUpdate();
		Message(DARK_BLUE, "You fail to mend your wounds and damage yourself!");
	}
	else
	{
		Message(DARK_BLUE, "You fail to mend your wounds");
	}

	/*if ((GetSkill(MEND) < noadvance) && (rand()%100 < 35) && (GetSkill(MEND) < 101))
	{
		this->SetSkill(MEND,++pp.skills[MEND]);
	}*/

	CheckAddSkill(MEND); 
}