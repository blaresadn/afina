#include "SimpleLRU.h"

namespace Afina {
    namespace Backend {

        bool SimpleLRU::_delete(backend_iterator it)
        {
            auto *ptr = &(it->second).get();
            free_mem += ptr->key.size() + ptr->value.size();
            _lru_index.erase(it);
            if (ptr->prev == nullptr && ptr->next == nullptr) {
                _lru_head = nullptr;
                _lru_end = nullptr;
                return true;
            }
            if (ptr->next == nullptr) {
                _lru_end = ptr->prev;
                ptr->prev->next = nullptr;
                return true;
            }
            if (ptr->prev == nullptr) {
                ptr->next->prev = nullptr;
                _lru_head = std::move(ptr->next);
                return true;
            }
            ptr->next->prev = ptr->prev;
            ptr->prev->next = std::move(ptr->next);
            return true;
        }

        bool SimpleLRU::_put(const std::string &key, const std::string &value)
        {
            size_t cur_size = key.size() + value.size();
            while (free_mem < cur_size) {
                Delete(_lru_end->key);
            }
            free_mem -= cur_size;
            auto new_node = new lru_node;
            new_node->key = key;
            new_node->value = value;
            new_node->prev = nullptr;
            if (_lru_head != nullptr) {
                _lru_head->prev = new_node;
            }
            new_node->next = std::move(_lru_head);
            _lru_index.insert({std::reference_wrapper<const std::string>(new_node->key),
                               std::reference_wrapper<lru_node>(*new_node)});
            _lru_head = std::unique_ptr<lru_node>(new_node);
            if (new_node->next == nullptr) {
                _lru_end = new_node;
            }
            return true;
        }

        bool SimpleLRU::_move_to_head(lru_node *ptr)
        {
            if (ptr->prev != nullptr) {
                _lru_head->prev = ptr;
                if (ptr->next != nullptr) {
                    ptr->next->prev = ptr->prev;
                } else {
                    _lru_end = ptr->prev;
                }
                auto tmp = std::move(_lru_head);
                _lru_head = std::move(ptr->prev->next);
                ptr->prev->next = std::move(ptr->next);
                ptr->next = std::move(tmp);
                ptr->prev = nullptr;
            }
            return true;
        }

        bool SimpleLRU::_set(lru_node *ptr, const std::string &value)
        {
            _move_to_head(ptr);
            while (free_mem + ptr->value.size() < value.size()) {
                Delete(_lru_end->key);
            }
            free_mem -= value.size();
            ptr->value = value;
            return true;
        }

// See MapBasedGlobalLockImpl.h
        bool SimpleLRU::Put(const std::string &key, const std::string &value)
        {
            if (key.size() + value.size() > _max_size) {
                return false;
            }
            auto result = _lru_index.find(key);
            if (result != _lru_index.end()) {
                return _set(&(result->second).get());
            }
            return _put(key, value);
        }

// See MapBasedGlobalLockImpl.h
        bool SimpleLRU::PutIfAbsent(const std::string &key, const std::string &value)
        {
            if (key.size() + value.size() > _max_size) {
                return false;
            }
            if (_lru_index.find(key) != _lru_index.end()) {
                return false;
            }
            return _put(key, value);
        }

// See MapBasedGlobalLockImpl.h
        bool SimpleLRU::Set(const std::string &key, const std::string &value)
        {
            if (key.size() + value.size() > _max_size) {
                return false;
            }
            auto result = _lru_index.find(key);
            if (result == _lru_index.end()) {
                return false;
            }
            auto *ptr = &(result->second).get();
            return _set(ptr, value);
        }

// See MapBasedGlobalLockImpl.h
        bool SimpleLRU::Delete(const std::string &key)
        {
            auto result = _lru_index.find(key);
            if (result == _lru_index.end()) {
                return false;
            }
            return _delete(result);
        }

// See MapBasedGlobalLockImpl.h
        bool SimpleLRU::Get(const std::string &key, std::string &value)
        {
            auto result = _lru_index.find(key);
            if (result == _lru_index.end()) {
                return false;
            }
            auto ptr = &(result->second).get();
            value = ptr->value;
            return _move_to_head(ptr);
        }

    } // namespace Backend
} // namespace Afina