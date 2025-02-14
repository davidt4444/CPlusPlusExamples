#include <cpprest/http_listener.h>
#include <cpprest/json.h>
#include <mysql_driver.h>
#include <mysql_connection.h>
#include <cppconn/statement.h>
#include <cppconn/prepared_statement.h>
#include <cppconn/resultset.h>
#include <string>
#include <iostream>

using namespace web;
using namespace http;
using namespace http::experimental::listener;

// MySQL Connection Information
const std::string server = "tcp://127.0.0.1:3306";
const std::string username = "your_username";
const std::string password = "your_password";
const std::string database = "your_database";

sql::mysql::MySQL_Driver *driver;
sql::Connection *con;

void handle_get(http_request request);
void handle_post(http_request request);
void handle_put(http_request request);
void handle_delete(http_request request);
void get_post(http_request request); // Added function declaration

int main() {
    driver = sql::mysql::get_mysql_driver_instance();
    con = driver->connect(server, username, password);
    con->setSchema(database);

    http_listener listener("http://localhost:8080/posts");

    listener.support(methods::GET, [](http_request request) {
        auto paths = uri::split_path(uri::decode(request.relative_uri().path()));
        if (paths.size() == 0) { // No ID, just list all posts
            handle_get(request);
        } else if (paths.size() == 1) { // There's an ID after posts
            request.set_request_uri(uri::decode(paths[0]));  // Set the URI to just the ID part
            get_post(request);
        } else {
            request.reply(status_codes::BadRequest, U("Invalid path"));
        }
    });
    listener.support(methods::POST, handle_post);
    listener.support(methods::PUT, handle_put);
    listener.support(methods::DEL, handle_delete);

    try {
        listener.open().wait();
        std::cout << "Listening for requests at: http://localhost:8080/posts and http://localhost:8080/posts/{id}" << std::endl;
        std::string line;
        std::getline(std::cin, line);
    } catch (std::exception const & e) {
        std::wcout << e.what() << std::endl;
    }

    listener.close().wait();
    delete con;
}

void handle_get(http_request request) {
    json::value result = json::value::array();
    try {
        sql::PreparedStatement *pstmt = con->prepareStatement("SELECT * FROM CPPPost");
        sql::ResultSet *res = pstmt->executeQuery();
        while (res->next()) {
            json::value obj;
            obj["id"] = json::value::number(res->getInt("id"));
            obj["title"] = json::value::string(res->getString("title"));
            // ... (add other fields accordingly)
            result[result.size()] = obj;
        }
        delete res;
        delete pstmt;
    } catch (sql::SQLException &e) {
        result = json::value::string(U("Error fetching data: ") + utility::conversions::to_string_t(e.what()));
    }
    request.reply(status_codes::OK, result);
}

void handle_post(http_request request) {
    request.extract_json().then([=](json::value postData) {
        try {
            sql::PreparedStatement *pstmt = con->prepareStatement("INSERT INTO CPPPost(title, content, author, category, createdAt) VALUES (?, ?, ?, ?, NOW())");
            pstmt->setString(1, utility::conversions::to_utf8string(postData.at("title").as_string()));
            pstmt->setString(2, utility::conversions::to_utf8string(postData.at("content").as_string()));
            pstmt->setString(3, utility::conversions::to_utf8string(postData.at("author").as_string()));
            pstmt->setString(4, utility::conversions::to_utf8string(postData.at("category").as_string()));
            pstmt->execute();
            delete pstmt;
            request.reply(status_codes::Created, json::value::string(U("Post created successfully")));
        } catch (sql::SQLException &e) {
            request.reply(status_codes::InternalError, json::value::string(U("Error inserting data: ") + utility::conversions::to_string_t(e.what())));
        }
    }).wait();
}

void handle_put(http_request request) {
    // Handle update logic here, similar to POST but with an UPDATE statement
    request.reply(status_codes::NotImplemented, U("PUT not implemented"));
}

void handle_delete(http_request request) {
    // Handle delete logic here
    request.reply(status_codes::NotImplemented, U("DELETE not implemented"));
}
void get_post(http_request request) {
    try {
        auto paths = uri::split_path(uri::decode(request.request_uri().to_string()));
        if (paths.size() != 1) {
            request.reply(status_codes::BadRequest, U("Invalid path"));
            return;
        }

        int postId = std::stoi(paths[0]);
        json::value result;

        sql::PreparedStatement *pstmt = con->prepareStatement("SELECT * FROM CPPPost WHERE id = ?");
        pstmt->setInt(1, postId);
        sql::ResultSet *res = pstmt->executeQuery();

        if (res->next()) {
            // ... (your existing code to populate the result JSON)
            request.reply(status_codes::OK, result);
        } else {
            request.reply(status_codes::NotFound, U("Post not found"));
        }

        delete res;
        delete pstmt;
    } catch (sql::SQLException &e) {
        request.reply(status_codes::InternalError, json::value::string(U("Error fetching data: ") + utility::conversions::to_string_t(e.what())));
    } catch (std::exception &e) {
        request.reply(status_codes::BadRequest, json::value::string(U("An error occurred: ") + utility::conversions::to_string_t(e.what())));
    }
}
