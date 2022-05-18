#include <algorithm>    // std::find

#include "StoreConnectionManager.hpp"

#include "hv/hlog.h"

namespace Store {

//Common::JSONElementObject &store_alias_config_list

bool StoreConnectionManager::init_stores_connections( Common::NJSONElementArray &store_connections_config_list ) {

  bool result { false };

  try {

    std::lock_guard<std::mutex> lock_guard( mutex_ );

    if ( vector_store_connection_on_hold_.size() == 0 ) { //Not already initited

      std::vector<std::string> store_name_list;
      store_name_list.reserve( store_connections_config_list.size() );

      for ( auto &store_connection_config: store_connections_config_list ) {

        const std::string& name = store_connection_config[ "name" ].is_string() ? store_connection_config[ "name" ].to_string(): "";

        u_int8_t pool = store_connection_config[ "pool" ].is_integer() ? store_connection_config[ "pool" ].to_integer(): 0;

        if ( Common::trim( name ) != "" &&
             pool > 0 ) {

          if ( std::find( store_name_list.begin(), store_name_list.end(), name ) == store_name_list.end() ) {

            store_name_list.push_back( name );

            for ( uint8_t connection_index = 0; connection_index < pool; connection_index++ ) {

              auto store_connection = Common::make_store_connection( store_connection_config );

              if ( store_connection ) {

                store_connection->index_ = connection_index;

                vector_store_connection_on_hold_.push_back( store_connection );

                map_name_to_queue_store_connection_[ store_connection->name() ].push( store_connection );

              }

            }

            // auto store_connection = Common::make_store_connection( store_connection_config );

            // if ( store_connection ) {

            //   store_connection->index_ = 0;

            //   u_int8_t pool = store_connection->pool();

            //   if ( vector_store_connection_on_hold_.capacity() != pool ) {

            //     vector_store_connection_on_hold_.reserve( pool );

            //   }

            //   do {

            //     if ( store_connection ) {

            //       vector_store_connection_on_hold_.push_back( store_connection );

            //       map_name_to_queue_store_connection_[ store_connection->name() ].push( store_connection );

            //       pool -= 1;

            //       store_connection = nullptr;

            //     }

            //     if ( !store_connection && pool > 0 ) {

            //       store_connection = Common::make_store_connection( store_connection_config );

            //       store_connection->index_ += 1;

            //     }

            //   } while ( pool > 0 );

            // }

          }
          else {

            hlogw( "The store connection with name [%s] already exists. Please check your config file.", name.c_str() );

          }

        }
        else {

          hlogw( "The store connection with name [%s] has invalid values. Please check your config file.", name.c_str() );

        }

      }

      result = true;

    }

  }
  catch ( const std::exception &ex ) {

    hloge( "Exception: %s", ex.what() );

    std::cout << "Exception: " << ex.what() << std::endl;

  }

  return result;

}

bool StoreConnectionManager::init_stores_connections_alias( Common::NJSONElementObject &stores_connections_alias_config_list ) {

  bool result { false };

  try {

    //auto stores_connections_alias = config_json[ "stores_connections_alias" ].as_object_ref();

    for ( auto it = stores_connections_alias_config_list.begin(); it != stores_connections_alias_config_list.end(); it++ ) {

      if ( Common::trim( it->first ) != "" ) {

        if ( it->second.is_string() &&
             Common::trim( it->second.to_string() ) != "" ) {

          if ( map_name_to_queue_store_connection_.find( it->second ) != map_name_to_queue_store_connection_.end() ) {

            if ( map_store_connection_alias_name_to_real_name_.find( it->first ) == map_store_connection_alias_name_to_real_name_.end() ) {

              map_store_connection_alias_name_to_real_name_[ it->first ] = it->second.to_string();

            }
            else {

              hlogw( "The alias with name: [%s] and value of [%s]. But already exists in alias name list. Please check your config file.",
                     it->first.c_str(),
                     it->second.to_string().c_str() );

            }

          }
          else {

            hlogw( "The alias with name: [%s] has value of [%s]. But not exists in stores connections name list. Please check your config file.",
                   it->first.c_str(),
                   it->second.to_string().c_str() );

          }

        }
        else {

          hlogw( "The alias with name: [%s] has invalid or empty value. Please check your config file." );

        }

      }
      else {

        hlogw( "The alias has invalid or empty name. Please check your config file." );

      }

      //std::cout << "key: " << it->first << std::endl;
      //std::cout << "value: " << it->second << std::endl;

    }

  }
  catch ( const std::exception &ex ) {

    hloge( "Exception: %s", ex.what() );

    std::cout << "Exception: " << ex.what() << std::endl;

  }

  return result;

}

bool StoreConnectionManager::store_connection_name_exists( const std::string& name ) {

  std::lock_guard<std::mutex> lock_guard( mutex_ );

  return map_name_to_queue_store_connection_.find( name ) != map_name_to_queue_store_connection_.end() ||
         map_store_connection_alias_name_to_real_name_.find( name ) !=  map_store_connection_alias_name_to_real_name_.end();

}

StoreConnectionSharedPtr StoreConnectionManager::lease_store_connection_by_name( const std::string& name ) {

  StoreConnectionSharedPtr result { nullptr };

  try {

    std::lock_guard<std::mutex> lock_guard( mutex_ );

    bool is_real_name = map_name_to_queue_store_connection_.find( name ) != map_name_to_queue_store_connection_.end();
    //bool is_alias_name = is_real_name ? false: ;

    if ( is_real_name ||
         map_store_connection_alias_name_to_real_name_.find( name ) != map_store_connection_alias_name_to_real_name_.end() ) {

      std::string real_name = is_real_name ? name: map_store_connection_alias_name_to_real_name_[ name ];

      //std::queue<Store::StoreConnectionSharedPtr> &queue = map_store_connection_[ real_name ];
      QueueStoreConnectionSharedPtr &queue = map_name_to_queue_store_connection_[ real_name ];

      if ( queue.size() > 0 ) {

        result = queue.front();

        result->borrowed_ = true;

        queue.pop();

      }

    }

  }
  catch ( const std::exception &ex ) {

    hloge( "Exception: %s", ex.what() );

    std::cout << "Exception: " << ex.what() << std::endl;

  }

  return result;

}

bool StoreConnectionManager::return_leased_store_connection( StoreConnectionSharedPtr &store_connection ) {

  bool result { false };

  try {

    if ( store_connection ) {

      std::lock_guard<std::mutex> lock_guard( mutex_ );

      const std::string name = store_connection->name();

      if ( map_name_to_queue_store_connection_.find( name ) != map_name_to_queue_store_connection_.end() &&
           store_connection->borrowed_ ) {

        QueueStoreConnectionSharedPtr &queue = map_name_to_queue_store_connection_[ name ];

        store_connection->borrowed_ = false;

        queue.push( store_connection );

      }

    }

  }
  catch ( const std::exception &ex ) {

    hloge( "Exception: %s", ex.what() );

    std::cout << "Exception: " << ex.what() << std::endl;

  }

  return result;

}

StoreSQLConnectionTransactionPtr StoreConnectionManager::transaction_by_id( const std::string& id ) {

  StoreSQLConnectionTransactionPtr result { nullptr };

  try {

    std::lock_guard<std::mutex> lock_guard( mutex_ );

    if ( map_id_to_store_sql_connection_transaction_.find( id ) != map_id_to_store_sql_connection_transaction_.end() ) {

      result = map_id_to_store_sql_connection_transaction_[ id ];

    }

  }
  catch ( const std::exception &ex ) {

    hloge( "Exception: %s", ex.what() );

    std::cout << "Exception: " << ex.what() << std::endl;

  }

  return result;

}

bool StoreConnectionManager::register_transaction_to_id( const std::string& id,
                                                         StoreSQLConnectionTransactionPtr &store_sql_connection_transaction ) {

  bool result { false };

  try {

    std::lock_guard<std::mutex> lock_guard( mutex_ );

    if ( map_id_to_store_sql_connection_transaction_.find( id ) == map_id_to_store_sql_connection_transaction_.end() ) {

      map_id_to_store_sql_connection_transaction_[ id ] = store_sql_connection_transaction;

      result = true;

    }

  }
  catch ( const std::exception &ex ) {

    hloge( "Exception: %s", ex.what() );

    std::cout << "Exception: " << ex.what() << std::endl;

  }

  return result;

}

bool StoreConnectionManager::unregister_transaction_by_id( const std::string& id ) {

  bool result { false };

  try {

    std::lock_guard<std::mutex> lock_guard( mutex_ );

    if ( map_id_to_store_sql_connection_transaction_.find( id ) != map_id_to_store_sql_connection_transaction_.end() ) {

      map_id_to_store_sql_connection_transaction_.erase( id );

      result = true;

    }

  }
  catch ( const std::exception &ex ) {

    hloge( "Exception: %s", ex.what() );

    std::cout << "Exception: " << ex.what() << std::endl;

  }

  return result;

}

StoreConnectionSharedPtr StoreConnectionManager::store_connection_by_id( const std::string& id ) {

  StoreConnectionSharedPtr result { nullptr };

  try {

    std::lock_guard<std::mutex> lock_guard( mutex_ );

    if ( map_id_to_store_connection_.find( id ) != map_id_to_store_connection_.end() ) {

      result = map_id_to_store_connection_[ id ];

    }

  }
  catch ( const std::exception &ex ) {

    hloge( "Exception: %s", ex.what() );

    std::cout << "Exception: " << ex.what() << std::endl;

  }

  return result;

}

bool StoreConnectionManager::register_store_connection_to_id( const std::string& id,
                                                              StoreConnectionSharedPtr &store_connection ) {

  bool result { false };

  try {

    std::lock_guard<std::mutex> lock_guard( mutex_ );

    if ( map_id_to_store_connection_.find( id ) == map_id_to_store_connection_.end() ) {

      map_id_to_store_connection_[ id ] = store_connection;

      result = true;

    }

  }
  catch ( const std::exception &ex ) {

    hloge( "Exception: %s", ex.what() );

    std::cout << "Exception: " << ex.what() << std::endl;

  }

  return result;

}

bool StoreConnectionManager::unregister_store_connection_by_id( const std::string& id ) {

  bool result { false };

  try {

    std::lock_guard<std::mutex> lock_guard( mutex_ );

    if ( map_id_to_store_connection_.find( id ) != map_id_to_store_connection_.end() ) {

      map_id_to_store_connection_.erase( id );

      result = true;

    }

  }
  catch ( const std::exception &ex ) {

    hloge( "Exception: %s", ex.what() );

    std::cout << "Exception: " << ex.what() << std::endl;

  }

  return result;

}

}
