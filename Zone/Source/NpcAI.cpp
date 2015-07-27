#include <cmath>
#include "NpcAI.h"
#include "entity.h"
#include "npc.h"
#include "client.h"
#include "faction.h"
#include "SpellsHandler.hpp"

extern SpellsHandler	spells_handler;

using namespace std;

// Adds Hate to close Mobs depending on faction,level, appearence and target
bool EntityList::AddHateToCloseMobs(NPC* sender, float dist, int social)
{
	if (sender == 0)
		return false;

	//Yeahlight: Merchants and bankers are never KoS and they do not assist anyone (need confirmation on the latter)
	if (sender->GetClass() == MERCHANT || sender->GetClass() == BANKER)
		return false;

	if (dist <= 0)
		dist = 160;

	bool debugFlag = true;

	//Yeahlight: Large NPCs (size > 8) have larger assist and agro radius checks
	//Yeahlight: TODO: Some races are visually HUGE, but have small sizes. Make special cases for these (dragons, wurms, giants, etc)
	int16 sizeModifier = sender->GetSize() - 8;
	if(sizeModifier < 0)
		sizeModifier = 0;
	else if (sizeModifier > 40)
		sizeModifier = 40;

	float dist2 = (dist + sizeModifier * 10) * (dist + sizeModifier * 10);

	LinkedListIterator<Entity*> iterator(list);
	iterator.Reset();
	
	while(iterator.MoreElements())
	{
		if(iterator.GetData()->IsMob() && iterator.GetData()->CastToMob()->GetOwner() != sender)
		{
			Mob* currentmob = iterator.GetData()->CastToMob();
			if(currentmob != sender)
			{
				//Yeahlight: Sender can see currentmob
				if(currentmob->VisibleToMob(sender->CastToMob()))
				{
					if(currentmob->IsClient())
					{
						//Yeahlight: PC is connected and is not charmed (NPCs never jump on a charmed PC)
						if(currentmob->CastToClient()->Connected() && currentmob->CastToClient()->GetOwnerID() == 0)
						{
							FACTION_VALUE lbc = (FACTION_VALUE)currentmob->CastToClient()->GetFactionLevel(currentmob->GetID(),sender->GetNPCTypeID(), currentmob->GetRace(), currentmob->GetClass(), DEITY_AGNOSTIC, sender->GetPrimaryFactionID(), sender);
							// Client is not KOS for this MoB - ignore
							if(!(lbc == FACTION_THREATENLY || lbc == FACTION_SCOWLS))
							{
								iterator.Advance();
								continue;
							}
							//Yeahlight: Currentmob is sitting, which always gives the sender NPC the standard agro range at worst
							if(currentmob->GetAppearance() == SittingAppearance)
							{
								switch(GetLevelCon(currentmob->GetLevel(), sender->GetLevel())) 
								{
									case CON_GREEN:
									case CON_BLUE2:
									case CON_BLUE:
									case CON_WHITE:
									{
										if (lbc == FACTION_SCOWLS)
											dist2 = (dist * dist)/5;
										else // FACTION_THREATENLY
											dist2 = (dist * dist)/6.5;
										break;
									}
									case CON_YELLOW:
									case CON_RED:
									{
										if (lbc == FACTION_SCOWLS)
											dist2 = (dist * dist)/4.5;
										else // FACTION_THREATENLY
											dist2 = (dist * dist)/6;
										break;
									}
									default:
									{
										if (lbc == FACTION_SCOWLS)
											dist2 = (dist * dist)/5;
										else // FACTION_THREATENLY
											dist2 = (dist * dist)/6.5;
										break;
									}
								}
							}
							//Yeahlight: Sender NPC is an undead NPC, which always gives them the standard agro range at worst
							else if(sender->GetBodyType() == BT_Undead)
							{
								switch(GetLevelCon(currentmob->GetLevel(), sender->GetLevel())) 
								{
									case CON_GREEN:
									{
										if (lbc == FACTION_SCOWLS)
											dist2 = (dist * dist)/7.5;
										else  // FACTION_THREATENLY
											dist2 = (dist * dist)/10;
										break;
									}
									case CON_BLUE2:
									case CON_BLUE:
									case CON_WHITE:
									{
										if (lbc == FACTION_SCOWLS)
											dist2 = (dist * dist)/5;
										else // FACTION_THREATENLY
											dist2 = (dist * dist)/6.5;
										break;
									}
									case CON_YELLOW:
									case CON_RED:
									{
										if (lbc == FACTION_SCOWLS)
											dist2 = (dist * dist)/4.5;
										else // FACTION_THREATENLY
											dist2 = (dist * dist)/6;
										break;
									}
									default:
									{
										if (lbc == FACTION_SCOWLS)
											dist2 = (dist * dist)/5;
										else // FACTION_THREATENLY
											dist2 = (dist * dist)/6.5;
										break;
									}
								}
							}
							//Yeahlight: Decrease attack radius based on level difference and faction rank
							else
							{
								switch(GetLevelCon(currentmob->GetLevel(), sender->GetLevel())) 
								{
									case CON_GREEN:
									{
										int16 levelModifier = currentmob->GetLevel() / 10;
										if(levelModifier < 1)
											levelModifier = 1;
										else if(levelModifier > 4)
											levelModifier = 4;
										//Yeahlight: If this mob is no longer green after the modifier, then give it a small agro range
										if(GetLevelCon(currentmob->GetLevel(), sender->GetLevel() + levelModifier) != CON_GREEN)
										{
											if (lbc == FACTION_SCOWLS)
												dist2 = (dist * dist)/7.5;
											else  // FACTION_THREATENLY
												dist2 = (dist * dist)/10;
										}
										else
										{
											if (lbc == FACTION_SCOWLS)
												dist2 = (dist * dist)/500;
											else  // FACTION_THREATENLY
												dist2 = (dist * dist)/666;
										}
										break;
									}
									case CON_BLUE2:
									case CON_BLUE:
									case CON_WHITE:
									{
										if (lbc == FACTION_SCOWLS)
											dist2 = (dist * dist)/5;
										else // FACTION_THREATENLY
											dist2 = (dist * dist)/6.5;
										break;
									}
									case CON_YELLOW:
									case CON_RED:
									{
										if (lbc == FACTION_SCOWLS)
											dist2 = (dist * dist)/4.5;
										else // FACTION_THREATENLY
											dist2 = (dist * dist)/6;
										break;
									}
									default:
									{
										if (lbc == FACTION_SCOWLS)
											dist2 = (dist * dist)/5;
										else // FACTION_THREATENLY
											dist2 = (dist * dist)/6.5;
										break;
									}
								}
							}
							float dist_no_root = currentmob->CastToClient()->DistNoRoot(sender);
							if(dist_no_root <= dist2)
							{
								//Yeahlight: Sender is not currently agro'ed on currentmob
								if(sender->GetNPCHate(currentmob) == 0)
								{
									//Yeahlight: Check LOS
									if(currentmob->CheckCoordLosNoZLeaps(currentmob->GetX(), currentmob->GetY(), currentmob->GetZ(), sender->GetX(), sender->GetY(), sender->GetZ()))
									{ 
										sender->AddToHateList(currentmob, 1);
										if(sender->IsNPC())
										{
											sender->CastToNPC()->onRoamPath = false;
											sender->CastToNPC()->onPath = false;
										}
										//Yeahlight: Undead NPCs will continue to agro more and more entities, the rest are satisified with one
										if(sender->GetBodyType() != BT_Undead)
											return true;
									}
								}
							}
						}
					}
					else if(iterator.GetData()->IsNPC())
					{
						NPC* currentnpc = iterator.GetData()->CastToNPC();				
						if(currentnpc->IsEngaged() || currentnpc->IsCharmed())
						{
							if(currentnpc->GetTarget())
							{
								// Decrease help radius based on level difference
								switch(GetLevelCon(currentnpc->GetTarget()->GetLevel(), sender->GetLevel()))
								{
									case CON_GREEN:
									{
										int16 levelModifier = currentnpc->GetTarget()->GetLevel() / 10;
										if(levelModifier < 1)
											levelModifier = 1;
										else if(levelModifier > 4)
											levelModifier = 4;
										//Yeahlight: If this mob is no longer green after the modifier, then give it a small social range
										if(GetLevelCon(currentnpc->GetTarget()->GetLevel(), sender->GetLevel() + levelModifier) != CON_GREEN)
										{
											social = (dist * dist)/7.5;
										}
										else
										{
											social = 0;
										}
										break;
									}
									case CON_BLUE2:
									case CON_BLUE:
									case CON_WHITE:
									{
										social = (dist * dist)/5;
										break;
									}
									case CON_YELLOW:
									case CON_RED:
									{
										social = (dist * dist)/4.5;
										break;
									}
									default:
									{
										social = (dist * dist)/5;
										break;
									}
								}
								// ok now we have an npc, same faction and engaged
								//Yeahlight: Ignore this if the sender's pFaction is 0
								if(sender->GetPrimaryFactionID() != 0 && sender->GetPrimaryFactionID() == currentnpc->GetPrimaryFactionID() && currentmob->DistNoRoot(sender) <= social)
								{
									//Yeahlight: A primary faction of 394 is reserved for nooby mobs, so we need to compare race to determine assistance.
									//           Also, nooby rats, bats and snakes (races 34, 36 and 37) never assist each other
									//           TODO: Snakes are not on the 394 faction
									if(sender->GetPrimaryFactionID() != 394 || (sender->GetRace() == currentnpc->GetRace() && sender->GetRace() != 36 && sender->GetRace() != 34 && sender->GetRace() != 37 && sender->GetLevel() < 10))
									{
										if(currentnpc->IsEngaged())
										{
											//sint32 tmp = (currentnpc->GetHateTop()->GetLevel() - sender->GetLevel());
											//if(tmp <= 0)
											//	tmp = 0;
											// Chance that he will join, exponential based on level Not much tested yet, may need to tune it
											//Yeahlight: I don't know about this 'chance to ignore' business. I don't remember ANY mobs ignoring me unless their scan timer did not fire in time
											if(true)//(float)rand()/RAND_MAX*100 > (float)tmp*tmp*0.51)
											{
												//Yeahlight: Check LOS
												if(sender->CheckCoordLosNoZLeaps(sender->GetX(), sender->GetY(), sender->GetZ(), currentmob->GetX(), currentmob->GetY(), currentmob->GetZ()))
												{
													Mob* myTarget = NULL;
													//Yeahlight: Sender will assist the current NPC by attacking its charmer, not the target of the current NPC
													if(currentnpc->IsCharmed())
													{
														myTarget = currentnpc->GetOwner();
													}
													else
													{
														myTarget = currentnpc->GetHateTop();
													}
													sender->AddToHateList(myTarget, 1);
													if(currentnpc->onPath)
													{
														sender->onRoamPath = false;
														sender->myPath = currentnpc->myPath;
														sender->myPathNode = currentnpc->myPathNode;
														sender->onPath = true;
														sender->myTargetsX = currentnpc->myTargetsX;
														sender->myTargetsY = currentnpc->myTargetsY;
														sender->myTargetsZ = currentnpc->myTargetsZ;
														sender->CastToNPC()->faceDestination(sender->CastToNPC()->myPathNode->x, sender->CastToNPC()->myPathNode->y);
														sender->CastToNPC()->SetDestination(sender->CastToNPC()->myPathNode->x, sender->CastToNPC()->myPathNode->y, sender->CastToNPC()->myPathNode->z);
														sender->CastToNPC()->StartWalking();
														sender->CastToNPC()->SetNWUPS(sender->GetAnimation() * 2.4 / 5);
														sender->CastToNPC()->MoveTowards(sender->x_dest, sender->y_dest, sender->CastToNPC()->GetNWUPS());
														sender->CastToNPC()->SendPosUpdate(false, NPC_UPDATE_RANGE, false);
														currentnpc->target->Message(LIGHTEN_BLUE, "Debug: %s has been socially trigged to aid %s in its pursuit.", sender->GetName(), currentnpc->GetName());
													}
													return true;
												}
											}
											//else
											//{
											//	sender->SetIgnoreTarget(currentnpc->GetHateTop());
											//}
										}
									}
								}
								// not in the same faction - maybe protect the victim? lets see
								else if(currentmob->DistNoRoot(sender) <= DEFAULT_NPC_SOCIAL_RANGE && currentnpc->GetHateTop() && currentnpc->GetHateTop()->IsClient())
								{
									//Yeahlight: Do I care (considered amiably or better) about the victim?
									if(currentnpc->GetHateTop()->CastToClient()->GetFactionLevel(currentnpc->GetHateTop()->GetID(),sender->GetNPCTypeID(), currentnpc->GetHateTop()->GetRace(), currentnpc->GetHateTop()->GetClass(), DEITY_AGNOSTIC, sender->GetPrimaryFactionID(),sender) <= FACTION_AMIABLE)
									{
										//Yeahlight: Check LOS
										if(sender->CheckCoordLosNoZLeaps(sender->GetX(), sender->GetY(), sender->GetZ(), currentmob->GetX(), currentmob->GetY(), currentmob->GetZ()))
										{  
											sender->AddToHateList(currentnpc, 1);
											return true;
										}
									}
								}
								//Yeahlight: Is this sender NPC scowling at the current, engaged NPC?
								else if(currentmob->DistNoRoot(sender) <= DEFAULT_NPC_SOCIAL_RANGE && currentmob->CastToNPC()->CheckForAggression(sender, currentmob) == FACTION_SCOWLS)
								{
									//Yeahlight: Check LOS
									if(sender->CheckCoordLosNoZLeaps(sender->GetX(), sender->GetY(), sender->GetZ(), currentmob->GetX(), currentmob->GetY(), currentmob->GetZ()))
									{
										sender->AddToHateList(currentmob, 1);
										return true;
									}
								}
							}
						}
						//Yeahlight: Sender is on the same pFaction as the currentnpc, they are both within share range and sender has a leash memory and currentnpc does not
						else if(sender->GetPrimaryFactionID() == currentnpc->GetPrimaryFactionID() && sender->fdistance(sender->GetX(), sender->GetY(), currentnpc->GetX(), currentnpc->GetY()) <= NPC_LEASH_SHARE_DISTANCE && strcmp(sender->GetLeashMemory(), "0") != 0 && strcmp(currentnpc->GetLeashMemory(), "0") == 0)
						{
							if(debugFlag)
							{
								Client* leash_client = entity_list.GetClientByName(sender->GetLeashMemory());
								if(leash_client != 0 && leash_client->IsClient() && leash_client->GetDebugMe())
								{
									leash_client->Message(LIGHTEN_BLUE, "Debug: Due to socializing with %s, %s's leash memory has been shared", currentnpc->GetName(), sender->GetName());
								}
							}
							currentnpc->SetLeashMemory(sender->GetLeashMemory());
							currentnpc->leash_timer->Trigger();
						}
						//Yeahlight: Is this sender NPC scowling at the current, unengaged NPC?
						//           TODO: 5000 range may not be appropriate for all situations
						else if(currentmob->DistNoRoot(sender) <= DEFAULT_NPC_SOCIAL_RANGE)
						{
							FACTION_VALUE lbc = currentmob->CastToNPC()->CheckForAggression(sender, currentmob);
							//Yeahlight: Is sender scowling at currentmob?
							if(lbc == FACTION_THREATENLY || lbc == FACTION_SCOWLS)
							{
								//Yeahlight: Check LOS
								if(sender->CheckCoordLosNoZLeaps(sender->GetX(), sender->GetY(), sender->GetZ(), currentmob->GetX(), currentmob->GetY(), currentmob->GetZ()))
								{
									sender->AddToHateList(currentmob,1);
									return true;
								}
							}
						}
						//Yeahlight: This NPC has has been chosen to receive a buff from the sender
						else if(rand()%1000 < 10 && sender->isCaster && currentmob->DistNoRoot(sender) <= (NPC_BUFF_RANGE * NPC_BUFF_RANGE))
						{
							//Yeahligher: NPC has LoS to sender
							if(sender->CheckCoordLos(sender->GetX(), sender->GetY(), sender->GetZ(), currentmob->GetX(), currentmob->GetY(), currentmob->GetZ()))
							{
								Spell* spellToCast = NULL;
								//Yeahlight: Browse the sender's buffs
								for(int i = 0; i < 2; i++)
								{
									//Yeahlight: This buff slot is "null"
									if(sender->myBuffs[i] == NULL_SPELL)
										continue;
									spellToCast = spells_handler.GetSpellPtr(sender->myBuffs[i]);
									bool permit = true;
									if(spellToCast && spellToCast->IsBuffSpell())
									{
										//Yeahlight: Iterate through the buff slots of the NPC
										for(int j = 0; j < 15; j++)
										{
											if(currentmob->buffs[j].spell && currentmob->buffs[j].spell->IsValidSpell() && currentmob->buffs[j].spell->GetSpellID() == spellToCast->GetSpellID())
											{
												permit = false;
												break;
											}
										}
										//Yeahlight: NPC is a valid target for this spell from sender
										if(permit)
										{
											sender->CastSpell(spellToCast, currentmob->GetID());
											sender->mySpellCounter++;
											return false;
										}
									}
								}
							}
						}
					}
				}
			}
			iterator.Advance();	
		}
		else
		{
			iterator.Advance();	
		}
		//Yeahlight: Zone freeze debug
		if(ZONE_FREEZE_DEBUG && rand()%ZONE_FREEZE_DEBUG == 1)
			EQC_FREEZE_DEBUG(__LINE__, __FILE__);
	}
	
	//Yeahlight: Check for self buffs
	if(sender->isCaster)
	{
		//Yeahlight: 3% chance this NPC will buff itself. We do not want all the NPCs in the zone buffing at once on bootup
		if(rand()%1000 < 30)
		{
			Spell* spellToCast = NULL;
			//Yeahlight: Browse the sender's buffs
			for(int i = 0; i < 2; i++)
			{
				//Yeahlight: This buff slot is "null"
				if(sender->myBuffs[i] == NULL_SPELL)
					continue;
				spellToCast = spells_handler.GetSpellPtr(sender->myBuffs[i]);
				bool permit = true;
				if(spellToCast && spellToCast->IsBuffSpell())
				{
					//Yeahlight: Iterate through the buff slots of the sender
					for(int j = 0; j < 15; j++)
					{
						//Yeahlight: This NPC has this buff on already
						if(sender->buffs[j].spell && sender->buffs[j].spell->IsValidSpell() && sender->buffs[j].spell->GetSpellID() == spellToCast->GetSpellID())
						{
							permit = false;
							break;
						}
					}
					//Yeahlight: This is a valid spell to cast
					if(permit)
					{
						sender->CastSpell(spellToCast, sender->GetID());
						sender->mySpellCounter++;
					}
				}
			}
		}
	}
	return false;
}

