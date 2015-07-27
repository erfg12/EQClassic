///////////////////////////////////////////////////
#include <logger.h>
#include "client.h"
#include "EQCException.hpp"
#include "ZoneGuildManager.h"
#include "groups.h"
#include "MessageTypes.h"

#ifdef EMBPERL
	#include "embparser.h"
#endif

using namespace EQC::Zone;

extern Zone*			zone;
extern WorldServer		worldserver;
extern ZoneGuildManager	zgm;
///////////////////////////////////////////////////

void Client::Message(MessageFormat Type, char* pMessage, ...)
{
	//-Cofruben: attempt to secure this.
	const int32 TAM_MAX = 2056;

	if (!pMessage)
	{
		EQC_MOB_EXCEPT("void Client::Message()", "pText = NULL");
	}

	int32 Len = strlen(pMessage);

	if (Len > TAM_MAX)
	{
		EQC_MOB_EXCEPT("void Client::Message()", "Length is too big");
	}

	va_list Args;
	char* Buffer = new char[Len*10+512];
	memset(Buffer, 0, Len*10+256);
	va_start(Args, pMessage);
	vsnprintf(Buffer, Len*10 + 256, pMessage, Args);
	va_end(Args);
	string NewString = Buffer;

	APPLAYER* app = new APPLAYER;
	app->opcode = OP_SpecialMesg;
	app->size = 4+strlen(Buffer)+1;
	app->pBuffer = new uchar[app->size];
	SpecialMesg_Struct* sm=(SpecialMesg_Struct*)app->pBuffer;
	sm->msg_type = Type;
	strcpy(sm->message, Buffer);
	QueuePacket(app);
	safe_delete(app);//delete app;
	safe_delete_array(Buffer);//delete[] Buffer;
}

///////////////////////////////////////////////////

void Client::Message(MessageFormat Type, const string& rMessage)
{
	this->Message(Type, (char*)rMessage.c_str(), 0);
}

///////////////////////////////////////////////////

