/*
 * sample http server
 * more detail see examples/httpd
 *
 */

// #include <chrono>
// #include <any>
#include <iostream>

#include "hv/hv.h"
#include "hv/hlog.h"
#include "hv/HttpServer.h"
#include "hv/hssl.h"
#include "hv/hmain.h"
#include "hv/hthread.h"
#include "hv/hasync.h"     // import hv::async
#include "hv/requests.h"   // import requests::async
#include "hv/hdef.h"

#include <soci/soci.h>

#include <bsoncxx/builder/basic/array.hpp>
#include <bsoncxx/builder/basic/document.hpp>
#include <bsoncxx/builder/basic/kvp.hpp>
#include <bsoncxx/types.hpp>
#include <bsoncxx/json.hpp>
#include <mongocxx/client.hpp>
#include <mongocxx/instance.hpp>
#include <mongocxx/uri.hpp>

#include "bcrypt/bcrypt.h"

#include <sw/redis++/redis++.h>

#include "xxhash.h"

#include "uuid_v4/uuid_v4.h"

#include "modules/common/Common.hpp"

#include "modules/handler/handler.hpp"

#include "modules/handler/Handlers.hpp"

#include "modules/common/StoreConnectionManager.hpp"

/*
 * #define TEST_HTTPS 1
 *
 * @build   ./configure --with-openssl && make clean && make
 *
 * @server  bin/http_server_test 8080
 *
 * @client  curl -v http://127.0.0.1:8080/ping
 *          curl -v https://127.0.0.1:8443/ping --insecure
 *          bin/curl -v http://127.0.0.1:8080/ping
 *          bin/curl -v https://127.0.0.1:8443/ping
 *
 */
#define TEST_HTTPS 0

// int convert_from_any( const std::any &param ) {

//   int result = 0;

//   try {

//     result = std::any_cast<int>( param );

//   }
//   catch ( const std::exception &ex ) {

//     std::cout << ex.what() << std::endl;

//   }

//   return result;

// }

bool init_stores() {

  int result { 0 };

  if ( Common::config_json[ "stores_connections" ].is_defined() &&
       Common::config_json[ "stores_connections" ].is_array() ) {

    auto stores_connections_config_list = Common::config_json[ "stores_connections" ].as_array_ref();

    Store::StoreConnectionManager::init_stores_connections( stores_connections_config_list );

    result = 1;

  }
  else {

    hlogw( "No stores_connections section found or is not array of valid objects. Please check server config file" );

  }

  if ( Common::config_json[ "stores_connections_alias" ].is_defined() &&
       Common::config_json[ "stores_connections_alias" ].is_object() ) {

    auto stores_connections_alias_config_object = Common::config_json[ "stores_connections_alias" ].as_object_ref();

    Store::StoreConnectionManager::init_stores_connections_alias( stores_connections_alias_config_object );

    // for ( auto it = stores_connections_alias.begin(); it != stores_connections_alias.end(); it++) {

    //   std::cout << "key: " << it->first << std::endl;
    //   std::cout << "value: " << it->second << std::endl;

    // }

    result += 1;

  }
  else {

    hlogw( "No stores_connections_alias section found or is not valid object. Please check server config file" );

  }

  return result >= 2;

}

