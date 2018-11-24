recordmanager="../../recordmanager/"
result="rmtestresult"
bitmap="../../filesystem/utils/MyBitMap.cpp"
mkdir build
g++ -c rmtest.cpp -o build/rmtest.o
g++ -c ${recordmanager}rm.cpp -o build/rm.o
g++ -c ${bitmap} -o build/bitmap.o
g++ build/rmtest.o build/rm.o build/bitmap.o -o rm
echo "执行结果 > ${result}"
./rm > $result
#g++ -E rmtest.cpp -o build/rmtest.ii
#g++ -E rm.cpp -o build/rm.ii