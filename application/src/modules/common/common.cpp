
#include "common.hpp"

namespace common {

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

    std::cout << "Exception: " << ex.what() << std::endl;

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

    std::cout << ex.what() << std::endl;

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

StoreConnection::StoreConnection( const std::string& name,
                                  const std::string& driver,
                                  const std::string& database,
                                  soci::session *sql_connection ):
                                  name_( name ),
                                  driver_( driver ),
                                  database_( database ),
                                  sql_connection_( sql_connection ),
                                  redis_connection_( nullptr ),
                                  mongo_client_( nullptr ),
                                  mongo_connection_( nullptr ) {

  //

}

StoreConnection::StoreConnection( const std::string& name,
                                  const std::string& driver,
                                  const std::string& database,
                                  sw::redis::Redis *redis_connection ):
                                  name_( name ),
                                  driver_( driver ),
                                  database_( database ),
                                  sql_connection_( nullptr ),
                                  redis_connection_( redis_connection ),
                                  mongo_client_( nullptr ),
                                  mongo_connection_( nullptr ) {

  //

}

StoreConnection::StoreConnection( const std::string& name,
                                  const std::string& driver,
                                  const std::string& database,
                                  mongocxx::client *mongo_client,
                                  mongocxx::v_noabi::database *mongo_database ):
                                  name_( name ),
                                  driver_( driver ),
                                  database_( database ),
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

soci::session* StoreConnection::sql_connection() const {

  return sql_connection_;

}

sw::redis::Redis* StoreConnection::redis_connection() const {

  return redis_connection_;

}

mongocxx::v_noabi::database* StoreConnection::mongo_connection() const {

  return mongo_connection_;

}

StoreConnectionSharedPtr make_store_connection( nanojson::element database_connection_config ) {

  StoreConnectionSharedPtr result { nullptr };

  try {

    const std::string& name = database_connection_config[ "name" ].to_string();

    const std::string& driver = database_connection_config[ "driver" ].to_string();

    const std::string& database = database_connection_config[ "database" ].to_string();

    const std::string& host = database_connection_config[ "host" ].to_string();

    const std::string& port = database_connection_config[ "port" ].to_string();

    const std::string& user = database_connection_config[ "user" ].to_string();

    const std::string& password = database_connection_config[ "password" ].to_string();

    std::cout << "Name: " << name << std::endl;
    std::cout << "Driver: " << driver << std::endl;
    std::cout << "Database: " << database << std::endl;
    std::cout << "Host: " << host << std::endl;
    std::cout << "Port: " << port << std::endl;
    std::cout << "User: " << user << std::endl;
    std::cout << "Password: " << password << std::endl;

    std::ostringstream connectionString;

    connectionString << "db=" << database;

    connectionString << " user=" << user;

    connectionString << " password='" << password << "'";

    connectionString << " host=" << host;

    if ( driver == "sql/mysql" ) {

      soci::session *connection = nullptr;

      try {

        connection = new soci::session( soci::mysql, connectionString.str() );

        if ( connection ) {

          result = std::make_shared<StoreConnection>( name,
                                                      driver,
                                                      database,
                                                      connection );

        }

      }
      catch ( const std::exception &ex ) {

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
                                                      connection );

        }

      }
      catch ( const std::exception &ex ) {

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
                                                      connection );

        }

      }
      catch ( const std::exception &ex ) {

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
                                                      connection );

        }

      }
      catch ( const std::exception &ex ) {

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
                                                      connection );

        }

      }
      catch ( const std::exception &ex ) {

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
                                                      connection );

        }

      }
      catch ( const std::exception &ex ) {

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

      const std::string& user = trim( database_connection_config[ "user" ].to_string() );
      const std::string& password = trim( database_connection_config[ "password" ].to_string() );

      if ( user != "" &&
           password != "" ) {

        url << user << ":" << password << "@";

      }

      url << database_connection_config[ "host" ].to_string() <<  ":" << database_connection_config[ "port" ].to_string();

      mongocxx::client *mongo_client = nullptr;
      mongocxx::v_noabi::database *mongo_connection = nullptr;

      //mongo "mongodb://${USER}:${DBPASSWORD}@<host>:<port>/admin?authSource=admin"
      //mongocxx::client client{ mongocxx::uri{ url.str() } };

      try {

        mongo_client = new mongocxx::client( mongocxx::uri{ url.str() } );

        mongo_connection = new mongocxx::v_noabi::database( mongo_client->database( database_connection_config[ "database" ].to_string() ) );

        if ( mongo_client ) {

          result = std::make_shared<StoreConnection>( name,
                                                      driver,
                                                      database,
                                                      mongo_client,
                                                      mongo_connection ); //client[ database_connection_config[ "database" ].to_string() ] );

        }

      }
      catch ( const std::exception& ex ) {

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
      url << "tcp://" << database_connection_config[ "host" ].to_string() << ":" << database_connection_config[ "port" ].to_string();

      sw::redis::Redis *connection = nullptr;

      try {

        connection = new sw::redis::Redis( url.str() );

        if ( connection ) {

          result = std::make_shared<StoreConnection>( name,
                                                      driver,
                                                      database,
                                                      connection );

        }

      }
      catch ( const std::exception &ex ) {

        std::cout << "Exception: " << ex.what() << std::endl;

        if ( connection ) {

          delete connection;
          connection = nullptr;

        }

      }

      const std::string& user = trim( database_connection_config[ "user" ].to_string() );
      const std::string& password = trim( database_connection_config[ "password" ].to_string() );

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

    std::cout << "Exception: " << ex.what() << std::endl;

  }

  return result;

}


