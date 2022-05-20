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

inline void execute_sql_query( const std::string& thread_id,
                               const std::string& authorization,
                               const std::string& execute_id,
                               const std::string& store,
                               const std::string& transaction_id,
                               const bool is_store_connection_local_transaction,
                               const Store::StoreSQLConnectionTransactionPtr& store_sql_connection_in_transaction,
                               const Store::StoreConnectionSharedPtr& store_connection,
                               const Common::NLOJSONObject& command_list,
                               std::size_t& execute_list_size,
                               const Common::NJSONElement& store_rule_list,
                               Common::NLOJSONObject& result ) {

  if ( store_sql_connection_in_transaction != nullptr ) {

    if ( store_sql_connection_in_transaction->is_active() ) {

      try {

        //Stay sure no other thread use this connection
        std::lock_guard<std::mutex> lock_guard( store_connection->mutex() );

        for ( auto command_index = 0; command_index < command_list.size(); command_index++ ) {

          try {

            const std::string& command = Common::trim( command_list[ command_index ] );

            if ( command != "" ) {

              const auto& check_command_result = Security::check_command_to_store_is_authorizated( authorization,
                                                                                                   store,
                                                                                                   command,
                                                                                                   store_rule_list );

              if ( check_command_result == 101 ) { //Command authorized

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
                          case soci::dt_xml:
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
              else if ( check_command_result == -1 ) {

                auto result_error = Common::build_detail_block_response( "ERROR_UNABLE_TO_CHECK_COMMAND_DENIED_OR_ALLOWED",
                                                                         "Unable to check the command is denied or allowed. Please check server config file",
                                                                         "A864A5A1057C-" + thread_id,
                                                                         "{ \"Id\": \"\", \"Store\": \"\" }" );

                result_error[ "Details" ][ "Id" ] = execute_id + "_" + std::to_string( command_index );
                result_error[ "Details" ][ "Store" ] = store;

                result[ "Errors" ][ execute_id + "_" + std::to_string( command_index ) ] = result_error;

              }
              else if ( check_command_result == -100 ) {

                auto result_error = Common::build_detail_block_response( "ERROR_STORE_WITH_NO_RULES_DEFINED",
                                                                         "The store not had rules section defined or invalid rules. Please check server config file",
                                                                         "EA2EF4E1854E-" + thread_id,
                                                                         "{ \"Id\": \"\", \"Store\": \"\" }" );

                result_error[ "Details" ][ "Id" ] = execute_id + "_" + std::to_string( command_index );
                result_error[ "Details" ][ "Store" ] = store;

                result[ "Errors" ][ execute_id + "_" + std::to_string( command_index ) ] = result_error;

              }
              else if ( check_command_result == -101 ) {

                auto result_error = Common::build_detail_block_response( "ERROR_COMMAND_EXECUTION_EXPLICIT_DENIED",
                                                                         "The command execution explicit denied",
                                                                         "CE6AD4378D52-" + thread_id,
                                                                         "{ \"Id\": \"\", \"Store\": \"\" }" );

                result_error[ "Details" ][ "Id" ] = execute_id + "_" + std::to_string( command_index );
                result_error[ "Details" ][ "Store" ] = store;

                result[ "Errors" ][ execute_id + "_" + std::to_string( command_index ) ] = result_error;

              }

            }
            else {

              auto result_error = Common::build_detail_block_response( "ERROR_TO_EXECUTE_COMMAND",
                                                                       "The command cannot be empty",
                                                                       "2951E9E3F368-" + thread_id,
                                                                       "{ \"Id\": \"\", \"Store\": \"\" }" );

              result_error[ "Details" ][ "Id" ] = execute_id + "_" + std::to_string( command_index );
              result_error[ "Details" ][ "Store" ] = store;

              result[ "Errors" ][ execute_id + "_" + std::to_string( command_index ) ] = result_error;

            }

          }
          catch ( const std::exception &ex ) {

            auto result_error = Common::build_detail_block_response( "ERROR_TO_EXECUTE_COMMAND",
                                                                     "Unexpected error to execute command",
                                                                     "B7A7E280921C-" + thread_id,
                                                                     "{ \"Id\": \"\", \"Store\": \"\", \"Message\": \"\" }" );

            result_error[ "Details" ][ "Id" ] = execute_id + "_" + std::to_string( command_index );
            result_error[ "Details" ][ "Store" ] = store;
            result_error[ "Details" ][ "Message" ] = ex.what();

            result[ "Errors" ][ execute_id + "_" + std::to_string( command_index ) ] = result_error;

          }

          execute_list_size += 1;

        }

        if ( is_store_connection_local_transaction  ) {

          store_sql_connection_in_transaction->commit(); //Apply the transaction

        }

      }
      catch ( const std::exception &ex ) {

        auto result_error = Common::build_detail_block_response( "ERROR_TO_EXECUTE_COMMAND",
                                                                 "Unexpected error to execute command",
                                                                 "722C178F170D-" + thread_id,
                                                                 "{ \"Id\": \"\", \"Store\": \"\", \"Message\": \"\" }" );

        result_error[ "Details" ][ "Id" ] = execute_id;
        result_error[ "Details" ][ "Store" ] = store;
        result_error[ "Details" ][ "Message" ] = ex.what();

        result[ "Errors" ][ execute_id ] = result_error;

        if ( is_store_connection_local_transaction  ) {

          store_sql_connection_in_transaction->rollback(); //Rollback the transaction

        }

      }

    }
    else {

      auto result_error = Common::build_detail_block_response( "ERROR_STORE_SQL_TRANSACTION_IS_NOT_ACTIVE",
                                                               "The Store SQL Transactin is not active",
                                                               "B87910868F92-" + thread_id,
                                                               "{ \"Id\": \"\", \"Store\": \"\" }" );

      result_error[ "Details" ][ "Id" ] = execute_id;
      result_error[ "Details" ][ "Store" ] = store;

      result[ "Errors" ][ execute_id ] = result_error;

    }

  }
  else {

    auto result_error = Common::build_detail_block_response( "ERROR_TRANSACTIONID_IS_INVALID",
                                                             "The transaction id is invalid or not found",
                                                             "63ECC28A0FAA-" + thread_id,
                                                             "{ \"Id\": \"\", \"Store\": \"\", \"TransactionId\": \"\" }" );

    result_error[ "Details" ][ "Id" ] = execute_id;
    result_error[ "Details" ][ "Store" ] = store;
    result_error[ "Details" ][ "TransactionId" ] = transaction_id;

    result[ "Errors" ][ execute_id ] = result_error;

  }

}

inline void execute_mongodb_query( const std::string& thread_id,
                                   const std::string& authorization,
                                   const std::string& execute_id,
                                   const std::string& store,
                                   const Store::StoreConnectionSharedPtr& store_connection,
                                   const Common::NLOJSONObject& command_list,
                                   std::size_t& execute_list_size,
                                   const Common::NJSONElement& store_rule_list,
                                   Common::NLOJSONObject& result ) {

  try {

    //Stay sure no other thread use this connection
    std::lock_guard<std::mutex> lock_guard( store_connection->mutex() );

    for ( auto command_index = 0; command_index < command_list.size(); command_index++ ) {

      try {

        const std::string& command = Common::trim( command_list[ command_index ] );

        if ( command != "" ) {

          const auto& check_command_result = Security::check_command_to_store_is_authorizated( authorization,
                                                                                                store,
                                                                                                command,
                                                                                                store_rule_list );

          if ( check_command_result == 101 ) { //Command authorized

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
          else if ( check_command_result == -1 ) {

            auto result_error = Common::build_detail_block_response( "ERROR_UNABLE_TO_CHECK_COMMAND_DENIED_OR_ALLOWED",
                                                                     "Unable to check the command is denied or allowed. Please check server config file",
                                                                     "1FA26CF9374E-" + thread_id,
                                                                     "{ \"Id\": \"\", \"Store\": \"\" }" );

            result_error[ "Details" ][ "Id" ] = execute_id + "_" + std::to_string( command_index );
            result_error[ "Details" ][ "Store" ] = store;

            result[ "Errors" ][ execute_id + "_" + std::to_string( command_index ) ] = result_error;

          }
          else if ( check_command_result == -100 ) {

            auto result_error = Common::build_detail_block_response( "ERROR_STORE_WITH_NO_RULES_DEFINED",
                                                                     "The store not had rules section defined or invalid rules. Please check server config file",
                                                                     "9D812C46F40F-" + thread_id,
                                                                     "{ \"Id\": \"\", \"Store\": \"\" }" );

            result_error[ "Details" ][ "Id" ] = execute_id + "_" + std::to_string( command_index );
            result_error[ "Details" ][ "Store" ] = store;

            result[ "Errors" ][ execute_id + "_" + std::to_string( command_index ) ] = result_error;

          }
          else if ( check_command_result == -101 ) {

            auto result_error = Common::build_detail_block_response( "ERROR_COMMAND_EXECUTION_EXPLICIT_DENIED",
                                                                     "The command execution explicit denied",
                                                                     "A6703B1C78D1-" + thread_id,
                                                                     "{ \"Id\": \"\", \"Store\": \"\" }" );

            result_error[ "Details" ][ "Id" ] = execute_id + "_" + std::to_string( command_index );
            result_error[ "Details" ][ "Store" ] = store;

            result[ "Errors" ][ execute_id + "_" + std::to_string( command_index ) ] = result_error;

          }

        }
        else {

          auto result_error = Common::build_detail_block_response( "ERROR_TO_EXECUTE_COMMAND",
                                                                   "The command cannot be empty",
                                                                   "A55D54A40072-" + thread_id,
                                                                   "{ \"Id\": \"\", \"Store\": \"\" }" );

          result_error[ "Details" ][ "Id" ] = execute_id + "_" + std::to_string( command_index );
          result_error[ "Details" ][ "Store" ] = store;

          result[ "Errors" ][ execute_id + "_" + std::to_string( command_index ) ] = result_error;

        }

      }
      catch ( const std::exception &ex ) {

        auto result_error = Common::build_detail_block_response( "ERROR_TO_EXECUTE_COMMAND",
                                                                  "Unexpected error to execute command",
                                                                  "9DFBD6D4E91D-" + thread_id,
                                                                  "{ \"Id\": \"\", \"Store\": \"\", \"Message\": \"\" }" );

        result_error[ "Details" ][ "Id" ] = execute_id + "_" + std::to_string( command_index );
        result_error[ "Details" ][ "Store" ] = store;
        result_error[ "Details" ][ "Message" ] = ex.what();

        result[ "Errors" ][ execute_id + "_" + std::to_string( command_index ) ] = result_error;

      }

      execute_list_size += 1;

    }

  }
  catch ( const std::exception &ex ) {

    auto result_error = Common::build_detail_block_response( "ERROR_TO_EXECUTE_COMMAND",
                                                             "Unexpected error to execute command",
                                                             "531422A5C77F-" + thread_id,
                                                             "{ \"Id\": \"\", \"Store\": \"\", \"Message\": \"\" }" );

    result_error[ "Details" ][ "Id" ] = execute_id;
    result_error[ "Details" ][ "Store" ] = store;
    result_error[ "Details" ][ "Message" ] = ex.what();

    result[ "Errors" ][ execute_id ] = result_error;

  }

}

inline void execute_redis_query( const std::string& thread_id,
                                 const std::string& authorization,
                                 const std::string& execute_id,
                                 const std::string& store,
                                 const Store::StoreConnectionSharedPtr& store_connection,
                                 const Common::NLOJSONObject& command_list,
                                 std::size_t& execute_list_size,
                                 const Common::NJSONElement& store_rule_list,
                                 Common::NLOJSONObject& result ) {

  try {

    //Stay sure no other thread use this connection
    std::lock_guard<std::mutex> lock_guard( store_connection->mutex() );

    for ( auto command_index = 0; command_index < command_list.size(); command_index++ ) {

      try {

        const std::string& command = Common::trim( command_list[ command_index ] );

        if ( command != "" ) {

          const auto& check_command_result = Security::check_command_to_store_is_authorizated( authorization,
                                                                                               store,
                                                                                               command,
                                                                                               store_rule_list );

          if ( check_command_result == 101 ) { //Command authorized

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
          else if ( check_command_result == -1 ) {

            auto result_error = Common::build_detail_block_response( "ERROR_UNABLE_TO_CHECK_COMMAND_DENIED_OR_ALLOWED",
                                                                     "Unable to check the command is denied or allowed. Please check server config file",
                                                                     "B971E75AC962-" + thread_id,
                                                                     "{ \"Id\": \"\", \"Store\": \"\" }" );

            result_error[ "Details" ][ "Id" ] = execute_id + "_" + std::to_string( command_index );
            result_error[ "Details" ][ "Store" ] = store;

            result[ "Errors" ][ execute_id + "_" + std::to_string( command_index ) ] = result_error;

          }
          else if ( check_command_result == -100 ) {

            auto result_error = Common::build_detail_block_response( "ERROR_STORE_WITH_NO_RULES_DEFINED",
                                                                     "The store not had rules section defined or invalid rules. Please check server config file",
                                                                     "0A52201289F5-" + thread_id,
                                                                     "{ \"Id\": \"\", \"Store\": \"\" }" );

            result_error[ "Details" ][ "Id" ] = execute_id + "_" + std::to_string( command_index );
            result_error[ "Details" ][ "Store" ] = store;

            result[ "Errors" ][ execute_id + "_" + std::to_string( command_index ) ] = result_error;

          }
          else if ( check_command_result == -101 ) {

            auto result_error = Common::build_detail_block_response( "ERROR_COMMAND_EXECUTION_EXPLICIT_DENIED",
                                                                     "The command execution explicit denied",
                                                                     "36CB651A76E2-" + thread_id,
                                                                     "{ \"Id\": \"\", \"Store\": \"\" }" );

            result_error[ "Details" ][ "Id" ] = execute_id + "_" + std::to_string( command_index );
            result_error[ "Details" ][ "Store" ] = store;

            result[ "Errors" ][ execute_id + "_" + std::to_string( command_index ) ] = result_error;

          }

        }
        else {

          auto result_error = Common::build_detail_block_response( "ERROR_TO_EXECUTE_COMMAND",
                                                                   "The command cannot be empty",
                                                                   "F5AC6481283D-" + thread_id,
                                                                   "{ \"Id\": \"\", \"Store\": \"\" }" );

          result_error[ "Details" ][ "Id" ] = execute_id + "_" + std::to_string( command_index );
          result_error[ "Details" ][ "Store" ] = store;

          result[ "Errors" ][ execute_id + "_" + std::to_string( command_index ) ] = result_error;

          // auto result_error = R"(
          //                         {
          //                           "Code": "ERROR_TO_EXECUTE_COMMAND",
          //                           "Message": "The command cannot be empty",
          //                           "Mark": "F5AC6481283D-",
          //                           "Details": {
          //                                        "Id": "",
          //                                        "Store": ""
          //                                      }
          //                         }
          //                       )"_json;

          // result_error[ "Mark" ] = result_error[ "Mark" ].get<std::string>() + thread_id;

          // result_error[ "Details" ][ "Id" ] = execute_id + "_" + std::to_string( command_index );
          // result_error[ "Details" ][ "Store" ] = store;

          // //result[ "Errors" ].push_back( result_error );
          // result[ "Errors" ][ execute_id + "_" + std::to_string( command_index ) ] = result_error;

        }

      }
      catch ( const std::exception &ex ) {

        auto result_error = Common::build_detail_block_response( "ERROR_TO_EXECUTE_COMMAND",
                                                                 "Unexpected error to execute command",
                                                                 "7973862D0898-" + thread_id,
                                                                 "{ \"Id\": \"\", \"Store\": \"\", \"Message\": \"\" }" );

        result_error[ "Details" ][ "Id" ] = execute_id + "_" + std::to_string( command_index );
        result_error[ "Details" ][ "Store" ] = store;
        result_error[ "Details" ][ "Message" ] = ex.what();

        result[ "Errors" ][ execute_id + "_" + std::to_string( command_index ) ] = result_error;

        // auto result_error = R"(
        //                         {
        //                           "Code": "ERROR_TO_EXECUTE_COMMAND",
        //                           "Message": "Unexpected error to execute command",
        //                           "Mark": "7973862D0898-",
        //                           "Details": {
        //                                        "Id": "",
        //                                        "Store": "",
        //                                        "Message": ""
        //                                      }
        //                         }
        //                       )"_json;

        // result_error[ "Mark" ] = result_error[ "Mark" ].get<std::string>() + thread_id;

        // result_error[ "Details" ][ "Id" ] = execute_id;
        // result_error[ "Details" ][ "Store" ] = store;
        // result_error[ "Details" ][ "Message" ] = ex.what();

        // //result[ "Errors" ].push_back( result_error );
        // result[ "Errors" ][ execute_id + "_" + std::to_string( command_index ) ] = result_error;

      }

      execute_list_size += 1;

    }

  }
  catch ( const std::exception &ex ) {

    auto result_error = Common::build_detail_block_response( "ERROR_TO_EXECUTE_COMMAND",
                                                             "Unexpected error to execute command",
                                                             "44E423104FB2-" + thread_id,
                                                             "{ \"Id\": \"\", \"Store\": \"\", \"Message\": \"\" }" );

    result_error[ "Details" ][ "Id" ] = execute_id;
    result_error[ "Details" ][ "Store" ] = store;
    result_error[ "Details" ][ "Message" ] = ex.what();

    result[ "Errors" ][ execute_id ] = result_error;

    // auto result_error = R"(
    //                         {
    //                           "Code": "ERROR_TO_EXECUTE_COMMAND",
    //                           "Message": "Unexpected error to execute command",
    //                           "Mark": "44E423104FB2-",
    //                           "Details": {
    //                                         "Id": "",
    //                                         "Store": "",
    //                                         "Message": ""
    //                                       }
    //                         }
    //                       )"_json;

    // result_error[ "Mark" ] = result_error[ "Mark" ].get<std::string>() + thread_id;

    // result_error[ "Details" ][ "Id" ] = execute_id;
    // result_error[ "Details" ][ "Store" ] = store;
    // result_error[ "Details" ][ "Message" ] = ex.what();

    // //result[ "Errors" ].push_back( result_error );
    // result[ "Errors" ][ execute_id ] = result_error;

  }

}

inline void execute_command_list( const std::string& thread_id,
                                  const std::string& authorization,
                                  const std::string& execute_id,
                                  const std::string& store,
                                  const std::string& transaction_id,
                                  const Common::NLOJSONObject& command_list,
                                  std::size_t& execute_list_size,
                                  const Common::NJSONElement& store_rule_list,
                                  Common::NLOJSONObject& result ) {

  if ( command_list.size() > 0 ) {

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

        execute_sql_query( thread_id,
                           authorization,
                           execute_id,
                           store,
                           transaction_id,
                           is_store_connection_local_transaction,
                           store_sql_connection_in_transaction,
                           store_connection,
                           command_list,
                           execute_list_size,
                           store_rule_list,
                           result );

      }
      else if ( store_connection->mongo_connection() ) {

        execute_mongodb_query( thread_id,
                               authorization,
                               execute_id,
                               store,
                               store_connection,
                               command_list,
                               execute_list_size,
                               store_rule_list,
                               result );

      }
      else if ( store_connection->redis_connection() ) {

        execute_redis_query( thread_id,
                             authorization,
                             execute_id,
                             store,
                             store_connection,
                             command_list,
                             execute_list_size,
                             store_rule_list,
                             result );

      }

      if ( is_store_connection_local_leased ) {

        Store::StoreConnectionManager::return_leased_store_connection( store_connection );

      }

    }
    else if ( is_store_connection_local_leased ) {

      auto result_error = Common::build_detail_block_response( "ERROR_NO_STORE_CONNECTION_AVAILABLE_IN_POOL",
                                                               "No more Store connection available in pool to begin transaction",
                                                               "F6A706854C0F-" + thread_id,
                                                               "{ \"Id\": \"\", \"Store\": \"\" }" );

      result_error[ "Details" ][ "Id" ] = execute_id;
      result_error[ "Details" ][ "Store" ] = store;

      result[ "Errors" ][ execute_id ] = result_error;

    }
    else if ( store_sql_connection_in_transaction == nullptr ) {

      auto result_error = Common::build_detail_block_response( "ERROR_TRANSACTIONID_IS_INVALID",
                                                               "The transaction id is invalid or not found",
                                                               "E9D6B6A0411C-" + thread_id,
                                                               "{ \"Id\": \"\", \"Store\": \"\", \"TransactionId\": \"\" }" );

      result_error[ "Details" ][ "Id" ] = execute_id;
      result_error[ "Details" ][ "Store" ] = store;
      result_error[ "Details" ][ "TransactionId" ] = transaction_id;

      result[ "Errors" ][ execute_id ] = result_error;

    }
    else {

      auto result_error = Common::build_detail_block_response( "ERROR_STORE_CONNECTION_NOT_FOUND",
                                                               "Store connection not found",
                                                               "FE8CDA468AF2-" + thread_id,
                                                               "{ \"Id\": \"\", \"Store\": \"\" }" );

      result_error[ "Details" ][ "Id" ] = execute_id;
      result_error[ "Details" ][ "Store" ] = store;
      result_error[ "Details" ][ "TransactionId" ] = transaction_id;

      result[ "Errors" ][ execute_id ] = result_error;

    }

  }
  else {

    auto result_error = Common::build_detail_block_response( "ERROR_MISSING_FIELD_COMMAND",
                                                             "The field Command is required as array of strings and cannot be empty or null",
                                                             "F093E072566F-" + thread_id,
                                                             "{ \"Id\": \"\", \"Store\": \"\" }" );

    result_error[ "Details" ][ "Id" ] = execute_id;
    result_error[ "Details" ][ "Store" ] = store;
    result_error[ "Details" ][ "TransactionId" ] = transaction_id;

    result[ "Errors" ][ execute_id ] = result_error;

  }

}

inline void execute_block_command_list( const std::string& thread_id,
                                        u_int16_t& status_code,
                                        const auto& json_body,
                                        const std::string& authorization,
                                        auto& result ) {

  const Common::NLOJSONObject& execute_list = json_body[ "Execute" ];

  std::size_t execute_list_size = 0;

  for ( auto &execute: execute_list ) {

    const std::string &execute_id = execute.get<std::string>();

    if ( json_body[ execute_id ].is_object() ) {

      const Common::NLOJSONObject& execute_block = json_body[ execute_id ];

      const std::string &store = Store::StoreConnectionManager::store_connection_name_to_real_name( execute_block[ "Store" ] );
      const std::string &transaction_id = execute_block[ "TransactionId" ].is_string() ?
                                          Common::trim( execute_block[ "TransactionId" ].get<std::string>() ):
                                          "";

      Common::NLOJSONObject command_list {};

      if ( execute_block[ "Command" ].is_array() ) {

        command_list = execute_block[ "Command" ];

      }

      if ( store != "" ) {

        Common::NJSONElement store_rule_list {};

        int16_t check_token_result = Security::check_autorization_is_valid_and_enabled( authorization,
                                                                                        store,
                                                                                        store_rule_list );

        if ( check_token_result == 1 ) { //OK

          execute_command_list( thread_id,
                                authorization,
                                execute_id,
                                store,
                                transaction_id,
                                command_list,
                                execute_list_size,
                                store_rule_list,
                                result );

        }
        else if ( check_token_result == -1 ) {

          status_code = 401; //Unhathorized

          result[ "StatusCode" ] = status_code;
          result[ "Code" ] = "ERROR_UNABLE_TO_CHECK_AUTORIZATION_TOKEN";
          result[ "Message" ] = "Unable to check the authorization token provided";
          result[ "Mark" ] = "85B202FEF269-" + thread_id;
          result[ "IsError" ] = true;

          break; //Not continue

        }
        else if ( check_token_result == -100 ) {

          status_code = 401; //Unhathorized

          result[ "StatusCode" ] = status_code;
          result[ "Code" ] = "ERROR_AUTORIZATION_TOKEN_IS_INVALID";
          result[ "Message" ] = "The authorization token provided is invalid";
          result[ "Mark" ] = "570D403F068E-" + thread_id;
          result[ "IsError" ] = true;

          break; //Not continue

        }
        else if ( check_token_result == -101 ) {

          status_code = 401; //Unhathorized

          result[ "StatusCode" ] = status_code;
          result[ "Code" ] = "ERROR_AUTORIZATION_TOKEN_IS_DISABLED";
          result[ "Message" ] = "The authorization token provided is disabled";
          result[ "Mark" ] = "C9E5EBB8D07B-" + thread_id;
          result[ "IsError" ] = true;

          break; //Not continue

        }
        else if ( check_token_result == -102 ) {

          status_code = 403; //Forbidden

          result[ "StatusCode" ] = status_code;
          result[ "Code" ] = "ERROR_STORE_NOT_RULES_DEFINED";
          result[ "Message" ] = "The store rules section in the config file not found are invalid. Please check server config file";
          result[ "Mark" ] = "9CF55E#CB6BAD-" + thread_id;
          result[ "IsError" ] = true;

          break; //Not continue

        }

      }
      else {

        auto result_error = Common::build_detail_block_response( "ERROR_STORE_NAME_NOT_FOUND",
                                                                 "The Store name not found",
                                                                 "AB8A4A9926B9-" + thread_id,
                                                                 "{ \"Id\": \"\", \"Store\": \"\" }" );

        result_error[ "Details" ][ "Id" ] = execute_id;
        result_error[ "Details" ][ "Store" ] = store;
        result_error[ "Details" ][ "TransactionId" ] = transaction_id;

        result[ "Errors" ][ execute_id ] = result_error;

      }

    }
    else {

      auto result_error = Common::build_detail_block_response( "ERROR_STORE_NAME_NOT_FOUND",
                                                               "The Store name not found in json body request",
                                                               "AB8A4A9926B9-" + thread_id,
                                                               "{ \"Id\": \"\" }" );

      result_error[ "Details" ][ "Id" ] = execute_id;

      result[ "Errors" ][ execute_id ] = result_error;

    }

  }

  if ( result[ "Code" ] == "" ) {

    if ( execute_list_size == result[ "Errors" ].size() ) {

      if ( status_code == 200 ) {

        status_code = 400; //Bad request

      }

      result[ "StatusCode" ] = status_code;
      result[ "Code" ] = "ERROR_ALL_COMMANDS_HAS_FAILED";
      result[ "Message" ] = "All commands has failed. Please check the Errors section for more details";
      result[ "Mark" ] = "2C4F94D79A1D-" + thread_id;
      result[ "IsError" ] = true;

    }
    else if ( execute_list_size == result[ "Data" ].size() ) {

      // if ( status_code == 200 ) {

      //   status_code = 200; //Ok

      // }

      //All query success
      result[ "Code" ] = "SUCCESS_FOR_ALL_COMMANDS";
      result[ "Message" ] = "All commands has succeeded. Please check the Data section for more details";
      result[ "Mark" ] = "1B42BAF3927B-" + thread_id;

    }
    else {

      if ( status_code == 200 ) {

        status_code = 202; //Accepted

      }

      //At least one query fails
      result[ "StatusCode" ] = status_code;
      result[ "Code" ] = "WARNING_NOT_ALL_COMMANDS_HAS_SUCCEEDED";
      result[ "Message" ] = "Not all commands has succeeded. Please check the Data, Warnings and Errors sections for more details";
      result[ "Mark" ] = "38C212E28716-" + thread_id;

    }

  }

}

int handler_store_query( const HttpContextPtr& ctx ) {

  auto type = ctx->type();

  u_int16_t status_code = 200;

  const std::string& thread_id = Common::xxHash_32( Common::get_thread_id() );

  if ( type == APPLICATION_JSON ) {

    try {

      const auto& json_body = hv::Json::parse( ctx->body() );

      //std::cout << json_body.dump( 2 ) << std::endl;

      if ( json_body[ "Autorization" ].is_null() == false &&
           Common::trim( json_body[ "Autorization" ] ) != "" ) {

        const std::string &authorization = Common::trim( json_body[ "Autorization" ] );

        if ( json_body[ "Execute" ].is_null() == false &&
             json_body[ "Execute" ].is_array() &&
             json_body[ "Execute" ].size() > 0 ) {

          auto result = Common::build_basic_response( 200, "", "", "", false, "" );

          execute_block_command_list( thread_id,
                                      status_code,
                                      json_body,
                                      authorization,
                                      result );

          ctx->response->content_type = APPLICATION_JSON;
          ctx->response->body = result.dump( 2 );

          //ctx->response->Json( result );

        }
        else {

          status_code = 400; //Bad request

          const std::string& thread_id = Common::xxHash_32( Common::get_thread_id() );

          const auto& result = Common::build_basic_response( status_code,
                                                             "ERROR_MISSING_FIELD_EXECUTE",
                                                             "The field Execute is required as array of strings and cannot be empty or null",
                                                             "FEE80B366130-" + thread_id,
                                                             true,
                                                             "" );

          ctx->response->content_type = APPLICATION_JSON;
          ctx->response->body = result.dump( 2 );

        }

      }
      else {

        status_code = 400; //Bad request

        const std::string& thread_id = Common::xxHash_32( Common::get_thread_id() );

        const auto& result = Common::build_basic_response( status_code,
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

    const auto& result = Common::build_basic_response( status_code,
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
