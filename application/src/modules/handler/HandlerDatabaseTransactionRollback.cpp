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

#include "Handlers.hpp"

namespace Handlers {

int handler_database_transaction_rollback( const HttpContextPtr& ctx ) {

  auto type = ctx->type();

  u_int16_t status_code = 200;

  if ( type == APPLICATION_JSON ) {

    try {

      auto json_body = hv::Json::parse( ctx->body() );

      if ( json_body[ "Autorization" ].is_null() == false &&
           Common::trim( json_body[ "Autorization" ] ) != "" ) {

        status_code = Handlers::check_token_is_valid_and_authorized( Common::trim( json_body[ "Autorization" ] ) );

        //check the token
        if ( status_code = 200 ) {

          if ( json_body[ "TransactionId" ].is_null() == false &&
               Common::trim( json_body[ "TransactionId" ] ) != "" ) {

            const std::string &transaction_id = json_body[ "TransactionId" ].get<std::string>();

            auto store_sql_connection_in_transaction = Store::StoreConnectionManager::transaction_by_id( transaction_id );

            if ( store_sql_connection_in_transaction != nullptr ) {

              if ( store_sql_connection_in_transaction->is_active() ) {

                store_sql_connection_in_transaction->rollback();

                auto store_connection = Store::StoreConnectionManager::store_connection_by_id( transaction_id );

                Store::StoreConnectionManager::return_leased_store_connection( store_connection );

                Store::StoreConnectionManager::unregister_transaction_by_id( transaction_id );

                Store::StoreConnectionManager::unregister_store_connection_by_id( transaction_id );

                status_code = 200; //Ok

                auto result = R"(
                                  {
                                    "StatusCode": 200,
                                    "Code": "SUCCESS_STORE_SQL_TRANSACTION_ROLLBACK",
                                    "Message": "Success rollback Store SQL Transaction",
                                    "Mark": "76024D77F6DB-",
                                    "Log": null,
                                    "IsError": false,
                                    "Errors": [],
                                    "Warnings": [],
                                    "Count": 1,
                                    "Data": [
                                              {
                                                "TransactionId": ""
                                              }
                                            ]
                                  }
                                )"_json;

                const std::string& thread_id = Common::xxHash_32( Common::get_thread_id() );

                result[ "Mark" ] = result[ "Mark" ].get<std::string>() + thread_id; //result[ "Mark" ].value + "-" + std::this_thread::get_id();

                result[ "Data" ][ 0 ][ "TransactionId" ] = transaction_id;

                //result[ "Message" ] = ex.what();
                ctx->response->Json( result );

              }
              else {

                status_code = 400; //Bad request

                auto result = R"(
                                  {
                                    "StatusCode": 400,
                                    "Code": "ERROR_STORE_SQL_TRANSACTION_IS_NOT_ACTIVE",
                                    "Message": "The Store SQL Transaction is not active",
                                    "Mark": "AF06CC241042-",
                                    "Log": null,
                                    "IsError": true,
                                    "Errors": [
                                                {
                                                  "Code": "ERROR_STORE_SQL_TRANSACTION_IS_NOT_ACTIVE",
                                                  "Message": "The Store SQL Transaction is not active",
                                                  "Details": null
                                                }
                                              ],
                                    "Warnings": [],
                                    "Count": 0,
                                    "Data": []
                                  }
                                )"_json;

                const std::string& thread_id = Common::xxHash_32( Common::get_thread_id() );

                result[ "Mark" ] = result[ "Mark" ].get<std::string>() + thread_id; //result[ "Mark" ].value + "-" + std::this_thread::get_id();

                //result[ "Message" ] = ex.what();
                ctx->response->Json( result );

              }

            }
            else {

              status_code = 400; //Bad request

              auto result = R"(
                                {
                                  "StatusCode": 400,
                                  "Code": "ERROR_TRANSACTIONID_IS_INVALID",
                                  "Message": "The transaction id is invalid or not found",
                                  "Mark": "FD2EF602E1FB-",
                                  "Log": null,
                                  "IsError": true,
                                  "Errors": [
                                              {
                                                "Code": "ERROR_TRANSACTIONID_IS_INVALID",
                                                "Message": "The transaction id is invalid or not found",
                                                "Details": null
                                              }
                                            ],
                                  "Warnings": [],
                                  "Count": 0,
                                  "Data": []
                                }
                              )"_json;

              const std::string& thread_id = Common::xxHash_32( Common::get_thread_id() );

              result[ "Mark" ] = result[ "Mark" ].get<std::string>() + thread_id; //result[ "Mark" ].value + "-" + std::this_thread::get_id();

              //result[ "Message" ] = ex.what();
              ctx->response->Json( result );

            }

          }
          else {

            status_code = 400; //Bad request

            auto result = R"(
                              {
                                "StatusCode": 400,
                                "Code": "ERROR_MISSING_FIELD_TRANSACTIONID",
                                "Message": "The field TransactionId is required and not empty and not null",
                                "Mark": "3D09F3D8C359-",
                                "Log": null,
                                "IsError": true,
                                "Errors": [
                                            {
                                              "Code": "ERROR_MISSING_FIELD_TRANSACTIONID",
                                              "Message": "The field TransactionId is required and not empty and not null",
                                              "Details": null
                                            }
                                          ],
                                "Warnings": [],
                                "Count": 0,
                                "Data": []
                              }
                            )"_json;

            const std::string& thread_id = Common::xxHash_32( Common::get_thread_id() );

            result[ "Mark" ] = result[ "Mark" ].get<std::string>() + thread_id; //result[ "Mark" ].value + "-" + std::this_thread::get_id();

            //result[ "Message" ] = ex.what();
            ctx->response->Json( result );

          }

        }
        else if ( status_code == 401 ) { //Unauthorized

          auto result = R"(
                            {
                              "StatusCode": 401,
                              "Code": "ERROR_AUTHORIZATION_TOKEN_NOT_VALID",
                              "Message": "The authorization token provided is not valid or not found",
                              "Mark": "5FA1400BFAEF-",
                              "Log": null,
                              "IsError": true,
                              "Errors": [
                                          {
                                            "Code": "ERROR_AUTHORIZATION_TOKEN_NOT_VALID",
                                            "Message": "The authorization token provided is not valid or not found",
                                            "Details": null
                                          }
                                        ],
                              "Warnings": [],
                              "Count": 0,
                              "Data": []
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
                              "Mark": "0212000C0442-",
                              "Log": null,
                              "IsError": true,
                              "Errors": [
                                          {
                                            "Code": "ERROR_NOT_ALLOWED_ACCESS_TO_STORE",
                                            "Message": "Not allowed access to store",
                                            "Details": null
                                          }
                                        ],
                              "Warnings": [],
                              "Count": 0,
                              "Data": []
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
                            "Message": "The field Autorization is required and not empty and not null",
                            "Mark": "B1A4BE329CF6-",
                            "Log": null,
                            "IsError": true,
                            "Errors": [
                                        {
                                          "Code": "ERROR_MISSING_FIELD_AUTORIZATION",
                                          "Message": "The field Autorization is required and not empty and not null",
                                          "Details": null
                                        }
                                      ],
                            "Warnings": [],
                            "Count": 0,
                            "Data": []
                          }
                        )"_json;

