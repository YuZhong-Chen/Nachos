echo "========================================="
../build.linux/nachos -d f -f
../build.linux/nachos -d f -cp FS_test1 /FS_test1
../build.linux/nachos -d f -e /FS_test1
../build.linux/nachos -d f -p /file1
../build.linux/nachos -d f -cp FS_test2 /FS_test2
../build.linux/nachos -d f -e /FS_test2
../build.linux/nachos -d f -l /
echo "========================================="
