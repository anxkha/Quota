@ECHO OFF

SET LIB=C:\Program Files (x86)\Microsoft SDKs\Windows\v7.0A\Lib;C:\Program Files (x86)\Microsoft Visual Studio 10.0\VC\lib
cl.exe /nologo /EHsc /c /D "_UNICODE" /D "UNICODE" /Zc:wchar_t /W3 /O2 /Oi /Fo"quota.obj" quota.cpp
link.exe /SUBSYSTEM:CONSOLE /NOLOGO /INCREMENTAL:NO /OUT:quota.exe quota.obj

SET LIB=C:\Program Files (x86)\Microsoft SDKs\Windows\v7.0A\Lib\x64;C:\Program Files (x86)\Microsoft Visual Studio 10.0\VC\lib\amd64
"C:\program files (x86)\microsoft visual studio 10.0\vc\bin\amd64\cl.exe" /nologo /EHsc /c /D "_UNICODE" /D "UNICODE" /Zc:wchar_t /W3 /O2 /Oi /Fo"quota64.obj" quota.cpp
"C:\program files (x86)\microsoft visual studio 10.0\vc\bin\amd64\link.exe" /SUBSYSTEM:CONSOLE /NOLOGO /MACHINE:X64 /INCREMENTAL:NO /OUT:quota64.exe quota64.obj