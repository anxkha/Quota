libpath = /LIBPATH:"C:\Program Files (x86)\Microsoft SDKs\Windows\v7.0A\Lib" /LIBPATH:"C:\Program Files (x86)\Microsoft Visual Studio 10.0\VC\lib"
libpath64 = /LIBPATH:"C:\Program Files (x86)\Microsoft SDKs\Windows\v7.0A\Lib\x64" /LIBPATH:"C:\Program Files (x86)\Microsoft Visual Studio 10.0\VC\lib\amd64"
cl = "C:\Program Files (x86)\Microsoft Visual Studio 10.0\VC\bin\cl.exe"
cl64 = "C:\Program Files (x86)\Microsoft Visual Studio 10.0\VC\bin\amd64\cl.exe"
link = "C:\Program Files (x86)\Microsoft Visual Studio 10.0\VC\bin\link.exe"
link64 = "C:\Program Files (x86)\Microsoft Visual Studio 10.0\VC\bin\amd64\link.exe"
clparam = /nologo /EHsc /c /D "_CONSOLE" /D "WIN32" /D "NDEBUG" /D "_UNICODE" /D "UNICODE" /Zc:wchar_t /Zc:forScope /Gd /GS /Gy /GL /W3 /O2 /Oi
linkparam = /LTCG /RELEASE /SUBSYSTEM:CONSOLE /NOLOGO /MACHINE:X86 /DYNAMICBASE /NXCOMPAT /OPT:REF /OPT:ICF /ALLOWISOLATION /INCREMENTAL:NO $(libpath)
linkparam64 = /LTCG /RELEASE /SUBSYSTEM:CONSOLE /NOLOGO /MACHINE:X64 /DYNAMICBASE /NXCOMPAT /OPT:REF /OPT:ICF /ALLOWISOLATION /INCREMENTAL:NO $(libpath64)
objs = quota.obj
objs64 = quota64.obj
libs = kernel32.lib advapi32.lib
target = quota.exe
target64 = quota64.exe

all: $(target) $(target64)

x64: $(target64)

clean:
	del $(objs) $(objs64) $(target) $(target64)

quota.obj: quota.cpp
	$(cl) $(clparam) /Fo"$@" quota.cpp

quota64.obj: quota.cpp
	$(cl64) $(clparam) /Fo"$@" quota.cpp

$(target): $(objs)
	$(link) $(linkparam) /OUT:$(target) $(libs) $**

$(target64): $(objs64)
	$(link64) $(linkparam64) /OUT:$(target64) $(libs) $**
