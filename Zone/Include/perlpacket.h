/* 
Harakiri Sept 22. 2009

This class is used to create opcode/structs dynamically through the perl interpreter,
when testing/bruteforcing opcodes it is easier to use perl, because you dont have to rebuild the zone
or restart the client.

The code was ported over from EQEMU 7.0
*/
#ifndef PERLPACKET_H
#define PERLPACKET_H

#include <string>
#include <vector>
using namespace std;

class Client;
const uint32 OP_Unknown = 0xFFFF;

class PerlPacket {
public:
	PerlPacket(uint32 opcode = OP_Unknown, uint32 len = 0);
	~PerlPacket();
	
	bool SetOpcode(uint32 opcode);
	void Resize(uint32 len);
	
	//sending functions
	void SendTo(Client *who);
	void SendToAll();
	
	/* Harakiri 
	These are pretty easy to understand, they each take a position into the packet and a value, and store that value in the 
	packet at that position. Each one specfies the data type of the element which is to be stored, which determines the length. 
	Each function checks the length of the data type against the length of the packet, and will not grow or overwrite the bounds of the packet.
	*/
	void Zero();
	void FromArray(int numbers[], uint32 length);
	void SetByte(uint32 pos, uint8 val);
	void SetShort(uint32 pos, uint16 val);
	void SetLong(uint32 pos, uint32 val);
	void SetFloat(uint32 pos, float val);
	void SetString(uint32 pos, char *str);
	
	void SetEQ1319(uint32 pos, float part13, float part19);
	void SetEQ1913(uint32 pos, float part19, float part13);
	
	//reading
	uint8 GetByte(uint32 pos);
	uint16 GetShort(uint32 pos);
	uint32 GetLong(uint32 pos);
	float GetFloat(uint32 pos);
	
protected:
	uint32 op;
	uint32 len;
	unsigned char *packet;
};


#endif
