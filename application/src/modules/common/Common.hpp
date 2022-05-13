#pragma once

#include <cstring>
#include <iostream>
#include <fstream>
#include <memory>
#include <regex>

#include <hv/json.hpp>
//#include "fifo_map.hpp"

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

using NJSONElement = nanojson::element;
using NJSONElementArray = nanojson::element::array_t;
using NJSONElementObject = nanojson::element::object_t;

// A workaround to give to use fifo_map as map, we are just ignoring the 'less' compare
// template<class K, class V, class dummy_compare, class A>
// using custom_fifo_map = nlohmann::fifo_map<K, V, nlohmann::fifo_map_compare<K>, A>;
using NLOJSONObject = nlohmann::ordered_json; //nlohmann::basic_json<custom_fifo_map>; //

StoreConnectionSharedPtr make_store_connection( NJSONElement &config_json );

const std::string get_thread_id();
const std::string xxHash_32( const std::string &to_hash );

//using SQLConnectionSharedPtr = std::shared_ptr<soci::session>;
//SQLConnectionSharedPtr make_sql_db_connection( nanojson::element config_json );

// using NoSQLConnectionVariant = std::variant<sw::redis::Redis,mongocxx::v_noabi::database>;
// using NoSQLConnectionSharedPtr = std::shared_ptr<NoSQLConnectionVariant>;
// NoSQLConnectionSharedPtr make_no_sql_db_connection( nanojson::element config_json );

}