// NoSQLConnectionSharedPtr make_no_sql_db_connection( nanojson::element database_connection_config ) {

//   NoSQLConnectionSharedPtr result { nullptr };

//   try {

//     const std::string& driver = database_connection_config[ "driver" ].to_string();

//     std::ostringstream connectionString;

//     connectionString << "driver=" + driver;

//     connectionString << "host=" + database_connection_config[ "host" ].to_string();

//     connectionString << "database=" + database_connection_config[ "database" ].to_string();

//     connectionString << "port=" + database_connection_config[ "port" ].to_string();

//     connectionString << "user=" + database_connection_config[ "user" ].to_string();

//     connectionString << "password='" + database_connection_config[ "password" ].to_string() << "'";

//     if ( driver == "no_sql/mongo_db" ) {

//       std::stringstream url;
//       url << "mongodb://";

//       const std::string& user = trim( database_connection_config[ "user" ].to_string() );
//       const std::string& password = trim( database_connection_config[ "password" ].to_string() );

//       if ( user != "" &&
//            password != "" ) {

//         url << user << ":" << password << "@";

//       }

//       url << database_connection_config[ "host" ].to_string() <<  ":" << database_connection_config[ "port" ].to_string();

//       //mongo "mongodb://${USER}:${DBPASSWORD}@<host>:<port>/admin?authSource=admin"
//       mongocxx::client connection{ mongocxx::uri{ url.str() } }; // "mongodb://" + database_connection_config[ "host" ].to_string() + ":" + database_connection_config[ "port" ].to_string() } };

//       //std::shared_ptr<mongocxx::v_noabi::database> database = std::make_shared<mongocxx::v_noabi::database>( connection[ database_connection_config[ "database" ].to_string() ] );
//       result = std::make_shared<NoSQLConnectionVariant>( connection[ database_connection_config[ "database" ].to_string() ] );

//     }
//     else if ( driver == "no_sql/redis" ) {

//       std::stringstream url;
//       url << "tcp://" << database_connection_config[ "host" ].to_string() << ":" << database_connection_config[ "port" ].to_string();

//       result = std::make_shared<NoSQLConnectionVariant>( url.str() );

//       const std::string& user = trim( database_connection_config[ "user" ].to_string() );
//       const std::string& password = trim( database_connection_config[ "password" ].to_string() );

//       if ( user != "" &&
//            password != "" ) {

//         std::get<sw::redis::Redis>( *result ).auth( user, password );

//       }
//       else if ( password != "" ) {

//         std::get<sw::redis::Redis>( *result ).auth( password );

//       }

//     }

//   }
//   catch ( const std::exception& ex ) {

//     std::cout << "Exception: " << ex.what() << std::endl;

//   }

//   return result;

// }

}
