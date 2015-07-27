EQClassic source code from January 1st, 2010

Build Dependencies
---
- MySQL 5.1 (http://dev.mysql.com/downloads/mysql/5.1.html)
  - Install with Developer option
- ActivePerl 5.16 (http://eqemu.github.io/downloads/ActivePerl-5.16.3.1604-MSWin32-x86-298023.msi)
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
  - C:\perl\lib\Core