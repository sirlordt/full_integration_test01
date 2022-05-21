#pragma once

#include <hv/hv.h>
#include <hv/HttpServer.h>

#include "../common/Common.hpp"

namespace Security {

struct CheckAuthorizationResult {

  int16_t value { 0 };
  Common::NJSONElement* rules { nullptr };
  u_int16_t max_active_transactions { 1 };

};

void check_autorization( const std::string &token,
                         const std::string &store,
                         CheckAuthorizationResult& result );

int16_t check_command_to_store_is_authorizated( const std::string& authorization,
                                                const std::string& store,
                                                const std::string& command,
                                                const Common::NJSONElement& rule_list );

}

namespace Handlers {


int handler_store_transaction_begin( const HttpContextPtr& ctx );
int handler_store_transaction_commit( const HttpContextPtr& ctx );
int handler_store_transaction_rollback( const HttpContextPtr& ctx );

int handler_store_query( const HttpContextPtr& ctx );

}
