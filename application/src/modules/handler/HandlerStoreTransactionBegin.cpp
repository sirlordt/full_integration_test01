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

  if ( type == APPLICATION_JSON ) {

    try {

      auto json_body = hv::Json::parse( ctx->body() );

      //std::cout << json_body.dump( 2 ) << std::endl;

      if ( json_body[ "Autorization" ].is_null() == false &&
           Common::trim( json_body[ "Autorization" ] ) != "" ) {

        //status_code = check_token_is_valid_and_enabled( Common::trim( json_body[ "Autorization" ] ) );

        //check the token
        //if ( status_code == 200 ) {

          if ( json_body[ "Store" ].is_null() == false &&
               Common::trim( json_body[ "Store" ] ) != "" ) {

            UUIDv4::UUIDGenerator<std::mt19937_64> uuidGenerator;

            const std::string &autorization = json_body[ "Autorization" ];
            const std::string &store = json_body[ "Store" ];
            const std::string &transaction_id = json_body[ "TransactionId" ].is_null() == false && Common::trim( json_body[ "TransactionId" ] ) != "" ?
                                                json_body[ "TransactionId" ].get<std::string>():
                                                uuidGenerator.getUUID().str();

            if ( Store::StoreConnectionManager::store_connection_name_exists( store ) ) {

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

                    status_code = 200; //Ok

                    auto result = Common::build_basic_response( status_code,
                                                                "SUCCESS_STORE_SQL_TRANSACTION_BEGIN",
                                                                "Success Store SQL Transaction begin",
                                                                "1D0EA78D055E-" + thread_id,
                                                                false,
                                                                "" );

                    result[ "Data" ][ "TransactionId" ] = transaction_id;

                    ctx->response->content_type = APPLICATION_JSON;
                    ctx->response->body = result.dump( 2 );

                    //ctx->response->Json( result );

                  }
                  else {

                    status_code = 400; //Bad request

                    auto result = Common::build_basic_response( status_code,
                                                                "ERROR_STORE_CONNECTION_IS_NOT_SQL_KIND",
                                                                "The Store connection is not SQL kind",
                                                                "87A015B5FB0A-" + thread_id,
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
                                                              "ERROR_NO_STORE_CONNECTION_AVAILABLE_IN_POOL",
                                                              "No more Store connection available in pool to begin transaction",
                                                              "AE7EDEF5C086-" + thread_id,
                                                              true,
                                                              "" );

                  ctx->response->content_type = APPLICATION_JSON;
                  ctx->response->body = result.dump( 2 );

                  //result[ "Message" ] = ex.what();
                  //ctx->response->Json( result );

                }

              }
              else {

                status_code = 200; //Ok

                auto result = Common::build_basic_response( status_code,
                                                            "SUCCESS_STORE_SQL_TRANSACTION_ALREADY_BEGUN",
                                                            "The Store SQL Transaction already begun",
                                                            "62346B62C268-" + thread_id,
                                                            false,
                                                            "" );

                result[ "Data" ][ "TransactionId" ] = transaction_id;

                ctx->response->content_type = APPLICATION_JSON;
                ctx->response->body = result.dump( 2 );

                //result[ "Message" ] = ex.what();
                //ctx->response->Json( result );

              }

            }
            else {

              status_code = 400; //Bad request

              auto result = Common::build_basic_response( status_code,
                                                          "ERROR_STORE_NAME_NOT_FOUND",
                                                          "The Store name not found",
                                                          "4E31F33D4145-" + thread_id,
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
                                                        "ERROR_MISSING_FIELD_STORE",
                                                        "The field Store is required and cannot be empty or null",
                                                        "BA4BF3C5FF90-" + thread_id,
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
                                                      "3501F233B5AB-" + thread_id,
                                                      true,
                                                      "" );

        }
        else if ( status_code == 403 ) { //Forbidden

          auto result = Common::build_basic_response( status_code,
                                                      "ERROR_NOT_ALLOWED_ACCESS_TO_STORE",
                                                      "Not allowed access to store",
                                                      "D00B72B67A38-" + thread_id,
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
                                                    "BA4BF3C5FF90-" + thread_id,
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
                                                  "D63A1315EB71-" + thread_id,
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
                                                "5AA705EE0690-" + thread_id,
                                                false,
                                                "" );

    ctx->response->content_type = APPLICATION_JSON;
    ctx->response->body = result.dump( 2 );

    //ctx->response->Json( result );

  }

  return status_code;

}

}
