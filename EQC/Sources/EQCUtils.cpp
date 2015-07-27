// ***************************************************************
//  EQCUtils   ·  date: 26/09/2007
//  -------------------------------------------------------------
//  Copyright (C) 2007 - All Rights Reserved
// ***************************************************************
// 
// ***************************************************************
#include "Logger.h"
#include <cstdarg>
#include <sstream>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include "windows.h"
#include "config.h"
#include "EQCUtils.hpp"
#include "EQCException.hpp"


using namespace std;

namespace EQC 
{
	namespace Common
	{

		string GetPlace(CodePlace Place) {
			switch(Place) {
				case CP_DATABASE:
					return "[  DB  ] ";
				case CP_CLIENT:
					return "[Client] ";
				case CP_SPELL:
					return "[Spell] ";
				case CP_WORLDSERVER:
					return "[World ] ";
				case CP_ZONESERVER: {
					return "[ Zone ] ";
				case CP_ATTACK:
					return "[Attack] ";
				case CP_UPDATES:
					return "[Updates] ";
				case CP_MERCHANT:
					return "[Merchant] ";
				case CP_ITEMS:
					return "[Item] ";
				case CP_PATHING:
					return "[Pathing] ";
				case CP_SHAREDMEMORY:
					return "[SharedMem] ";
				case CP_QUESTS:
					return "[Quests] ";
				default: break;
				}
			}
			return "[Common] ";
		}

		void PrintF(CodePlace Place, const char* pText, ...)
		{
		
			if (!pText) 
			{
				EQC_EXCEPT("EQC::Common::PrintF", "pText = NULL");
			}		

			va_list Args;

			va_list args;
			va_start(args, pText);		

			// Harakiri console will be handled by logger if EQDEBUG = 3
			LogFile->writePVA(EQCLog::Debug, GetPlace(Place).c_str(), pText,args);
			va_end(args);
		}


		void Log(EQCLog::LogIDs id, CodePlace Place, const char* pText, ...)
		{
		
			if (!pText) 
			{
				EQC_EXCEPT("EQC::Common::Log", "pText = NULL");
			}

			va_list Args;

			va_list args;
			va_start(args, pText);		

			// Harakiri console will be handled by logger if EQDEBUG = 3
			LogFile->writePVA(id, GetPlace(Place).c_str(), pText,args);
			va_end(args);
		}

		string ToString(const char* pText, ...) 
		{
			const int32 TAM_MAX = 2056;
			if (!pText) EQC_EXCEPT("string ToString()", "pText = NULL");
			int32 Len = strlen(pText);
			if (Len > TAM_MAX) EQC_EXCEPT("string ToString()", "Length is too big");
			va_list Args;
			char* Buffer = new char[Len*10];
			memset(Buffer, 0, Len*10);
			va_start(Args, pText);
			vsnprintf(Buffer, Len*10, pText, Args);
			va_end(Args);
			string NewString = Buffer;
			safe_delete_array(Buffer);//delete[] Buffer;
			return NewString;
		}



		string ToString(const double x){
			std::ostringstream o;
			if (!(o << x)) return STRING_ERROR;
			return o.str();
		}
		string ToString(const sint16 x){
			std::ostringstream o;
			if (!(o << x)) return STRING_ERROR;
			return o.str();
		}


		void CloseSocket(int socket)
		{
#ifdef WIN32
			closesocket(socket);
#else
			close(socket);
#endif
		}



		int GetLastSocketError()
		{
			int i = 0;
#ifdef WIN32
			i = WSAGetLastError();
#else
			i = strerror(errno);
#endif
			return i;
		}

		string ReadKey(string FileName, string Key){
			fstream File(FileName.c_str(), fstream::in);
			if(File.fail()){
				return "*ERROR*";
			}
			char Tmp[256];
			while(!File.eof()){
				memset(Tmp, 0, sizeof(Tmp));
				File.getline(Tmp, sizeof(Tmp));
				string Linea = Tmp;
				string::size_type Posicion = Linea.find(Key.c_str());
				if(Posicion != string::npos)
					return Linea.substr(Posicion + Key.length() + 1, Linea.length() - Posicion - Key.length() - 1);
				//Yeahlight: Zone freeze debug
				if(0 && rand()%0 == 1)
					EQC_FREEZE_DEBUG(__LINE__, __FILE__);
			}
			return "*ERROR*";
		}



