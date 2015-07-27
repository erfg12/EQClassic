// ***************************************************************
//  APPlayer   ·  date: 17/05/2007
//  -------------------------------------------------------------
//  Copyright (C) 2007 - All Rights Reserved
// ***************************************************************
// 
// ***************************************************************
#ifndef APPLAYER_H
#define APPLAYER_H

#include <iostream>

template <typename type> void SAFE_DELETE(type &del){if(del){delete[]del; del=0;}}

namespace EQC
{
	namespace Common
	{
		namespace Network
		{
			class APPLAYER
			{
			public:
				APPLAYER(int16 in_opcode = 0, int32 in_size = 0) 
				{
					if(in_size <= 0)
					{
						// cant have negative size
						this->size = 0;

						// If the size is 0, zero out the buffer
						this->pBuffer = 0;
					}
					else
					{
						this->size = in_size;

						// else, create the buffer and zero out to the size it shoudl be
						this->pBuffer = new uchar[this->size];
						memset(this->pBuffer, 0, this->size);
					}

					this->opcode = in_opcode; // need checking to ensure that its an expected opcode
				}


				// Size of OpCode
				int32  size;

				// Opcode
				int16  opcode;

				// Buffer of packet
				uchar* pBuffer;

				~APPLAYER() 
				{
					// delete the buffer
					SAFE_DELETE(pBuffer); 
				}
			};
		}
	}
}
#endif