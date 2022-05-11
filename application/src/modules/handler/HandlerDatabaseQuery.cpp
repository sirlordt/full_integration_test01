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

int handler_database_query( const HttpContextPtr& ctx ) {

  auto type = ctx->type();

  return ctx->response->String( "database/query" ); //ctx->send( ctx->body(), ctx->type() );

}

}
