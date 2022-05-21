#include <string>

#include "hv/hlog.h"
#include "hv/HttpServer.h"
#include "hv/hssl.h"
#include "hv/hmain.h"
#include "hv/hthread.h"
#include "hv/hasync.h"     // import hv::async
#include "hv/requests.h"   // import requests::async
#include "hv/hdef.h"

#include "../modules/common/Common.hpp"

#include "../modules/common/StoreConnectionManager.hpp"

#include "../modules/common/Response.hpp"

#include "Handlers.hpp"

namespace Handlers {

int handler_store_transaction_begin( const HttpContextPtr& ctx ) {

  auto type = ctx->type();

  u_int16_t status_code = 200;

  const std::string& thread_id = Common::xxHash_32( Common::get_thread_id() );

  auto result = Common::build_basic_response( 200, "", "", "", false, "" );

  if ( type == APPLICATION_JSON ) {

    try {

      auto json_body = hv::Json::parse( ctx->body() );

      //std::cout << json_body.dump( 2 ) << std::endl;

      if ( json_body[ "Authorization" ].is_null() == false &&
           Common::trim( json_body[ "Authorization" ] ) != "" ) {

        if ( json_body[ "Store" ].is_null() == false &&
             Common::trim( json_body[ "Store" ] ) != "" ) {

          UUIDv4::UUIDGenerator<std::mt19937_64> uuidGenerator;

          const std::string &authorization = json_body[ "Authorization" ];
          const std::string &store = Store::StoreConnectionManager::store_connection_name_to_real_name( json_body[ "Store" ] );
          const std::string &transaction_id = json_body[ "TransactionId" ].is_null() == false && Common::trim( json_body[ "TransactionId" ] ) != "" ?
                                              json_body[ "TransactionId" ].get<std::string>():
                                              uuidGenerator.getUUID().str();

          if ( store != "" ) {

            Common::NJSONElement store_rule_list {};

            Security::CheckAuthorizationResult check_authorization_result { 1, &store_rule_list, 1 };

            Security::check_autorization( authorization,
                                          store,
                                          check_authorization_result );

            if ( check_authorization_result.value == 1 ) { //OK

              if ( Store::StoreConnectionManager::active_transaction_count_by_authorization( authorization ) < check_authorization_result.max_active_transactions ) {

                auto store_sql_connection_in_transaction = Store::StoreConnectionManager::transaction_by_id( transaction_id );

                if ( store_sql_connection_in_transaction == nullptr ) {

                  auto store_connection = Store::StoreConnectionManager::lease_store_connection_by_name( store );

                  if ( store_connection ) {

                    if ( store_connection->sql_connection() ) {

                      store_sql_connection_in_transaction = store_connection->sql_connection()->begin(); //transaction begin

                      Store::StoreConnectionManager::register_transaction_to_id( transaction_id,
                                                                                store_sql_connection_in_transaction );

                      Store::StoreConnectionManager::register_store_connection_to_id( transaction_id,
                                                                                      store_connection );

                      Store::StoreConnectionManager::register_transaction_id_to_authorization( authorization,
                                                                                              transaction_id );

                      //status_code = 200; //Ok

                      //result[ "StatusCode" ] = status_code;
                      result[ "Code" ] = "SUCCESS_STORE_SQL_TRANSACTION_BEGIN";
                      result[ "Message" ] = "Success Store SQL Transaction begin";
                      result[ "Mark" ] = "1D0EA78D055E-" + thread_id;
                      result[ "Data" ][ "TransactionId" ] = transaction_id;

                    }
                    else {

                      status_code = 400; //Bad request

                      result[ "StatusCode" ] = status_code;
                      result[ "Code" ] = "ERROR_STORE_CONNECTION_IS_NOT_SQL_KIND";
                      result[ "Message" ] = "The Store connection is not SQL kind";
                      result[ "Mark" ] = "9DF0F2038E3B-" + thread_id;
                      result[ "IsError" ] = true;

                    }

                  }
                  else {

                    status_code = 400; //Bad request

                    result[ "StatusCode" ] = status_code;
                    result[ "Code" ] = "ERROR_NO_STORE_CONNECTION_AVAILABLE_IN_POOL";
                    result[ "Message" ] = "No more Store connection available in pool to begin transaction";
                    result[ "Mark" ] = "22770DDDB3CB-" + thread_id;
                    result[ "IsError" ] = true;

                  }

                }
                else {

                  //status_code = 200; //Ok

                  //result[ "StatusCode" ] = status_code;
                  result[ "Code" ] = "SUCCESS_STORE_SQL_TRANSACTION_ALREADY_BEGUN";
                  result[ "Message" ] = "The Store SQL Transaction already begun";
                  result[ "Mark" ] = "62346B62C268-" + thread_id;
                  result[ "Data" ][ "TransactionId" ] = transaction_id;

                }

              }
              else {

                status_code = 400; //Bad request

                result[ "StatusCode" ] = status_code;
                result[ "Code" ] = "ERROR_TOO_MANY_ACTIVE_TRANSACTIONS";
                result[ "Message" ] = "Too many active transaction by authorization token";
                result[ "Mark" ] = "4D11F8A81525-" + thread_id;
                result[ "IsError" ] = true;

              }

            }
            else if ( check_authorization_result.value == -1 ) { //Exception in Security::check_autorization_is_valid_and_enabled

              status_code = 401; //Unhathorized

              result[ "StatusCode" ] = status_code;
              result[ "Code" ] = "ERROR_UNABLE_TO_CHECK_AUTORIZATION_TOKEN";
              result[ "Message" ] = "Unable to check the authorization token provided";
              result[ "Mark" ] = "D85038B80FF5-" + thread_id;
              result[ "IsError" ] = true;

            }
            else if ( check_authorization_result.value == -100 ) {

              status_code = 401; //Unhathorized

              result[ "StatusCode" ] = status_code;
              result[ "Code" ] = "ERROR_AUTORIZATION_TOKEN_IS_INVALID";
              result[ "Message" ] = "The authorization token provided is invalid";
              result[ "Mark" ] = "BE7C247FCA76-" + thread_id;
              result[ "IsError" ] = true;

            }
            else if ( check_authorization_result.value == -101 ) {

              status_code = 401; //Unhathorized

              result[ "StatusCode" ] = status_code;
              result[ "Code" ] = "ERROR_AUTORIZATION_TOKEN_IS_DISABLED";
              result[ "Message" ] = "The authorization token provided is disabled";
              result[ "Mark" ] = "3B303C7C6A4F-" + thread_id;
              result[ "IsError" ] = true;

            }
            else if ( check_authorization_result.value == -102 ) {

              status_code = 403; //Forbidden

              result[ "StatusCode" ] = status_code;
              result[ "Code" ] = "ERROR_STORE_RULES_NOT_DEFINED";
              result[ "Message" ] = "The store rules section in the config file not found or are invalid. Please check server config file";
              result[ "Mark" ] = "F172A3DBBC84-" + thread_id;
              result[ "IsError" ] = true;

            }

          }
          else {

            status_code = 400; //Bad request

            result[ "StatusCode" ] = status_code;
            result[ "Code" ] = "ERROR_STORE_NAME_NOT_FOUND";
            result[ "Message" ] = "The Store name not found";
            result[ "Mark" ] = "4E31F33D4145-" + thread_id;
            result[ "IsError" ] = true;

          }

        }
        else {

          status_code = 400; //Bad request

          result[ "StatusCode" ] = status_code;
          result[ "Code" ] = "ERROR_MISSING_FIELD_STORE";
          result[ "Message" ] = "The field Store is required and cannot be empty or null";
          result[ "Mark" ] = "BA4BF3C5FF90-" + thread_id;
          result[ "IsError" ] = true;

        }

      }
      else {

        status_code = 400; //Bad request

        result[ "StatusCode" ] = status_code;
        result[ "Code" ] = "ERROR_MISSING_FIELD_AUTORIZATION";
        result[ "Message" ] = "The field Autorization is required and cannot be empty or null";
        result[ "Mark" ] = "BA4BF3C5FF90-" + thread_id;
        result[ "IsError" ] = true;

      }

    }
    catch ( const std::exception &ex ) {

      status_code = 500; //Internal server error

      const std::string& thread_id = Common::xxHash_32( Common::get_thread_id() );

      result[ "StatusCode" ] = status_code;
      result[ "Code" ] = "ERROR_UNEXPECTED_EXCEPTION";
      result[ "Message" ] = "Unexpected error. Please read the server log for more details";
      result[ "Mark" ] = "D63A1315EB71-" + thread_id;
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
    result[ "Mark" ] = "5AA705EE0690-" + thread_id;
    result[ "IsError" ] = true;

  }

  ctx->response->content_type = APPLICATION_JSON;
  ctx->response->body = result.dump( 2 );

  //ctx->response->Json( result );

  return status_code;

}

}
