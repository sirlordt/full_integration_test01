#pragma once

#include "hv/hv.h"
#include "hv/HttpServer.h"

namespace Handlers {

u_int16_t check_token_is_valid_and_authorized( const std::string &token );

int handler_database_transaction_begin( const HttpContextPtr& ctx );
int handler_database_transaction_commit( const HttpContextPtr& ctx );
int handler_database_transaction_rollback( const HttpContextPtr& ctx );

int handler_database_query( const HttpContextPtr& ctx );

}
