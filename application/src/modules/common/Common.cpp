#include <string>
#include <thread>
// #include <iostream>
// #include <sstream>
// #include <syncstream>
// #include <cstring>
// #include <atomic>
// #include <sys/time.h>

#include "Common.hpp"

#include "hv/hlog.h"

#include "xxhash.h"

namespace Common {

NJSONElement config_json {};

const std::string get_file_path( const std::string& file )
{

  std::string result = file;

  size_t found = file.find_last_of( "/\\" );

  if ( found != std::string::npos )
  {

    result = file.substr( 0, found ) + "/";

  }

  return result;

}

std::stringstream read_text_file_content( const std::string& path_file_to_read )
{

  std::stringstream result {};

  std::ifstream stream_file_input = {};

  try
  {

    stream_file_input = std::ifstream( path_file_to_read );

    if ( stream_file_input.is_open() )
    {

      std::string line {};

      while ( std::getline( stream_file_input, line ) )
      {
        result << line;
      }

      stream_file_input.close();

    }

  }
  catch ( const std::exception& ex )
  {

    hlogi( "Exception: %s", ex.what() );

    //std::cout << "Exception: " << ex.what() << std::endl;

    if ( stream_file_input.is_open() )
    {

      stream_file_input.close();

    }

  }

  return result;

}

nanojson::element get_config( const std::string& config_path_file )
{

  nanojson::element result {};

  try {

    auto config_content = read_text_file_content( config_path_file );

    result = nanojson::element::from_string( config_content.str() );

  }
  catch ( const std::exception &ex ) {

    hlogi( "Exception: %s", ex.what() );

    //std::cout << ex.what() << std::endl;

  }

  return result;

}

std::string ltrim( const std::string &s ) {

  return std::regex_replace( s, std::regex( "^\\s+" ), std::string( "" ) );

}

std::string rtrim( const std::string &s ) {

  return std::regex_replace( s, std::regex( "\\s+$" ), std::string( "" ) );

}

std::string trim( const std::string &s ) {

  return ltrim( rtrim( s ) );

}

// SQLConnectionSharedPtr make_sql_db_connection( nanojson::element database_connection_config )
// {

//   SQLConnectionSharedPtr result { nullptr };

//   try {

//     const std::string& driver = database_connection_config[ "driver" ].to_string();

//     std::ostringstream connectionString;

//     connectionString << "driver=" + driver;

//     connectionString << "host=" + database_connection_config[ "host" ].to_string();

//     connectionString << "port=" + database_connection_config[ "port" ].to_string();

//     connectionString << "database=" + database_connection_config[ "database" ].to_string();

//     connectionString << "user=" + database_connection_config[ "user" ].to_string();

//     connectionString << "password='" + database_connection_config[ "password" ].to_string() << "'";

//     if ( driver == "sql/mysql" ) {

//       result = std::make_shared<soci::session>( soci::mysql, connectionString.str() );

//     }
//     else if ( driver == "sql/empty" ) {

//       result = std::make_shared<soci::session>( soci::empty, connectionString.str() );

//     }
//     else if ( driver == "sql/postgresql" ) {

//       //result = std::make_shared<soci::session>( soci::postgresql, connectionString.str() );

//     }
//     else if ( driver == "sql/firebird" ) {

//       result = std::make_shared<soci::session>( soci::firebird, connectionString.str() );

//     }
//     else if ( driver == "sql/sqlite3" ) {

//       result = std::make_shared<soci::session>( soci::sqlite3, connectionString.str() );

//     }
//     else if ( driver == "sql/odbc" ) {

//       result = std::make_shared<soci::session>( soci::odbc, connectionString.str() );

//     }

//   }
//   catch ( const std::exception& ex ) {

//     std::cout << "Exception: " << ex.what() << std::endl;

//   }

//   return result;

// }

std::ostream& operator<<( std::ostream &stream, NJSONElement &json_element ) {

  stream << "Name: " << json_element[ "name" ].to_string() << std::endl;
  stream << "Driver: " << json_element[ "driver" ].to_string() << std::endl;
  stream << "Database: " << json_element[ "database" ].to_string() << std::endl;
  stream << "Host: " << json_element[ "host" ].to_string() << std::endl;
  stream << "Port: " << json_element[ "port" ].to_string() << std::endl;
  stream << "User: " << json_element[ "user" ].to_string() << std::endl;
  stream << "Password: " << json_element[ "password" ].to_string() << std::endl;
  stream << "Pool: " << json_element[ "pool" ].to_string() << std::endl;

  return stream;

}

StoreConnectionSharedPtr make_store_connection( NJSONElement &store_connection_config ) {

  StoreConnectionSharedPtr result { nullptr };

  try {

    const std::string& name = store_connection_config[ "name" ].to_string();

    const std::string& driver = store_connection_config[ "driver" ].to_string();

    const std::string& database = store_connection_config[ "database" ].to_string();

    const std::string& host = store_connection_config[ "host" ].to_string();

    const std::string& port = store_connection_config[ "port" ].to_string();

    const std::string& user = store_connection_config[ "user" ].to_string();

    const std::string& password = store_connection_config[ "password" ].to_string();

    const u_int8_t pool = store_connection_config[ "pool" ].to_integer();

    //std::cout << store_connection_config << std::endl;

    // std::cout << "Name: " << name << std::endl;
    // std::cout << "Driver: " << driver << std::endl;
    // std::cout << "Database: " << database << std::endl;
    // std::cout << "Host: " << host << std::endl;
    // std::cout << "Port: " << port << std::endl;
    // std::cout << "User: " << user << std::endl;
    // std::cout << "Password: " << password << std::endl;
    // std::cout << "Pool: " << std::to_string( pool ) << std::endl;

    std::ostringstream connectionString;

    connectionString << "db=" << database;

    connectionString << " user=" << user;

    connectionString << " password='" << password << "'";

    connectionString << " host=" << host;

    connectionString << " port=" << port;

    if ( driver == "sql/mysql" ) {

      soci::session *connection = nullptr;

      try {

        connection = new soci::session( soci::mysql, connectionString.str() );

        if ( connection ) {

          result = std::make_shared<StoreConnection>( name,
                                                      driver,
                                                      database,
                                                      host,
                                                      std::stoi( port ),
                                                      user,
                                                      password,
                                                      pool,
                                                      connection );

        }

      }
      catch ( const std::exception &ex ) {

        hloge( "Exception: %s", ex.what() );
        //hlogi( "Exception: Error" );

        std::cout << "Exception: " << ex.what() << std::endl;

        if ( connection ) {

          delete connection;
          connection = nullptr;

        }

      }

    }
    else if ( driver == "sql/empty" ) {

      soci::session *connection = nullptr;

      try {

        //connection = new soci::session( soci::empty, connectionString.str() );

        if ( connection ) {

          result = std::make_shared<StoreConnection>( name,
                                                      driver,
                                                      database,
                                                      host,
                                                      std::stoi( port ),
                                                      user,
                                                      password,
                                                      pool,
                                                      connection );

        }

      }
      catch ( const std::exception &ex ) {

        hlogi( "Exception: %s", ex.what() );

        std::cout << "Exception: " << ex.what() << std::endl;

        if ( connection ) {

          delete connection;
          connection = nullptr;

        }

      }

    }
    else if ( driver == "sql/postgresql" ) {

      soci::session *connection = nullptr;

      try {

        //connection = new soci::session( soci::postgresql, connectionString.str() );

        if ( connection ) {

          result = std::make_shared<StoreConnection>( name,
                                                      driver,
                                                      database,
                                                      host,
                                                      std::stoi( port ),
                                                      user,
                                                      password,
                                                      pool,
                                                      connection );

        }

      }
      catch ( const std::exception &ex ) {

        hlogi( "Exception: %s", ex.what() );

        std::cout << "Exception: " << ex.what() << std::endl;

        if ( connection ) {

          delete connection;
          connection = nullptr;

        }

      }

    }
    else if ( driver == "sql/firebird" ) {

      soci::session *connection = nullptr;

      try {

        //connection = new soci::session( soci::firebird, connectionString.str() );

        if ( connection ) {

          result = std::make_shared<StoreConnection>( name,
                                                      driver,
                                                      database,
                                                      host,
                                                      std::stoi( port ),
                                                      user,
                                                      password,
                                                      pool,
                                                      connection );

        }

      }
      catch ( const std::exception &ex ) {

        hlogi( "Exception: %s", ex.what() );

        std::cout << "Exception: " << ex.what() << std::endl;

        if ( connection ) {

          delete connection;
          connection = nullptr;

        }

      }

    }
    else if ( driver == "sql/sqlite3" ) {

      soci::session *connection = nullptr;

      try {

        //connection = new soci::session( soci::sqlite3, connectionString.str() );

        if ( connection ) {

          result = std::make_shared<StoreConnection>( name,
                                                      driver,
                                                      database,
                                                      host,
                                                      std::stoi( port ),
                                                      user,
                                                      password,
                                                      pool,
                                                      connection );

        }

      }
      catch ( const std::exception &ex ) {

        hlogi( "Exception: %s", ex.what() );

        std::cout << "Exception: " << ex.what() << std::endl;

        if ( connection ) {

          delete connection;
          connection = nullptr;

        }

      }

    }
    else if ( driver == "sql/odbc" ) {

      soci::session *connection = nullptr;

      try {

        //connection = new soci::session( soci::odbc, connectionString.str() );

        if ( connection ) {

          result = std::make_shared<StoreConnection>( name,
                                                      driver,
                                                      database,
                                                      host,
                                                      std::stoi( port ),
                                                      user,
                                                      password,
                                                      pool,
                                                      connection );

        }

      }
      catch ( const std::exception &ex ) {

        hlogi( "Exception: %s", ex.what() );

        std::cout << "Exception: " << ex.what() << std::endl;

        if ( connection ) {

          delete connection;
          connection = nullptr;

        }

      }

    }
    else if ( driver == "no_sql/mongo_db" ) {

      std::stringstream url;
      url << "mongodb://";

      const std::string& user = trim( store_connection_config[ "user" ].to_string() );
      const std::string& password = trim( store_connection_config[ "password" ].to_string() );

      if ( user != "" &&
           password != "" ) {

        url << user << ":" << password << "@";

      }

      url << store_connection_config[ "host" ].to_string() <<  ":" << store_connection_config[ "port" ].to_string();

      mongocxx::client *mongo_client = nullptr;
      mongocxx::v_noabi::database *mongo_connection = nullptr;

      //mongo "mongodb://${USER}:${DBPASSWORD}@<host>:<port>/admin?authSource=admin"
      //mongocxx::client client{ mongocxx::uri{ url.str() } };

      try {

        mongo_client = new mongocxx::client( mongocxx::uri{ url.str() } );

        mongo_connection = new mongocxx::v_noabi::database( mongo_client->database( store_connection_config[ "database" ].to_string() ) );

        if ( mongo_client ) {

          result = std::make_shared<StoreConnection>( name,
                                                      driver,
                                                      database,
                                                      host,
                                                      std::stoi( port ),
                                                      user,
                                                      password,
                                                      pool,
                                                      mongo_client,
                                                      mongo_connection ); //client[ database_connection_config[ "database" ].to_string() ] );

        }

      }
      catch ( const std::exception& ex ) {

        hlogi( "Exception: %s", ex.what() );

        std::cout << "Exception: " << ex.what() << std::endl;

        if ( mongo_connection ) {

          delete mongo_connection;
          mongo_connection = nullptr;

        }

        if ( mongo_client ) {

          delete mongo_client;
          mongo_client = nullptr;

        }

      }

    }
    else if ( driver == "no_sql/redis" ) {

      std::stringstream url;
      url << "tcp://" << store_connection_config[ "host" ].to_string() << ":" << store_connection_config[ "port" ].to_string();

      sw::redis::Redis *connection = nullptr;

      try {

        connection = new sw::redis::Redis( url.str() );

        if ( connection ) {

          result = std::make_shared<StoreConnection>( name,
                                                      driver,
                                                      database,
                                                      host,
                                                      std::stoi( port ),
                                                      user,
                                                      password,
                                                      pool,
                                                      connection );

        }

      }
      catch ( const std::exception &ex ) {

        hlogi( "Exception: %s", ex.what() );

        std::cout << "Exception: " << ex.what() << std::endl;

        if ( connection ) {

          delete connection;
          connection = nullptr;

        }

      }

      const std::string& user = trim( store_connection_config[ "user" ].to_string() );
      const std::string& password = trim( store_connection_config[ "password" ].to_string() );

      if ( user != "" &&
           password != "" ) {

        result->redis_connection()->auth( user, password );

      }
      else if ( password != "" ) {

        result->redis_connection()->auth( password );

      }

    }

  }
  catch ( const std::exception& ex ) {

    hloge( "Exception: %s", ex.what() );

    std::cout << "Exception: " << ex.what() << std::endl;

  }

  return result;

}

const std::string get_thread_id() {

  auto thread_id = std::this_thread::get_id();

  std::stringstream ss;

  ss << thread_id;

  return ss.str();

}

const std::string xxHash_32( const std::string &to_hash ) {

  XXH32_hash_t xxhash32 = XXH32( to_hash.c_str(), to_hash.size(), 0 );

  std::stringstream ss;

  ss << std::hex << xxhash32;

  return ss.str();

}


}
