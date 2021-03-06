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

inline bool key_exists( const auto& json_data, const auto& key ) {

  bool result = false;

  try {

    const auto& field = json_data.at( key );

    result = true;

  }
  catch ( const std::exception &ex ) {


  }

  return result;

}

inline void execute_sql_query( const std::string& thread_id,
                               const std::string& authorization,
                               const std::string& execute_id,
                               const std::string& store,
                               const std::string& transaction_id,
                               const bool is_store_connection_local_transaction,
                               const Store::StoreSQLConnectionTransactionPtr& store_sql_connection_in_transaction,
                               const Store::StoreConnectionSharedPtr& store_connection,
                               const Common::NLOJSONObject& command_list,
                               const Common::NLOJSONObject& map_list,
                               std::size_t& execute_list_size,
                               const Common::NANOJSONElement& store_rule_list,
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

                  bool transform_json_raw_row = false;

                  Common::NLOJSONObject* map_command_response { nullptr };

                  if ( command_index < map_list.size() ) {

                    if ( map_list[ command_index ].is_object() &&
                         map_list[ command_index ].size() > 0 ) {

                      map_command_response = const_cast<Common::NLOJSONObject*>( &map_list[ command_index ] );
                      transform_json_raw_row = true;

                    }

                  }

                  std::vector<std::string> key_prefix_list;

                  std::map<std::string,std::vector<std::string>> map_key_prefix_to_vector_field_name;

                  soci::rowset<soci::row> rs = ( store_connection->sql_connection()->prepare << command );

                  //auto data_list = nlohmann::ordered_json::array(); //hv::Json::array();
                  auto data_list = nlohmann::json::array(); //hv::Json::array();

                  size_t row_index = 0;

                  for ( soci::rowset<soci::row>::const_iterator it = rs.begin(); it != rs.end(); ++it ) {

                    soci::row const& row = *it;

                    std::ostringstream doc;

                    doc << '{';

                    for( std::size_t i = 0; i < row.size(); ++i ) {

                      auto props = row.get_properties( i );

                      const auto& field_name = props.get_name();

                      doc << '\"' << field_name << "\":";

                      if ( transform_json_raw_row && row_index == 0 ) {

                        const auto& partial_field_name = field_name.substr( 0, 2 );

                        const auto& iterator = std::find( key_prefix_list.begin(),
                                                          key_prefix_list.end(),
                                                          partial_field_name );

                        if ( iterator == key_prefix_list.end() ) {

                          key_prefix_list.push_back( partial_field_name );

                        }

                        map_key_prefix_to_vector_field_name[ partial_field_name ].push_back( field_name );

                      }

                      if ( row.get_indicator( i ) != soci::i_null ) {

                        switch( props.get_data_type() ) {
                          case soci::dt_string:
                          case soci::dt_xml: {

                            const std::string value = row.get<std::string>( i );

                            if ( value != "" &&
                                 value[ 0 ] == '{' &&
                                 value[ value.length() - 1 ] == '}' ) {

                              try {

                                [[maybe_unused]] auto temp = hv::Json::parse( value );

                                doc << value;

                              }
                              catch ( ... ) {

                                doc << '\"' << value << "\"";

                              }

                            }
                            else {

                              doc << '\"' << value << "\"";

                            }

                            break;

                          }
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

                    //auto raw_json_row = Common::NANOJSONElement::from_string( doc.str() );
                    //auto raw_json_row = nlohmann::ordered_json::parse( doc.str() );
                    //const nlohmann::ordered_json&
                    auto raw_json_row = nlohmann::json::parse( doc.str() );

                    row_index += 1;

                    data_list.push_back( raw_json_row );

                  }

                  result[ "Count" ] = row_index;

                  if ( transform_json_raw_row ) {

                    std::map<std::string,std::shared_ptr<nlohmann::json>> map_key_to_root_json {};

                    std::map<std::string,nlohmann::json *> map_key_to_json_object {};

                    const auto &root_prefix_value = (*map_command_response)[ "__ROOT__" ][ "__PREFIX__" ].get<std::string>();

                    for ( int row_index = 0; row_index < data_list.size(); row_index++ ) {

                      auto &json_raw_row = data_list[ row_index ];

                      std::string root_key_value {};

                      for ( const auto& key_field_name: (*map_command_response)[ "__ROOT__" ][ "__KEY__" ] ) {

                        if ( root_key_value.empty() ) {

                          root_key_value.append( json_raw_row[ key_field_name.get<std::string>() ].get<std::string>() );

                        }
                        else {

                          root_key_value.append( ";" );
                          root_key_value.append( json_raw_row[ key_field_name.get<std::string>() ].get<std::string>() );

                        }

                      }

                      for ( auto &key_prefix_value: key_prefix_list ) {

                        // if ( key_prefix_value == "E_" ) {

                        //   std::cout << key_prefix_value << std::endl;

                        // }

                        const auto& map_command_response_key_prefix = (*map_command_response)[ key_prefix_value ];

                        std::string current_json_object_key_value {};

                        bool missing_key_value = false;

                        if ( key_prefix_value != root_prefix_value ) {

                          for ( const std::string& key: map_command_response_key_prefix[ "__KEY__" ] ) {

                            if ( json_raw_row[ key ].is_null() == false &&
                                 json_raw_row[ key ].is_string() ) {

                              if ( current_json_object_key_value.empty() ) {

                                current_json_object_key_value.append( json_raw_row[ key ].get<std::string>() );

                              }
                              else {

                                current_json_object_key_value.append( ";" );
                                current_json_object_key_value.append( json_raw_row[ key ].get<std::string>() );

                              }

                            }
                            else {

                              missing_key_value = true;
                              break;

                            }

                          }

                        }
                        else {

                          current_json_object_key_value = root_key_value;

                        }

                        if ( missing_key_value == false ) {

                          nlohmann::json* current_json_object { nullptr };

                          const bool current_json_object_key_value_not_found = map_key_to_json_object.find( current_json_object_key_value ) == map_key_to_json_object.end();

                          if ( current_json_object_key_value_not_found &&
                               key_prefix_value == root_prefix_value ) {

                            map_key_to_root_json[ root_key_value ] = std::make_shared<nlohmann::json>();

                            current_json_object = map_key_to_root_json[ root_key_value ].get();

                            (*current_json_object)[ "__PREFIX__" ] = key_prefix_value;

                          }

                          if ( current_json_object_key_value_not_found ) {

                            std::string parent_json_object_key_value {};

                            if ( map_command_response_key_prefix.is_null() == false &&
                                 map_command_response_key_prefix[ "__PARENT_KEY__" ].is_array() &&
                                 map_command_response_key_prefix[ "__PARENT_KEY__" ].size() > 0 ) {

                              for ( const std::string& key: map_command_response_key_prefix[ "__PARENT_KEY__" ] ) {

                                if ( json_raw_row[ key ].is_null() == false &&
                                     json_raw_row[ key ].is_string() ) {

                                  if ( parent_json_object_key_value.empty() ) {

                                    parent_json_object_key_value.append( json_raw_row[ key ].get<std::string>() );

                                  }
                                  else {

                                    parent_json_object_key_value.append( ";" );
                                    parent_json_object_key_value.append( json_raw_row[ key ].get<std::string>() );

                                  }

                                }
                                else {

                                  missing_key_value = true;
                                  break;

                                }

                              }

                              if ( missing_key_value == false ) {

                                if ( map_key_to_json_object.find( parent_json_object_key_value ) != map_key_to_json_object.end() &&
                                     map_command_response_key_prefix[ "__PARENT_FIELD__" ].is_string() ) {

                                  auto& temp_json = (*map_key_to_json_object[ parent_json_object_key_value ]);

                                  nlohmann::json json_to_add;
                                  json_to_add[ "__PREFIX__" ] = key_prefix_value;

                                  temp_json[ map_command_response_key_prefix[ "__PARENT_FIELD__" ].get<std::string>() ].push_back( std::move( json_to_add ) );

                                  //Capture the last position in the array
                                  current_json_object = &temp_json[ map_command_response_key_prefix[ "__PARENT_FIELD__" ].get<std::string>() ]
                                                                  [ temp_json[ map_command_response_key_prefix[ "__PARENT_FIELD__" ].get<std::string>() ].size() - 1 ];

                                  // std::cout << "***** map_key_to_root_json (1) *****" << std::endl;
                                  // std::cout << map_key_to_root_json[ root_key_value ]->dump( 2 ) << std::endl;
                                  // std::cout << "***** map_key_to_root_json (1) *****" << std::endl;

                                  // std::cout << "***** temp_json *****" << std::endl;
                                  // std::cout << temp_json.dump( 2 ) << std::endl;
                                  // std::cout << "***** temp_json *****" << std::endl;

                                }

                              }

                            }

                            if ( missing_key_value == false ) {

                              for ( const auto& field_name: map_key_prefix_to_vector_field_name[ key_prefix_value ] ) {

                                auto& field_value = json_raw_row[ field_name ];

                                (*current_json_object)[ field_name ] = field_value;

                              }

                              map_key_to_json_object[ current_json_object_key_value ] = current_json_object;

                            }

                            // else if ( current_json_object == nullptr ) {

                            //   current_json_object = map_key_to_json_object[ current_json_object_key_value ];

                            // }

                            // if ( key_prefix_value == root_prefix_value ) {

                            //   map_key_to_root_json[ root_key_value ] = std::shared_ptr<nlohmann::json>( current_json_object );

                            // }

                          }

                          // if ( key_prefix_value == "E_" ) {

                          //   std::cout << key_prefix_value << std::endl;

                          // }

                          // if ( map_key_to_root_json.find( root_key_value ) != map_key_to_root_json.end() ) {

                          //   std::cout << "***** begin -- map_key_to_root_json (1) *****" << std::endl;
                          //   std::cout << map_key_to_root_json[ root_key_value ]->dump( 2 ) << std::endl;
                          //   std::cout << "***** end -- map_key_to_root_json (1) *****" << std::endl;

                          // }

                        }

                      }

                      /*
                      for ( auto &key_prefix: key_prefix_list ) {

                        const auto& map_command_response_key_prefix = (*map_command_response)[ key_prefix ];

                        std::shared_ptr<nlohmann::json> temp_json_object { json_row_root_object };
                        //nlohmann::json temp_json_object {};

                        if ( key_prefix != root_prefix_value ) {

                          temp_json_object = std::make_shared<nlohmann::json>();

                        }

                        (*temp_json_object)[ "__PREFIX__" ] = key_prefix;

                        for ( const auto& field_name: map_key_prefix_to_vector_field_name[ key_prefix ] ) {

                          auto& field_value = json_row[ field_name ];

                          (*temp_json_object)[ field_name ] = field_value;

                          // if ( key_prefix == main_prefix_value ) {

                          //   //(*json_row_root_object)[ key_prefix ][ field_name ] = field_value;
                          //   (*json_row_root_object)[ field_name ] = field_value;
                          //   //(*json_row_root_object)[ "__prefix__" ] = key_prefix;

                          // }
                          // else {

                          //   temp_json_object[ field_name ] = field_value;
                          //   //temp_json_object[ "__prefix__" ] = key_prefix;

                          // }

                        }

                        //if ( map_key_prefix_to_vector_field_root.find( key_prefix ) != map_key_prefix_to_vector_field_root.end() ) {
                        //if ( (*map_command_response)[ key_prefix ][ "Root" ].type() == nlohmann::detail::value_t::array ) {
                        if ( map_command_response_key_prefix.type() == nlohmann::detail::value_t::object &&
                             map_command_response_key_prefix[ "Root" ].type() == nlohmann::detail::value_t::array ) {

                          nlohmann::json* temp_json_row_root_object = json_row_root_object.get();

                          //for ( const auto& root_field_name: map_key_prefix_to_vector_field_root[ key_prefix ] ) {
                          //for ( const auto& root_field_name: (*map_command_response)[ key_prefix ][ "Root" ] ) {
                          for ( const auto& root_field_name: map_command_response_key_prefix[ "Root" ] ) {

                            //auto parent_temp_json_row_root_object = temp_json_row_root_object;

                            if ( temp_json_row_root_object->type() == nlohmann::detail::value_t::object ) {

                              temp_json_row_root_object = &(*temp_json_row_root_object)[ root_field_name.get<std::string>() ];

                            }
                            else {

                              temp_json_row_root_object = nullptr;
                              break;

                            }

                            if ( temp_json_row_root_object->type() == nlohmann::detail::value_t::array ) {

                              //if ( map_key_prefix_to_vector_field_parent.find( key_prefix ) != map_key_prefix_to_vector_field_parent.end() ) {
                              //if ( (*map_command_response)[ key_prefix ][ "Parent" ].type() == nlohmann::detail::value_t::object ) {
                              if ( map_command_response_key_prefix[ "Parent" ].type() == nlohmann::detail::value_t::object ) {

                                //bool object_found_in_array = false;

                                bool match_same_prefix_key = false;

                                for ( auto& object_in_array: *temp_json_row_root_object ) {

                                  bool local_found = true;

                                  match_same_prefix_key = object_in_array[ "__PREFIX__" ] == (*temp_json_object)[ "__PREFIX__" ];

                                  if ( match_same_prefix_key == false ) {

                                    //for ( const auto& field_parent_match: map_key_prefix_to_vector_field_parent[ key_prefix ] ) {
                                    // for ( auto it = (*map_command_response)[ key_prefix ][ "Parent" ].begin();
                                    //       it != (*map_command_response)[ key_prefix ][ "Parent" ].end();
                                    //       ++it ) {
                                    for ( auto it = map_command_response_key_prefix[ "Parent" ].begin();
                                          it != map_command_response_key_prefix[ "Parent" ].end();
                                          ++it ) {

                                      //if ( object_in_array[ field_parent_match.second ] != (*temp_json_object)[ field_parent_match.first ] ) {
                                      if ( object_in_array[ it.value().get<std::string>() ] != (*temp_json_object)[ it.key() ] ) {

                                        local_found = false;
                                        break;

                                      }

                                    }

                                  }
                                  else {

                                    //for ( const auto& field_key: map_key_prefix_to_vector_field_key[ key_prefix ] ) {
                                    //for ( const auto& field_key: (*map_command_response)[ key_prefix ][ "Key" ] ) {
                                    for ( const auto& field_key: map_command_response_key_prefix[ "Key" ] ) {

                                      if ( object_in_array[ field_key.get<std::string>() ] != (*temp_json_object)[ field_key.get<std::string>() ] ) {

                                        local_found = false;
                                        break;

                                      }

                                    }

                                  }

                                  if ( local_found ) {

                                    if ( match_same_prefix_key == false ) {

                                      temp_json_row_root_object = &object_in_array;

                                    }
                                    else {

                                      temp_json_row_root_object = nullptr; //not add already exists

                                    }

                                    //object_found_in_array = true;
                                    break;

                                  }

                                }

                                // if ( object_found_in_array == false ) {

                                //   temp_json_row_root_object = nullptr;

                                // }

                              }

                              if ( temp_json_row_root_object == nullptr ) {

                                break;

                              }

                            }

                          }

                          if ( temp_json_row_root_object  != nullptr ) {

                            temp_json_row_root_object->push_back( *temp_json_object );

                            // std::cout << "+++++" << std::endl;
                            // std::cout << json_row_root_object->dump( 2 ) << std::endl; //map_key_to_row_json[ main_key.first ]->dump( 2 ) << std::endl;
                            // std::cout << "+++++" << std::endl;

                          }

                        }

                        / *
                        if ( key_prefix == "B_" ) {

                          //(*json_row_root_object)[ "A_" ][ key_prefix ].push_back( temp_json_object );
                          (*json_row_root_object)[ key_prefix ].push_back( temp_json_object );

                        }
                        else if ( key_prefix == "C_" ) {

                          //(*json_row_root_object)[ "A_" ][ "B_" ][ 0 ][ key_prefix ].push_back( temp_json_object );
                          (*json_row_root_object)[ "B_" ][ 0 ][ key_prefix ].push_back( temp_json_object );

                        }
                        * /

                      }
                      */

                      //main_key_list.push( main_key_value );

                    }

                    data_list.clear();

                    size_t count = 0;

                    result[ "Count" ] = count;

                    //std::cout << "*****" << std::endl;

                    for ( const auto &main_key: map_key_to_root_json  ) {

                      //std::cout << main_key.second->dump( 2 ) << std::endl; //map_key_to_row_json[ main_key.first ]->dump( 2 ) << std::endl;

                      data_list.emplace_back( std::move( *main_key.second ) );

                      count += 1;

                      //result[ "Count" ] = result[ "Count" ].get<int>() + 1;

                    }

                    result[ "Count" ] = count;

                    //std::cout << "*****" << std::endl;

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

            hloge( "Exception: %s", ex.what() );

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

        hloge( "Exception: %s", ex.what() );

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
                                   const Common::NANOJSONElement& store_rule_list,
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

              std::istringstream ss { command }; //"Select * From sysPerson Where { \"FirstName\": { \"$regex\": \"^Tom[a??]s\", \"$options\" : \"i\" } }"

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

        hloge( "Exception: %s", ex.what() );

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

    hloge( "Exception: %s", ex.what() );

  }

}

inline void execute_redis_query( const std::string& thread_id,
                                 const std::string& authorization,
                                 const std::string& execute_id,
                                 const std::string& store,
                                 const Store::StoreConnectionSharedPtr& store_connection,
                                 const Common::NLOJSONObject& command_list,
                                 std::size_t& execute_list_size,
                                 const Common::NANOJSONElement& store_rule_list,
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

                    hloge( "Exception: %s", ex.what() );

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

        hloge( "Exception: %s", ex.what() );

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

    hloge( "Exception: %s", ex.what() );

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
                                  const Common::NLOJSONObject& map_list,
                                  std::size_t& execute_list_size,
                                  const Common::NANOJSONElement& store_rule_list,
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
                           map_list,
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
      Common::NLOJSONObject map_list {};

      if ( execute_block[ "Command" ].is_array() ) {

        command_list = execute_block[ "Command" ];

      }

      try {

        if ( execute_block.at( "Map" ).is_array() ) {

          map_list = execute_block[ "Map" ];

        }

      }
      catch ( const std::exception &ex ) {


      }

      if ( store != "" ) {

        Common::NANOJSONElement store_rule_list {};

        Security::CheckAuthorizationResult check_authorization_result { 1, nullptr, 1 };

        Security::check_autorization( authorization,
                                      store,
                                      check_authorization_result );

        if ( check_authorization_result.value == 1 ) { //OK

          execute_command_list( thread_id,
                                authorization,
                                execute_id,
                                store,
                                transaction_id,
                                command_list,
                                map_list,
                                execute_list_size,
                                check_authorization_result.rules ? *check_authorization_result.rules: store_rule_list,
                                //store_rule_list,
                                result );

        }
        else if ( check_authorization_result.value == -1 ) { //Exception in Security::check_autorization_is_valid_and_enabled

          status_code = 401; //Unhathorized

          result[ "StatusCode" ] = status_code;
          result[ "Code" ] = "ERROR_UNABLE_TO_CHECK_AUTORIZATION_TOKEN";
          result[ "Message" ] = "Unable to check the authorization token provided";
          result[ "Mark" ] = "85B202FEF269-" + thread_id;
          result[ "IsError" ] = true;

          break; //Not continue

        }
        else if ( check_authorization_result.value == -100 ) {

          status_code = 401; //Unhathorized

          result[ "StatusCode" ] = status_code;
          result[ "Code" ] = "ERROR_AUTORIZATION_TOKEN_IS_INVALID";
          result[ "Message" ] = "The authorization token provided is invalid";
          result[ "Mark" ] = "570D403F068E-" + thread_id;
          result[ "IsError" ] = true;

          break; //Not continue

        }
        else if ( check_authorization_result.value == -101 ) {

          status_code = 401; //Unhathorized

          result[ "StatusCode" ] = status_code;
          result[ "Code" ] = "ERROR_AUTORIZATION_TOKEN_IS_DISABLED";
          result[ "Message" ] = "The authorization token provided is disabled";
          result[ "Mark" ] = "C9E5EBB8D07B-" + thread_id;
          result[ "IsError" ] = true;

          break; //Not continue

        }
        else if ( check_authorization_result.value == -102 ) {

          status_code = 403; //Forbidden

          result[ "StatusCode" ] = status_code;
          result[ "Code" ] = "ERROR_STORE_RULES_NOT_DEFINED";
          result[ "Message" ] = "The store rules section in the config file not found or are invalid. Please check server config file";
          result[ "Mark" ] = "9CF55ECB6BAD-" + thread_id;
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
                                                               "32249E938B78-" + thread_id,
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

  auto result = Common::build_basic_response( 200, "", "", "", false, "" );

  if ( type == APPLICATION_JSON ) {

    try {

      const auto& json_body = hv::Json::parse( ctx->body() );

      //std::cout << json_body.dump( 2 ) << std::endl;

      if ( json_body[ "Authorization" ].is_null() == false &&
           Common::trim( json_body[ "Authorization" ] ) != "" ) {

        const std::string &authorization = Common::trim( json_body[ "Authorization" ] );

        if ( json_body[ "Execute" ].is_null() == false &&
             json_body[ "Execute" ].is_array() &&
             json_body[ "Execute" ].size() > 0 ) {

          execute_block_command_list( thread_id,
                                      status_code,
                                      json_body,
                                      authorization,
                                      result );

        }
        else {

          status_code = 400; //Bad request

          result[ "StatusCode" ] = status_code;
          result[ "Code" ] = "ERROR_MISSING_FIELD_EXECUTE";
          result[ "Message" ] = "The field Execute is required as array of strings and cannot be empty or null";
          result[ "Mark" ] = "FEE80B366130-" + thread_id;
          result[ "IsError" ] = true;

        }

      }
      else {

        status_code = 400; //Bad request

        result[ "StatusCode" ] = status_code;
        result[ "Code" ] = "ERROR_MISSING_FIELD_AUTORIZATION";
        result[ "Message" ] = "The field Autorization is required and cannot be empty or null";
        result[ "Mark" ] = "E97A1FD111E8-" + thread_id;
        result[ "IsError" ] = true;

      }

    }
    catch ( const std::exception &ex ) {

      status_code = 500; //Internal server error

      result[ "StatusCode" ] = status_code;
      result[ "Code" ] = "ERROR_UNEXPECTED_EXCEPTION";
      result[ "Message" ] = "Unexpected error. Please read the server log for more details";
      result[ "Mark" ] = "F9B4CCCCAC92-" + thread_id;
      result[ "IsError" ] = true;

      result[ "Errors" ][ "Code" ] = "ERROR_INVALID_JSON_BODY_DATA";
      result[ "Errors" ][ "Message" ] = ex.what();
      result[ "Errors" ][ "Details" ] = {};

      hloge( "Exception: %s", ex.what() );

    }

  }
  else {

    status_code = 400; //Bad request

    result[ "StatusCode" ] = status_code;
    result[ "Code" ] = "JSON_BODY_FORMAT_REQUIRED";
    result[ "Message" ] = "JSON format is required";
    result[ "Mark" ] = "456EE56536AA-" + thread_id;
    result[ "IsError" ] = true;

  }

  ctx->response->content_type = APPLICATION_JSON;
  ctx->response->body = result.dump( 2 );

  //ctx->response->Json( result );

  return status_code;

}

}
