echo "========================================="
../build.linux/nachos -d f -f
# ../build.linux/nachos -cp num_100.txt /h1
echo "========================================="
../build.linux/nachos -d f -mkdir /t0
echo "========================================="
../build.linux/nachos -d f -mkdir /t1
echo "========================================="
# ../build.linux/nachos -d f -mkdir /t2
# echo "========================================="
# ../build.linux/nachos -d f -mkdir /t3
# echo "========================================="
../build.linux/nachos -d f -cp num_100.txt /t0/f1
echo "========================================="
# ../build.linux/nachos -d f -cp num_100.txt /t1/f2
# ../build.linux/nachos -d f -mkdir /t0/aa
# ../build.linux/nachos -d f -mkdir /t0/bb
# ../build.linux/nachos -d f -mkdir /t0/cc
# ../build.linux/nachos -cp num_100.txt /t0/bb/f1
# ../build.linux/nachos -cp num_100.txt /t0/bb/f2
# ../build.linux/nachos -cp num_100.txt /t0/bb/f3
# ../build.linux/nachos -cp num_100.txt /t0/bb/f4
# echo "========================================="
../build.linux/nachos -l /
echo "========================================="
# ../build.linux/nachos -l /t0
# echo "========================================="
# ../build.linux/nachos -lr /
# echo "========================================="
# ../build.linux/nachos -p /t0/f1
# echo "========================================="
# ../build.linux/nachos -p /t0/bb/f3
