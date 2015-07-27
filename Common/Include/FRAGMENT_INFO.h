// ***************************************************************
//  FRAGMENT_INFO   ·  date: 17/05/2007
//  -------------------------------------------------------------
//  Copyright (C) 2007 - All Rights Reserved
// ***************************************************************
// 
// ***************************************************************
#ifndef FRAGMENT_INFO_H
#define FRAGMENT_INFO_H

namespace EQC
{
	namespace Common
	{
		namespace Network
		{
			struct FRAGMENT_INFO
			{
				int16 dwSeq;	//TODO: What is this one?
				int16 dwCurr;	//TODO: What is this one?
				int16 dwTotal;	//TODO: What is this one?
			};
		}
	}
}
#endif