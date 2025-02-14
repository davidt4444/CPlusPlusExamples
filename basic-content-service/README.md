# CPlusPlusExamples

## Ubuntu 24.04 or nginx
ssh dathigpe@192.168.40.149
sudo apt update
sudo apt upgrade
### Install cpprestsdk
https://askubuntu.com/questions/165868/installing-mysql-connector-c
sudo apt install libmysqlcppconn-dev
### Install mysql-connector-cpp
sudo apt install libcpprest-dev
### Install boost
sudo apt install libboost-all-dev
### Install clang
sudo apt install clang
### MySQL 
Update the username, password and database in bcs.cpp
Also, run the sql for the cpppost object in cpppost.sql from the mysql client.
### compile
clang++ -std=c++11 -I/usr/local/include bcs.cpp -o bcs \
  -L/usr/local/lib \
  -lcpprest \
  -lboost_system \
  -lboost_thread \
  -lpthread \
  -lcrypto \
  -lssl \
  -lmysqlcppconn
### run 
./bcs

