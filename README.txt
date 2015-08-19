EQClassic source code from January 1st, 2010

Build Dependencies
---
- MySQL 5.1 (http://dev.mysql.com/downloads/mysql/5.1.html)
  - Install with Developer option
- ActivePerl 5.12 (http://eqemu.github.io/downloads/ActivePerl-5.12.3.1204-MSWin32-x86-294330.msi)
- OpenSSL (This is bundled with project, in Dependencies folder)
- zlib (in Dependencies folder)


Build Setup
---
You may need to make updates the the location of includes and lib folders for MySQL and ActivePerl if they are not in the default locations:
- MySQL
  - C:\Program Files (x85)\MySQL\MySQL Server 5.1\include
  - C:\Program Files (x85)\MySQL\MySQL Server 5.1\lib
  - C:\Program Files (x85)\MySQL\MySQL Server 5.1\lib\debug
- ActivePerl
  - C:\Perl512\lib\Core

The source assumes your perl 5.12 core is installed in the C:\Perl512 directory. Please install Perl to this directory.
  
Known Issues
---
- You have to compile this source using ActivePerl 5.12 but the server needs 5.14 (x64) for quests to work. Just rename the Perl514.dll to Perl512.dll when running the server.
- NPCs will not spawn without the map files being present in the /maps/maps folder.
- NPCs will not roam unless specified in the database their roam range, and nodes placed in game.
