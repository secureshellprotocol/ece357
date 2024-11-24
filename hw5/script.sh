#!/bin/bash
gcc ./test1.c -o ./test1.out
gcc ./test2.c -o ./test2.out
gcc ./test3.c -o ./test3.out
gcc ./test4.c -o ./test4.out

./test1.out
result=$?
case $result in
    1) echo "Test #1 (0 -> Pass, 255 -> Fail, 254 -> Error, Otherwise Signal):" $result;;
    254) echo "Test #1 (0 -> Pass, 255 -> Fail, 254 -> Error, Otherwise Signal):" $result;;
    255) echo "Test #1 (0 -> Pass, 255 -> Fail, 254 -> Error, Otherwise Signal):" $result;;
    *) signame=$(kill -l $result)
    echo "Test #1 (0 -> Pass, 255 -> Fail, 254 -> Error, Otherwise Signal):" $result = SIG$signame;;
esac

./test2.out
echo "Test #2 (0 -> Pass, 1 -> Fail, 255 -> Error):" $?

./test3.out
echo "Test #3 (0 -> Pass, 1 -> Fail, 255 -> Error):" $?

./test4.out
echo "Test #4 (0 -> Pass, 1 -> Fail, 255 -> Error):" $?

rm -f ./test*.out
