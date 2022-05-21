#pragma once

#include <cstdint>

#include "Common.hpp"

//#include <hv/http_content.h>

namespace Common {

/*
  //Old versions

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
             Message: "Unexpected error. Please read the server log for more details",
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

/*

  auto result = R"(
                    {
                      "StatusCode": 400,
                      "Code": "ERROR_MISSING_FIELD_EXECUTE",
                      "Message": "The field Execute is required as array of strings and cannot be empty or null",
                      "Mark": "FEE80B366130-",
                      "Log": null,
                      "IsError": true,
                      "Errors": {
                                  "0": [
                                         {
                                           "Code": "ERROR_MISSING_FIELD_EXECUTE",
                                           "Message": "The field Execute is required as array of strings and cannot be empty or null",
                                           "Details": null
                                         }
                                       ]
                                },
                      "Warnings": {},
                      "Count": 0,
                      "Data": {}
                    }
                  )"_json;

*/

NLOJSONObject build_basic_response( uint16_t status_code,
                                    const std::string& code,
                                    const std::string& message,
                                    const std::string& mark,
                                    bool is_error,
                                    const std::string& log );

NLOJSONObject build_detail_block_response( const std::string& code,
                                           const std::string& message,
                                           const std::string& mark,
                                           const std::string& details );

}
