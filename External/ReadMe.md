boost安装说明

boost1.6.3
vs2015
windows 10

b2 --build-dir="E:\GitRepo\BoostDemo\boost_1_63_0\boost-build-32" --build-type=complete toolset=msvc-14.0 --stagedir="E:\GitRepo\BoostDemo\boost_1_63_0\stage\lib\x86"

b2 --build-dir="E:\GitRepo\BoostDemo\boost_1_63_0\boost-build-64" --build-type=complete toolset=msvc-14.0 address-model=64  --stagedir="E:\GitRepo\BoostDemo\boost_1_63_0\stage\lib\x64"