
#include "Handlers.hpp"

#include <regex>

#include <boost/algorithm/string.hpp>

namespace Security {

void check_autorization( const std::string& authorization,
                         const std::string& store,
                         CheckAuthorizationResult& result ) {

  try {

    if ( Common::config_json[ "security" ].is_object() &&
         Common::config_json[ "security" ].size() > 0 ) {

      const auto& tokens = Common::config_json[ "security" ][ "tokens" ];

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

// CheckAuthorizationResult check_autorization( const std::string& authorization,
//                                              const std::string& store ) {

//   CheckAuthorizationResult result { -1, nullptr, 0 }; //Unable to process the authorization token


// }

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

    std::string method {}; //GET, POST, PUT, DELETE
    std::string url {};
    std::string body {};

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

      if ( rule[ "method" ].is_string() ) {

        method = rule[ "method" ].as_string_ref();

      }

      if ( rule[ "url" ].is_string() ) {

        url = rule[ "url" ].as_string_ref();

      }

      if ( rule[ "body" ].is_string() ) {

        body = rule[ "body" ].as_string_ref();

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
        else if ( rule_kind == "remote_call" ) {

          //

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
