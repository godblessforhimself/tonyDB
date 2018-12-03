indexing="./"
result="result"
bitmap="../filesystem/utils/MyBitMap.cpp"
mkdir build
g++ -std=c++11 -c indexing_test.cpp -o build/test.o
g++ -std=c++11 -c ${indexing}indexing.cpp -o build/indexing.o
g++ -std=c++11 -c ${bitmap} -o build/bitmap.o
g++ -std=c++11 -c ../const.cpp -o build/const.o
g++ -std=c++11 build/const.o build/test.o build/indexing.o build/bitmap.o -o indexing
echo "执行结果 > ${result}"
./indexing > $result
#g++ -E rmtest.cpp -o build/rmtest.ii
#g++ -E rm.cpp -o build/rm.ii