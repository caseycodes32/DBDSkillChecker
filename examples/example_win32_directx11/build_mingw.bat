@REM Build for Visual Studio compiler. Run your copy of vcvars32.bat or vcvarsall.bat to setup command-line compiler.
@set OUT_DIR=Debug
@set OUT_EXE=example_win32_directx11
@set INCLUDES=-I imgui/ -I imgui/backends
@set SOURCES=main.cpp imgui\backends\imgui_impl_dx11.cpp imgui\backends\imgui_impl_win32.cpp imgui\imgui*.cpp
@set LIBS=-ld3d11 -ld3dcompiler -ldwmapi
mkdir %OUT_DIR%
g++ -DUNICODE %INCLUDES% %SOURCES% -g -o %OUT_DIR%/%OUT_EXE%.exe --static -mwindows %LIBS% %LIBS%


pause
