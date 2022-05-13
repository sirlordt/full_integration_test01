#pragma once

#include <memory>
#include <vector>
#include <queue>
#include <mutex>

//#include <nanojson/nanojson.hpp>
#include "Common.hpp"

#include "StoreConnection.hpp"

namespace Store {

using VectorStoreConnectionSharedPtr = std::vector<StoreConnectionSharedPtr>;
using QueueStoreConnectionSharedPtr = std::queue<StoreConnectionSharedPtr>;
using MapNameToQueueStoreConnectionSharedPtr = std::map<std::string,QueueStoreConnectionSharedPtr>;
using MapTransactionIdToStoreSQLConnectionTransactionSharedPtr = std::map<std::string,StoreSQLConnectionTransactionPtr>;
using MapIdToStoreConnectionSharedPtr = std::map<std::string,StoreConnectionSharedPtr>;
using MapStoreConnectionAliasNameToRealName = std::map<std::string,std::string>;

class StoreConnectionManager {
public:

  StoreConnectionManager() = delete;
  StoreConnectionManager( StoreConnectionManager &store_connection_manager ) = delete;
  StoreConnectionManager( StoreConnectionManager &&store_connection_manager ) = delete;

  StoreConnectionManager &operator=( StoreConnectionManager &store_connection_manager ) = delete;
  StoreConnectionManager &&operator=( StoreConnectionManager &&store_connection_manager ) = delete;

  static bool init_stores_connections( Common::NJSONElementArray &stores_connections_config_list );
  static bool init_stores_connections_alias( Common::NJSONElementObject &stores_connections_alias_config_list );

  static bool store_connection_name_exists( const std::string& name );

  static StoreConnectionSharedPtr lease_store_connection_by_name( const std::string& name );
  static bool return_leased_store_connection( StoreConnectionSharedPtr &store_connection );

  static StoreSQLConnectionTransactionPtr transaction_by_id( const std::string& id );
  static bool register_transaction_to_id( const std::string& id,
                                          StoreSQLConnectionTransactionPtr &store_sql_connection_transaction );
  static bool unregister_transaction_by_id( const std::string& id );

  static StoreConnectionSharedPtr store_connection_by_id( const std::string& id );
  static bool register_store_connection_to_id( const std::string& id,
                                               StoreConnectionSharedPtr &store_connection );
  static bool unregister_store_connection_by_id( const std::string& id );

private:

  inline static std::mutex mutex_ {};

  inline static VectorStoreConnectionSharedPtr vector_store_connection_on_hold_ {}; //Prevent release the shared_ptr

  inline static MapNameToQueueStoreConnectionSharedPtr map_name_to_queue_store_connection_ {};


  inline static MapTransactionIdToStoreSQLConnectionTransactionSharedPtr map_id_to_store_sql_connection_transaction_ {};

  inline static MapIdToStoreConnectionSharedPtr map_id_to_store_connection_ {};

  inline static MapStoreConnectionAliasNameToRealName map_store_connection_alias_name_to_real_name_ {};

  //inline static std::shared_ptr<StoreConnectionManager> store_connection_manager_;

  //StoreConnectionManager( nanojson::element database_connection_config );

};

}
