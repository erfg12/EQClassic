rmdir /s /q "Build"
rmdir /s /q "LS\build"
rmdir /s /q "Zone\Build"
rmdir /s /q "World\Build"
rmdir /s /q "World\Intermediate"
rmdir /s /q "SharedMemory\Intermediate"
rmdir /s /q "Zone\Intermediate"
del /S /F /AH *.suo
del /S /F /AH logfile.txt