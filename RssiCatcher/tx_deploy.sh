rmmod rssicatcher
rm -rf tx_test

gcc tx_test.c -o tx_test
./tx_test

ps aux | grep "tx_test" |grep -v grep| cut -c 9-15 | xargs kill -9
