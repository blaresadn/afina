#include "SimpleLRU.h"

namespace Afina {
namespace Backend {

bool SimpleLRU::PrDelete(KeyIt result)
{
    auto ref = result->second;
    free_mem += ref.get().key.size() + ref.get().value.size();
    _lru_index.erase(result);
    if (ref.get().prev == nullptr && ref.get().next == nullptr) {
        _lru_head = nullptr;
        _lru_end = nullptr;
        return true;
    }
    if (ref.get().next == nullptr) {
        _lru_end = ref.get().prev;
        ref.get().prev->next = nullptr;
        return true;
    }
    if (ref.get().prev == nullptr) {
        ref.get().next->prev = nullptr;
        _lru_head = std::move(ref.get().next);
        return true;
    }
    ref.get().next->prev = ref.get().prev;
    ref.get().prev->next = std::move(ref.get().next);
    return true;
}

bool SimpleLRU::PrPut(const std::string &key, const std::string &value)
{
    size_t cur_size = key.size() + value.size();
    if (cur_size > _max_size) {
        return false;
    }
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

// See MapBasedGlobalLockImpl.h
bool SimpleLRU::Put(const std::string &key, const std::string &value)
{
    auto result = _lru_index.find(key);
    if (result != _lru_index.end()) {
        PrDelete(result);
    }
    return PrPut(key, value);
}

// See MapBasedGlobalLockImpl.h
bool SimpleLRU::PutIfAbsent(const std::string &key, const std::string &value)
{
    if (_lru_index.find(key) != _lru_index.end()) {
        return false;
    }
    return PrPut(key, value);
}

// See MapBasedGlobalLockImpl.h
bool SimpleLRU::Set(const std::string &key, const std::string &value)
{
    auto result = _lru_index.find(key);
    if (result == _lru_index.end()) {
        return false;
    }
    PrDelete(result);
    return PrPut(key, value);
}

// See MapBasedGlobalLockImpl.h
bool SimpleLRU::Delete(const std::string &key)
{
    auto result = _lru_index.find(key);
    if (result == _lru_index.end()) {
        return false;
    }
    return PrDelete(result);
}

// See MapBasedGlobalLockImpl.h
bool SimpleLRU::Get(const std::string &key, std::string &value)
{
    auto result = _lru_index.find(key);
    if (result == _lru_index.end()) {
        return false;
    }
    value = (result->second).get().value;
    return true;
}

} // namespace Backend
} // namespace Afina
