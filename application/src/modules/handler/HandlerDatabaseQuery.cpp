#include <string>

#include <boost/algorithm/string.hpp>

#include <hv/hlog.h>
#include <hv/HttpServer.h>
#include <hv/hssl.h>
#include <hv/hmain.h>
#include <hv/hthread.h>
#include <hv/hasync.h>     // import hv::async
#include <hv/requests.h>   // import requests::async
#include <hv/hdef.h>

#include "../modules/common/Common.hpp"

#include "../modules/common/StoreConnectionManager.hpp"

#include "Handlers.hpp"

#include "../modules/common/Response.hpp"

namespace Handlers {

int handler_database_query( const HttpContextPtr& ctx ) {

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
                const std::string &kind = execute_block[ "Kind" ].is_string() ?
                                          Common::trim( execute_block[ "Kind" ].get<std::string>() ):
                                          "";

                nlohmann::json command_list {};

                if ( execute_block[ "Command" ].is_array() ) {

                  command_list = execute_block[ "Command" ];

                }

                // const std::string &command = execute_block[ "Command" ].is_string() ?
                //                              Common::trim( execute_block[ "Command" ].get<std::string>() ):
                //                              "";

                if ( Store::StoreConnectionManager::store_connection_name_exists( store ) ) {

                  if ( command_list.size() > 0 ) {

                    if ( kind == "query" ||
                         kind == "insert" ||
                         kind == "update" ||
                         kind == "delete" ) {

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

                                for ( auto command_index = 0; command_index < command_list.size(); command_index++ ) {

                                  try {

                                    const std::string& command = Common::trim( command_list[ command_index ] );

                                    if ( command != "" ) {

                                      std::string sql_command { command.substr( 0, 6 ) };

                                      boost::algorithm::to_lower( sql_command ); //.starts_with( "select" );

                                      bool select_sql = sql_command.starts_with( "select" );

                                      if ( select_sql ) {

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

                                        }

                                        result[ "Count" ] = result[ "Count" ].get<int>() + 1;
                                        result[ "Data" ][ execute_id + "_" + std::to_string( command_index ) ] = data_list;

                                      }
                                      else {

                                        soci::statement st = ( store_connection->sql_connection()->prepare << command );
                                        st.execute( true );

                                        auto affected_rows = st.get_affected_rows();

                                        auto data_list = hv::Json::array();

                                        data_list.push_back( hv::Json::parse( "{ \"Kind\": \"" + kind + "\", \"AffectedRows\": " + std::to_string( affected_rows ) + " }" ) );

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

                          //Execute mongo query

                          //Execute mongo query

                        }
                        else if ( store_connection->redis_connection() ) {

                          //Execute redis query

                          //Execute redis query

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
                                                  "Mark": "FE68C4A587DD-",
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
                                                  "Mark": "6240DC032C30-",
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
                                                  "Mark": "10D7992371E1-",
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
                    else if ( kind == "" ) {

                      auto result_error = R"(
                                              {
                                                "Code": "ERROR_MISSING_FIELD_KIND",
                                                "Message": "The field Kind is required and cannot be empty or null",
                                                "Mark": "3C1B1AE8B821-",
                                                "Details": {
                                                             "Id": "",
                                                             "Store": "",
                                                             "ValidKindAre": [ "insert", "delete", "update", "query" ]
                                                           }
                                              }
                                            )"_json;

                      result_error[ "Mark" ] = result_error[ "Mark" ].get<std::string>() + thread_id;

                      result_error[ "Details" ][ "Id" ] = execute_id;
                      result_error[ "Details" ][ "Store" ] = store;

                      //result[ "Errors" ].push_back( result_error );
                      result[ "Errors" ][ execute_id ] = result_error;

                    }
                    else {

                      auto result_error = R"(
                                              {
                                                "Code": "ERROR_EXECUTE_KIND_NOT_VALID",
                                                "Message": "The execute block kind is not valid",
                                                "Mark": "FA2D3EBDB316-",
                                                "Details": {
                                                             "Id": "",
                                                             "Store": "",
                                                             "ValidKindAre": [ "insert", "delete", "update", "query" ]
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
                                              "Mark": "C4492DAB7BC8-",
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

            // auto result = R"(
            //                   {
            //                     "StatusCode": 400,
            //                     "Code": "ERROR_MISSING_FIELD_EXECUTE",
            //                     "Message": "The field Execute is required as array of strings and cannot be empty or null",
            //                     "Mark": "FEE80B366130-",
            //                     "Log": null,
            //                     "IsError": true,
            //                     "Errors": {},
            //                     "Warnings": {},
            //                     "Count": 0,
            //                     "Data": {}
            //                   }
            //                 )"_json;

            //result[ "Mark" ] = result[ "Mark" ].get<std::string>() + thread_id; //result[ "Mark" ].value + "-" + std::this_thread::get_id();

            //ctx->response->Json( result );

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

        // auto result = R"(
        //                   {
        //                     "StatusCode": 400,
        //                     "Code": "ERROR_MISSING_FIELD_AUTORIZATION",
        //                     "Message": "The field Autorization is required and cannot be empty or null",
        //                     "Mark": "E97A1FD111E8-",
        //                     "Log": null,
        //                     "IsError": true,
        //                     "Errors": {},
        //                     "Warnings": {},
        //                     "Count": 0,
        //                     "Data": {}
        //                   }
        //                 )"_json;

        //const std::string& thread_id = Common::xxHash_32( Common::get_thread_id() );

        //result[ "Mark" ] = result[ "Mark" ].get<std::string>() + thread_id; //result[ "Mark" ].value + "-" + std::this_thread::get_id();

        //ctx->response->Json( result );

      }

    }
    catch ( const std::exception &ex ) {

      status_code = 500; //Internal server error

      const std::string& thread_id = Common::xxHash_32( Common::get_thread_id() );

      auto result = Common::build_basic_response( status_code,
                                                  "ERROR_INVALID_JSON_BODY_DATA",
                                                  "Must be a valid json data format",
                                                  "F9B4CCCCAC92-" + thread_id,
                                                  true,
                                                  "" );


      // auto result = R"(
      //                   {
      //                     "StatusCode": 500,
      //                     "Code": "ERROR_INVALID_JSON_BODY_DATA",
      //                     "Message": "Must be a valid json data format",
      //                     "Mark": "F9B4CCCCAC92-",
      //                     "Log": null,
      //                     "IsError": true,
      //                     "Errors": {},
      //                     "Warnings": {},
      //                     "Count": 0,
      //                     "Data": {}
      //                   }
      //                 )"_json;

      //const std::string& thread_id = Common::xxHash_32( Common::get_thread_id() );

      //result[ "Mark" ] = result[ "Mark" ].get<std::string>() + thread_id; //result[ "Mark" ].value + "-" + std::this_thread::get_id();

      //result[ "Message" ] = ex.what();
      result[ "Errors" ][ "Message" ] = ex.what();

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

    // auto result = R"(
    //                   {
    //                     "StatusCode": 400,
    //                     "Code": "JSON_BODY_FORMAT_REQUIRED",
    //                     "Message": "JSON format is required",
    //                     "Mark": "456EE56536AA-",
    //                     "Log": null,
    //                     "IsError": true,
    //                     "Errors": {},
    //                     "Warnings": {},
    //                     "Count": 0,
    //                     "Data": {}
    //                   }
    //                 )"_json;

    // const std::string& thread_id = Common::xxHash_32( Common::get_thread_id() );

    // result[ "Mark" ] = result[ "Mark" ].get<std::string>() + thread_id; //result[ "Mark" ].value + "-" + std::this_thread::get_id();

    //ctx->response->Json( result );

  }

  return status_code;

}

}
