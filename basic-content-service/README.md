# CPlusPlusExamples

## Ubuntu 24.04 or nginx
sudo apt update <br />
sudo apt upgrade<br />
### Install cpprestsdk
sudo apt install libmysqlcppconn-dev<br />
If not installed<br />
https://ubuntu.com/server/docs/install-and-configure-a-mysql-server<br />
sudo apt install mysql-server<br />
sudo service mysql status<br />
sudo service mysql restart<br />
mysql<br />
Change user to a different existing user and enter a new pass at MyNewPass
ALTER USER 'user'@'localhost' IDENTIFIED WITH mysql_native_password BY 'MyNewPass';<br />
flush privileges;<br />
create database cppbase;<br />
use cppbase;<br />
run the create statement in spppost.sql<br />
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
clang++ -std=c++11 -I/usr/local/include bcs.cpp -o bcs \
  -L/usr/local/lib \
  -lcpprest \
  -lboost_system \
  -lboost_thread \
  -lpthread \
  -lcrypto \
  -lssl \
  -lmysqlcppconn<br />
### run 
./bcs<br />
### test
Open/attach a new terminal
If not installed, install curl
sudo apt install curl
curl -X POST "http://localhost:8080/posts" \
     -H "Content-Type: application/json" \
     -d '{
           "title": "Sample Title",
           "content": "This is a sample content for testing.",
           "author": "John Doe",
           "category": "Test",
           "likesCount": 0,
           "isPublished": true,
           "views": 0
         }'
curl "http://localhost:8080/posts"
curl "http://localhost:8080/posts/1"
curl -X PUT "http://localhost:8080/posts/1" \
     -H "Content-Type: application/json" \
     -d '{
           "title": "Updated Title",
           "content": "This content has been updated.",
           "likesCount": 5
         }'
curl -X DELETE "http://localhost:8080/posts/1"

## alternate impl with bcs_class
clang++ -std=c++11 -I/usr/local/include bcs_class.cpp -o bcs_class \
  -L/usr/local/lib \
  -lcpprest \
  -lboost_system \
  -lboost_thread \
  -lpthread \
  -lcrypto \
  -lssl \
  -lmysqlcppconn<br />
