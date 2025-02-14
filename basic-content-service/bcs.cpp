#include <cpprest/http_listener.h>
#include <cpprest/json.h>
#include <mysql_driver.h>
#include <mysql_connection.h>
#include <cppconn/statement.h>
#include <cppconn/prepared_statement.h>
#include <cppconn/resultset.h>
#include <string>
#include <iostream>

#include <vector>

namespace utility {
namespace conversions {
    inline std::string join(const std::string& separator, const std::vector<std::string>& parts) {
        if (parts.empty()) return "";
        std::string result = parts[0];
        for (size_t i = 1; i < parts.size(); ++i) {
            result += separator + parts[i];
        }
        return result;
    }
}
}
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
            obj["content"] = json::value::string(res->getString("content"));
            obj["createdAt"] = json::value::string(res->getString("createdAt"));
            obj["author"] = json::value::string(res->getString("author"));
            obj["category"] = json::value::string(res->getString("category"));
            obj["updatedAt"] = json::value::string(res->getString("updatedAt"));
            obj["likesCount"] = json::value::string(res->getString("likesCount"));
            obj["authorId"] = json::value::string(res->getString("authorId"));
            obj["isPublished"] = json::value::string(res->getString("isPublished"));
            obj["views"] = json::value::string(res->getString("views"));
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
    try {
        auto paths = uri::split_path(uri::decode(request.relative_uri().path()));
        if (paths.size() != 2 || paths[0] != "") { // Expecting /posts/{id}
            request.reply(status_codes::BadRequest, U("Invalid path"));
            return;
        }

        int postId = std::stoi(paths[1]); // ID should be the second element in the path

        request.extract_json().then([=](json::value updateData) {
            sql::PreparedStatement *pstmt = nullptr;
            try {
                // Construct SQL query dynamically based on which fields are updated
                std::stringstream sql;
                sql << "UPDATE CPPPost SET ";

                std::vector<std::string> fields;
                if (updateData.has_field("title")) {
                    fields.push_back("title = ?");
                }
                if (updateData.has_field("content")) {
                    fields.push_back("content = ?");
                }
                if (updateData.has_field("author")) {
                    fields.push_back("author = ?");
                }
                if (updateData.has_field("category")) {
                    fields.push_back("category = ?");
                }
                if (updateData.has_field("isPublished")) {
                    fields.push_back("isPublished = ?");
                }
                if (updateData.has_field("likesCount")) {
                    fields.push_back("likesCount = ?");
                }
                if (updateData.has_field("views")) {
                    fields.push_back("views = ?");
                }

                if (fields.empty()) {
                    request.reply(status_codes::BadRequest, U("No fields to update"));
                    return;
                }

                sql << utility::conversions::to_utf8string(utility::conversions::join(", ", fields));
                sql << " WHERE id = ?";

                pstmt = con->prepareStatement(sql.str());
                int paramIndex = 1;
                if (updateData.has_field("title")) {
                    pstmt->setString(paramIndex++, utility::conversions::to_utf8string(updateData.at("title").as_string()));
                }
                if (updateData.has_field("content")) {
                    pstmt->setString(paramIndex++, utility::conversions::to_utf8string(updateData.at("content").as_string()));
                }
                if (updateData.has_field("author")) {
                    pstmt->setString(paramIndex++, utility::conversions::to_utf8string(updateData.at("author").as_string()));
                }
                if (updateData.has_field("category")) {
                    pstmt->setString(paramIndex++, utility::conversions::to_utf8string(updateData.at("category").as_string()));
                }
                if (updateData.has_field("isPublished")) {
                    pstmt->setBoolean(paramIndex++, updateData.at("isPublished").as_bool());
                }
                if (updateData.has_field("likesCount")) {
                    pstmt->setInt(paramIndex++, updateData.at("likesCount").as_integer());
                }
                if (updateData.has_field("views")) {
                    pstmt->setInt(paramIndex++, updateData.at("views").as_integer());
                }
                pstmt->setInt(paramIndex, postId);

                int affectedRows = pstmt->executeUpdate();
                if (affectedRows > 0) {
                    request.reply(status_codes::OK, json::value::string(U("Post updated successfully")));
                } else {
                    request.reply(status_codes::NotFound, U("Post not found or no changes were made"));
                }
            } catch (sql::SQLException &e) {
                request.reply(status_codes::InternalError, json::value::string(U("Error updating data: ") + utility::conversions::to_string_t(e.what())));
            } catch (std::exception &e) {
                request.reply(status_codes::BadRequest, json::value::string(U("An error occurred: ") + utility::conversions::to_string_t(e.what())));
            }
            delete pstmt;
        }).wait();
    } catch (std::exception &e) {
        request.reply(status_codes::BadRequest, json::value::string(U("An error occurred: ") + utility::conversions::to_string_t(e.what())));
    }
}

void handle_delete(http_request request) {
    try {
        auto paths = uri::split_path(uri::decode(request.relative_uri().path()));
        if (paths.size() != 1 ) { // Expecting /posts/{id}
            request.reply(status_codes::BadRequest, U("Invalid path"));
            return;
        }

        int postId = std::stoi(paths[1]); // ID should be the second element in the path

        sql::PreparedStatement *pstmt = con->prepareStatement("DELETE FROM CPPPost WHERE id = ?");
        pstmt->setInt(1, postId);
        int affectedRows = pstmt->executeUpdate();

        if (affectedRows > 0) {
            request.reply(status_codes::OK, json::value::string(U("Post deleted successfully")));
        } else {
            request.reply(status_codes::NotFound, U("Post not found or already deleted"));
        }

        delete pstmt;
    } catch (sql::SQLException &e) {
        request.reply(status_codes::InternalError, json::value::string(U("Error deleting data: ") + utility::conversions::to_string_t(e.what())));
    } catch (std::exception &e) {
        request.reply(status_codes::BadRequest, json::value::string(U("An error occurred: ") + utility::conversions::to_string_t(e.what())));
    }
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
            json::value obj;
            obj["id"] = json::value::number(res->getInt("id"));
            obj["title"] = json::value::string(res->getString("title"));
            obj["content"] = json::value::string(res->getString("content"));
            obj["createdAt"] = json::value::string(res->getString("createdAt"));
            obj["author"] = json::value::string(res->getString("author"));
            obj["category"] = json::value::string(res->getString("category"));
            obj["updatedAt"] = json::value::string(res->getString("updatedAt"));
            obj["likesCount"] = json::value::string(res->getString("likesCount"));
            obj["authorId"] = json::value::string(res->getString("authorId"));
            obj["isPublished"] = json::value::string(res->getString("isPublished"));
            obj["views"] = json::value::string(res->getString("views"));
            result= obj;
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
