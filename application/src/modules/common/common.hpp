#pragma once

#include <cstring>
#include <iostream>
#include <fstream>
#include <memory>
#include <regex>

#include <nanojson/nanojson.hpp>

#include <uuid_v4/uuid_v4.h>

//SQL

//SOCI core
#include <soci/soci.h>

#include <soci/empty/soci-empty.h>
#include <soci/mysql/soci-mysql.h>
//#include <soci/postgresql/soci-postgresql.h>
#include <soci/firebird/soci-firebird.h>
#include <soci/sqlite3/soci-sqlite3.h>
#include <soci/odbc/soci-odbc.h>

//No SQL

//MongoDB
#include <bsoncxx/builder/basic/array.hpp>
#include <bsoncxx/builder/basic/document.hpp>
#include <bsoncxx/builder/basic/kvp.hpp>
#include <bsoncxx/types.hpp>
#include <bsoncxx/json.hpp>
#include <mongocxx/client.hpp>
#include <mongocxx/instance.hpp>
#include <mongocxx/uri.hpp>

//Redis
#include <sw/redis++/redis++.h>

namespace common {

const std::string get_file_path( const std::string& file );
std::stringstream read_text_file_content( const std::string& path_file_to_read );
nanojson::element get_config( const std::string& config_path_file );

class StoreConnection {
public:

  StoreConnection( const std::string& name,
                   const std::string& driver,
                   const std::string& database,
                   soci::session *sql_connection );
  StoreConnection( const std::string& name,
                   const std::string& driver,
                   const std::string& database,
                   sw::redis::Redis *redis_connection );
  StoreConnection( const std::string& name,
                   const std::string& driver,
                   const std::string& database,
                   mongocxx::client *mongo_client,
                   mongocxx::v_noabi::database *mongo_database );
  ~StoreConnection();

  soci::session* sql_connection() const;
  sw::redis::Redis* redis_connection() const;
  mongocxx::v_noabi::database* mongo_connection() const;

private:
  std::string name_;
  std::string driver_;
  std::string database_;

  soci::session *sql_connection_ { nullptr };
  sw::redis::Redis *redis_connection_ { nullptr };
  mongocxx::client *mongo_client_ { nullptr };
  mongocxx::v_noabi::database *mongo_connection_ { nullptr };
};

using StoreConnectionSharedPtr = std::shared_ptr<StoreConnection>;
StoreConnectionSharedPtr make_store_connection( nanojson::element config_json );

//using SQLConnectionSharedPtr = std::shared_ptr<soci::session>;
//SQLConnectionSharedPtr make_sql_db_connection( nanojson::element config_json );

// using NoSQLConnectionVariant = std::variant<sw::redis::Redis,mongocxx::v_noabi::database>;
// using NoSQLConnectionSharedPtr = std::shared_ptr<NoSQLConnectionVariant>;
// NoSQLConnectionSharedPtr make_no_sql_db_connection( nanojson::element config_json );

}
