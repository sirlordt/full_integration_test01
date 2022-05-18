#include "Response.hpp"

//#include <fmt/core.h>

#include <boost/format.hpp>
//#include <boost/algorithm/string.hpp>

#include <sstream>

#include <hv/hlog.h>
//#include <hv/http_content.h>

namespace Common {

NLOJSONObject build_basic_response( uint16_t status_code,
                                    const std::string& code,
                                    const std::string& message,
                                    const std::string& mark,
                                    bool is_error,
                                    const std::string& log ) {

  NLOJSONObject result;

  try {

    const std::string json_template = R"(
                                          {
                                            "StatusCode": %1%,
                                            "Code": "%2%",
                                            "Message": "%3%",
                                            "Mark": "%4%",
                                            "Log": %6%,
                                            "IsError": %5%,
                                            "Errors": {},
                                            "Warnings": {},
                                            "Count": 0,
                                            "Data": {}
                                          }
                                        )";

    boost::format fmt = boost::format( json_template ) % status_code
                                                       % code
                                                       % message
                                                       % mark
                                                       % ( is_error ? "true": "false" )
                                                       % ( log != "" ? "\"" + log + "\"": "null" );

    result = nlohmann::ordered_json::parse( fmt.str() ); //nlohmann::json::parse( fmt.str() ); //nlohmann::ordered_json::parse( fmt.str() ); //hv::Json::parse( fmt.str() );

    // result = {
    //            { "StatusCode", status_code },
    //            { "Code", code },
    //            { "Message", message },
    //            { "Mark", mark },
    //            { "Log", ( log != "" ? log: "" ) },
    //            { "IsError", ( is_error ? "true": "false" ) },
    //            { "Errors", {} },
    //            { "Warnings", {} },
    //            { "Count", 0 },
    //            { "Data", {} }
    //          };

    // result[ "StatusCode" ] = status_code;
    // result[ "Code" ] = code;
    // result[ "Message" ] = message;
    // result[ "Mark" ] = mark;
    // result[ "Log" ] = ( log != "" ? log: "" );
    // result[ "IsError" ] = ( is_error ? "true": "false" );
    // result[ "Errors" ] = nlohmann::json::parse( "{ \"Message\": \"\" }" );
    // result[ "Warnings" ] = nlohmann::json::parse( "{}" );
    // result[ "Count" ] = 0;
    // result[ "Data" ] = nlohmann::json::parse( "{}" );

    //std::cout << result.dump( 2 ) << std::endl;

    // NLOJSONObject j;
    // j["f"] = 5;
    // j["a"] = 2;

    // std::cout << j.dump( 2 ) << std::endl;

    //boost::replace_all( json_template, "foo", "bar");

    // std::stringstream ss;

    // ss << R"(
    //          {
    //            "StatusCode": {0},
    //            "Code": "{1}",
    //            "Message": "{2}",
    //            "Mark": "{3}",
    //            "Log": "{4}",
    //            "IsError": {5},
    //            "Errors": {},
    //            "Warnings": {},
    //            "Count": 0,
    //            "Data": {}
    //          }
    //        )";

    //std::string s = fmt::format( "{ {0} }", status_code, code );

    //std::string x;

    // std::string s = fmt::format( R"(
    //                                  "StatusCode": {0},
    //                                  "Code": "{1}",
    //                                  "Message": "{2}",
    //                                  "Mark": "{3}",
    //                                  "Log": "{4}",
    //                                  "IsError": {5},
    //                                  "Errors": {},
    //                                )",
    //                                status_code,
    //                                code,
    //                                message,
    //                                mark,
    //                                log,
    //                                false );

  }
  catch ( const std::exception &ex ) {

    hloge( "Exception: %s", ex.what() );

    std::cout << "Exception: " << ex.what() << std::endl;

  }

  return result;

}

NLOJSONObject build_detail_block_response( const std::string& code,
                                           const std::string& message,
                                           const std::string& mark,
                                           const std::string& details ) {

  NLOJSONObject result;

  try {

    const std::string json_template = R"(
                                          {
                                            "Code": "%1%",
                                            "Message": "%2%",
                                            "Mark": "%3%",
                                            "Details": %4%
                                          }
                                        )";

    boost::format fmt = boost::format( json_template ) % code
                                                       % message
                                                       % mark
                                                       % ( details != "" ? details: "{}" );

    //const auto &temp = fmt.str();

    result = nlohmann::ordered_json::parse( fmt.str() );

  }
  catch ( const std::exception &ex ) {

    hloge( "Exception: %s", ex.what() );

    std::cout << "Exception: " << ex.what() << std::endl;

  }

  return result;

}

}
