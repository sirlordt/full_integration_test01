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

int handler_store_transaction_rollback( const HttpContextPtr& ctx ) {

  auto type = ctx->type();

  u_int16_t status_code = 200;

  const std::string& thread_id = Common::xxHash_32( Common::get_thread_id() );

  if ( type == APPLICATION_JSON ) {

    try {

      auto json_body = hv::Json::parse( ctx->body() );

      if ( json_body[ "Autorization" ].is_null() == false &&
           Common::trim( json_body[ "Autorization" ] ) != "" ) {

        //status_code = Handlers::check_token_is_valid_and_enabled( Common::trim( json_body[ "Autorization" ] ) );

        //check the token
        //if ( status_code == 200 ) {

          if ( json_body[ "TransactionId" ].is_null() == false &&
               Common::trim( json_body[ "TransactionId" ] ) != "" ) {

            const std::string &transaction_id = json_body[ "TransactionId" ].get<std::string>();

            auto store_sql_connection_in_transaction = Store::StoreConnectionManager::transaction_by_id( transaction_id );

            if ( store_sql_connection_in_transaction != nullptr ) {

              if ( store_sql_connection_in_transaction->is_active() ) {

                auto store_connection = Store::StoreConnectionManager::store_connection_by_id( transaction_id );

                try {

                  if ( store_connection ) {

                    //Stay sure no other thread use this connection
                    store_connection->mutex().lock();

                  }

                  store_sql_connection_in_transaction->rollback();

                  Store::StoreConnectionManager::unregister_transaction_by_id( transaction_id );

                  Store::StoreConnectionManager::unregister_store_connection_by_id( transaction_id );

                  if ( store_connection ) {

                    Store::StoreConnectionManager::return_leased_store_connection( store_connection );

                    store_connection->mutex().unlock(); //Release the lock

                  }

                }
                catch ( ... ) {

                  if ( store_connection ) {

                    store_connection->mutex().unlock(); //Release the lock

                  }

                }

                status_code = 200; //Ok

                auto result = Common::build_basic_response( status_code,
                                                            "SUCCESS_STORE_SQL_TRANSACTION_ROLLBACK",
                                                            "Success rollback Store SQL Transaction",
                                                            "76024D77F6DB-" + thread_id,
                                                            false,
                                                            "" );

                result[ "Data" ][ 0 ][ "TransactionId" ] = transaction_id;

                if ( store_connection == nullptr ) {

                  result[ "Warnings" ][ "Code" ] = "WARNING_STORE_CONECTION_FROM_TRANSACTION_ID_NOT_FOUND";
                  result[ "Warnings" ][ "Message" ] = "Warning the store connection from transaction id not found";
                  result[ "Warnings" ][ "Details" ] = {};

                }

                ctx->response->content_type = APPLICATION_JSON;
                ctx->response->body = result.dump( 2 );

                //result[ "Message" ] = ex.what();
                //ctx->response->Json( result );

              }
              else {

                status_code = 400; //Bad request

                auto result = Common::build_basic_response( status_code,
                                                            "ERROR_STORE_SQL_TRANSACTION_IS_NOT_ACTIVE",
                                                            "The Store SQL Transaction is not active",
                                                            "AF06CC241042-" + thread_id,
                                                            true,
                                                            "" );

                ctx->response->content_type = APPLICATION_JSON;
                ctx->response->body = result.dump( 2 );

                //result[ "Message" ] = ex.what();
                //ctx->response->Json( result );

              }

            }
            else {

              status_code = 400; //Bad request

              auto result = Common::build_basic_response( status_code,
                                                          "ERROR_TRANSACTIONID_IS_INVALID",
                                                          "The transaction id is invalid or not found",
                                                          "FD2EF602E1FB-" + thread_id,
                                                          true,
                                                          "" );

              ctx->response->content_type = APPLICATION_JSON;
              ctx->response->body = result.dump( 2 );

              //result[ "Message" ] = ex.what();
              //ctx->response->Json( result );

            }

          }
          else {

            status_code = 400; //Bad request

            auto result = Common::build_basic_response( status_code,
                                                        "ERROR_MISSING_FIELD_TRANSACTIONID",
                                                        "The field TransactionId is required and cannot be empty or null",
                                                        "3D09F3D8C359-" + thread_id,
                                                        true,
                                                        "" );

            ctx->response->content_type = APPLICATION_JSON;
            ctx->response->body = result.dump( 2 );

            //result[ "Message" ] = ex.what();
            //ctx->response->Json( result );

          }

        /*
        }
        else if ( status_code == 401 ) { //Unauthorized

          auto result = Common::build_basic_response( status_code,
                                                      "ERROR_AUTHORIZATION_TOKEN_NOT_VALID",
                                                      "The authorization token provided is not valid or not found",
                                                      "5FA1400BFAEF-" + thread_id,
                                                      true,
                                                      "" );

        }
        else if ( status_code == 403 ) { //Forbidden

          auto result = Common::build_basic_response( status_code,
                                                      "ERROR_NOT_ALLOWED_ACCESS_TO_STORE",
                                                      "Not allowed access to store",
                                                      "0212000C0442-" + thread_id,
                                                      true,
                                                      "" );

        }
        */

      }
      else {

        status_code = 400; //Bad request

        auto result = Common::build_basic_response( status_code,
                                                    "ERROR_MISSING_FIELD_AUTORIZATION",
                                                    "The field Autorization is required and cannot be empty or null",
                                                    "B1A4BE329CF6-" + thread_id,
                                                    true,
                                                    "" );

        ctx->response->content_type = APPLICATION_JSON;
        ctx->response->body = result.dump( 2 );

        //result[ "Message" ] = ex.what();
        //ctx->response->Json( result );

        //hlogw( "Exception: %s", ex.what() );

      }

    }
    catch ( const std::exception &ex ) {

      status_code = 500; //Internal server error

      const std::string& thread_id = Common::xxHash_32( Common::get_thread_id() );

      auto result = Common::build_basic_response( status_code,
                                                  "ERROR_UNEXPECTED_EXCEPTION",
                                                  "Unexpected error. Please read the server log for more details.",
                                                  "64F8D1C61626-" + thread_id,
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

    auto result = Common::build_basic_response( status_code,
                                                "JSON_BODY_FORMAT_REQUIRED",
                                                "JSON format is required",
                                                "DD633CCBA53A-" + thread_id,
                                                true,
                                                "" );

    ctx->response->content_type = APPLICATION_JSON;
    ctx->response->body = result.dump( 2 );

    //ctx->response->Json( result );

  }

  return status_code;

}

}
