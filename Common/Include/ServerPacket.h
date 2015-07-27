// ***************************************************************
//  Database Handler   ·  date: 16/05/2007
//  -------------------------------------------------------------
//  Copyright (C) 2007 - All Rights Reserved
// ***************************************************************
// Handles interaction directly with MySQL
// ***************************************************************

#ifndef SERVERPACKET_H
#define SERVERPACKET_H

namespace EQC
{
	namespace Common
	{
		namespace Network
		{
			class ServerPacket
			{
			public:
				~ServerPacket() 
				{ 
					safe_delete(pBuffer); 
				}

				ServerPacket(int16 in_opcode = 0, int32 in_size = 0) 
				{
					if (in_size <= 0)
					{
						// if size is less than or equal to zero

						// set size to 0 as we cant have a negative size
						this->size = 0;

						// Zero out buffer
						this->pBuffer = 0;
					}
					else
					{
						this->size = in_size;

						this->pBuffer = new uchar[this->size];
						memset(this->pBuffer, 0, this->size);
					}

					// Set OpCode
					this->opcode = in_opcode;
				}

				int16  size;
				int16  opcode;
				uchar* pBuffer;
			};
		}
	}
}
#endif