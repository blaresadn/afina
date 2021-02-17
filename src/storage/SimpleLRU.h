#ifndef AFINA_STORAGE_SIMPLE_LRU_H
#define AFINA_STORAGE_SIMPLE_LRU_H

#include <map>
#include <memory>
#include <mutex>
#include <string>

#include <afina/Storage.h>

namespace Afina {
    namespace Backend {

/**
 * # Map based implementation
 * That is NOT thread safe implementaiton!!
 */
        class SimpleLRU : public Afina::Storage {
        public:
            SimpleLRU(size_t max_size = 1024) : _max_size(max_size), free_mem(max_size) {}

            ~SimpleLRU() {
                _lru_index.clear();
                if (_lru_head != nullptr) {
                    while (_lru_end->prev != nullptr) {
                        lru_node *cur_node = _lru_end;
                        _lru_end = _lru_end->prev;
                        cur_node->prev->next = nullptr;
                    }
                    _lru_head = nullptr;
                }
            }

            // Implements Afina::Storage interface
            bool Put(const std::string &key, const std::string &value) override;

            // Implements Afina::Storage interface
            bool PutIfAbsent(const std::string &key, const std::string &value) override;

            // Implements Afina::Storage interface
            bool Set(const std::string &key, const std::string &value) override;

            // Implements Afina::Storage interface
            bool Delete(const std::string &key) override;

            // Implements Afina::Storage interface
            bool Get(const std::string &key, std::string &value) override;

        private:
            // LRU cache node
            using lru_node = struct lru_node {
                const std::string key;
                std::string value;
                lru_node *prev;
                std::unique_ptr<lru_node> next;

                lru_node(const std::string &key): key(key) {}
            };

            using backend_type = std::map<std::reference_wrapper<const std::string>, std::reference_wrapper<lru_node>>;
            using backend_iterator = typename backend_type::iterator;

            // Maximum number of bytes could be stored in this cache.
            // i.e all (keys+values) must be not greater than the _max_size
            std::size_t _max_size;
            std::size_t free_mem;

            // Main storage of lru_nodes, elements in this list ordered descending by "freshness": in the head
            // element that wasn't used for longest time.
            //
            // List owns all nodes
            std::unique_ptr<lru_node> _lru_head = nullptr;
            lru_node *_lru_end = nullptr;

            // Index of nodes from list above, allows fast random access to elements by lru_node#key
            std::map<std::reference_wrapper<const std::string>, std::reference_wrapper<lru_node>,
                        std::less<std::string>> _lru_index;

            // Private backend methods
            bool _put(const std::string &key, const std::string &value);

            bool _delete(backend_iterator key);

            bool _move_to_head(lru_node *ptr);

            bool _set(lru_node *ptr, const std::string &value);
        };

    } // namespace Backend
} // namespace Afina

#endif // AFINA_STORAGE_SIMPLE_LRU_H