@echo off
      
call "C:\Program Files (x86)\Microsoft Visual Studio 14.0\VC\vcvarsall.bat" x64
     
pushd ..\build

set CompilerFlags=-MT -EHa- -FC -nologo -Z7 -W4 -WX -Od -wd4100 -wd4127 -wd4505  -wd4201 -wd4189 -wd4996 -wd4244 -wd4267 -wd4456 -wd4311 -wd4301 -wd4311 -wd4302 -wd4312 -wd4477 -wd4838 -wd4701
set LinkerFlags=-incremental:no
cl  %CompilerFlags% ..\code\main.cpp /link %LinkerFlags%

popd
    