int main( int argc, char** argv ) {

  HV_MEMCHECK;

  // struct timeval tv;
  // struct tm* tm = NULL;
  // gettimeofday( &tv, NULL );
  // time_t tt = tv.tv_sec;
  // tm = localtime( &tt );
  // int year,month,day,hour,min,sec,us;

  // year     = tm->tm_year + 1900;
  // month    = tm->tm_mon  + 1;
  // day      = tm->tm_mday;
  // hour     = tm->tm_hour;
  // min      = tm->tm_min;
  // sec      = tm->tm_sec;
  // us       = tv.tv_usec / 1000;

  main_ctx_init( argc, argv );

  const char *path = argv[ 0 ];

  std::cout << "Current path: " << path << std::endl;

  const std::string config_file_path = Common::get_file_path( path ) + "config/main.json";

  std::cout << "Config File: " << config_file_path << std::endl;

  Common::config_json = Common::get_config( config_file_path );

  init_stores();

  int port = Common::config_json[ "port" ].is_number() ? Common::config_json[ "port" ].to_integer(): 8080;

  // if ( argc > 1 ) {

  //   port = atoi( argv[ 1 ] );

  // }

  if ( port <= 0 ) port = 8080;

  HttpService router;

  router.preprocessor = Handler::preprocessor;
  router.postprocessor = Handler::postprocessor;
  //router.largeFileHandler = Handler::largeFileHandler;
  router.errorHandler = Handler::errorHandler;

  std::string base_path = Common::config_json[ "base_path" ].is_string() ? Common::config_json[ "base_path" ].to_string(): "";

  std::stringstream end_point;

  end_point << base_path;

  const bool add_separator = base_path[ base_path.size() - 1 ] != '/';

  if ( add_separator ) {

    end_point << "/";

  }

  end_point << "system/store/transaction/begin";

  router.POST( end_point.str().c_str(), Handlers::handler_store_transaction_begin );

  // end_point.seekp( 0 );
  // end_point.seekg( 0 );
  // end_point.str( "" );
  // end_point.clear();

  {

    auto temp = std::stringstream();
    end_point.swap( temp ); //Clear the buffer

  }

  end_point << base_path;

  if ( add_separator ) {

    end_point << "/";

  }

  end_point << "system/store/transaction/commit";

  //std::cout << end_point.str() << std::endl;

  router.POST( end_point.str().c_str(), Handlers::handler_store_transaction_commit );

  //end_point.clear();

  {

    auto temp = std::stringstream();
    end_point.swap( temp ); //Clear the buffer

  }

  end_point << base_path;

  if ( add_separator ) {

    end_point << "/";

  }

  end_point << "system/store/transaction/rollback";

  router.POST( end_point.str().c_str(), Handlers::handler_store_transaction_rollback );

  //end_point.clear();

  {

    auto temp = std::stringstream();
    end_point.swap( temp ); //Clear the buffer

  }

  end_point << base_path;

  if ( add_separator ) {

    end_point << "/";

  }

  end_point << "system/store/query";

  router.POST( end_point.str().c_str(), Handlers::handler_store_query );

  router.GET( "/ping", []( HttpRequest* req, HttpResponse* resp ) {

    return resp->String( "pong" );

  });

  router.GET( "/data", []( HttpRequest* req, HttpResponse* resp ) {

    static char data[] = "0123456789";
    return resp->Data( data, 10 /*, false */ );

  });

  router.GET( "/paths", [&router]( HttpRequest* req, HttpResponse* resp ) {

    return resp->Json( router.Paths() );

  });

  router.GET( "/get", []( HttpRequest* req, HttpResponse* resp ) {

    resp->json[ "origin" ] = req->client_addr.ip;
    resp->json[ "url" ] = req->url;
    resp->json[ "args" ] = req->query_params;
    resp->json[ "headers" ] = req->headers;
    return 200;

  });

  router.POST( "/echo", [](const HttpContextPtr& ctx) {

    return ctx->send( ctx->body(), ctx->type() );

  });

  // curl -v http://ip:port/www.*
  // curl -v http://ip:port/www.example.com
  router.GET( "/www.*", []( const HttpRequestPtr& req, const HttpResponseWriterPtr& writer ) {

    HttpRequestPtr req2(new HttpRequest);
    req2->url = req->path.substr(1);

    requests::async( req2, [writer]( const HttpResponsePtr& resp2 ){

      writer->Begin();

      if ( resp2 == NULL ) {

        writer->WriteStatus(HTTP_STATUS_NOT_FOUND);
        writer->WriteHeader( "Content-Type", "text/html" );
        writer->WriteBody( "<center><h1>404 Not Found</h1></center>" );

      }
      else {

        writer->WriteResponse(resp2.get());

      }

      writer->End();

    });

  });

  // curl -v http://127.0.0.1:8080/upload -d '@README.md'
  // curl -v http://127.0.0.1:8080/upload -F 'file=@README.md'
  router.POST( "/upload", Handler::upload );
  router.GET( "/downloads/*", Handler::largeFileHandler );

  // //router.base_url

  // //std::cout << main_ctx_t. << std::endl;
  // //hlog_set_file("test.log");
  http_server_t server;
  server.service = &router;
  server.port = port;
// #if TEST_HTTPS
//     server.https_port = 8443;
//     hssl_ctx_init_param_t param;
//     memset(&param, 0, sizeof(param));
//     param.crt_file = "cert/server.crt";
//     param.key_file = "cert/server.key";
//     param.endpoint = HSSL_SERVER;
//     if (hssl_ctx_init(&param) == NULL) {
//         fprintf(stderr, "hssl_ctx_init failed!\n");
//         return -20;
//     }
// #endif

  // uncomment to test multi-processes
  // server.worker_processes = 4;
  // uncomment to test multi-threads

  //hlogi("(Before) Log file is: %s", g_main_ctx.logfile);

  int threads = std::stoi( Common::config_json[ "threads" ] );

  server.worker_threads = threads > 0 ? threads: 4;

  http_server_run( &server, 0 );

  // std::string password = "top_secret";

  // // ***** BCRYPT *****
  // std::string hash = bcrypt::generateHash( password );

  // std::cout << "Hash: " << hash << std::endl;

  // std::cout << "\"" << password << "\" : " << bcrypt::validatePassword( password, hash ) << std::endl;
  // std::cout << "\"wrong\" : " << bcrypt::validatePassword( "wrong", hash ) << std::endl;
  // // ***** BCRYPT *****

  // // ***** XXHASH *****
  // // XXH64_hash_t xxhash64 = XXH64(password.c_str(), password.length(), 0);

  // XXH32_hash_t xxhash32 = XXH32( password.c_str(), password.length(), 0 );

  // //From Dec => Hex
  // std::stringstream ss;
  // ss<< std::hex << xxhash32; // int decimal_value
  // std::string res ( ss.str() );

  // std::cout << "xxHahs32: " << res << std::endl;

  // //From Hex => Dec
  // // std::stringstream ss;
  // // ss  << hex_value ; // std::string hex_value
  // // ss >> std::hex >> decimal_value ; //int decimal_value

  // // std::cout << decimal_value ;
  // // ***** XXHASH *****

  // if ( config_json[ "base_path" ].is_defined() ) {

  //   std::cout << "config_json[ \"base_path\" ] = " << config_json[ "base_path" ].to_string() << std::endl;

  // }

  // if ( config_json[ "databases" ].is_defined() ) {

  //   auto databases_connection_config = config_json[ "databases" ].as_array();

  //   Store::StoreConnectionManager::init_connection_manager( databases_connection_config );

    // auto store_connection = Store::StoreConnectionManager::lease_store_connection_by_name( "sql_01" );

    // /*
    // for ( auto & database_connection_config: databases_connection_config  ) {

    //   auto store_connection = Common::make_store_connection( database_connection_config );
    //   */

    //   if ( store_connection ) {

    //     std::cout << "Connection index: " << store_connection->index() << std::endl;

    //     if ( store_connection->sql_connection() ) {

    //       std::cout << "*** SQL = " << store_connection->sql_connection()->get_backend_name() << " ***" << std::endl;

    //       soci::transaction *transaction { nullptr };

    //       try {

    //         transaction = store_connection->sql_connection()->begin();

    //         const std::string id = "";

    //         soci::rowset<soci::row> rs = ( store_connection->sql_connection()->prepare << "Select * From sysPerson where Id != :id", soci::use( id, "id" ) );

    //         for ( soci::rowset<soci::row>::const_iterator it = rs.begin(); it != rs.end(); ++it ) {

    //           soci::row const& row = *it;

    //           std::string const& extra_data = row.get_indicator( "ExtraData" ) == soci::i_null ? "NULL": row.get<std::string>( "ExtraData" );

    //           // dynamic data extraction from each row:
    //           std::cout << "Id: " << row.get<std::string>( "Id" ) << std::endl
    //                     << "Name: " << row.get<std::string>( "FirstName" ) << " " << row.get<std::string>( "LastName" ) << std::endl
    //                     << "ExtraData: " << extra_data << std::endl;

    //         }

    //         transaction->commit();

    //       }
    //       catch ( const std::exception &ex ) {

    //         if ( transaction &&
    //              transaction->is_active() ) {

    //           transaction->rollback();

    //         }

    //         std::cout << ex.what() << std::endl;

    //       }

    //     }

    //     if ( store_connection->redis_connection() ) {

    //       std::cout << "*** NO SQL = Redis ***" << std::endl;

    //       store_connection->redis_connection();

    //       auto val = store_connection->redis_connection()->get( "key1" );

    //       if ( !val ) {

    //         // initialize random seed: /
    //         srand( time( nullptr ) );

    //         // generate secret number between 1 and 10:
    //         int random_value = rand() % 10 + 1;

    //         store_connection->redis_connection()->set( "key1", std::to_string( random_value ), std::chrono::seconds( 60 ) );

    //         val = store_connection->redis_connection()->get( "key1" );

    //         std::cout << "Key1 (From Set): " << *val << std::endl;

    //       }
    //       else {

    //         // Dereference val to get the returned value of std::string type.
    //         std::cout << "Key1 (From Cache): " << *val << std::endl;

    //       }   // else key doesn't exist.


    //     }

    //     if ( store_connection->mongo_connection() ) {

    //       std::cout << "*** NO SQL = Mongo ***" << std::endl;

    //       bsoncxx::document::value restaurant_doc = bsoncxx::from_json( "{ \"Id\": 1, \"FirstName\": \"Loly Valentina\", \"LastName\": \"Gomez Fermin\" }" );

    //       // We choose to move in our document here, which transfers ownership to insert_one()
    //       auto result = (*store_connection->mongo_connection())[ "sysPerson" ].insert_one( std::move( restaurant_doc ) );

    //       if ( result->inserted_id().type() == bsoncxx::type::k_oid ) {

    //         bsoncxx::oid id = result->inserted_id().get_oid().value;
    //         std::string id_str = id.to_string();
    //         std::cout << "Inserted id: " << id_str << std::endl;

    //       }
    //       else {

    //         std::cout << "Inserted id was not an OID type" << std::endl;

    //       }

    //     }

    //   }

    // //}

    // Store::StoreConnectionManager::return_leased_store_connection( store_connection );

    // store_connection = Store::StoreConnectionManager::lease_store_connection_by_name( "sql_01" );

    // if ( store_connection ) {

    //   std::cout << "Connection index: " << store_connection->index() << std::endl;

    // }

    // Store::StoreConnectionManager::return_leased_store_connection( store_connection );

    /*
    // ***** SOCI *****
    try {

      // std::cout << "config_json[ \"driver\" ] = " << databases[ "driver" ].to_string() << std::endl;

      // std::shared_ptr<soci::session> db_connection = common::make_db_connection( database_main_config );

      // soci::transaction * transaction1 = db_connection->begin(); //( db_connection );

      // //process_a not apply the transaction, is not local created
      // common::process_a( transaction1 );

      // transaction1->commit(); //Now the transaction is not active

      // if ( transaction1 == db_connection->begin() ) {

      //   //Still work. process_a generate a local transaction by self,
      //   //and apply your local generated transaction
      //   common::process_a( transaction1 );

      //   //Still work. process_b generate a local transaction by self,
      //   //and pass to nested call in process_a, but process_a not apply that.
      //   //process_b must apply your local generated transaction
      //   common::process_b( transaction1 );

      // }

      // delete transaction1;

      // soci::transaction transaction2 { *db_connection }; //( db_connection );

      // common::process_c( &transaction2 );

      // if ( &transaction2 == db_connection->current_transaction() ) {

      //     common::process_c( db_connection->current_transaction() );

      //     if ( &transaction2 == db_connection->begin() ) {

      //         common::process_c( db_connection->begin() );

      //     }

      // }

      // transaction2.rollback();

      // if ( &transaction2 == db_connection->begin() &&
      //      db_connection->current_transaction()->by_session() == false ) {

      //     common::process_c( db_connection->begin() );

      // }

      // transaction2.rollback();

    }
    catch ( const std::exception &ex ) {

      std::cout << ex.what() << std::endl;

    }
    // ***** SOCI *****

    // ***** REDIS *****
    try {

      sw::redis::Redis redis = sw::redis::Redis( "tcp://127.0.0.1:6379" );

      redis.auth( "12345678" );
      //redis.command("AUTH", "12345678");

      // initialize random seed: /
      srand( time( NULL ) );

      // generate secret number between 1 and 10:
      int secret = rand() % 10 + 1;

      auto val = redis.get( "key1" );    // val is of type OptionalString. See 'API Reference' section for details.

      //sw::redis::UpdateType type = sw::redis::UpdateType::ALWAYS;

      if ( !val ) {

        redis.set( "key1", std::to_string( secret ), std::chrono::seconds(30));

        val = redis.get( "key1" );

        std::cout << "Key1 (From Set): " << *val << std::endl;

      }
      else {

        // Dereference val to get the returned value of std::string type.
        std::cout << "Key1 (From Cache): " << *val << std::endl;

      }   // else key doesn't exist.

    }
    catch ( const std::exception &ex ) {

      std::cout << ex.what() << std::endl;

    }
    // ***** REDIS *****
    */

    // ***** MONGODB *****
    // try {

    //   //mongocxx::instance inst{};
    //   mongocxx::client conn{ mongocxx::uri{ "mongodb://localhost:27017" } };

    //   mongocxx::v_noabi::database db = conn[ "TestDB" ];

    //   using bsoncxx::builder::basic::kvp;
    //   using bsoncxx::builder::basic::make_array;
    //   using bsoncxx::builder::basic::make_document;

    //   // bsoncxx::document::value restaurant_doc = make_document(
    //   //     kvp("address",
    //   //         make_document(kvp("street", "2 Avenue"),
    //   //                       kvp("zipcode", 10075),
    //   //                       kvp("building", "1480"),
    //   //                       kvp("coord", make_array(-73.9557413, 40.7720266)))),
    //   //     kvp("borough", "Manhattan"),
    //   //     kvp("cuisine", "Italian"),
    //   //     kvp("grades",
    //   //         make_array(
    //   //             make_document(kvp("date", bsoncxx::types::b_date{std::chrono::milliseconds{12323}}),
    //   //                           kvp("grade", "A"),
    //   //                           kvp("score", 11)),
    //   //             make_document(
    //   //                 kvp("date", bsoncxx::types::b_date{std::chrono::milliseconds{121212}}),
    //   //                 kvp("grade", "B"),
    //   //                 kvp("score", 17)))),
    //   //     kvp("name", "Vella"),
    //   //     kvp("restaurant_id", "41704620"));

    //   //https://github.com/mongodb/mongo-cxx-driver/blob/master/examples/mongocxx/query.cpp
    //   //https://stackoverflow.com/questions/3305561/how-to-query-mongodb-with-like
    //   std::cout << bsoncxx::to_json( make_document(kvp("grade.score", make_document( kvp( "$gt", 30 ) ) ) ) ) << std::endl;

    //   bsoncxx::document::value restaurant_doc = bsoncxx::from_json( "{ \"id\": 1, \"first_name\": \"Loly Valentina\", \"last_name\": \"Gomez Fermin\" }" );

    //   // We choose to move in our document here, which transfers ownership to insert_one()
    //   auto result = db[ "restaurants" ].insert_one( std::move( restaurant_doc ) );

    //   if ( result->inserted_id().type() == bsoncxx::type::k_oid ) {

    //     bsoncxx::oid id = result->inserted_id().get_oid().value;
    //     std::string id_str = id.to_string();
    //     std::cout << "Inserted id: " << id_str << std::endl;

    //   }
    //   else {

    //     std::cout << "Inserted id was not an OID type" << std::endl;

    //   }

    //   //auto cursor = db["restaurants"].find( bsoncxx::from_json( "{ \"first_name\": { \"$eq\": \"Tomas\" } }" ) );
    //   //Contains
    //   auto cursor = db["restaurants"].find( bsoncxx::from_json( "{ \"first_name\": { \"$regex\": \"Rafael$\", \"$options\" : \"i\" } }" ) ); Like 'Rafael%'
    //   auto cursor = db["restaurants"].find( bsoncxx::from_json( "{ \"first_name\": { \"$regex\": \"^Rafael\", \"$options\" : \"i\" } }" ) ); Like '%Rafael'
    //   auto cursor = db["restaurants"].find( bsoncxx::from_json( "{ \"first_name\": { \"$regex\": \"Rafael\", \"$options\" : \"i\" } }" ) ); Like '%Rafael%'
    //   auto cursor = db["restaurants"].find( bsoncxx::from_json( "{ \"first_name\": { \"$regex\": \"Tom[aá]s\", \"$options\" : \"i\" } }" ) ); Like '%Tomás%' Like '%Tomas%'

    //   for (auto&& doc : cursor) {

    //     std::cout << bsoncxx::to_json( doc ) << std::endl;

    //   }

    // }
    // catch ( const std::exception &ex ) {

    //   std::cout << ex.what() << std::endl;

    // }
    // ***** MONGODB *****
    /**/

  //}
  // std::any any_value = 10;

  // int x = convert_from_any( any_value );

  // std::cout << "x(int:10): " << x << std::endl;

  // any_value = "Hello!!!";

  // x = convert_from_any( any_value );

  // std::cout << "x(string:Hello!!!): " << x << std::endl;

  std::cout << "Running server at 0.0.0.0:" + std::to_string( port ) + base_path << std::endl;
  std::cout << "Press enter to exit" << std::endl;
  hlogi("Log file is: %s", g_main_ctx.logfile);

  // SAFE_FREE( g_main_ctx.save_argv[ 0 ] );
  // SAFE_FREE( g_main_ctx.save_argv );
  // SAFE_FREE( g_main_ctx.cmdline );
  // SAFE_FREE( g_main_ctx.save_envp[ 0 ] );
  // SAFE_FREE( g_main_ctx.save_envp );
  // press Enter to stop
  while ( getchar() != '\n' );
  http_server_stop(&server);

  main_ctx_finish();

  return 0;

}