void Client::ChannelMessageReceived(int8 chan_num, int8 language, char* message, char* targetname) 
{
	if(!message)
	{
		EQC_MOB_EXCEPT("void Client::ChannelMessageReceived()", "message == NULL");
	}
	

	// Harakiri - we should check this more carefully
	// what if a client starts a sentence with # ?
	if (message[0] == '#' && chan_num == MessageChannel_SAY)
	{
		this->ProcessCommand(message);
	}
	else
	{
		zgm.LoadGuilds();
		Guild_Struct* guilds = zgm.GetGuildList();

		if (chan_num == MessageChannel_SHOUT || chan_num == MessageChannel_AUCTION) // Shout, auction
		{ 
			entity_list.ChannelMessage(this, chan_num, language, message);
		}
		else if (chan_num == MessageChannel_SAY) // say
		{
					

			//Yeahlight: Please keep this here, we may need this for the sacrifice spell request
			////Yeahlight: Resurrection request response by the client
			//if(GetPendingRez())
			//{
			//	//Yeahlight: Player accepts the resurrection
			//	if(strcasecmp(message, "yes") == 0)
			//	{
			//		ServerPacket* pack = new ServerPacket(ServerOP_RezzPlayer, sizeof(RezzPlayer_Struct));
			//		RezzPlayer_Struct* sem = (RezzPlayer_Struct*) pack->pBuffer;
			//		memset(pack->pBuffer, 0, sizeof(RezzPlayer_Struct));
			//		sem->rezzopcode = OP_RezzAnswer;
			//		sem->exp = GetPendingRezExp();
			//		strcpy(sem->owner, this->GetName());
			//		sem->action = 1;
			//		worldserver.SendPacket(pack);
			//		safe_delete(pack);
			//		return;
			//	}
			//	//Yeahlight: Player declines the resurrection
			//	else if(strcasecmp(message, "no") == 0)
			//	{
			//		ServerPacket* pack = new ServerPacket(ServerOP_RezzPlayer, sizeof(RezzPlayer_Struct));
			//		RezzPlayer_Struct* sem = (RezzPlayer_Struct*) pack->pBuffer;
			//		memset(pack->pBuffer, 0, sizeof(RezzPlayer_Struct));
			//		sem->rezzopcode = OP_RezzAnswer;
			//		sem->exp = GetPendingRezExp();
			//		strcpy(sem->owner, this->GetName());
			//		sem->action = 0;
			//		worldserver.SendPacket(pack);
			//		safe_delete(pack);
			//		return;
			//	}
			//}
			//Yeahlight: PC is voice grafting
			if(GetVoiceGrafting())
			{
				Mob* myPet = GetPet();
				//Yeahlight: PC has an NPC pet to talk through
				if(myPet && myPet->IsNPC())
				{
					entity_list.ChannelMessage(myPet, chan_num, language, message);
				}
				else
				{
					entity_list.ChannelMessage(this, chan_num, language, message);
				}
			}
			else
			{
				entity_list.ChannelMessage(this, chan_num, language, message);
			}	
		
			// Harakiri Quest Event SAY
			#ifdef EMBPERL
			if (target != 0 && target->IsNPC()) {
				if(perlParser->HasQuestSub(target->GetNPCTypeID(),QuestEventSubroutines[EVENT_SAY])){
					
					if (DistNoRootNoZ(target) <= 200) {
							//if(target->CastToNPC()->IsMoving() && !target->CastToNPC()->IsOnHatelist(target))
							//target->CastToNPC()->PauseWandering(RuleI(NPC, SayPauseTimeInSec));
						perlParser->Event(EVENT_SAY, target->GetNPCTypeID(), message, target->CastToNPC(), this, language);					
					}
				}
			}
			#endif
		}
		else if (chan_num == MessageChannel_OCC) // Quagmire - moved ooc to worldwide since moved guildsay to, err, guilds
		{ 
			entity_list.ChannelMessage(this, chan_num, language, message);
			//Yeahlight: OOC is not a global channel
			//if (!worldserver.SendChannelMessage(this, 0, 5, 0, language, message))
			//{
			//	Message(BLACK, "Error: World server disconnected");
			//}
		}
		else if (chan_num == MessageChannel_GUILDSAY) // 0 = guildsay
		{
			if (guilddbid == 0 || guildeqid >= 512)
			{
				Message(BLACK, "Error: You arent in a guild.");
			}
			else if (!guilds[guildeqid].rank[guildrank].speakgu)
			{
				Message(BLACK, "Error: You dont have permission to speak to the guild.");
			}
			else if (!worldserver.SendChannelMessage(this, targetname, chan_num, guilddbid, language, message))
			{
				Message(BLACK, "Error: World server disconnected");
			}
		}
		else if (chan_num == MessageChannel_TELL) // 7 = tell
		{
			//Cofruben: better in a constant!
			const int MessageRange = 2000;
			char npcNameSuffix[] = "_CHARM";
			char npcNamePrefix[100] = "";
			char compareName[100] = "";
			Mob* mypet = this->GetPet();
			strcpy(npcNamePrefix, this->GetName());
			strcat(npcNamePrefix, npcNameSuffix);
			strncpy(compareName, targetname, strlen(targetname) - 2);

			//Yeahlight: This client is attempting to communicate with its pet
			if(mypet != 0 && (strstr(mypet->GetName(),targetname) != NULL || strcmp(compareName, npcNamePrefix) == 0))
			{
				//Yeahlight: Pet is an NPC
				if(mypet->IsNPC())
				{
					//Tazadar: Added this to forbid chanter to give orders 
					//Yeahlight: Charmed pets follow all orders, though
					if(this->GetClass() != ENCHANTER || mypet->CastToNPC()->IsCharmed())
					{
						if (strstr(_strupr(message), "ATTACK") != NULL && target) {
							if ((target->IsClient() && target->GetID() != this->GetDuelTarget()) || strcmp(target->GetName(), mypet->GetName())==0 ) // Jester: Don't attack other players. Tazadar : Pet dont attack yourself !!
								return;
							Message(WHITE,"%s tells you, 'Attacking %s, Master.'", mypet->CastToNPC()->GetName(), target->IsNPC() ? target->CastToNPC()->GetProperName() : this->target->GetName());
							if (mypet->CastToNPC()->IsEngaged() == false) zone->AddAggroMob(); //merkur
							mypet->SetTarget(target);
							mypet->CastToNPC()->AddToHateList(target);
							if(mypet->GetPetOrder()==SPO_Sit){
								mypet->SetPetOrder(SPO_Follow);
								mypet->CastToNPC()->SendAppearancePacket(this->GetPetID(), SAT_Position_Update,0x64,true);
							}
						}
						else if (strstr(_strupr(message), "TAUNT") != NULL) {
							if(mypet->CastToNPC()->GetTaunting() == false)
							{
								mypet->CastToNPC()->StartTaunting();
								entity_list.MessageClose(mypet, false, DEFAULT_MESSAGE_RANGE, BLACK, "%s says, 'Now taunting attackers, Master.'", mypet->CastToNPC()->GetName());
							}
							else
							{
								mypet->CastToNPC()->StopTaunting();
								entity_list.MessageClose(mypet, false, DEFAULT_MESSAGE_RANGE, BLACK, "%s says, 'No longer taunting attackers, Master.'", mypet->CastToNPC()->GetName());
							}
						}
						else if (strstr(_strupr(message), "BACK OFF") != NULL || strstr(_strupr(message), "AS YOU WERE") != NULL) {
							entity_list.MessageClose(mypet, false, DEFAULT_MESSAGE_RANGE, BLACK, "%s says, 'Sorry, Master.. calming down.'", mypet->CastToNPC()->GetName());
							mypet->CastToNPC()->WhipeHateList();
						}
						else if(strstr(_strupr(message), "GUARD HERE") != NULL || strstr(_strupr(message), "GUARD") != NULL) {
							entity_list.MessageClose(mypet, false, DEFAULT_MESSAGE_RANGE, BLACK, "%s says, 'Guarding with my life.. oh splendid one.'",mypet->CastToNPC()->GetName());
							mypet->SetPetOrder(SPO_Guard);
							mypet->SaveGuardSpot();
						}
						else if (strstr(_strupr(message), "SIT") != NULL) {
							mypet->SetPetOrder(SPO_Sit);
							entity_list.MessageClose(mypet, false, DEFAULT_MESSAGE_RANGE, BLACK, "%s says, 'Changing position, Master.'",mypet->CastToNPC()->GetName());
							//Yeahlight: TODO: What? Telling a pet to sit will cancel channeling?
							//mypet->InterruptSpell(); //Baron-Sprite: No cast 4 u. // neotokyo: i guess the pet should start casting
							mypet->CastToNPC()->SendAppearancePacket(this->GetPetID(), SAT_Position_Update,0x6e,true);
						}
						else if (strstr(_strupr(message), "STAND") != NULL) {
							mypet->SetPetOrder(SPO_Sit);
							mypet->SaveGuardSpot();
							entity_list.MessageClose(mypet, false, DEFAULT_MESSAGE_RANGE, BLACK, "%s says, 'Changing position, Master.'",mypet->CastToNPC()->GetName());
							mypet->CastToNPC()->SendAppearancePacket(this->GetPetID(), SAT_Position_Update,0x64,true);
						}
						else if (strstr(_strupr(message), "FOLLOW") != NULL || strstr(_strupr(message), "GUARD ME") != NULL || strstr(_strupr(message), "FOLLOW ME") != NULL) {
							mypet->SetPetOrder(SPO_Follow);
							entity_list.MessageClose(mypet, false, DEFAULT_MESSAGE_RANGE, BLACK, "%s says, 'Following Master.'", mypet->CastToNPC()->GetName());
							mypet->CastToNPC()->SendAppearancePacket(this->GetPetID(), SAT_Position_Update, 0x64, true);
						}
					}
					if (strstr(_strupr(message), "LEADER") != NULL || strstr(_strupr(message), "WHO LEADER") != NULL) {
						entity_list.MessageClose(mypet, false, DEFAULT_MESSAGE_RANGE, WHITE, "%s says, 'My leader is %s'", mypet->CastToNPC()->GetName(),this->GetName());
					}
					else if (strstr(_strupr(message), "GET LOST") != NULL) {
						entity_list.MessageClose(mypet, false, DEFAULT_MESSAGE_RANGE, WHITE, "As you wish! Oh great one.");
						//Yeahlight: NPC is not a real pet, but rather a charmed NPC
						if(mypet->CastToNPC()->IsCharmed())
						{
							mypet->BuffFadeByEffect(SE_Charm);
						}
						else
						{
							mypet->CastToNPC()->Depop();
						}
					}
					else if (strstr(_strupr(message), "REPORT HEALTH") != NULL) {
						Message(WHITE, "I have %d percent of my hps left", (int8)mypet->GetHPRatio());
					}
					//printf("* Tried to use a pet command for %s\n", mypet->GetName());
				}
				//Yeahlight: Pet is a client
				else if(mypet->IsClient())
				{
					//Tazadar: Added this to forbid chanter to give orders 
					//Yeahlight: Charmed pets follow all orders, though
					if(this->GetClass() != ENCHANTER || mypet->CastToClient()->IsCharmed())
					{
						if (strstr(_strupr(message), "ATTACK") != NULL && target) {
							if ((target->IsClient() && target->GetID() != this->GetDuelTarget()) || strcmp(target->GetName(), mypet->GetName()) == 0) // Jester: Don't attack other players. Tazadar : Pet dont attack yourself !!
								return;
							Message(WHITE,"%s tells you, 'Attacking %s, Master.'", mypet->GetName(), target->IsNPC() ? target->CastToNPC()->GetProperName() : this->target->GetName());
							mypet->SetTarget(target);
							if(mypet->GetPetOrder() == SPO_Sit){
								mypet->SetPetOrder(SPO_Follow);
								mypet->SendAppearancePacket(this->GetPetID(), SAT_Position_Update, SAPP_Sitting_To_Standing, true);
								mypet->SendAppearancePacket(this->GetPetID(), SAT_Position_Update, SAPP_Lose_Control, true);
							}
						}
						else if (strstr(_strupr(message), "BACK OFF") != NULL || strstr(_strupr(message), "AS YOU WERE") != NULL) {
							entity_list.MessageClose(mypet, false, DEFAULT_MESSAGE_RANGE, BLACK, "%s says, 'Sorry, Master.. calming down.'", mypet->GetName());
							mypet->SetTarget(mypet->GetOwner());
						}
						else if(strstr(_strupr(message),"GUARD HERE") != NULL) {
							entity_list.MessageClose(mypet, false, DEFAULT_MESSAGE_RANGE, BLACK, "%s says, 'Guarding with my life.. oh splendid one.'",mypet->GetName());
							mypet->SetPetOrder(SPO_Guard);
							mypet->SaveGuardSpot();
						}
						else if (strstr(_strupr(message),"SIT") != NULL) {
							mypet->SetPetOrder(SPO_Sit);
							entity_list.MessageClose(mypet, false, DEFAULT_MESSAGE_RANGE, BLACK, "%s says, 'Changing position, Master.'",mypet->GetName());
							//Yeahlight: TODO: What? Telling a pet to sit will cancel channeling?
							//mypet->InterruptSpell(); //Baron-Sprite: No cast 4 u. // neotokyo: i guess the pet should start casting
							mypet->SendAppearancePacket(this->GetPetID(), SAT_Position_Update, SAPP_Standing_To_Sitting, true);
							mypet->SendAppearancePacket(this->GetPetID(), SAT_Position_Update, SAPP_Lose_Control, true);
						}
						else if (strstr(_strupr(message),"STAND") != NULL) {
							mypet->SetPetOrder(SPO_Sit);
							mypet->SaveGuardSpot();
							entity_list.MessageClose(mypet, false, DEFAULT_MESSAGE_RANGE, BLACK, "%s says, 'Changing position, Master.'",mypet->GetName());
							mypet->SendAppearancePacket(this->GetPetID(), SAT_Position_Update, SAPP_Sitting_To_Standing, true);
							mypet->SendAppearancePacket(this->GetPetID(), SAT_Position_Update, SAPP_Lose_Control, true);
						}
						else if (strstr(_strupr(message), "FOLLOW") != NULL || strstr(_strupr(message), "GUARD ME") != NULL || strstr(_strupr(message), "FOLLOW ME") != NULL) {
							entity_list.MessageClose(mypet, false, DEFAULT_MESSAGE_RANGE, BLACK, "%s says, 'Following Master.'",mypet->GetName());
							mypet->SendAppearancePacket(this->GetPetID(), SAT_Position_Update, SAPP_Sitting_To_Standing, true);
							mypet->SendAppearancePacket(this->GetPetID(), SAT_Position_Update, SAPP_Lose_Control, true);
							mypet->SetTarget(NULL);
							mypet->SetPetOrder(SPO_Follow);
						}
					}
					if (strstr(_strupr(message), "LEADER") != NULL || strstr(_strupr(message), "WHO LEADER") != NULL) {
						entity_list.MessageClose(mypet, false, DEFAULT_MESSAGE_RANGE, WHITE, "%s says, 'My leader is %s'", mypet->GetName(),this->GetName());
					}
					else if (strstr(_strupr(message), "GET LOST") != NULL) {
						entity_list.MessageClose(mypet, false, DEFAULT_MESSAGE_RANGE, WHITE, "As you wish! Oh great one.");
						//Yeahlight: PC is not a real pet
						mypet->BuffFadeByEffect(SE_Charm);
					}
					else if (strstr(_strupr(message), "REPORT HEALTH") != NULL) {
						Message(WHITE, "I have %d percent of my hps left", (int8)mypet->GetHPRatio());
					}
					//printf("* Tried to use a pet command for %s\n", mypet->GetName());
				}
			}
			else if (!worldserver.SendChannelMessage(this, targetname, chan_num, 0, language, message))
			{
				Message(BLACK, "Error: World server disconnected");
			}
		}
		else if (chan_num == MessageChannel_BROADCAST || chan_num == MessageChannel_GMSAY) // 6 = broadcast, 11 = GMSAY
		{
			if (!(admin >= 80))
			{
				Message(BLACK, "Error: Only GMs can use this channel");
			}
			else if (!worldserver.SendChannelMessage(this, targetname, chan_num, 0, language, message))
			{
				Message(BLACK, "Error: World server disconnected");
			}
		}
		else if(chan_num == MessageChannel_GSAY) // 2 = gsay & targetname is cycled for each member in group (/gsay in a 6 man group sends 5 diff msgs)
		{
			if(this->IsGrouped())
			{
				entity_list.GetGroupByClient(this)->GroupMessage(this, targetname,language, message);
			}
		}
		else
		{
			Message(BLACK, "Channel not implemented");
		} 

	}
}

