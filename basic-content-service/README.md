# CPlusPlusExamples

## Ubuntu 24.04 or nginx
sudo apt update <br />
sudo apt upgrade<br />
### Install cpprestsdk
sudo apt install libmysqlcppconn-dev<br />
### Install mysql-connector-cpp
sudo apt install libcpprest-dev<br />
### Install boost
sudo apt install libboost-all-dev<br />
### Install clang
sudo apt install clang<br />
### MySQL 
Update the username, password and database in bcs.cpp<br />
Also, run the sql for the cpppost object in cpppost.sql from the mysql client.<br />
### compile
clang++ -std=c++11 -I/usr/local/include bcs.cpp -o bcs \<br />
  -L/usr/local/lib \<br />
  -lcpprest \<br />
  -lboost_system \<br />
  -lboost_thread \<br />
  -lpthread \<br />
  -lcrypto \<br />
  -lssl \<br />
  -lmysqlcppconn<br />
### run 
./bcs<br />