//Yeahlight: Clean up this mess
//Yeahlight: TODO: Most of these ranges are incorrect by a few levels
//Green: 0x02
//Blue: 0x04
//White: 0x00
//Yellow: 0x0F
//Red: 0x0D
int32 GetLevelCon(int8 PlayerLevel, int8 NPCLevel)
{
	sint8 tmp = NPCLevel - PlayerLevel;
	int32 conlevel;
	if (PlayerLevel <= 12)
	{
		conlevel =  (tmp <= -4) ? 0x02:
	(tmp >=-3 && tmp <= -1) ? 0x04:
	(tmp == 0) ? 0x00:			
	(tmp >= 1 && tmp <= 2) ?0x0F:    
	0x0D;							
	}
	else if (PlayerLevel >= 13 && PlayerLevel <= 24)
	{
		conlevel =  (tmp <= -6) ? 0x02:			
	(tmp >=-5 && tmp <= -1) ? 0x04:
	(tmp == 0) ? 0x00:
	(tmp >= 1 && tmp <= 2) ?0x0F:
	0x0D;
	}
	else if (PlayerLevel >= 25 && PlayerLevel <= 40)
	{
								conlevel =  (tmp <= -11) ? 0x02:
	(tmp >=-10 && tmp <= -8) ? 0x04:
	(tmp == 0) ? 0x00:
	(tmp >= 1 && tmp <= 2) ?0x0F:
	0x0D;
	}
	else if (PlayerLevel >= 41 && PlayerLevel <= 49)
	{
								conlevel =  (tmp <= -12) ? 0x02:
	(tmp >=-11 && tmp <= -1) ? 0x04:
	(tmp == 0) ? 0x00:
	(tmp >= 1 && tmp <= 2) ?0x0F:
	0x0D;
	}
	else if (PlayerLevel >= 50)
	{
								conlevel =  (tmp <= -14) ? 0x02:
	(tmp >=-13 && tmp <= -1) ? 0x04:
	(tmp == 0) ? 0x00:
	(tmp >= 1 && tmp <= 2) ?0x0F:
	0x0D;
	};
	return conlevel;
}

