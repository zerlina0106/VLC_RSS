make clean
make


rmmod rssicatcher
insmod rssicatcher.ko

rm -rf client
gcc client.c -o client

mknod /dev/RssiCatcher c 200 0
ls /dev/RssiCatcher
./client /dev/RssiCatcher r