///////////////////////////////////////////////////

void Client::ChannelMessageSend(char* from, char* to, int8 chan_num, int8 language, char* message, ...)
{
	if ((chan_num == 11 || chan_num == 10) && !(this->Admin() >= 80)) // dont need to send /pr & /pet to everybody
	{
		return;
	}

	va_list argptr;	
	// Harakiri NPC quest text can be quite long!
	char buffer[4096];

	va_start(argptr, message);
	vsnprintf(buffer, 4096, message, argptr);
	va_end(argptr);

	//Yeahlight: TODO: The new client kept crashing with buffers size 1 and under. The
	//                 +5 below appears to prevent this from happening. Need to look
	//                 into this.
	APPLAYER* app = new APPLAYER(OP_ChannelMessage, sizeof(ChannelMessage_Struct) + strlen(buffer) + 5);

	//app.opcode = OP_ChannelMessage;
	//app.size = sizeof(ChannelMessage_Struct)+strlen(buffer)+1;
	//app.pBuffer = new uchar[app.size];
	memset(app->pBuffer, 0, app->size);
	ChannelMessage_Struct* cm = (ChannelMessage_Struct*) app->pBuffer;

	if (from == 0)
	{
		strcpy(cm->sender, "ZServer");
	}
	else if (from[0] == 0)
	{
		strcpy(cm->sender, "ZServer");
	}
	else
	{
		strcpy(cm->sender, from);
	}

	if (to != 0)
	{
		strcpy((char *) cm->targetname, to);
	}
	else
	{
		cm->targetname[0] = 0;
	}

	cm->language = language;
	cm->chan_num = chan_num;
	// One of these tells the client how well we know the language
	cm->cm_unknown4[0] = 0xff;
	cm->cm_unknown4[1] = 0xff;
	//////////////////////////////////////////////////////////////
	//Yeahlight: One of these bytes describes how drunk the sender is
	cm->cm_unknown2[0] = 0x00;
	cm->cm_unknown2[1] = 0x00;
	cm->cm_unknown2[2] = 0x00;
	cm->cm_unknown2[3] = 0x00;
	cm->cm_unknown2[4] = 0x00;
	cm->cm_unknown2[5] = 0x00;
	cm->cm_unknown2[6] = 0x00;
	cm->cm_unknown2[7] = 0x00;
	cm->cm_unknown2[8] = 0x00;
	//////////////////////////////////////////////////////////////
	strcpy(&cm->message[0], buffer);

	if (chan_num == 7 && from != 0) 
	{ 
		strcpy(cm->targetname, pp.name); 
	}

	QueuePacket(app);
	safe_delete(app);
}

///////////////////////////////////////////////////