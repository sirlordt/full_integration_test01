#pragma once

#include <cstring>
#include <iostream>
#include <fstream>
#include <memory>
#include <regex>

#include <nanojson/nanojson.hpp>

#include <uuid_v4/uuid_v4.h>

#include "StoreConnection.hpp"

//SQL

//SOCI core
// #include <soci/soci.h>

// #include <soci/empty/soci-empty.h>
// #include <soci/mysql/soci-mysql.h>
// //#include <soci/postgresql/soci-postgresql.h>
// #include <soci/firebird/soci-firebird.h>
// #include <soci/sqlite3/soci-sqlite3.h>
// #include <soci/odbc/soci-odbc.h>

// //No SQL

// //MongoDB
// #include <bsoncxx/builder/basic/array.hpp>
// #include <bsoncxx/builder/basic/document.hpp>
// #include <bsoncxx/builder/basic/kvp.hpp>
// #include <bsoncxx/types.hpp>
// #include <bsoncxx/json.hpp>
// #include <mongocxx/client.hpp>
// #include <mongocxx/instance.hpp>
// #include <mongocxx/uri.hpp>

// //Redis
// #include <sw/redis++/redis++.h>

namespace Common {

const std::string get_file_path( const std::string& file );
std::stringstream read_text_file_content( const std::string& path_file_to_read );
nanojson::element get_config( const std::string& config_path_file );

std::string trim( const std::string &s );

using Store::StoreConnectionSharedPtr;
using Store::StoreConnection;

using JSONElement = nanojson::element;
using JSONElementArray = nanojson::element::array_t;
using JSONElementObject = nanojson::element::object_t;

StoreConnectionSharedPtr make_store_connection( JSONElement &config_json );

const std::string get_thread_id();
const std::string xxHash_32( const std::string &to_hash );

//using SQLConnectionSharedPtr = std::shared_ptr<soci::session>;
//SQLConnectionSharedPtr make_sql_db_connection( nanojson::element config_json );

// using NoSQLConnectionVariant = std::variant<sw::redis::Redis,mongocxx::v_noabi::database>;
// using NoSQLConnectionSharedPtr = std::shared_ptr<NoSQLConnectionVariant>;
// NoSQLConnectionSharedPtr make_no_sql_db_connection( nanojson::element config_json );

}
