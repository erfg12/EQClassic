// ***************************************************************
//  APPlayer   ·  date: 17/05/2007
//  -------------------------------------------------------------
//  Copyright (C) 2007 - All Rights Reserved
// ***************************************************************
// 
// ***************************************************************
#ifndef ACK_INFO_H
#define ACK_INFO_H

namespace EQC
{
	namespace Common
	{
		namespace Network
		{
			// Contain ack stuff
			struct ACK_INFO
			{
				ACK_INFO() 
				{
					// Set properties to 0
					dwARQ = 0;
					dbASQ_high = dbASQ_low = 0;
					dwGSQ = 0; 
				}

				int16   dwARQ;			// Comment: Current request ack
				int16   dbASQ_high : 8;	//TODO: What is this one?
				int16	dbASQ_low  : 8;	//TODO: What is this one?
				int16   dwGSQ;			// Comment: Main sequence number SHORT#2

			};
		}
	}
}

#endif