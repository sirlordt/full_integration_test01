#pragma once

#include <memory>

//SOCI core
#include <soci/soci.h>
#include <soci/transaction.h>

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

namespace Store {

class StoreConnection {
public:

  StoreConnection() = delete;
  StoreConnection( StoreConnection &store_connection ) = delete;
  StoreConnection( StoreConnection &&store_connection ) = delete;

  StoreConnection( const std::string& name,
                   const std::string& driver,
                   const std::string& database,
                   const std::string& host,
                   uint16_t port,
                   const std::string& user,
                   const std::string& password,
                   u_int8_t pool,
                   soci::session *sql_connection );
  StoreConnection( const std::string& name,
                   const std::string& driver,
                   const std::string& database,
                   const std::string& host,
                   uint16_t port,
                   const std::string& user,
                   const std::string& password,
                   u_int8_t pool,
                   sw::redis::Redis *redis_connection );
  StoreConnection( const std::string& name,
                   const std::string& driver,
                   const std::string& database,
                   const std::string& host,
                   uint16_t port,
                   const std::string& user,
                   const std::string& password,
                   u_int8_t pool,
                   mongocxx::client *mongo_client,
                   mongocxx::v_noabi::database *mongo_database );
  ~StoreConnection();

  StoreConnection& operator=( StoreConnection &store_connection ) = delete;
  StoreConnection& operator=( StoreConnection &&store_connection ) = delete;

  soci::session* sql_connection() const;
  sw::redis::Redis* redis_connection() const;
  mongocxx::v_noabi::database* mongo_connection() const;

  u_int8_t index() const;
  const std::string& name() const;
  const std::string& driver() const;
  const std::string& database() const;
  const std::string& host() const;
  uint16_t port() const;
  const std::string& user() const;
  const std::string& password() const;
  u_int8_t pool() const;
  bool borrowed() const;
  std::mutex& mutex();

private:

  uint8_t index_ { 0 };
  std::string name_ { "" };
  std::string driver_ { "" };
  std::string database_{ "" };
  std::string host_{ "" };
  uint16_t port_{ 0 };
  std::string user_{ "" };
  std::string password_{ "" };
  uint8_t pool_ { 0 };
  bool borrowed_ { false };

  std::mutex mutex_ {};

  soci::session *sql_connection_ { nullptr };
  sw::redis::Redis *redis_connection_ { nullptr };
  mongocxx::client *mongo_client_ { nullptr };
  mongocxx::v_noabi::database *mongo_connection_ { nullptr };

  friend class StoreConnectionManager;

};

std::ostream& operator<<( std::ostream& stream, StoreConnection& storeConnection );

using StoreConnectionSharedPtr = std::shared_ptr<StoreConnection>;
using StoreSQLConnectionTransactionPtr = soci::transaction*;
//typedef soci::transaction *StoreSQLConnectionTransactionPtr;

};
