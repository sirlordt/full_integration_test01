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

/*

  result = {
             StatusCode: 200, //Ok
             Code: "SUCCESS_GET_DRIVER_POSITION",
             Message: await I18NManager.translate( strLanguage, "Success get driver position." ),
             Mark: "AECC8FB185E4" + ( cluster.worker && cluster.worker.id ? "-" + cluster.worker.id : "" ),
             LogId: null,
             IsError: false,
             Errors: [],
             Warnings: [],
             Count: lastDriverPositionList.length,
             Data: lastDriverPositionList
           };

  result = {
             StatusCode: 400, //Bad request
             Code: "ERROR_UNKNOWN_MODEL",
             Message: strMessage,
             Mark: "5AA705EE0690" + ( cluster.worker && cluster.worker.id ? "-" + cluster.worker.id : "" ),
             LogId: null,
             IsError: true,
             Errors: [
                       {
                         Code: "ERROR_UNKNOWN_MODEL",
                         Message: strMessage,
                         Details: null
                       }
                     ],
             Warnings: [],
             Count: 0,
             Data: []
           };

  result = {
             StatusCode: 500, //Internal server error
             Code: "ERROR_UNEXPECTED",
             Message: "Unexpected error. Please read the server log for more details.",
             LogId: error.LogId,
             Mark: strMark,
             IsError: true,
             Errors: [
                       {
                         Code: error.name,
                         Message: error.message,
                         Details: await SystemUtilities.processErrorDetails( error ) //error
                       }
                     ],
             Warnings: [],
             Count: 0,
             Data: []
           };

*/

namespace Handlers {

int handler_database_transaction_begin( const HttpContextPtr& ctx ) {

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

                    auto result = R"(
                                      {
                                        "StatusCode": 200,
                                        "Code": "SUCCESS_STORE_SQL_TRANSACTION_BEGIN",
                                        "Message": "Success Store SQL Transaction begin",
                                        "Mark": "1D0EA78D055E-",
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
                                        "Code": "ERROR_STORE_CONNECTION_IS_NOT_SQL_KIND",
                                        "Message": "The Store connection is not SQL kind",
                                        "Mark": "87A015B5FB0A-",
                                        "Log": null,
                                        "IsError": true,
                                        "Errors": [
                                                    {
                                                      "Code": "ERROR_STORE_CONNECTION_IS_NOT_SQL_KIND",
                                                      "Message": "The Store connection is not SQL kind",
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
                                      "Code": "ERROR_NO_STORE_CONNECTION_AVAILABLE_IN_POOL",
                                      "Message": "No more Store connection available in pool to begin transaction",
                                      "Mark": "AE7EDEF5C086-",
                                      "Log": null,
                                      "IsError": true,
                                      "Errors": [
                                                  {
                                                    "Code": "ERROR_NO_STORE_CONNECTION_AVAILABLE_IN_POOL",
                                                    "Message": "No more Store connection available in pool to begin transaction",
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

                status_code = 200; //Ok

                auto result = R"(
                                  {
                                    "StatusCode": 200,
                                    "Code": "SUCCESS_STORE_SQL_TRANSACTION_ALREADY_BEGUN",
                                    "Message": "The Store SQL Transaction already begun",
                                    "Mark": "62346B62C268-",
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

            }
            else {

              status_code = 400; //Bad request

              auto result = R"(
                                {
                                  "StatusCode": 400,
                                  "Code": "ERROR_STORE_NAME_NOT_FOUND",
                                  "Message": "The Store name not found",
                                  "Mark": "4E31F33D4145-",
                                  "Log": null,
                                  "IsError": true,
                                  "Errors": [
                                              {
                                                "Code": "ERROR_STORE_NAME_NOT_FOUND",
                                                "Message": "The Store name not found",
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
                                "Code": "ERROR_MISSING_FIELD_STORE",
                                "Message": "The field Store is required and not empty and not null",
                                "Mark": "BA4BF3C5FF90-",
                                "Log": null,
                                "IsError": true,
                                "Errors": [
                                            {
                                              "Code": "ERROR_MISSING_FIELD_STORE",
                                              "Message": "The field Store is required and not empty and not null",
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
                              "Mark": "3501F233B5AB-",
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
                              "Mark": "D00B72B67A38-",
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
                            "Mark": "BA4BF3C5FF90-",
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
                          "Mark": "BA4BF3C5FF90-",
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
                        "Mark": "5AA705EE0690-",
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
