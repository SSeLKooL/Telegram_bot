# How to run (Linux)
Don't forget to add your bot token.
## Before Compile, in repo: 

sudo apt-get install g++ make binutils cmake libboost-system-dev libssl-dev zlib1g-dev libcurl4-openssl-dev 

git clone https://github.com/reo7sp/tgbot-cpp 

cd tgbot-cpp 

cmake . 

make -j4 

sudo make install


## Compile: 

g++ Telegram_bot.cpp -o telegram_bot --std=c++14 -I/usr/local/include -lTgBot -lboost_system -lssl -lcrypto -lpthread Launch: ./telegram_bot

## Launch: 

./telegram_bot
