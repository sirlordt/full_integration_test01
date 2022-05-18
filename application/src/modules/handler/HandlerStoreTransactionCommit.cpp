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

  if ( type == APPLICATION_JSON ) {

    try {

      auto json_body = hv::Json::parse( ctx->body() );

      if ( json_body[ "Autorization" ].is_null() == false &&
           Common::trim( json_body[ "Autorization" ] ) != "" ) {

        status_code = Handlers::check_token_is_valid_and_enabled( Common::trim( json_body[ "Autorization" ] ) );

        //check the token
        if ( status_code == 200 ) {

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

                  store_sql_connection_in_transaction->commit();

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

                auto result = R"(
                                  {
                                    "StatusCode": 200,
                                    "Code": "SUCCESS_STORE_SQL_TRANSACTION_COMMIT",
                                    "Message": "Success commit Store SQL Transaction",
                                    "Mark": "840BC8F6452E-",
                                    "Log": null,
                                    "IsError": false,
                                    "Errors": {}
                                    "Warnings": {},
                                    "Count": 1,
                                    "Data": {
                                              "TransactionId": ""
                                            }
                                  }
                                )"_json;

                const std::string& thread_id = Common::xxHash_32( Common::get_thread_id() );

                result[ "Mark" ] = result[ "Mark" ].get<std::string>() + thread_id; //result[ "Mark" ].value + "-" + std::this_thread::get_id();

                result[ "Data" ][ 0 ][ "TransactionId" ] = transaction_id;

                ctx->response->content_type = APPLICATION_JSON;
                ctx->response->body = result.dump( 2 );

                //result[ "Message" ] = ex.what();
                //ctx->response->Json( result );

              }
              else {

                status_code = 400; //Bad request

                auto result = R"(
                                  {
                                    "StatusCode": 400,
                                    "Code": "ERROR_STORE_SQL_TRANSACTION_IS_NOT_ACTIVE",
                                    "Message": "The Store SQL Transactin is not active",
                                    "Mark": "E7B7762CD85E-",
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

                ctx->response->content_type = APPLICATION_JSON;
                ctx->response->body = result.dump( 2 );

                //result[ "Message" ] = ex.what();
                //ctx->response->Json( result );

              }

            }
            else {

              status_code = 400; //Bad request

              auto result = R"(
                                {
                                  "StatusCode": 400,
                                  "Code": "ERROR_TRANSACTIONID_IS_INVALID",
                                  "Message": "The transaction id is invalid or not found",
                                  "Mark": "E7B98C331792-",
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

              ctx->response->content_type = APPLICATION_JSON;
              ctx->response->body = result.dump( 2 );

              //result[ "Message" ] = ex.what();
              //ctx->response->Json( result );

            }

          }
          else {

            status_code = 400; //Bad request

            auto result = R"(
                              {
                                "StatusCode": 400,
                                "Code": "ERROR_MISSING_FIELD_TRANSACTIONID",
                                "Message": "The field TransactionId is required and cannot be empty or null",
                                "Mark": "0FE53C912836-",
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

            ctx->response->content_type = APPLICATION_JSON;
            ctx->response->body = result.dump( 2 );

            //result[ "Message" ] = ex.what();
            //ctx->response->Json( result );

          }

        }
        else if ( status_code == 401 ) { //Unauthorized

          auto result = R"(
                            {
                              "StatusCode": 401,
                              "Code": "ERROR_AUTHORIZATION_TOKEN_NOT_VALID",
                              "Message": "The authorization token provided is not valid or not found",
                              "Mark": "CDF0C67A4582-",
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
                              "Mark": "829B51ED7AE4-",
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

        auto result = R"(
                          {
                            "StatusCode": 400,
                            "Code": "ERROR_MISSING_FIELD_AUTORIZATION",
                            "Message": "The field Autorization is required and cannot be empty or null",
                            "Mark": "3EB0E43D2773-",
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
                                                  "CF8DF1BD6DB7-" + thread_id,
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

    auto result = R"(
                      {
                        "StatusCode": 400,
                        "Code": "JSON_BODY_FORMAT_REQUIRED",
                        "Message": "JSON format is required",
                        "Mark": "5AA705EE0690-",
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

    ctx->response->content_type = APPLICATION_JSON;
    ctx->response->body = result.dump( 2 );

    //ctx->response->Json( result );

  }

  return status_code;

}

}
