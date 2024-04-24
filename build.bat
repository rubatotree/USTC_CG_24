cd .\Framework3D\build\
cmake --build . --target=engine_test --parallel=32
cd ..\Binaries\Debug\
.\engine_test.exe