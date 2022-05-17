#include <string>
#include <array>

#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/regex.hpp>

#include <hv/hlog.h>
#include <hv/HttpServer.h>
#include <hv/hssl.h>
#include <hv/hmain.h>
#include <hv/hthread.h>
#include <hv/hasync.h>     // import hv::async
#include <hv/requests.h>   // import requests::async
#include <hv/hdef.h>

#include <sw/redis++/redis++.h>
#include <sw/redis++/reply.h>
#include <sw/redis++/command_args.h>

#include "../modules/common/Common.hpp"

#include "../modules/common/StoreConnectionManager.hpp"

#include "Handlers.hpp"

#include "../modules/common/Response.hpp"

namespace Handlers {

int handler_store_query( const HttpContextPtr& ctx ) {

  auto type = ctx->type();

  u_int16_t status_code = 200;

  if ( type == APPLICATION_JSON ) {

    try {

      auto json_body = hv::Json::parse( ctx->body() );

      //std::cout << json_body.dump( 2 ) << std::endl;

      if ( json_body[ "Autorization" ].is_null() == false &&
           Common::trim( json_body[ "Autorization" ] ) != "" ) {

        status_code = check_token_is_valid_and_authorized( Common::trim( json_body[ "Autorization" ] ) );

        //check the token
        if ( status_code = 200 ) {

          if ( json_body[ "Execute" ].is_null() == false &&
               json_body[ "Execute" ].is_array() &&
               json_body[ "Execute" ].size() > 0 ) {

            //All query fails
            const std::string& thread_id = Common::xxHash_32( Common::get_thread_id() );

            auto execute_list = json_body[ "Execute" ];  //.array();

            std::size_t execute_list_size = 0; //json_body[ "Execute" ].size();

            // auto result = R"(
            //                   {
            //                     "StatusCode": 200,
            //                     "Code": "",
            //                     "Message": "",
            //                     "Mark": "",
            //                     "Log": null,
            //                     "IsError": false,
            //                     "Errors": {},
            //                     "Warnings": {},
            //                     "Count": 0,
            //                     "Data": {}
            //                   }
            //                 )"_json;

            auto result = Common::build_basic_response( 200, "", "", "", false, "" );

            for ( auto &execute: execute_list ) {

              const std::string &execute_id = execute.get<std::string>();

              if ( json_body[ execute_id ].is_object() ) {

                auto &execute_block = json_body[ execute_id ];

                const std::string &store = execute_block[ "Store" ];
                const std::string &transaction_id = execute_block[ "TransactionId" ].is_string() ?
                                                    Common::trim( execute_block[ "TransactionId" ].get<std::string>() ):
                                                    "";
                // const std::string &kind = execute_block[ "Kind" ].is_string() ?
                //                           Common::trim( execute_block[ "Kind" ].get<std::string>() ):
                //                           "";

                nlohmann::json command_list {};

                if ( execute_block[ "Command" ].is_array() ) {

                  command_list = execute_block[ "Command" ];

                }

                // const std::string &command = execute_block[ "Command" ].is_string() ?
                //                              Common::trim( execute_block[ "Command" ].get<std::string>() ):
                //                              "";

                if ( Store::StoreConnectionManager::store_connection_name_exists( store ) ) {

                  if ( command_list.size() > 0 ) {

                    // if ( kind == "query" ||
                    //      kind == "insert" ||
                    //      kind == "update" ||
                    //      kind == "delete" ) {

                    Store::StoreConnectionSharedPtr store_connection { nullptr };

                    Store::StoreSQLConnectionTransactionPtr store_sql_connection_in_transaction { nullptr };

                    bool is_store_connection_local_transaction = false;

                    bool is_store_connection_local_leased = false;

                    if ( transaction_id == "" ) {

                      store_connection = Store::StoreConnectionManager::lease_store_connection_by_name( store );

                      is_store_connection_local_leased = true;

                      if ( store_connection &&
                           store_connection->sql_connection() ) {

                        //Only if sql connection
                        store_sql_connection_in_transaction = store_connection->sql_connection()->begin(); //transaction begin

                        is_store_connection_local_transaction = true;

                      }

                    }
                    else {

                      store_connection = Store::StoreConnectionManager::store_connection_by_id( transaction_id );

                      store_sql_connection_in_transaction = Store::StoreConnectionManager::transaction_by_id( transaction_id );

                    }

                    if ( store_connection ) {

                      if ( store_connection->sql_connection() ) {

                        if ( store_sql_connection_in_transaction != nullptr ) {

                          if ( store_sql_connection_in_transaction->is_active() ) {

                            try {

                              //Stay sure no other thread use this connection
                              std::lock_guard<std::mutex> lock_guard( store_connection->mutex() );

                              for ( auto command_index = 0; command_index < command_list.size(); command_index++ ) {

                                try {

                                  const std::string& command = Common::trim( command_list[ command_index ] );

                                  if ( command != "" ) {

                                    std::string command_kind { command.substr( 0, 6 ) };

                                    boost::algorithm::to_lower( command_kind ); //.starts_with( "select" );

                                    bool is_select_command = command_kind.starts_with( "select" );

                                    if ( is_select_command ) {

                                      soci::rowset<soci::row> rs = ( store_connection->sql_connection()->prepare << command );

                                      auto data_list = hv::Json::array();

                                      for ( soci::rowset<soci::row>::const_iterator it = rs.begin(); it != rs.end(); ++it ) {

                                        soci::row const& row = *it;

                                        std::ostringstream doc;

                                        doc << '{';

                                        for( std::size_t i = 0; i < row.size(); ++i ) {

                                          auto props = row.get_properties( i );

                                          doc << '\"' << props.get_name() << "\":";

                                          if ( row.get_indicator( i ) != soci::i_null ) {

                                            switch( props.get_data_type() ) {
                                              case soci::dt_string:
                                                  doc << '\"' << row.get<std::string>( i ) << "\"";
                                                  break;
                                              case soci::dt_double:
                                                  doc << row.get<double>( i );
                                                  break;
                                              case soci::dt_integer:
                                                  doc << row.get<int>( i );
                                                  break;
                                              case soci::dt_long_long:
                                                  doc << row.get<long long>( i );
                                                  break;
                                              case soci::dt_unsigned_long_long:
                                                  doc << row.get<unsigned long long>( i );
                                                  break;
                                              case soci::dt_date:
                                                  std::tm when = row.get<std::tm>( i );
                                                  doc << '\"' << asctime( &when ) << "\"";
                                                  break;
                                            }

                                          }
                                          else {

                                            doc << "null";

                                          }

                                          if ( i < row.size() - 1 ) {

                                            doc << ",";

                                          }

                                        }

                                        doc << "}" << std::endl;

                                        data_list.push_back( hv::Json::parse( doc.str() ) );
                                        result[ "Count" ] = result[ "Count" ].get<int>() + 1;

                                      }

                                      result[ "Data" ][ execute_id + "_" + std::to_string( command_index ) ] = data_list;

                                    }
                                    else {

                                      soci::statement st = ( store_connection->sql_connection()->prepare << command );
                                      st.execute( true );

                                      auto affected_rows = st.get_affected_rows();

                                      auto data_list = hv::Json::array();

                                      auto json = hv::Json::parse( "{ \"Command\": \"" + command_kind + "\", \"AffectedRows\": " + std::to_string( affected_rows ) + " }" );

                                      if ( command_kind == "insert" ) {

                                        std::stringstream ss { command };

                                        std::string insert, into, table;

                                        ss >> insert >> into >> table;

                                        long long last_insert_id = -1;

                                        if ( store_connection->sql_connection()->get_last_insert_id( table, last_insert_id ) ) {

                                          json[ "LastInsertId" ] = last_insert_id;

                                        }
                                        else {

                                          json[ "LastInsertId" ] = last_insert_id;

                                        }

                                      }

                                      data_list.push_back( json );

                                      result[ "Count" ] = result[ "Count" ].get<int>() + 1;
                                      result[ "Data" ][ execute_id + "_" + std::to_string( command_index ) ] = data_list;

                                    }

                                  }
                                  else {

                                    auto result_error = R"(
                                                            {
                                                              "Code": "ERROR_TO_EXECUTE_COMMAND",
                                                              "Message": "The command cannot be empty",
                                                              "Mark": "2951E9E3F368-",
                                                              "Details": {
                                                                           "Id": "",
                                                                           "Store": "",
                                                                           "Message": ""
                                                                         }
                                                            }
                                                          )"_json;

                                    result_error[ "Mark" ] = result_error[ "Mark" ].get<std::string>() + thread_id;

                                    result_error[ "Details" ][ "Id" ] = execute_id + "_" + std::to_string( command_index );
                                    result_error[ "Details" ][ "Store" ] = store;

                                    //result[ "Errors" ].push_back( result_error );
                                    result[ "Errors" ][ execute_id + "_" + std::to_string( command_index ) ] = result_error;

                                  }

                                }
                                catch ( const std::exception &ex ) {

                                  auto result_error = R"(
                                                          {
                                                            "Code": "ERROR_TO_EXECUTE_COMMAND",
                                                            "Message": "Unexpected error to execute command",
                                                            "Mark": "B7A7E280921C-",
                                                            "Details": {
                                                                         "Id": "",
                                                                         "Store": "",
                                                                         "Message": ""
                                                                       }
                                                          }
                                                        )"_json;

                                  result_error[ "Mark" ] = result_error[ "Mark" ].get<std::string>() + thread_id;

                                  result_error[ "Details" ][ "Id" ] = execute_id + "_" + std::to_string( command_index );
                                  result_error[ "Details" ][ "Store" ] = store;
                                  result_error[ "Details" ][ "Message" ] = ex.what();

                                  //result[ "Errors" ].push_back( result_error );
                                  result[ "Errors" ][ execute_id + "_" + std::to_string( command_index ) ] = result_error;

                                }

                                execute_list_size += 1;

                              }

                              if ( is_store_connection_local_transaction  ) {

                                store_sql_connection_in_transaction->commit(); //Apply the transaction

                              }

                            }
                            catch ( const std::exception &ex ) {

                              auto result_error = R"(
                                                      {
                                                        "Code": "ERROR_TO_EXECUTE_COMMAND",
                                                        "Message": "Unexpected error to execute command",
                                                        "Mark": "722C178F170D-",
                                                        "Details": {
                                                                     "Id": "",
                                                                     "Store": "",
                                                                     "Message": ""
                                                                   }
                                                      }
                                                    )"_json;

                              result_error[ "Mark" ] = result_error[ "Mark" ].get<std::string>() + thread_id;

                              result_error[ "Details" ][ "Id" ] = execute_id;
                              result_error[ "Details" ][ "Store" ] = store;
                              result_error[ "Details" ][ "Message" ] = ex.what();

                              //result[ "Errors" ].push_back( result_error );
                              result[ "Errors" ][ execute_id ] = result_error;

                              if ( is_store_connection_local_transaction  ) {

                                store_sql_connection_in_transaction->rollback(); //Rollback the transaction

                              }

                            }

                          }
                          else {

                            auto result_error = R"(
                                                    {
                                                      "Code": "ERROR_STORE_SQL_TRANSACTION_IS_NOT_ACTIVE",
                                                      "Message": "The Store SQL Transactin is not active",
                                                      "Mark": "B87910868F92-",
                                                      "Details": {
                                                                   "Id": "",
                                                                   "Store": ""
                                                                 }
                                                    }
                                                  )"_json;

                            result_error[ "Mark" ] = result_error[ "Mark" ].get<std::string>() + thread_id;

                            result_error[ "Details" ][ "Id" ] = execute_id;
                            result_error[ "Details" ][ "Store" ] = store;

                            //result[ "Errors" ].push_back( result_error );
                            result[ "Errors" ][ execute_id ] = result_error;

                          }

                        }
                        else {

                          auto result_error = R"(
                                                  {
                                                    "Code": "ERROR_TRANSACTIONID_IS_INVALID",
                                                    "Message": "The transaction id is invalid or not found",
                                                    "Mark": "63ECC28A0FAA-",
                                                    "Details": {
                                                                 "Id": "",
                                                                 "Store": "",
                                                                 "TransactionId": ""
                                                               }
                                                  }
                                                )"_json;

                          result_error[ "Mark" ] = result_error[ "Mark" ].get<std::string>() + thread_id;

                          result_error[ "Details" ][ "Id" ] = execute_id;
                          result_error[ "Details" ][ "Store" ] = store;
                          result_error[ "Details" ][ "TransactionId" ] = transaction_id;

                          //result[ "Errors" ].push_back( result_error );
                          result[ "Errors" ][ execute_id ] = result_error;

                        }

                      }
                      else if ( store_connection->mongo_connection() ) {

                        try {

                          //Stay sure no other thread use this connection
                          std::lock_guard<std::mutex> lock_guard( store_connection->mutex() );

                          for ( auto command_index = 0; command_index < command_list.size(); command_index++ ) {

                            try {

                              const std::string& command = Common::trim( command_list[ command_index ] );

                              if ( command != "" ) {

                                std::string command_kind { command.substr( 0, 6 ) };

                                boost::algorithm::to_lower( command_kind ); //.starts_with( "select" );

                                if ( command_kind == "select" ) {

                                  std::istringstream ss { command }; //"Select * From sysPerson Where { \"FirstName\": { \"$regex\": \"^Tom[aá]s\", \"$options\" : \"i\" } }"

                                  std::string select, asterik, from, table, where, json_document;

                                  ss >> select >> asterik >> from >> table >> where; // >> json_document;

                                  std::size_t begin_json = command.find( " {", 0 );

                                  json_document = command.substr( begin_json + 1 );

                                  auto mongo_connection = store_connection->mongo_connection();

                                  auto cursor = (*mongo_connection)[ table ].find( bsoncxx::from_json( json_document ) );

                                  auto data_list = hv::Json::array();

                                  for ( auto&& doc : cursor ) {

                                    data_list.push_back( hv::Json::parse( bsoncxx::to_json( doc ) ) );
                                    result[ "Count" ] = result[ "Count" ].get<int>() + 1;

                                  }

                                  result[ "Data" ][ execute_id + "_" + std::to_string( command_index ) ] = data_list;

                                }
                                else if ( command_kind == "insert" ) {

                                  std::istringstream ss { command };

                                  std::string insert, into, table, json_document;

                                  ss >> insert >> into >> table; // >> json_document;

                                  std::size_t begin_json = command.find( " {", 0 );

                                  json_document = command.substr( begin_json + 1 );

                                  bsoncxx::document::value bjson_document = bsoncxx::from_json( json_document );

                                  auto mongo_connection = store_connection->mongo_connection();

                                  auto mongo_result = (*mongo_connection)[ table ].insert_one( std::move( bjson_document ) );

                                  std::string oid_str {};

                                  if ( mongo_result->inserted_id().type() == bsoncxx::type::k_oid ) {

                                    bsoncxx::oid oid = mongo_result->inserted_id().get_oid().value;
                                    oid_str = oid.to_string();

                                  }
                                  else {

                                    oid_str = "_";

                                  }

                                  auto data_list = hv::Json::array();

                                  data_list.push_back( hv::Json::parse( "{ \"Command\": \"" + command_kind + "\", \"_Id\": \"" + oid_str + "\" }" ) );

                                  result[ "Count" ] = result[ "Count" ].get<int>() + 1;
                                  result[ "Data" ][ execute_id + "_" + std::to_string( command_index ) ] = data_list;

                                }
                                else if ( command_kind == "update" ) {

                                  std::istringstream ss { command };

                                  std::string update, into, table, json_document, json_where;

                                  ss >> update >> into >> table; // >> json_document;

                                  std::size_t begin_json = command.find( " {", 0 );

                                  std::size_t end_json = command.find( " Where", begin_json );

                                  if ( end_json == std::string::npos ) {

                                    end_json = command.find( " where", begin_json );

                                  }

                                  json_document = command.substr( begin_json + 1, end_json - begin_json - 1 );

                                  bsoncxx::document::value bjson_document = bsoncxx::from_json( json_document );

                                  begin_json = end_json + 6;

                                  json_where = command.substr( begin_json );

                                  bsoncxx::document::value bjson_where = bsoncxx::from_json( json_where );

                                  auto mongo_connection = store_connection->mongo_connection();

                                  auto mongo_result = (*mongo_connection)[ table ].update_one( std::move( bjson_where ), std::move( bjson_document ) );

                                  auto data_list = hv::Json::array();

                                  auto json = hv::Json::parse( "{ \"Command\": \"" + command_kind + "\", \"AffectedRows\": " + std::to_string( mongo_result->modified_count() ) + " }" );

                                  data_list.push_back( json );

                                  result[ "Count" ] = result[ "Count" ].get<int>() + 1;
                                  result[ "Data" ][ execute_id + "_" + std::to_string( command_index ) ] = data_list;

                                }
                                else if ( command_kind == "delete" ) {

                                  std::istringstream ss { command };

                                  std::string insert, into, table, json_document;

                                  ss >> insert >> into >> table; // >> json_document;

                                  std::size_t begin_json = command.find( " {", 0 );

                                  json_document = command.substr( begin_json + 1 );

                                  bsoncxx::document::value bjson_document = bsoncxx::from_json( json_document );

                                  auto mongo_connection = store_connection->mongo_connection();

                                  auto mongo_result = (*mongo_connection)[ table ].delete_one( std::move( bjson_document ) );

                                  auto data_list = hv::Json::array();

                                  auto json = hv::Json::parse( "{ \"Command\": \"" + command_kind + "\", \"AffectedRows\": " + std::to_string( mongo_result->deleted_count() ) + " }" );

                                  data_list.push_back( json );

                                  result[ "Count" ] = result[ "Count" ].get<int>() + 1;
                                  result[ "Data" ][ execute_id + "_" + std::to_string( command_index ) ] = data_list;

                                }

                              }
                              else {

                                auto result_error = R"(
                                                        {
                                                          "Code": "ERROR_TO_EXECUTE_COMMAND",
                                                          "Message": "The command cannot be empty",
                                                          "Mark": "A55D54A40072-",
                                                          "Details": {
                                                                       "Id": "",
                                                                       "Store": "",
                                                                       "Message": ""
                                                                     }
                                                        }
                                                      )"_json;

                                result_error[ "Mark" ] = result_error[ "Mark" ].get<std::string>() + thread_id;

                                result_error[ "Details" ][ "Id" ] = execute_id + "_" + std::to_string( command_index );
                                result_error[ "Details" ][ "Store" ] = store;

                                //result[ "Errors" ].push_back( result_error );
                                result[ "Errors" ][ execute_id + "_" + std::to_string( command_index ) ] = result_error;

                              }

                            }
                            catch ( const std::exception &ex ) {

                              auto result_error = R"(
                                                      {
                                                        "Code": "ERROR_TO_EXECUTE_COMMAND",
                                                        "Message": "Unexpected error to execute command",
                                                        "Mark": "9DFBD6D4E91D-",
                                                        "Details": {
                                                                     "Id": "",
                                                                     "Store": "",
                                                                     "Message": ""
                                                                   }
                                                      }
                                                    )"_json;

                              result_error[ "Mark" ] = result_error[ "Mark" ].get<std::string>() + thread_id;

                              result_error[ "Details" ][ "Id" ] = execute_id;
                              result_error[ "Details" ][ "Store" ] = store;
                              result_error[ "Details" ][ "Message" ] = ex.what();

                              //result[ "Errors" ].push_back( result_error );
                              result[ "Errors" ][ execute_id + "_" + std::to_string( command_index ) ] = result_error;

                              if ( is_store_connection_local_transaction  ) {

                                store_sql_connection_in_transaction->rollback(); //Rollback the transaction

                              }

                            }

                            execute_list_size += 1;

                          }

                        }
                        catch ( const std::exception &ex ) {

                          auto result_error = R"(
                                                  {
                                                    "Code": "ERROR_TO_EXECUTE_COMMAND",
                                                    "Message": "Unexpected error to execute command",
                                                    "Mark": "531422A5C77F-",
                                                    "Details": {
                                                                 "Id": "",
                                                                 "Store": "",
                                                                 "Message": ""
                                                               }
                                                  }
                                                )"_json;

                          result_error[ "Mark" ] = result_error[ "Mark" ].get<std::string>() + thread_id;

                          result_error[ "Details" ][ "Id" ] = execute_id;
                          result_error[ "Details" ][ "Store" ] = store;
                          result_error[ "Details" ][ "Message" ] = ex.what();

                          //result[ "Errors" ].push_back( result_error );
                          result[ "Errors" ][ execute_id ] = result_error;

                          if ( is_store_connection_local_transaction  ) {

                            store_sql_connection_in_transaction->rollback(); //Rollback the transaction

                          }

                        }

                      }
                      else if ( store_connection->redis_connection() ) {

                        try {

                          //Stay sure no other thread use this connection
                          std::lock_guard<std::mutex> lock_guard( store_connection->mutex() );

                          //std::cout << command_list.size();

                          for ( auto command_index = 0; command_index < command_list.size(); command_index++ ) {

                            try {

                              const std::string& command = Common::trim( command_list[ command_index ] );

                              if ( command != "" ) {

                                std::vector<std::string> command_args;  //{ "SET", "KEY", "{ \"key_01\": \"value_01\", \"key_02\": \"value_02\" }" };

                                //boost::algorithm::split( command_args, command, boost::is_any_of( ", " ) ); //boost::is_any_of( ", " )

                                boost::algorithm::split_regex( command_args, command, boost::regex( "__ " ) ) ;

                                // for ( std::string element: command_args ) {

                                //   std::cout << element << std::endl;

                                // }

                                auto result_value = store_connection->redis_connection()->command( command_args.begin(), command_args.end() ); //, command_args.end() );

                                if ( result_value ) {

                                  if ( sw::redis::reply::is_string( *result_value ) ||
                                       sw::redis::reply::is_status( *result_value ) ) {

                                    std::string s_result = sw::redis::reply::parse<std::string>( *result_value );

                                    hv::Json json_result;

                                    if ( s_result[ 0 ] == '{' ) {

                                      //Possible json
                                      try {

                                        json_result = hv::Json::parse(s_result );

                                        result[ "Data" ][ execute_id + "_" + std::to_string( command_index ) ] = json_result;

                                      }
                                      catch ( std::exception &ex ) {

                                        result[ "Data" ][ execute_id + "_" + std::to_string( command_index ) ] = s_result;

                                      }

                                    }
                                    else {

                                      result[ "Data" ][ execute_id + "_" + std::to_string( command_index ) ] = s_result;

                                    }

                                  }
                                  else if ( sw::redis::reply::is_integer( *result_value ) ) {

                                    long long ll_result = sw::redis::reply::parse<long long>( *result_value );

                                    result[ "Data" ][ execute_id + "_" + std::to_string( command_index ) ] = ll_result;

                                  }
                                  else if ( sw::redis::reply::is_array( *result_value ) ) {

                                    std::vector<std::string> v_result = sw::redis::reply::parse<std::vector<std::string>>( *result_value );

                                    result[ "Data" ][ execute_id + "_" + std::to_string( command_index ) ] = v_result;

                                  }
                                  else if ( sw::redis::reply::is_nil( *result_value ) ) {

                                    result[ "Data" ][ execute_id + "_" + std::to_string( command_index ) ] = "";

                                  }

                                  result[ "Count" ] = result[ "Count" ].get<int>() + 1;

                                }
                                else {

                                  result[ "Data" ][ execute_id + "_" + std::to_string( command_index ) ] = "<NULL_RESPONSE_FROM_REDIS>";
                                  result[ "Count" ] = result[ "Count" ].get<int>() + 1;

                                }

                              }
                              else {

                                auto result_error = R"(
                                                        {
                                                          "Code": "ERROR_TO_EXECUTE_COMMAND",
                                                          "Message": "The command cannot be empty",
                                                          "Mark": "F5AC6481283D-",
                                                          "Details": {
                                                                       "Id": "",
                                                                       "Store": "",
                                                                       "Message": ""
                                                                     }
                                                        }
                                                      )"_json;

                                result_error[ "Mark" ] = result_error[ "Mark" ].get<std::string>() + thread_id;

                                result_error[ "Details" ][ "Id" ] = execute_id + "_" + std::to_string( command_index );
                                result_error[ "Details" ][ "Store" ] = store;

                                //result[ "Errors" ].push_back( result_error );
                                result[ "Errors" ][ execute_id + "_" + std::to_string( command_index ) ] = result_error;

                              }

                            }
                            catch ( const std::exception &ex ) {

                              auto result_error = R"(
                                                      {
                                                        "Code": "ERROR_TO_EXECUTE_COMMAND",
                                                        "Message": "Unexpected error to execute command",
                                                        "Mark": "7973862D0898-",
                                                        "Details": {
                                                                     "Id": "",
                                                                     "Store": "",
                                                                     "Message": ""
                                                                   }
                                                      }
                                                    )"_json;

                              result_error[ "Mark" ] = result_error[ "Mark" ].get<std::string>() + thread_id;

                              result_error[ "Details" ][ "Id" ] = execute_id;
                              result_error[ "Details" ][ "Store" ] = store;
                              result_error[ "Details" ][ "Message" ] = ex.what();

                              //result[ "Errors" ].push_back( result_error );
                              result[ "Errors" ][ execute_id + "_" + std::to_string( command_index ) ] = result_error;

                              if ( is_store_connection_local_transaction  ) {

                                store_sql_connection_in_transaction->rollback(); //Rollback the transaction

                              }

                            }

                            execute_list_size += 1;

                          }

                        }
                        catch ( const std::exception &ex ) {

                          auto result_error = R"(
                                                  {
                                                    "Code": "ERROR_TO_EXECUTE_COMMAND",
                                                    "Message": "Unexpected error to execute command",
                                                    "Mark": "44E423104FB2-",
                                                    "Details": {
                                                                 "Id": "",
                                                                 "Store": "",
                                                                 "Message": ""
                                                               }
                                                  }
                                                )"_json;

                          result_error[ "Mark" ] = result_error[ "Mark" ].get<std::string>() + thread_id;

                          result_error[ "Details" ][ "Id" ] = execute_id;
                          result_error[ "Details" ][ "Store" ] = store;
                          result_error[ "Details" ][ "Message" ] = ex.what();

                          //result[ "Errors" ].push_back( result_error );
                          result[ "Errors" ][ execute_id ] = result_error;

                          if ( is_store_connection_local_transaction  ) {

                            store_sql_connection_in_transaction->rollback(); //Rollback the transaction

                          }

                        }

                      }

