/* 
Harakiri Sept 22. 2009

This class is used to create opcode/structs dynamically through the perl interpreter,
when testing/bruteforcing opcodes it is easier to use perl, because you dont have to rebuild the zone
or restart the client.

The code was ported over from EQEMU 7.0
*/

#include "logger.h"
#include <stdlib.h>
#include "perlpacket.h"
#include "client.h"
#include "entity.h"
#include "packet_dump.h"
#include "MiscFunctions.h"

PerlPacket::PerlPacket(uint32 opcode, uint32 length) {
	SetOpcode(opcode);
	packet = NULL;
	len = 0;
	Resize(length);
}

PerlPacket::~PerlPacket() {
	if(packet != NULL)
		safe_delete_array(packet);
}

bool PerlPacket::SetOpcode(uint32 opcode) {
	op = opcode;
	return(op != OP_Unknown);
}

void PerlPacket::Resize(uint32 length) {
	Zero();
	if(len == length)
		return;
	if(packet != NULL)
		safe_delete_array(packet);
	len = length;
	if(len == 0)
		packet = NULL;
	else {
		packet = new unsigned char[len];
		Zero();
	}
}

//sending functions
void PerlPacket::SendTo(Client *who) {
	if(!who || op == OP_Unknown || (len > 0 && packet == NULL))
		return;
	
	APPLAYER *outapp = new APPLAYER(op, len);
	if(len > 0)
		memcpy(outapp->pBuffer, packet, len);
		
	DumpPacketHex(outapp);
	EQC::Common::Log(EQCLog::Debug,CP_QUESTS, "PerlPacket::SendTo(Client) opcode = %d", op);
	
	who->QueuePacket(outapp);
}

void PerlPacket::SendToAll() {
	if(op == OP_Unknown || (len > 0 && packet == NULL))
		return;
	
	APPLAYER *outapp = new APPLAYER(op, len);
	if(len > 0)
		memcpy(outapp->pBuffer, packet, len);

	DumpPacketHex(outapp);
	EQC::Common::Log(EQCLog::Debug,CP_QUESTS, "PerlPacket::SendToAll() opcode = %d", op);
	
	entity_list.QueueClients(NULL, outapp, false);
	safe_delete(outapp);
}

//editing
void PerlPacket::Zero() {
	if(len == 0 || packet == NULL)
		return;
	memset(packet, 0, len);
}

void PerlPacket::FromArray(int numbers[], uint32 length) {
	if(length == 0)
		return;
	Resize(length);
	uint32 r;
	for(r = 0; r < length; r++) {
		packet[r] = numbers[r] & 0xFF;
	}
}

void PerlPacket::SetByte(uint32 pos, uint8 val) {
	if(pos + sizeof(val) > len || packet == NULL)
		return;
	uint8 *p = (uint8 *) (packet + pos);
	*p = val;
}

void PerlPacket::SetShort(uint32 pos, uint16 val) {
	if(pos + sizeof(val) > len || packet == NULL)
		return;
	uint16 *p = (uint16 *) (packet + pos);
	*p = val;
}

void PerlPacket::SetLong(uint32 pos, uint32 val) {
	if(pos + sizeof(val) > len || packet == NULL)
		return;
	uint32 *p = (uint32 *) (packet + pos);
	*p = val;
}

void PerlPacket::SetFloat(uint32 pos, float val) {
	if(pos + sizeof(val) > len || packet == NULL)
		return;
	float *p = (float *) (packet + pos);
	*p = val;
}

void PerlPacket::SetString(uint32 pos, char *str) {
	int slen = strlen(str);
	if(pos + slen > len || packet == NULL)
		return;
	strcpy((char *)(packet+pos), str);
}

#pragma pack(1)
struct EQ1319 {
	sint32 part13:13,
		   part19:19;
};
struct EQ1913 {
	sint32 part19:19,
		   part13:13;
};
#pragma pack()

void PerlPacket::SetEQ1319(uint32 pos, float part13, float part19) {
	if(pos + sizeof(EQ1319) > len || packet == NULL)
		return;
	EQ1319 *p = (EQ1319 *) (packet + pos);
	p->part19 = FloatToEQ19(part19);
	p->part13 = FloatToEQ13(part13);
}

void PerlPacket::SetEQ1913(uint32 pos, float part19, float part13) {
	if(pos + sizeof(EQ1913) > len || packet == NULL)
		return;
	EQ1913 *p = (EQ1913 *) (packet + pos);
	p->part19 = FloatToEQ19(part19);
	p->part13 = FloatToEQ13(part13);
}
	
//reading
uint8 PerlPacket::GetByte(uint32 pos) {
	if(pos + sizeof(uint8) > len || packet == NULL)
		return(0);
	uint8 *p = (uint8 *) (packet + pos);
	return(*p);
}

uint16 PerlPacket::GetShort(uint32 pos) {
	if(pos + sizeof(uint16) > len || packet == NULL)
		return(0);
	uint16 *p = (uint16 *) (packet + pos);
	return(*p);
}

uint32 PerlPacket::GetLong(uint32 pos) {
	if(pos + sizeof(uint32) > len || packet == NULL)
		return(0);
	uint32 *p = (uint32 *) (packet + pos);
	return(*p);
}

float PerlPacket::GetFloat(uint32 pos) {
	if(pos + sizeof(float) > len || packet == NULL)
		return(0);
	float *p = (float *) (packet + pos);
	return(*p);
}



