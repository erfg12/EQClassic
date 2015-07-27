# Microsoft Developer Studio Project File - Name="Login" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Console Application" 0x0103

CFG=Login - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "Login.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "Login.mak" CFG="Login - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "Login - Win32 Release" (based on "Win32 (x86) Console Application")
!MESSAGE "Login - Win32 Debug" (based on "Win32 (x86) Console Application")
!MESSAGE "Login - Win32 MiniLogin" (based on "Win32 (x86) Console Application")
!MESSAGE "Login - Win32 PublicLogin" (based on "Win32 (x86) Console Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "Login - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "../Build"
# PROP Intermediate_Dir "../Build/Login"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /c
# ADD CPP /nologo /MT /w /W0 /GX /Zi /O2 /Ob2 /D "LOGINCRYPTO" /D "INVERSEXY" /D _WIN32_WINNT=0x0400 /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /FR /YX /FD /c
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo /o"../Build/Login/Login.bsc"
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib wsock32.lib zlib.lib mysqlclient.lib /nologo /subsystem:console /map:"../Build/Login.map" /debug /machine:I386
# SUBTRACT LINK32 /pdb:none

!ELSEIF  "$(CFG)" == "Login - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Login___Win32_Debug"
# PROP BASE Intermediate_Dir "Login___Win32_Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "../build/login/Debug"
# PROP Intermediate_Dir "../build/login/debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /GZ /c
# ADD CPP /nologo /MTd /Gm /GX /ZI /Od /D "LOGINCRYPTO" /D "INVERSEXY" /D _WIN32_WINNT=0x0400 /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_MBCS" /FR /YX /FD /GZ /c
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /debug /machine:I386 /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib wsock32.lib zlib.lib mysqlclient.lib /nologo /subsystem:console /debug /machine:I386 /nodefaultlib:"LIBCMT" /out:"../build/login/Debug/LoginDebug.exe" /pdbtype:sept
# SUBTRACT LINK32 /pdb:none

!ELSEIF  "$(CFG)" == "Login - Win32 MiniLogin"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Login___Win32_MiniLogin"
# PROP BASE Intermediate_Dir "Login___Win32_MiniLogin"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "../Build"
# PROP Intermediate_Dir "../Build/MiniLogin"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MT /w /W0 /GX /O2 /Ob2 /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /D "BUILD_FOR_WINDOWS" /FR /YX /FD /c
# ADD CPP /nologo /MT /w /W0 /GX /O2 /Ob2 /D _WIN32_WINNT=0x0400 /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /D "MINILOGIN" /FR /YX /FD /c
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo /o"../Build/Login/Login.bsc"
# ADD BSC32 /nologo /o"../Build/MiniLogin/Login.bsc"
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib wsock32.lib zlib.lib mysqlclient.lib /nologo /subsystem:console /machine:I386
# SUBTRACT BASE LINK32 /pdb:none
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib wsock32.lib zlib.lib mysqlclient.lib /nologo /subsystem:console /machine:I386 /out:"../Build/MiniLogin.exe"
# SUBTRACT LINK32 /pdb:none

!ELSEIF  "$(CFG)" == "Login - Win32 PublicLogin"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Login___Win32_PublicLogin"
# PROP BASE Intermediate_Dir "Login___Win32_PublicLogin"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "../Build"
# PROP Intermediate_Dir "../Build/PublicLogin"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MT /w /W0 /GX /O2 /Ob2 /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /D "BUILD_FOR_WINDOWS" /FR /YX /FD /c
# ADD CPP /nologo /MT /w /W0 /GX /O2 /Ob2 /D _WIN32_WINNT=0x0400 /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /D "PUBLICLOGIN" /FR /YX /FD /c
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo /o"../Build/Login/Login.bsc"
# ADD BSC32 /nologo /o"../Build/Login/Login.bsc"
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib wsock32.lib zlib.lib mysqlclient.lib /nologo /subsystem:console /machine:I386
# SUBTRACT BASE LINK32 /pdb:none
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib wsock32.lib zlib.lib mysqlclient.lib /nologo /subsystem:console /machine:I386 /out:"../Build/PublicLogin.exe"
# SUBTRACT LINK32 /pdb:none

!ENDIF 

# Begin Target

# Name "Login - Win32 Release"
# Name "Login - Win32 Debug"
# Name "Login - Win32 MiniLogin"
# Name "Login - Win32 PublicLogin"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\client.cpp
# End Source File
# Begin Source File

SOURCE=.\EQCrypto.cpp

!IF  "$(CFG)" == "Login - Win32 Release"

!ELSEIF  "$(CFG)" == "Login - Win32 Debug"

!ELSEIF  "$(CFG)" == "Login - Win32 MiniLogin"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "Login - Win32 PublicLogin"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\logindatabase.cpp

!IF  "$(CFG)" == "Login - Win32 Release"

!ELSEIF  "$(CFG)" == "Login - Win32 Debug"

!ELSEIF  "$(CFG)" == "Login - Win32 MiniLogin"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "Login - Win32 PublicLogin"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\LWorld.cpp
# End Source File
# Begin Source File

SOURCE=.\net.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\client.h
# End Source File
# Begin Source File

SOURCE=.\EQCrypto.h
# End Source File
# Begin Source File

SOURCE=.\login_opcodes.h
# End Source File
# Begin Source File

SOURCE=.\login_structs.h
# End Source File
# Begin Source File

SOURCE=.\LWorld.h
# End Source File
# Begin Source File

SOURCE=.\net.h
# End Source File
# End Group
# Begin Group "Common Source Files"

# PROP Default_Filter ".cpp"
# Begin Source File

SOURCE=..\common\crc32.cpp
# End Source File
# Begin Source File

SOURCE=..\common\database.cpp

!IF  "$(CFG)" == "Login - Win32 Release"

!ELSEIF  "$(CFG)" == "Login - Win32 Debug"

!ELSEIF  "$(CFG)" == "Login - Win32 MiniLogin"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "Login - Win32 PublicLogin"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\common\dbcore.cpp
# End Source File
# Begin Source File

SOURCE=..\common\DBMemLeak.cpp
# End Source File
# Begin Source File

SOURCE=..\common\debug.cpp
# End Source File
# Begin Source File

SOURCE=..\common\EQNetwork.cpp
# End Source File
# Begin Source File

SOURCE=..\common\md5.cpp
# End Source File
# Begin Source File

SOURCE=..\common\MiscFunctions.cpp
# End Source File
# Begin Source File

SOURCE=..\common\Mutex.cpp
# End Source File
# Begin Source File

SOURCE=..\common\packet_dump.cpp
# End Source File
# Begin Source File

SOURCE=..\common\packet_functions.cpp
# End Source File
# Begin Source File

SOURCE=..\common\TCPConnection.cpp
# End Source File
# Begin Source File

SOURCE=..\common\timer.cpp
# End Source File
# End Group
# Begin Group "Common Header Files"

# PROP Default_Filter ".h"
# Begin Source File

SOURCE=..\common\classes.h
# End Source File
# Begin Source File

SOURCE=..\common\crc32.h
# End Source File
# Begin Source File

SOURCE=..\common\database.h
# End Source File
# Begin Source File

SOURCE=..\common\DBMemLeak.h
# End Source File
# Begin Source File

SOURCE=..\common\debug.h
# End Source File
# Begin Source File

SOURCE=..\common\deity.h
# End Source File
# Begin Source File

SOURCE=..\common\eq_opcodes.h
# End Source File
# Begin Source File

SOURCE=..\common\eq_packet_structs.h
# End Source File
# Begin Source File

SOURCE=..\common\EQCheckTable.h
# End Source File
# Begin Source File

SOURCE=..\common\EQFragment.h
# End Source File
# Begin Source File

SOURCE=..\common\EQNetwork.h
# End Source File
# Begin Source File

SOURCE=..\common\EQOpcodes.h
# End Source File
# Begin Source File

SOURCE=..\common\EQPacket.h
# End Source File
# Begin Source File

SOURCE=..\common\EQPacketManager.h
# End Source File
# Begin Source File

SOURCE=..\common\errmsg.h
# End Source File
# Begin Source File

SOURCE=..\common\Guilds.h
# End Source File
# Begin Source File

SOURCE=..\common\linked_list.h
# End Source File
# Begin Source File

SOURCE=..\common\md5.h
# End Source File
# Begin Source File

SOURCE=..\common\MiscFunctions.h
# End Source File
# Begin Source File

SOURCE=..\common\moremath.h
# End Source File
# Begin Source File

SOURCE=..\common\Mutex.h
# End Source File
# Begin Source File

SOURCE=..\common\packet_dump.h
# End Source File
# Begin Source File

SOURCE=..\common\packet_dump_file.h
# End Source File
# Begin Source File

SOURCE=..\common\packet_functions.h
# End Source File
# Begin Source File

SOURCE=..\common\queue.h
# End Source File
# Begin Source File

SOURCE=..\common\queues.h
# End Source File
# Begin Source File

SOURCE=..\common\races.h
# End Source File
# Begin Source File

SOURCE=..\common\Seperator.h
# End Source File
# Begin Source File

SOURCE=..\common\servertalk.h
# End Source File
# Begin Source File

SOURCE=..\common\TCPConnection.h
# End Source File
# Begin Source File

SOURCE=..\common\timer.h
# End Source File
# Begin Source File

SOURCE=..\common\types.h
# End Source File
# Begin Source File

SOURCE=..\common\version.h
# End Source File
# End Group
# Begin Group "Text Files"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\Protocol.txt
# End Source File
# Begin Source File

SOURCE=.\Tables.txt
# End Source File
# Begin Source File

SOURCE=.\ThanksTo.txt
# End Source File
# End Group
# End Target
# End Project