                      if ( is_store_connection_local_leased ) {

                        Store::StoreConnectionManager::return_leased_store_connection( store_connection );

                      }

                    }
                    else if ( is_store_connection_local_leased ) {

                      auto result_error = R"(
                                              {
                                                "Code": "ERROR_NO_STORE_CONNECTION_AVAILABLE_IN_POOL",
                                                "Message": "No more Store connection available in pool to begin transaction",
                                                "Mark": "F6A706854C0F-",
                                                "Details": {
                                                             "Id": "",
                                                             "Store": ""
                                                           }
                                              }
                                            )"_json;

                      result_error[ "Mark" ] = result_error[ "Mark" ].get<std::string>() + thread_id;

                      result_error[ "Details" ][ "Id" ] = execute_id;
                      result_error[ "Details" ][ "Store" ] = store;

                      //result[ "Errors" ].push_back( result_error );
                      result[ "Errors" ][ execute_id ] = result_error;

                    }
                    else if ( store_sql_connection_in_transaction == nullptr ) {

                      auto result_error = R"(
                                              {
                                                "Code": "ERROR_TRANSACTIONID_IS_INVALID",
                                                "Message": "The transaction id is invalid or not found",
                                                "Mark": "E9D6B6A0411C-",
                                                "Details": {
                                                             "Id": "",
                                                             "Store": "",
                                                             "TransactionId": ""
                                                           }
                                              }
                                            )"_json;

                      result_error[ "Mark" ] = result_error[ "Mark" ].get<std::string>() + thread_id;

                      result_error[ "Details" ][ "Id" ] = execute_id;
                      result_error[ "Details" ][ "Store" ] = store;
                      result_error[ "Details" ][ "TransactionId" ] = transaction_id;

                      //result[ "Errors" ].push_back( result_error );
                      result[ "Errors" ][ execute_id ] = result_error;

                    }
                    else {

                      auto result_error = R"(
                                              {
                                                "Code": "ERROR_STORE_CONNECTION_NOT_FOUND",
                                                "Message": "Store connection not found",
                                                "Mark": "FE8CDA468AF2-",
                                                "Details": {
                                                             "Id": "",
                                                             "Store": ""
                                                           }
                                              }
                                            )"_json;

                      result_error[ "Mark" ] = result_error[ "Mark" ].get<std::string>() + thread_id;

                      result_error[ "Details" ][ "Id" ] = execute_id;
                      result_error[ "Details" ][ "Store" ] = store;

                      //result[ "Errors" ].push_back( result_error );
                      result[ "Errors" ][ execute_id ] = result_error;

                    }

                  }
                  else {

                    auto result_error = R"(
                                            {
                                              "Code": "ERROR_MISSING_FIELD_COMMAND",
                                              "Message": "The field Command is required as array of strings and cannot be empty or null",
                                              "Mark": "F093E072566F-",
                                              "Details": {
                                                           "Id": "",
                                                           "Store": ""
                                                         }
                                            }
                                          )"_json;

                    result_error[ "Mark" ] = result_error[ "Mark" ].get<std::string>() + thread_id;

                    result_error[ "Details" ][ "Id" ] = execute_id;
                    result_error[ "Details" ][ "Store" ] = store;

                    //result[ "Errors" ].push_back( result_error );
                    result[ "Errors" ][ execute_id ] = result_error;

                  }

                }
                else {

                  auto result_error = R"(
                                          {
                                            "Code": "ERROR_STORE_NAME_NOT_FOUND",
                                            "Message": "The Store name not found",
                                            "Mark": "AB8A4A9926B9-",
                                            "Details": {
                                                         "Id": "",
                                                         "Store": ""
                                                       }
                                          }
                                        )"_json;

                  result_error[ "Mark" ] = result_error[ "Mark" ].get<std::string>() + thread_id;

                  result_error[ "Details" ][ "Id" ] = execute_id;
                  result_error[ "Details" ][ "Store" ] = store;

                  //result[ "Errors" ].push_back( result_error );
                  result[ "Errors" ][ execute_id ] = result_error;

                }

              }
              else {

                auto result_error = R"(
                                        {
                                          "Code": "ERROR_EXECUTE_BLOCK_NOT_FOUND",
                                          "Message": "The Execute block not found",
                                          "Mark": "C08C98F164C4-",
                                          "Details": {
                                                       "Id": ""
                                                     }
                                        }
                                      )"_json;

                result_error[ "Mark" ] = result_error[ "Mark" ].get<std::string>() + thread_id;

                result_error[ "Details" ][ "Id" ] = execute_id;

                //result[ "Errors" ].push_back( result_error );
                result[ "Errors" ][ execute_id ] = result_error;

              }

            }

            if ( execute_list_size == result[ "Errors" ].size() ) {

              status_code = 400; //Bad request

              result[ "StatusCode" ] = status_code;
              result[ "Code" ] = "ERROR_FOR_ALL_EXECUTE_BLOCK_FAILED";
              result[ "Message" ] = "All execute block failed. Please check the Errors section for more details";
              result[ "Mark" ] = "2C4F94D79A1D-" + thread_id;
              result[ "IsError" ] = true;

            }
            else if ( execute_list_size == result[ "Data" ].size() ) {

              status_code = 200; //Ok

              //All query success
              result[ "Code" ] = "SUCCESS_FOR_ALL_EXECUTE_BLOCK";
              result[ "Message" ] = "All execute block success. Please check the Data section for more details";
              result[ "Mark" ] = "1B42BAF3927B-" + thread_id;

            }
            else {

              status_code = 202; //Accepted

              //At least one query fails
              result[ "StatusCode" ] = status_code;
              result[ "Code" ] = "WARNING_NOT_SUCCESS_FOR_ALL_EXECUTE_BLOCK";
              result[ "Message" ] = "Not all execute block success. Please check the Data, Warnings and Errors sections for more details";
              result[ "Mark" ] = "38C212E28716-" + thread_id;

            }

            ctx->response->content_type = APPLICATION_JSON;
            ctx->response->body = result.dump( 2 );

            //ctx->response->Json( result );

          }
          else {

            status_code = 400; //Bad request

            const std::string& thread_id = Common::xxHash_32( Common::get_thread_id() );

            auto result = Common::build_basic_response( status_code,
                                                        "ERROR_MISSING_FIELD_EXECUTE",
                                                        "The field Execute is required as array of strings and cannot be empty or null",
                                                        "FEE80B366130-" + thread_id,
                                                        true,
                                                        "" );

            ctx->response->content_type = APPLICATION_JSON;
            ctx->response->body = result.dump( 2 );

          }

        }
        else if ( status_code == 401 ) { //Unauthorized

          auto result = R"(
                            {
                              "StatusCode": 401,
                              "Code": "ERROR_AUTHORIZATION_TOKEN_NOT_VALID",
                              "Message": "The authorization token provided is not valid or not found",
                              "Mark": "AB20A712FDF4-",
                              "Log": null,
                              "IsError": true,
                              "Errors": {},
                              "Warnings": {},
                              "Count": 0,
                              "Data": {}
                            }
                          )"_json;

          const std::string& thread_id = Common::xxHash_32( Common::get_thread_id() );

          result[ "Mark" ] = result[ "Mark" ].get<std::string>() + thread_id; //result[ "Mark" ].value + "-" + std::this_thread::get_id();

        }
        else if ( status_code == 403 ) { //Forbidden

          auto result = R"(
                            {
                              "StatusCode": 403,
                              "Code": "ERROR_NOT_ALLOWED_ACCESS_TO_STORE",
                              "Message": "Not allowed access to store",
                              "Mark": "0936D63075E9-",
                              "Log": null,
                              "IsError": true,
                              "Errors": {},
                              "Warnings": {},
                              "Count": 0,
                              "Data": {}
                            }
                          )"_json;

          const std::string& thread_id = Common::xxHash_32( Common::get_thread_id() );

          result[ "Mark" ] = result[ "Mark" ].get<std::string>() + thread_id; //result[ "Mark" ].value + "-" + std::this_thread::get_id();

        }

      }
      else {

        status_code = 400; //Bad request

        const std::string& thread_id = Common::xxHash_32( Common::get_thread_id() );

        auto result = Common::build_basic_response( status_code,
                                                    "ERROR_MISSING_FIELD_AUTORIZATION",
                                                    "The field Autorization is required and cannot be empty or null",
                                                    "E97A1FD111E8-" + thread_id,
                                                    true,
                                                    "" );

        ctx->response->content_type = APPLICATION_JSON;
        ctx->response->body = result.dump( 2 );

      }

    }
    catch ( const std::exception &ex ) {

      status_code = 500; //Internal server error

      const std::string& thread_id = Common::xxHash_32( Common::get_thread_id() );

      auto result = Common::build_basic_response( status_code,
                                                  "ERROR_UNEXPECTED_EXCEPTION",
                                                  "Unexpected error. Please read the server log for more details.",
                                                  "F9B4CCCCAC92-" + thread_id,
                                                  true,
                                                  "" );

      result[ "Errors" ][ "Code" ] = "ERROR_INVALID_JSON_BODY_DATA";
      result[ "Errors" ][ "Message" ] = ex.what();
      result[ "Errors" ][ "Details" ] = {};

      ctx->response->content_type = APPLICATION_JSON;
      ctx->response->body = result.dump( 2 );

      //ctx->response->Json( result );

      hloge( "Exception: %s", ex.what() );

    }

  }
  else {

    status_code = 400; //Bad request

    const std::string& thread_id = Common::xxHash_32( Common::get_thread_id() );

    auto result = Common::build_basic_response( status_code,
                                                "JSON_BODY_FORMAT_REQUIRED",
                                                "JSON format is required",
                                                "456EE56536AA-" + thread_id,
                                                true,
                                                "" );

    ctx->response->content_type = APPLICATION_JSON;
    ctx->response->body = result.dump( 2 );

  }

  return status_code;

}

}
