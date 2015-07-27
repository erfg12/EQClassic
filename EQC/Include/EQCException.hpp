// ***************************************************************
//  EQCException   ·  date: 09/26/2007
//  -------------------------------------------------------------
//  Copyright (C) 2007 - All Rights Reserved
// ***************************************************************
// 
// ***************************************************************
#ifndef EQCEXCEPTION_HPP
#define EQCEXCEPTION_HPP
#ifdef WIN32
	#include <Windows.h>
#endif
#include <string>
#include <fstream>
#include "types.h"
using namespace std;
#define EQC_EXCEPT(Function, Description) throw EQC::Common::EQCException(Function, Description, __LINE__, __FILE__)
#define EQC_MOB_EXCEPT(Function, Description) { if(this->IsClient()) { this->CastToClient()->Message(BLACK, "Problem found: Disconnecting you!"); Sleep(50); } throw EQC::Common::EQCException(this->GetName(), Function, Description, __LINE__, __FILE__); }
#define EQC_FREEZE_DEBUG(line, file) { ofstream myfile; myfile.open("ZoneLoopDebug.txt", ios::app); myfile<<"File: "<<file<<" | Line: "<<line<<endl; myfile.close(); }

namespace EQC
{
	namespace Common
	{
		class EQCException
		{
		public:
			EQCException(string Function, string Description, int32 Line, const char* File);
			EQCException(string PlayerName, string Function, string Description, int32 Line, const char* File);
			~EQCException() {}

			void PopException();
			void FileDumpException(const string& FileName);
			void WriteZoneFreezeDebugToFile(int Line, const char* File);

		private:
			string		mFunction, mDescription, mFile, mMsg, mPlayerName;
			int32		mLine;
		};
	}
}
#endif