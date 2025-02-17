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
#include <sstream>

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

class CPPPost {
public:
    int id;
    std::string uniqueId;
    std::string title;
    std::string content;
    std::string createdAt;
    std::string author;
    std::string category;
    std::string updatedAt;
    int likesCount;
    int authorId;
    bool isPublished;
    int views;

    CPPPost() : id(0), likesCount(0), authorId(0), isPublished(true), views(0) {}

    json::value to_json() const {
        json::value j;
        j["id"] = json::value::number(id);
        j["uniqueId"] = json::value::string(utility::conversions::to_string_t(uniqueId));
        j["title"] = json::value::string(utility::conversions::to_string_t(title));
        j["content"] = json::value::string(utility::conversions::to_string_t(content));
        j["createdAt"] = json::value::string(utility::conversions::to_string_t(createdAt));
        j["author"] = json::value::string(utility::conversions::to_string_t(author));
        j["category"] = json::value::string(utility::conversions::to_string_t(category));
        j["updatedAt"] = updatedAt.empty() ? json::value::null() : json::value::string(utility::conversions::to_string_t(updatedAt));
        j["likesCount"] = json::value::number(likesCount);
        j["authorId"] = authorId ? json::value::number(authorId) : json::value::null();
        j["isPublished"] = json::value::boolean(isPublished);
        j["views"] = json::value::number(views);
        return j;
    }

    void from_json(const json::value& j) {
        if (j.has_field("title")) title = utility::conversions::to_utf8string(j.at("title").as_string());
        if (j.has_field("content")) content = utility::conversions::to_utf8string(j.at("content").as_string());
        if (j.has_field("author")) author = utility::conversions::to_utf8string(j.at("author").as_string());
        if (j.has_field("category")) category = utility::conversions::to_utf8string(j.at("category").as_string());
        if (j.has_field("likesCount")) likesCount = j.at("likesCount").as_integer();
        if (j.has_field("isPublished")) isPublished = j.at("isPublished").as_bool();
        if (j.has_field("views")) views = j.at("views").as_integer();
        // Note: id, createdAt, updatedAt, authorId are usually set by the database
    }
};

void handle_get(http_request request);
void handle_post(http_request request);
void handle_put(http_request request);
void handle_delete(http_request request);
void get_post(http_request request);

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
            CPPPost post;
            post.id = res->getInt("id");
            post.uniqueId = res->getString("uniqueId");
            post.title = res->getString("title");
            post.content = res->getString("content");
            post.createdAt = res->getString("createdAt");
            post.author = res->getString("author");
            post.category = res->getString("category");
            if (!res->isNull("updatedAt")) post.updatedAt = res->getString("updatedAt");
            post.likesCount = res->getInt("likesCount");
            if (!res->isNull("authorId")) post.authorId = res->getInt("authorId");
            post.isPublished = res->getBoolean("isPublished");
            post.views = res->getInt("views");
            result[result.size()] = post.to_json();
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
            CPPPost newPost;
            newPost.from_json(postData);

            sql::PreparedStatement *pstmt = con->prepareStatement("INSERT INTO CPPPost(title, content, author, category, likesCount, isPublished, views) VALUES (?, ?, ?, ?, ?, ?, ?)");
            pstmt->setString(1, newPost.title);
            pstmt->setString(2, newPost.content);
            pstmt->setString(3, newPost.author);
            pstmt->setString(4, newPost.category);
            pstmt->setInt(5, newPost.likesCount);
            pstmt->setBoolean(6, newPost.isPublished);
            pstmt->setInt(7, newPost.views);
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
        if (paths.size() != 1) { // Expecting /posts/{id}
            request.reply(status_codes::BadRequest, U("Invalid path"));
            return;
        }

        int postId = std::stoi(paths[0]); // ID should be the second element in the path

        request.extract_json().then([=](json::value updateData) {
            CPPPost updatedPost;
            updatedPost.id = postId;
            updatedPost.from_json(updateData);

            sql::PreparedStatement *pstmt = nullptr;
            try {
                std::stringstream sql;
                sql << "UPDATE CPPPost SET ";

                std::vector<std::string> fields;
                if (!updatedPost.title.empty()) fields.push_back("title = ?");
                if (!updatedPost.content.empty()) fields.push_back("content = ?");
                if (!updatedPost.author.empty()) fields.push_back("author = ?");
                if (!updatedPost.category.empty()) fields.push_back("category = ?");
                if (updateData.has_field("isPublished")) fields.push_back("isPublished = ?");
                if (updateData.has_field("likesCount")) fields.push_back("likesCount = ?");
                if (updateData.has_field("views")) fields.push_back("views = ?");

                if (fields.empty()) {
                    request.reply(status_codes::BadRequest, U("No fields to update"));
                    return;
                }

                for (const auto& field : fields) sql << field << ", ";
                //sql.seekp(-2, std::ios_base::end); // Remove last comma
                sql << " updatedAt = NOW()";
                sql << " WHERE id = ?";

                pstmt = con->prepareStatement(sql.str());
                int paramIndex = 1;
                if (!updatedPost.title.empty()) pstmt->setString(paramIndex++, updatedPost.title);
                if (!updatedPost.content.empty()) pstmt->setString(paramIndex++, updatedPost.content);
                if (!updatedPost.author.empty()) pstmt->setString(paramIndex++, updatedPost.author);
                if (!updatedPost.category.empty()) pstmt->setString(paramIndex++, updatedPost.category);
                if (updateData.has_field("isPublished")) pstmt->setBoolean(paramIndex++, updatedPost.isPublished);
                if (updateData.has_field("likesCount")) pstmt->setInt(paramIndex++, updatedPost.likesCount);
                if (updateData.has_field("views")) pstmt->setInt(paramIndex++, updatedPost.views);
                pstmt->setInt(paramIndex, updatedPost.id);

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

        int postId = std::stoi(paths[0]); // ID should be the second element in the path

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
        CPPPost post;

        sql::PreparedStatement *pstmt = con->prepareStatement("SELECT * FROM CPPPost WHERE id = ?");
        pstmt->setInt(1, postId);
        sql::ResultSet *res = pstmt->executeQuery();

        if (res->next()) {
            post.id = res->getInt("id");
            post.uniqueId = res->getString("uniqueId");
            post.title = res->getString("title");
            post.content = res->getString("content");
            post.createdAt = res->getString("createdAt");
            post.author = res->getString("author");
            post.category = res->getString("category");
            if (!res->isNull("updatedAt")) post.updatedAt = res->getString("updatedAt");
            post.likesCount = res->getInt("likesCount");
            if (!res->isNull("authorId")) post.authorId = res->getInt("authorId");
            post.isPublished = res->getBoolean("isPublished");
            post.views = res->getInt("views");
            request.reply(status_codes::OK, post.to_json());
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
