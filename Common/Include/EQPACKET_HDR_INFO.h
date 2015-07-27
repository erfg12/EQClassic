// ***************************************************************
//  EQPACKET_HDR_INFO   ·  date: 17/05/2007
//  -------------------------------------------------------------
//  Copyright (C) 2007 - All Rights Reserved
// ***************************************************************
// 
// ***************************************************************
#ifndef EQPACKET_HDR_INFO_H
#define EQPACKET_HDR_INFO_H

namespace EQC
{
	namespace Common
	{
		namespace Network
		{
			/************ PACKETS ************/
			struct EQPACKET_HDR_INFO
			{
				int8 a0_Unknown  :   1; //TODO: What is this one?
				int8 a1_ARQ      :   1; //TODO: What is this one?
				int8 a2_Closing  :   1; //TODO: What is this one?
				int8 a3_Fragment :   1; //TODO: What is this one?
				int8 a4_ASQ      :   1; //TODO: What is this one?
				int8 a5_SEQStart :   1; //TODO: What is this one?
				int8 a6_Closing  :   1; //TODO: What is this one?
				int8 a7_SEQEnd   :   1; //TODO: What is this one?
				int8 b0_SpecARQ  :   1; //TODO: What is this one?
				int8 b1_Unknown  :   1; //TODO: What is this one?
				int8 b2_ARSP     :   1; //TODO: What is this one?
				int8 b3_Unknown  :   1; //TODO: What is this one?
				int8 b4_Unknown  :   1; //TODO: What is this one?
				int8 b5_Unknown  :   1; //TODO: What is this one?
				int8 b6_Unknown  :   1; //TODO: What is this one?
				int8 b7_Unknown  :   1; //TODO: What is this one?
			};
		}
	}
}
#endif