		bool IsAlphaNumericStringOnly(char* str)
		{
			for (int i = 0; i <strlen(str); i++)
			{
				if ((str[i] < 'a' || str[i] > 'z') && (str[i] < 'A' || str[i] > 'Z') && (str[i] < '0' || str[i] > '9'))
				{
					return false;
				}
			}

			return true;
		}

#ifdef WIN32
		char Ver_name[100];
		DWORD Ver_build, Ver_min, Ver_maj, Ver_pid;

		const char* GetOSName()
		{
			strcpy(Ver_name, "Unknown operating system");

			OSVERSIONINFO Ver_os;
			Ver_os.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);

			if(!(GetVersionEx(&Ver_os))) 
			{
				return Ver_name;
			}

			Ver_build = Ver_os.dwBuildNumber & 0xFFFF;
			Ver_min = Ver_os.dwMinorVersion;
			Ver_maj = Ver_os.dwMajorVersion;
			Ver_pid = Ver_os.dwPlatformId;

			if ((Ver_pid == 1) && (Ver_maj == 4))
			{
				if ((Ver_min < 10) && (Ver_build == 950))
				{
					strcpy(Ver_name, "Microsoft Windows 95");
				}
				else if ((Ver_min < 10) && 
					((Ver_build > 950) && (Ver_build <= 1080)))
				{
					strcpy(Ver_name, "Microsoft Windows 95 SP1");
				}
				else if ((Ver_min < 10) && (Ver_build > 1080))
				{
					strcpy(Ver_name, "Microsoft Windows 95 OSR2");
				}
				else if ((Ver_min == 10) && (Ver_build == 1998))
				{
					strcpy(Ver_name, "Microsoft Windows 98");
				}
				else if ((Ver_min == 10) && 
					((Ver_build > 1998) && (Ver_build < 2183)))
				{
					strcpy(Ver_name, "Microsoft Windows 98, Service Pack 1");
				}
				else if ((Ver_min == 10) && (Ver_build >= 2183))
				{
					strcpy(Ver_name, "Microsoft Windows 98 Second Edition");
				}
				else if (Ver_min == 90)
				{
					strcpy(Ver_name, "Microsoft Windows ME");
				}
			}
			else if (Ver_pid == 2)
			{
				if ((Ver_maj == 3) && (Ver_min == 51))
				{
					strcpy(Ver_name, "Microsoft Windows NT 3.51");
				}
				else if ((Ver_maj == 4) && (Ver_min == 0))
				{
					strcpy(Ver_name, "Microsoft Windows NT 4");
				}
				else if ((Ver_maj == 5) && (Ver_min == 0))
				{
					strcpy(Ver_name, "Microsoft Windows 2000");
				}
				else if ((Ver_maj == 5) && (Ver_min == 1))
				{
					strcpy(Ver_name, "Microsoft Windows XP");
				}
				else if ((Ver_maj == 5) && (Ver_min == 2))
				{
					strcpy(Ver_name, "Microsoft Windows 2003");
				}
			}

			return Ver_name;
		}	


		bool IsRunningOnWindowsServer()
		{
			OSVERSIONINFO Ver_os;
			Ver_os.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);

			if(!(GetVersionEx(&Ver_os))) 
			{
				return false;  // if wecant get the OS ver, bail out!
			}

			Ver_build = Ver_os.dwBuildNumber & 0xFFFF;
			Ver_min = Ver_os.dwMinorVersion;
			Ver_maj = Ver_os.dwMajorVersion;
			Ver_pid = Ver_os.dwPlatformId;

			bool ret = false; // default to false to ensure it must pass

			if(Ver_pid == 2)
			{
				if ((Ver_maj == 3) && (Ver_min == 51))
				{
					ret = true;
				}
				else if ((Ver_maj == 4) && (Ver_min == 0))
				{
					ret = true;
				}
				else if ((Ver_maj == 5) && (Ver_min == 0))
				{
					ret = true;
				}
				else if ((Ver_maj == 5) && (Ver_min == 1))
				{
					ret = true;
				}
				else if ((Ver_maj == 5) && (Ver_min == 2))
				{
					ret = true;
				}
			}
			else
			{
				ret =  false;
			}

			return ret;
		}
#endif
	}
} // end of namespace

#ifdef GETTIME
void GetCurrentTime()
{
	time_t t = time(0);
	struct tm *lt = localtime(&t);
	std::cout << std::setfill('0');
	std::cout << std::setw(4) << lt->tm_year + 1900
		<< std::setw(2) << lt->tm_mon + 1
		<< std::setw(2) << lt->tm_mday
		<< std::setw(2) << lt->tm_hour
		<< std::setw(2) << lt->tm_min
		<< std::setw(2) << lt->tm_sec
		<< std::endl;
}
#endif

