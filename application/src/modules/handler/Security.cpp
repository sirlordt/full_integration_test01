
#include "Handlers.hpp"

#include <regex>

#include <boost/algorithm/string.hpp>

namespace Security {

int16_t check_autorization_is_valid_and_enabled( const std::string& authorization,
                                                 const std::string& store,
                                                 Common::NJSONElement& rules ) {

  int16_t result { -1 }; //Unable to process the authorization token

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

            auto token_options = authorization_section_found ?
                                 tokens[ authorization ]:
                                 tokens[ "*" ];

            const bool store_section_found = ( token_options[ store ].is_object() &&
                                               token_options[ store ].size() > 0 );

            if ( store_section_found ||
                (
                  token_options[ "*" ].is_object() &&
                  token_options[ "*" ].size() > 0
                ) ) {

              rules = store_section_found ?
                      token_options[ store ][ "rules" ]:
                      token_options[ "*" ][ "rules" ];

            }

            //std::cout << rules.to_string() << std::endl;

            if ( rules.is_object() &&
                 rules.size() > 0 &&
                 rules[ "deny" ].is_array() &&
                 rules[ "allow" ].is_array() ) {

              result = 1;

            }
            else {

              result = -102; //No rules defined to store

            }

          }
          else {

            result = -101; //Token disabled

          }

        }
        else {

          result = -100; //Token not found. Unhathorized

        }

      }

    }

  }
  catch ( const std::exception &ex ) {

    hloge( "Exception: %s", ex.what() );

    std::cout << "Exception: " << ex.what() << std::endl;

  }

  return result;

}

struct MatchRuleResult {

  int16_t value;
  std::string action;

};

inline MatchRuleResult match_rule( const std::string& authorization,
                                   const std::string& store,
                                   const std::string& command,
                                   const Common::NJSONElement& rule ) {

  MatchRuleResult result { 0, "" };

  try {

    std::string rule_kind {}; //Exact expression by default
    std::string rule_expression {};
    std::string action {};

    std::string verb {}; //GET, POST, PUT, DELETE
    std::string url {};

    if ( rule.is_object() ) {

      if ( rule[ "kind" ].is_string() ) {

        rule_kind = rule[ "kind" ].as_string_ref();

      }

      if ( rule[ "expression" ].is_string() ) {

        rule_expression = rule[ "expression" ].as_string_ref();

      }

      if ( rule[ "verb" ].is_string() ) {

        verb = rule[ "verb" ].as_string_ref();

      }

      if ( rule[ "url" ].is_string() ) {

        url = rule[ "url" ].as_string_ref();

      }

      if ( rule[ "action" ].is_string() ) {

        result.action = Common::trim( rule[ "action" ].as_string_ref() );

      }

    }
    else if ( rule.is_string() ) {

      rule_expression = rule.as_string_ref();

    }

    rule_expression = Common::trim( rule_expression );

    if ( rule_expression != "" ) {

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
                                                const Common::NJSONElement& rule_list ) {

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
