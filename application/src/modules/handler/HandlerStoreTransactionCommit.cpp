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

int handler_store_transaction_commit( const HttpContextPtr& ctx ) {

  auto type = ctx->type();

  u_int16_t status_code = 200;

  const std::string& thread_id = Common::xxHash_32( Common::get_thread_id() );

  auto result = Common::build_basic_response( 200, "", "", "", false, "" );

  if ( type == APPLICATION_JSON ) {

    try {

      auto json_body = hv::Json::parse( ctx->body() );

      if ( json_body[ "Authorization" ].is_null() == false &&
           Common::trim( json_body[ "Authorization" ] ) != "" ) {

        if ( json_body[ "TransactionId" ].is_null() == false &&
             Common::trim( json_body[ "TransactionId" ] ) != "" ) {

          const std::string &transaction_id = json_body[ "TransactionId" ].get<std::string>();

          auto store_sql_connection_in_transaction = Store::StoreConnectionManager::transaction_by_id( transaction_id );

          if ( store_sql_connection_in_transaction != nullptr ) {

            if ( store_sql_connection_in_transaction->is_active() ) {

              const std::string &authorization = json_body[ "Authorization" ];

              auto store_connection = Store::StoreConnectionManager::store_connection_by_id( transaction_id );

              //Common::NJSONElement store_rule_list {};

              Security::CheckAuthorizationResult check_authorization_result { 1, nullptr, 0 };

              if ( store_connection ) {

                Security::check_autorization( authorization,
                                              store_connection->name(),
                                              check_authorization_result );

              }

              if ( check_authorization_result.value == 1 ) { //OK

                try {

                  if ( store_connection ) {

                    //Stay sure no other thread use this connection
                    store_connection->mutex().lock();

                  }

                  store_sql_connection_in_transaction->commit();

                  Store::StoreConnectionManager::unregister_transaction_by_id( transaction_id );

                  Store::StoreConnectionManager::unregister_store_connection_by_id( transaction_id );

                  Store::StoreConnectionManager::unregister_transaction_id_by_authorization( authorization,
                                                                                             transaction_id );

                  if ( store_connection ) {

                    Store::StoreConnectionManager::return_leased_store_connection( store_connection );

                    store_connection->mutex().unlock(); //Release the lock

                  }

                }
                catch ( ... ) {

                  hloge( "Exception: Catch 6D91DFEDDF4B" );

                  if ( store_connection ) {

                    store_connection->mutex().unlock(); //Release the lock

                  }

                }

                //status_code = 200; //Ok

                //result[ "StatusCode" ] = status_code;
                result[ "Code" ] = "SUCCESS_STORE_SQL_TRANSACTION_COMMIT";
                result[ "Message" ] = "Success commit Store SQL Transaction";
                result[ "Mark" ] = "840BC8F6452E-" + thread_id;
                result[ "Data" ][ "TransactionId" ] = transaction_id;

              }
              else if ( check_authorization_result.value == -1 ) { //Exception in Security::check_autorization_is_valid_and_enabled

                status_code = 401; //Unhathorized

                result[ "StatusCode" ] = status_code;
                result[ "Code" ] = "ERROR_UNABLE_TO_CHECK_AUTORIZATION_TOKEN";
                result[ "Message" ] = "Unable to check the authorization token provided";
                result[ "Mark" ] = "23EBEAB73980-" + thread_id;
                result[ "IsError" ] = true;

              }
              else if ( check_authorization_result.value == -100 ) {

                status_code = 401; //Unhathorized

                result[ "StatusCode" ] = status_code;
                result[ "Code" ] = "ERROR_AUTORIZATION_TOKEN_IS_INVALID";
                result[ "Message" ] = "The authorization token provided is invalid";
                result[ "Mark" ] = "2AAF71ACD484-" + thread_id;
                result[ "IsError" ] = true;

              }
              else if ( check_authorization_result.value == -101 ) {

                status_code = 401; //Unhathorized

                result[ "StatusCode" ] = status_code;
                result[ "Code" ] = "ERROR_AUTORIZATION_TOKEN_IS_DISABLED";
                result[ "Message" ] = "The authorization token provided is disabled";
                result[ "Mark" ] = "60D75DCBEADF-" + thread_id;
                result[ "IsError" ] = true;

              }
              else if ( check_authorization_result.value == -102 ) {

                status_code = 403; //Forbidden

                result[ "StatusCode" ] = status_code;
                result[ "Code" ] = "ERROR_STORE_RULES_NOT_DEFINED";
                result[ "Message" ] = "The store rules section in the config file not found or are invalid. Please check server config file";
                result[ "Mark" ] = "88C9CA1758F6-" + thread_id;
                result[ "IsError" ] = true;

              }

            }
            else {

              status_code = 400; //Bad request

              result[ "StatusCode" ] = status_code;
              result[ "Code" ] = "ERROR_STORE_SQL_TRANSACTION_IS_NOT_ACTIVE";
              result[ "Message" ] = "The Store SQL Transaction is not active";
              result[ "Mark" ] = "E7B7762CD85E-" + thread_id;
              result[ "IsError" ] = true;

            }

          }
          else {

            status_code = 400; //Bad request

            result[ "StatusCode" ] = status_code;
            result[ "Code" ] = "ERROR_TRANSACTIONID_IS_INVALID";
            result[ "Message" ] = "The transaction id is invalid or not found";
            result[ "Mark" ] = "E7B98C331792-" + thread_id;
            result[ "IsError" ] = true;

          }

        }
        else {

          status_code = 400; //Bad request

          result[ "StatusCode" ] = status_code;
          result[ "Code" ] = "ERROR_MISSING_FIELD_TRANSACTIONID";
          result[ "Message" ] = "The field TransactionId is required and cannot be empty or null";
          result[ "Mark" ] = "0FE53C912836-" + thread_id;
          result[ "IsError" ] = true;

        }

      }
      else {

        status_code = 400; //Bad request

        result[ "StatusCode" ] = status_code;
        result[ "Code" ] = "ERROR_MISSING_FIELD_AUTORIZATION";
        result[ "Message" ] = "The field Autorization is required and cannot be empty or null";
        result[ "Mark" ] = "3EB0E43D2773-" + thread_id;
        result[ "IsError" ] = true;

      }

    }
    catch ( const std::exception &ex ) {

      status_code = 500; //Internal server error

      result[ "StatusCode" ] = status_code;
      result[ "Code" ] = "ERROR_UNEXPECTED_EXCEPTION";
      result[ "Message" ] = "Unexpected error. Please read the server log for more details";
      result[ "Mark" ] = "CF8DF1BD6DB7-" + thread_id;
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
