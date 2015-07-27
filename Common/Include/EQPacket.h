// ***************************************************************
//  EQCException   ·  date: 21/12/2007
//  -------------------------------------------------------------
//  Copyright (C) 2007 - All Rights Reserved
// ***************************************************************
// EQPacket.h: interface for the EQPacket class.
// ***************************************************************

#ifndef EQPACKET_H
#define EQPACKET_H

#include <cstring>
#include "types.h"
#include "eq_opcodes.h"
#include "EQPACKET_HDR_INFO.h"
#include "FRAGMENT_INFO.h"

using namespace EQC::Common::Network;

/************ DEFINES ************/           
// Agz: Added 2 to max_header_size, out of memory when first packet was a fragment packet
#define MAX_HEADER_SIZE 18		//TODO: What is this one?
#define ACK_ONLY_SIZE   6		//TODO: What is this one?
#define ACK_ONLY		0x400	//TODO: What is this one?
/*****************************************************************************/


namespace EQC
{
	namespace Common
	{
		namespace Network
		{
			class EQPacket  
			{
			public:
				// Constructor
				EQPacket();

				// Destructor
				~EQPacket();

				void  DecodePacket(int16 length, uchar *pPacket);
				uchar* ReturnPacket(int16 *dwLength);

				void AddAdditional(int size, uchar *pAdd) 
				{   
					// if pExtra exsists, delete it
					if(!this->pExtra)
					{
						safe_delete_array(this->pExtra);//delete[] this->pExtra;
					}

					// create a new instance
					this->pExtra = new uchar[size];

					// copy the size of pAdd to pExtra
					memcpy( (void*)this->pExtra, (void*) pAdd, size);
				}       

				void Clear(void) 
				{  
					// Clear & zero out Fields
					*((int16*)&HDR)		   = 0;
					this->dwSEQ            = 0;        
					this->dwARSP           = 0;
					this->dwARQ            = 0;
					this->dbASQ_low        = 0;        
					this->dbASQ_high       = 0;
					this->dwOpCode         = 0;    
					this->fraginfo.dwCurr  = 0;
					this->fraginfo.dwSeq   = 0;
					this->fraginfo.dwTotal = 0;
					this->dwExtraSize      = 0;
					this->pExtra           = 0;
					this->resend_count	   = 0;
				}


				EQPACKET_HDR_INFO   HDR;				//TODO: What is this one?
				int16				dwSEQ;				// Sequence number
				int16				dwARSP;				//TODO: What is this one?
				int16				dwARQ;				//TODO: What is this one?
				int16				dbASQ_high : 8;		//TODO: What is this one?
				int16				dbASQ_low  : 8;		//TODO: What is this one?
				int16				dwOpCode;			//Not all packet have opcodes. 
				FRAGMENT_INFO		fraginfo;			//Fragment info
				int16				dwExtraSize;		//Size of additional info.
				uchar				*pExtra;			//Additional information
				int8				resend_count;		// Quagmire: Moving resend count to a packet by packet basis

				// Quagmire: Made the CRC stuff static and public. Makes things easier elsewhere.
				static int32 GenerateCRC(int32 b, int32 bufsize, uchar *buf);
			private:
				static int32 RoL(int32 in, int32 bits);
				static int32 CRCLookup(uchar idx);
			};
		}
	}
}
#endif
