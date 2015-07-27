// ***************************************************************
//  EQCException   ·  date: 26/09/2007
//  -------------------------------------------------------------
//  Copyright (C) 2007 - All Rights Reserved
// ***************************************************************
// 
// ***************************************************************
#ifdef WIN32
	#include <Windows.h>
#endif
#include <cstdio>
#include <fstream>
#include "EQCUtils.hpp"
#include "EQCException.hpp"
using namespace std;

namespace EQC 
{
	namespace Common
	{
		//---------------------------------------------------------------------------------------------
		EQCException::EQCException(string Function, string Description, int32 Line, const char* File)
		{
			mFunction = Function;
			mDescription = Description;
			mLine = Line;
			mFile = File;
			this->mMsg =				"--------------------------------------------------\n";
			this->mMsg += EQC::Common::ToString("Exception occurred in %s:%i!\n", File, Line);
			this->mMsg += EQC::Common::ToString("-Function: %s\n", Function.c_str());
			this->mMsg += EQC::Common::ToString("-Description: %s\n\n", Description.c_str());
		}

		//---------------------------------------------------------------------------------------------

		EQCException::EQCException(string PlayerName, string Function, string Description, int32 Line, const char* File){
			mPlayerName = PlayerName;
			mFunction = Function;
			mDescription = Description;
			mLine = Line;
			mFile = File;
			this->mMsg =				"--------------------------------------------------\n";
			this->mMsg += EQC::Common::ToString("Exception occurred in %s:%i!\n", File, Line);
			this->mMsg += EQC::Common::ToString("-Playername: \"%s\"\n", PlayerName.c_str());
			this->mMsg += EQC::Common::ToString("-Function: %s\n", Function.c_str());
			this->mMsg += EQC::Common::ToString("-Description: %s\n\n", Description.c_str());
		}

		//---------------------------------------------------------------------------------------------

		void EQCException::PopException(){
			#ifdef WIN32
				MessageBoxA(HWND_DESKTOP, this->mMsg.c_str(), "EQC Exception", MB_OK);
			#else
				printf("%s\n", mMsg.c_str());
			#endif
		}

		//---------------------------------------------------------------------------------------------

		void EQCException::FileDumpException(const string& FileName){
			fstream File (FileName.c_str(), fstream::in | fstream::out);
			if (File.fail())
				File.open(FileName.c_str(), fstream::out);
			else
				File.seekg(0, fstream::end); File.seekp(0, fstream::end);
			File.write(this->mMsg.c_str(), this->mMsg.length());
			File.close();
		}

		//---------------------------------------------------------------------------------------------

		void EQCException::WriteZoneFreezeDebugToFile(int Line, const char* File)
		{
			ofstream myfile;
			myfile.open("ZoneLoopDebug.txt", ios::app);
			myfile<<"File: "<<File<<" | Line: "<<Line<<endl;
			myfile.close();
		}

		//---------------------------------------------------------------------------------------------
	}
}