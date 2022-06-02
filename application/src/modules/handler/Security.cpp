
#include "Handlers.hpp"

#include <regex>

#include <boost/format.hpp>
#include <boost/algorithm/string.hpp>

//#include <boost/date_time/gregorian/gregorian.hpp>
//#include <boost/date_time/posix_time/ptime.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
//#include <boost/gregorian/date.hpp>

#include "hv/axios.h"

namespace Security {

void check_autorization( const std::string& authorization,
                         const std::string& store,
                         CheckAuthorizationResult& result ) {

  try {

    if ( Common::config_json[ "security" ].is_object() &&
         Common::config_json[ "security" ].size() > 0 ) {

      const auto& fecth_tokens = Common::config_json[ "security" ][ "fetch_tokens" ];

      auto& tokens = Common::config_json[ "security" ][ "tokens" ];

      if ( fecth_tokens.is_object() &&
           fecth_tokens[ "enabled" ].is_boolean() &&
           fecth_tokens[ "enabled" ].as_boolean() &&
           fecth_tokens[ "request" ].is_object() ) {

        if ( tokens.is_object() &&
             tokens.size() > 0 ) {

          boost::posix_time::ptime now = boost::posix_time::microsec_clock::local_time();

          bool refresh_needed = false;
          bool token_found = false;
          std::string on_fail_to_refresh = fecth_tokens[ "on_fail_to_refresh" ].is_string() ? fecth_tokens[ "on_fail_to_refresh" ].as_string(): "delete";

          if ( tokens[ authorization ].is_object() &&
               tokens[ authorization ].size() > 0 ) {

            token_found = true;

            if ( tokens[ authorization ][ "last_check" ].is_string() ) {

              const auto& last_check = Common::trim( tokens[ authorization ][ "last_check" ].as_string_ref() );

              const auto ttl_minutes = tokens[ authorization ][ "ttl_minutes" ].is_number() ? tokens[ authorization ][ "ttl_minutes" ].as_integer(): 1;

              //boost::gregorian::date last_check_as_date_time {};
              boost::posix_time::ptime last_check_as_date_time {};

              if ( last_check != "" ) {

                //last_check_as_date_time = boost::gregorian::from_simple_string( last_check );

                last_check_as_date_time = boost::posix_time::from_iso_string( last_check );

              }

              if ( ( now - last_check_as_date_time ).total_seconds() / 60 >= ttl_minutes ) {

                refresh_needed = true;

              }

            }

          }

          if ( token_found == false || refresh_needed ) {

            const auto request = fecth_tokens[ "request" ].to_json_string();

            boost::format fmt = boost::format( request ) % authorization;

            // const auto request = R"(
            //                          {
            //                            "method": "GET",
            //                            "url": "http://127.0.0.1:8080/fetch/token/info",
            //                            "headers": {
            //                                         "Content-Type": "application/json"
            //                                       },
            //                            "params": {
            //                                        "Authorization": "%1%"
            //                                      },
            //                            "body": {
            //                                    },
            //                            "timeout": 10000
            //                          }
            //                        )";

            auto resp = axios::axios( fmt.str().c_str() );

            if ( resp == nullptr ) {

              //Fail to comunicate with the end point
              if ( on_fail_to_refresh == "delete" ||
                   on_fail_to_refresh == "" ) {

                if ( token_found ) {

                  //auto& token = const_cast<Common::NANOJSONElementObject&>( tokens[ authorization ].as_object_ref() ); // = Common::NANOJSONElement();

                  //token.erase( token.begin(), token.end() ); // = Common::NANOJSONElement::undefined();

                  auto& tokens_ref = const_cast<Common::NANOJSONElementObject&>( tokens.as_object_ref() ); //[ authorization ].as_object_ref() ); // = Common::NANOJSONElement();

                  tokens_ref.erase( authorization );

                }

              }

            }
            else if ( resp->status_code >= 200 && resp->status_code <= 299 ) {

              //Sucess
              auto& tokens_ref = const_cast<Common::NANOJSONElementObject&>( tokens.as_object_ref() ); //[ authorization ].as_object_ref() ); // = Common::NANOJSONElement();

              tokens_ref[ authorization ] = Common::NANOJSONElement::from_string( resp->body );

              auto &token = const_cast<Common::NANOJSONElementObject&>( tokens_ref[ authorization ].as_object_ref() );

              //std::cout << tokens.to_json_string( 2 ) << std::endl;

              //std::cout << tokens[ authorization ].to_json_string( 2 ) << std::endl;

              //std::cout << "last_check: " << tokens[ authorization ][ "last_check" ] << std::endl;

              auto &last_check = token[ "last_check" ].as_string_ref();

              last_check = boost::posix_time::to_iso_string( now );

              //std::cout << "Last check: " << token[ "last_check" ].as_string() << std::endl;

              //token[ "last_check" ] = boost::posix_time::to_iso_string( now );

              //auto& last_check = const_cast<nanojson::element::string_t&>( token[ "last_check" ].as_string_ref() );

            }
            else if ( token_found ) {

              auto& tokens_ref = const_cast<Common::NANOJSONElementObject&>( tokens.as_object_ref() ); //[ authorization ].as_object_ref() ); // = Common::NANOJSONElement();

              tokens_ref.erase( authorization );

              // auto& token = const_cast<Common::NANOJSONElementObject&>( tokens[ authorization ].as_object_ref() ); // = Common::NANOJSONElement();

              // token.erase( token.begin(), token.end() );

            }

            //std::cout << tokens.to_json_string( 2 ) << std::endl;

          }

        }

      }

      if ( tokens.is_object() &&
           tokens.size() > 0 ) {

        const bool authorization_section_found = ( tokens[ authorization ].is_object() &&
                                                   tokens[ authorization ].size() > 0 );

        if ( authorization_section_found ||
            (
              tokens[ "*" ].is_object() &&
              tokens[ "*" ].size() > 0
            ) ) {

          if ( authorization_section_found == false ||
              (
                tokens[ authorization ][ "enabled" ].is_boolean() &&
                tokens[ authorization ][ "enabled" ].as_boolean() == true
              ) ) {

            const auto& token_options = authorization_section_found ?
                                        tokens[ authorization ]:
                                        tokens[ "*" ];

            const bool store_section_found = ( token_options[ store ].is_object() &&
                                               token_options[ store ].size() > 0 );

            if ( store_section_found ||
                (
                  token_options[ "*" ].is_object() &&
                  token_options[ "*" ].size() > 0
                ) ) {

              result.rules = store_section_found ?
                             const_cast<Common::NANOJSONElement *>( &token_options[ store ][ "rules" ] ):
                             const_cast<Common::NANOJSONElement *>( &token_options[ "*" ][ "rules" ] );

            }

            //std::cout << rules.to_string() << std::endl;

            if ( result.rules &&
                 result.rules->is_object() &&
                 result.rules->size() > 0 &&
                 (*result.rules)[ "deny" ].is_array() &&
                 (*result.rules)[ "allow" ].is_array() ) {

              if ( (
                     store_section_found &&
                     token_options[ store ][ "max_active_transactions" ].is_integer()
                   )
                   ||
                   token_options[ "*" ][ "max_active_transactions" ].is_integer() ) {

                const auto max_active_transactions = store_section_found ?
                                                     token_options[ store ][ "max_active_transactions" ].as_integer():
                                                     token_options[ "*" ][ "max_active_transactions" ].as_integer();

                if ( max_active_transactions > 0 ) {

                  result.max_active_transactions = max_active_transactions;

                }

              }

              result.value = 1;

            }
            else {

              result.value = -102; //No rules defined to store

            }

          }
          else {

            result.value = -101; //Token disabled

          }

        }
        else {

          result.value = -100; //Token not found. Unhathorized

        }

      }

    }

  }
  catch ( const std::exception &ex ) {

    hloge( "Exception: %s", ex.what() );

    std::cout << "Exception: " << ex.what() << std::endl;

  }

}

struct MatchRuleResult {

