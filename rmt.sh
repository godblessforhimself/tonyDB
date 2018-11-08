g++ -c rmtest.cpp -o build/rmtest.o
g++ -c rm.cpp -o build/rm.o
g++ build/rmtest.o build/rm.o -o rm
echo '执行结果 > rm_result.txt'
./rm > rm_result.txt
#g++ -E rmtest.cpp -o build/rmtest.ii
#g++ -E rm.cpp -o build/rm.ii