        const std::string& thread_id = Common::xxHash_32( Common::get_thread_id() );

        result[ "Mark" ] = result[ "Mark" ].get<std::string>() + thread_id; //result[ "Mark" ].value + "-" + std::this_thread::get_id();

        //result[ "Message" ] = ex.what();
        ctx->response->Json( result );

        //hlogw( "Exception: %s", ex.what() );

      }

    }
    catch ( const std::exception &ex ) {

      status_code = 400; //Bad request

      auto result = R"(
                        {
                          "StatusCode": 400,
                          "Code": "ERROR_INVALID_JSON_BODY_DATA",
                          "Message": "Must be a valid json data format",
                          "Mark": "710036C7BFB0-",
                          "Log": null,
                          "IsError": true,
                          "Errors": [
                                      {
                                        "Code": "ERROR_INVALID_JSON_BODY_DATA",
                                        "Message": "Must be a valid json data format",
                                        "Details": null
                                      }
                                    ],
                          "Warnings": [],
                          "Count": 0,
                          "Data": []
                        }
                      )"_json;

      const std::string& thread_id = Common::xxHash_32( Common::get_thread_id() );

      result[ "Mark" ] = result[ "Mark" ].get<std::string>() + thread_id; //result[ "Mark" ].value + "-" + std::this_thread::get_id();

      //result[ "Message" ] = ex.what();
      result[ "Errors" ][ 0 ][ "Message" ] = ex.what();

      ctx->response->Json( result );

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
                        "Mark": "DD633CCBA53A-",
                        "Log": null,
                        "IsError": true,
                        "Errors": [
                                    {
                                      "Code": "JSON_BODY_FORMAT_REQUIRED",
                                      "Message": "JSON format is required",
                                      "Details": null
                                    }
                                  ],
                        "Warnings": [],
                        "Count": 0,
                        "Data": []
                      }
                    )"_json;

    const std::string& thread_id = Common::xxHash_32( Common::get_thread_id() );

    result[ "Mark" ] = result[ "Mark" ].get<std::string>() + thread_id; //result[ "Mark" ].value + "-" + std::this_thread::get_id();

    ctx->response->Json( result );

  }

  return status_code;

}

}