  int16_t value;
  std::string action;

};

inline MatchRuleResult match_rule( const std::string& authorization,
                                   const std::string& store,
                                   const std::string& command,
                                   const Common::NANOJSONElement& rule ) {

  MatchRuleResult result { 0, "" };

  try {

    bool rule_enabled = true;
    std::string rule_kind {}; //Exact expression by default
    std::string rule_expression {};
    std::string action {};

    if ( rule.is_object() ) {

      if ( rule[ "enabled" ].is_boolean() ) {

        rule_enabled = rule[ "enabled" ].as_boolean();

      }

      if ( rule[ "kind" ].is_string() ) {

        rule_kind = rule[ "kind" ].as_string_ref();

      }

      if ( rule[ "expression" ].is_string() ) {

        rule_expression = rule[ "expression" ].as_string_ref();

      }

      if ( rule[ "action" ].is_string() ) {

        result.action = Common::trim( rule[ "action" ].as_string_ref() );

      }

    }
    else if ( rule.is_string() ) {

      rule_expression = rule.as_string_ref();

    }

    rule_expression = Common::trim( rule_expression );

    if ( rule_expression != "" && rule_enabled ) {

      if ( rule_expression != "*" ) {

        if ( rule_kind == "reg_exp" ) {

          std::regex reg_exp( rule_expression, std::regex_constants::icase );

          if ( std::regex_match( command, reg_exp ) ) {

            result.value = 104; //Regular expression match found

          }

        }
        else if ( rule_kind == "fetch_command_authorized" ) {

          if ( rule[ "request" ].is_object() ) {

            const auto request = rule[ "request" ].to_json_string();

            boost::format fmt = boost::format( request ) % authorization
                                                         % store
                                                         % command;

            auto resp = axios::axios( fmt.str().c_str() );

            // const char* strReq = R"({
            //         "method": "POST",
            //         "url": "http://127.0.0.1:8080/fetch/command/authorized",
            //         "headers": {
            //                      "Content-Type": "application/json"
            //                    },
            //         "params": {
            //                   },
            //         "body": {
            //                   "Authorization": "%1%",
            //                   "Store": "%2%",
            //                   "Command": "%3%"
            //                 }
            //     })";

            // auto resp = axios::axios( strReq );

            //std::cout << resp->Dump( true, true ) << std::endl;

            if ( resp == nullptr ) {

              result.value = -101; //Fail to contact the peer

            }
            else if ( resp->status_code >= 200 && resp->status_code <= 299 ) { //Ok

              result.value = 103; //Command authorized

            }
            else {

              result.value = -102; //Command not authorized

            }

          }
          else {

            result.value = -104; //Command not authorized, no request section

          }

        }
        else {

          if ( boost::iequals( command, rule_expression ) ) {

            result.value = 102; //Exact match found

          }

        }

      }
      else {

        result.value = 101; //Match by asterick * = any

      }

    }
    // else {

    //   result = -101; //No valid rule
    //   break;

    // }

  }
  catch ( const std::exception &ex ) {

    result.value = -1; //Exception

    hloge( "Exception: %s", ex.what() );

    std::cout << "Exception: " << ex.what() << std::endl;

  }

  return result;

}

int16_t check_command_to_store_is_authorizated( const std::string& authorization,
                                                const std::string& store,
                                                const std::string& command,
                                                const Common::NANOJSONElement& rule_list ) {

  int16_t result = -1; //Unable to process the command in store authorized

  try {

    if ( rule_list.is_object() &&
         rule_list.size() > 0 &&
         rule_list[ "deny" ].is_array() &&
         rule_list[ "allow" ].is_array() ) {

      //std::cout << rule_list.to_json_string( 2 ) << std::endl;

      if ( rule_list[ "list" ].is_array() &&
           rule_list[ "list" ].size() > 0 ) {

        for ( const auto &rule: rule_list[ "list" ].as_array() ) {

          const auto& match_rule_result = match_rule( authorization,
                                                      store,
                                                      command,
                                                      rule );

          if ( match_rule_result.value == -1 ) { //Exception in match_rule

            result = -101; //Rule explicit denied by exception in match_rule
            break;

          }
          else if ( match_rule_result.value >= 101 ) {

            if ( match_rule_result.action == "allow" ) {

              result = 101; //Rule explicit allowed

            }
            else {

              result = -101; //Rule explicit denied

            }

            break;

          }

        }

      }

      if ( result == -1 ) {

        if ( rule_list[ "deny" ].size() > 0 ) {

          for ( const auto &rule: rule_list[ "deny" ].as_array() ) {

            const auto& match_rule_result = match_rule( authorization,
                                                        store,
                                                        command,
                                                        rule );

            if ( match_rule_result.value == -1 || //Exception in match_rule
                 match_rule_result.value >= 101 ) {

              result = -101; //Rule explicit denied
              break;

            }

          }

        }

      }

      if ( result == -1 ) {

        if ( rule_list[ "allow" ].size() > 0 ) {

          for ( const auto &rule: rule_list[ "allow" ].as_array() ) {

            const auto& match_rule_result = match_rule( authorization,
                                                        store,
                                                        command,
                                                        rule );

            if ( match_rule_result.value == -1 ) { //Exception in match_rule

              result = -101; //Rule explicit denied by exception in match_rule
              break;

            }
            else if ( match_rule_result.value >= 101 ) {

              result = 101; //Rule explicit allowed
              break;

            }

          }

        }

      }

      if ( result == -1 ) { //Not explicit explicit and not explicit denied?

        if ( Common::config_json[ "security" ].is_object() &&
             Common::config_json[ "security" ].size() > 0 &&
             Common::config_json[ "security" ][ "default" ].is_string() ) {

          result = Common::config_json[ "security" ][ "default" ].as_string() == "allow" ? 101: -101;

        }
        else {

          result = -101; //Hardcoded deny

        }

      }

    }
    else {

      result = -100; //No rules defined to store

    }

  }
  catch ( const std::exception &ex ) {

    hloge( "Exception: %s", ex.what() );

    std::cout << "Exception: " << ex.what() << std::endl;

  }

  return result;

}

}