//o--------------------------------------------------------------
//| PetFaceClosestEntity; Yeahlight, June 16, 2009
//o--------------------------------------------------------------
//| Faces the pet towards the closest non-owner entity in range
//o--------------------------------------------------------------
void EntityList::PetFaceClosestEntity(NPC* pet)
{
	//Yeahlight: NPC is NULL; abort
	if(pet == NULL)
		return;

	const Mob* petOwner = pet->GetOwner();

	//Yeahlight: For whatever reason, this pet does not have an owner; abort
	if(petOwner == NULL)
		return;

	float petX = pet->GetX();
	float petY = pet->GetY();
	float petZ = pet->GetZ();
	
	Mob* closestEntity = NULL;
	float distance = 99999.99;
	float tempDistance = 0;

	LinkedListIterator<Entity*> iterator(list);
	iterator.Reset();
	
	while(iterator.MoreElements())
	{
		//Yeahlight: Entity is a mob, is not a corpse, is not the pet's owner and is not the pet itself
		if(iterator.GetData()->IsMob() && iterator.GetData()->IsCorpse() == false && iterator.GetData()->CastToMob() != petOwner && iterator.GetData()->CastToMob() != pet->CastToMob())
		{
			Mob* entity = iterator.GetData()->CastToMob();
			//Yeahlight: Preliminary distance checking to save CPU time
			if(abs(petX - entity->GetX()) < 350 && abs(petY - entity->GetY()) < 350 && abs(petZ - entity->GetZ()) < 100)
			{
				tempDistance = pet->fdistance(petX, petY, entity->GetX(), entity->GetY());
				//Yeahlight: Good enough, bail out now
				if(tempDistance < 25)
				{
					pet->faceDestination(entity->GetX(), entity->GetY());
					pet->SendPosUpdate(false, NPC_UPDATE_RANGE, false);
					return;
				}
				//Yeahlight: Found a closer NPC within 350 range, record it
				if(tempDistance < distance)
				{
					distance = tempDistance;
					closestEntity = entity;
				}
			}
		}
		iterator.Advance();
		//Yeahlight: Zone freeze debug
		if(ZONE_FREEZE_DEBUG && rand()%ZONE_FREEZE_DEBUG == 1)
			EQC_FREEZE_DEBUG(__LINE__, __FILE__);
	}

	//Yeahlight: Pet found an entity to eye ball
	if(closestEntity)
	{
		pet->faceDestination(closestEntity->GetX(), closestEntity->GetY());
		pet->SendPosUpdate(false, NPC_UPDATE_RANGE, false);
	}
}