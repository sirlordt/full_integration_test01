
#include "StoreConnection.hpp"

namespace Store {

StoreConnection::StoreConnection( const std::string& name,
                                  const std::string& driver,
                                  const std::string& database,
                                  const std::string& host,
                                  uint16_t port,
                                  const std::string& user,
                                  const std::string& password,
                                  u_int8_t pool,
                                  soci::session *sql_connection ):
                                  index_( 0 ),
                                  name_( name ),
                                  driver_( driver ),
                                  database_( database ),
                                  host_( host ),
                                  port_( port ),
                                  user_( user ),
                                  password_( password ),
                                  pool_( pool ),
                                  borrowed_( false ),
                                  sql_connection_( sql_connection ),
                                  redis_connection_( nullptr ),
                                  mongo_client_( nullptr ),
                                  mongo_connection_( nullptr ) {

  //

}

StoreConnection::StoreConnection( const std::string& name,
                                  const std::string& driver,
                                  const std::string& database,
                                  const std::string& host,
                                  uint16_t port,
                                  const std::string& user,
                                  const std::string& password,
                                  u_int8_t pool,
                                  sw::redis::Redis *redis_connection ):
                                  index_( 0 ),
                                  name_( name ),
                                  driver_( driver ),
                                  database_( database ),
                                  host_( host ),
                                  port_( port ),
                                  user_( user ),
                                  password_( password ),
                                  pool_( pool ),
                                  borrowed_( false ),
                                  sql_connection_( nullptr ),
                                  redis_connection_( redis_connection ),
                                  mongo_client_( nullptr ),
                                  mongo_connection_( nullptr ) {

  //

}

StoreConnection::StoreConnection( const std::string& name,
                                  const std::string& driver,
                                  const std::string& database,
                                  const std::string& host,
                                  uint16_t port,
                                  const std::string& user,
                                  const std::string& password,
                                  u_int8_t pool,
                                  mongocxx::client *mongo_client,
                                  mongocxx::v_noabi::database *mongo_database ):
                                  index_( 0 ),
                                  name_( name ),
                                  driver_( driver ),
                                  database_( database ),
                                  host_( host ),
                                  port_( port ),
                                  user_( user ),
                                  password_( password ),
                                  pool_( pool ),
                                  borrowed_( false ),
                                  sql_connection_( nullptr ),
                                  redis_connection_( nullptr ),
                                  mongo_client_( mongo_client ),
                                  mongo_connection_( mongo_database ) {

  //

}

StoreConnection::~StoreConnection() {

  if ( sql_connection_ ) {

    delete sql_connection_;
    sql_connection_ = nullptr;

  }

  if ( redis_connection_ ) {

    delete redis_connection_;
    redis_connection_ = nullptr;

  }

  if ( mongo_connection_ ) {

    delete mongo_connection_;
    mongo_connection_ = nullptr;

  }

  if ( mongo_client_ ) {

    delete mongo_client_;
    mongo_client_ = nullptr;

  }

}

u_int8_t StoreConnection::index() const {

  return index_;

}

const std::string& StoreConnection::name() const {

  return name_;

}

const std::string& StoreConnection::driver() const {

  return driver_;

}

const std::string& StoreConnection::database() const {

  return database_;

}

const std::string& StoreConnection::host() const {

  return host_;

}

uint16_t StoreConnection::port() const {

  return port_;

}

const std::string& StoreConnection::user() const {

  return user_;

}

const std::string& StoreConnection::password() const {

  return password_;

}

u_int8_t StoreConnection::pool() const {

  return pool_;

}

bool StoreConnection::borrowed() const {

  return borrowed_;

}

std::mutex& StoreConnection::mutex() {

  return mutex_;

}

soci::session* StoreConnection::sql_connection() const {

  return sql_connection_;

}

sw::redis::Redis* StoreConnection::redis_connection() const {

  return redis_connection_;

}

mongocxx::v_noabi::database* StoreConnection::mongo_connection() const {

  return mongo_connection_;

}

std::ostream& operator<<( std::ostream& stream, StoreConnection& storeConnection ) {

  stream << "Name: " << storeConnection.name() << std::endl;
  stream << "Driver: " << storeConnection.driver() << std::endl;
  stream << "Database: " << storeConnection.database() << std::endl;
  stream << "Host: " << storeConnection.host() << std::endl;
  stream << "Port: " << storeConnection.port() << std::endl;
  stream << "User: " << storeConnection.user() << std::endl;
  stream << "Password: " << storeConnection.password() << std::endl;
  stream << "Pool: " << std::to_string( storeConnection.pool() ) << std::endl;
  stream << "Borrowed: " << storeConnection.borrowed() << std::endl;

  return stream;

}

}
