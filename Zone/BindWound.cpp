#include "client.h"
#include "skills.h"
#include "mob.h"
#include "itemtypes.h"

//////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////// MORAJ - BIND WOUND  08/2009 /////////////////////////////////////////////
//////////////////////// todo - FORMULA CONFIRM //////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////


void Client::ProcessOP_BindWound(APPLAYER* pApp)//MORAJ
{
	//Yeahlight: Purge client's invisibility
	CancelAllInvisibility();

	if(pApp->size != sizeof(BindWound_Struct)){
		cout << "Invalid BindWound Packet Size" << endl;
		return;
	}

	BindWound_Struct* bw = (BindWound_Struct*)pApp->pBuffer;
	Mob* tar = entity_list.GetMob(bw->to);
	BindWound(tar,false);
	return;
}

bool Client::BindWound(Mob* bindtar,bool fail){
	APPLAYER* bwout_app = new APPLAYER(OP_BindWound,sizeof(BindWound_Struct));
	BindWound_Struct* bwout = (BindWound_Struct*)bwout_app->pBuffer;

	if(!fail){

		if(GetFeigned()){													// Break Feign Death for Bind Wound
			SetFeigned(false);												//
		}

		int cap = 50;														// Set % Cap/Heal Cap
		int hp_cap = bindtar->GetMaxHP()/2;									//
		if(GetClass() == MONK && GetSkill(BIND_WOUND) > 200){				//
			cap = 70;														//
			hp_cap += bindtar->GetMaxHP()/5;								//
		}
		if(!bindwound_timer->Enabled()){                                    // Begin Bind Wound
			
			if(bindtar->GetHP() >= hp_cap){									// INITIAL CAP CHECK
				Message(DARK_BLUE, "You cannot bandage your target past %d percent of their hit points.", cap);
				bwout->type = 3;											// 
				QueuePacket(bwout_app);										//
				bwout->type = 2;											//
				QueuePacket(bwout_app);										//
				safe_delete(bwout_app);										//
				return true;
			}

			int16 bndg_slot = FindItemTypeInInventory(ItemTypeBandage);			// Consume Bandage
			RemoveOneCharge(bndg_slot,true);								//

			bindwound_timer->Start(10000);									// Start Timer

			bindwound_target = bindtar;

			bwout->type = 3;												// Unlock Interface
			QueuePacket(bwout_app);											//
			bwout->type = 0;												//

			if(!bindtar){													// Target Dead
				bwout->type = 4;											//
				QueuePacket(bwout_app);										//
				bwout->type = 0;											//
				bindwound_timer->Disable();									//
				bindwound_target = 0;										//
			}
			else if(bindtar!=this)											// Send Bind Message to target if not binding self
				bindtar->CastToClient()->Message(DARK_BLUE,"You are being bandaged. Stay relatively still.");
		}
		else{																// END Bind Wound

			bindwound_timer->Disable();										// Stop Timer

			bindwound_target = 0;

			if(!bindtar){													// Target Not Found
				bwout->type = 5;											//
				QueuePacket(bwout_app);										//
				bwout->type = 0;											//
			}
			
			if((!GetFeigned() && bindtar->DistNoRoot(this->CastToMob()) <= 400)){ //// **** TAKEN FROM EQEMU, FD Check/Distance?
				bwout->type = 1;											// Finish Bind
				QueuePacket(bwout_app);										//
				bwout->type = 0;											//

				if(this!=bindtar)											// Message to target
					bindtar->CastToClient()->Message(YELLOW,"The bandaging is complete.");
	
				CheckAddSkill(BIND_WOUND);									// Check Skill Up



				if(bindtar->GetHP() < hp_cap){								// END CAP CHECK (Does not include heal amount)
					int heal = 3;											//   Set Heal Amount
					if(GetSkill(BIND_WOUND) > 200)							//   //
						heal += GetSkill(BIND_WOUND)*4/10;					//   //
					else if(GetSkill(BIND_WOUND) >= 10)						//   //
						heal += GetSkill(BIND_WOUND)/4;						//   //

					bindtar->SetHP(bindtar->GetHP()+heal);					// Set Hp + Update
					bindtar->SendHPUpdate();								//
				}
				else{														// Over Cap
					Message(DARK_BLUE, "You cannot bandage your target past %d percent of their hit points.", cap);
					if(this != bindtar)										//
						bindtar->CastToClient()->Message(DARK_BLUE, "You cannot be bandaged past %d percent of your max hit points.",cap);
				}
			}
			else if(bindtar!=this){											// Failed Moved/Moved Away
				bwout->type = 6;											//
				QueuePacket(bwout_app);										//
				bwout->type = 0;											//
			}
			else{

				//Feigned/Out of Range and binding self Not sure if anything needs to go here or not
			}
		}
	}
	else if(bindwound_timer->Enabled()){									// Failed: Ended Via Sit/Stand/Feign

		if(!GetFeigned()){													// Sit/Stand
			bindwound_timer->Disable();										//
			bwout->type = 3;												//
			QueuePacket(bwout_app);											//
			bwout->type = 7;												//
			QueuePacket(bwout_app);											//
		}
		else{
			bindwound_timer->Disable();										// Feign Death
			bindwound_target = 0;											//
			Message(YELLOW,"You have moved and your attempt to bandage has failed.");
		}
	}
	safe_delete(bwout_app);													// Delete packet
	return true;
}