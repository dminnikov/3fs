#pragma once

#include <list>
#include <folly/container/F14Map.h>

namespace folly {

template <class K, class V, class H = std::hash<K>, class E = std::equal_to<V>>
class OrderedMap {
 public:
  using key_type = const K;
  using mapped_type = V;
  using value_type = std::pair<key_type, mapped_type>;
  using list_type = std::list<value_type>;
  using size_type = typename list_type::size_type;
  using iterator = typename list_type::iterator;
  using const_iterator = typename list_type::const_iterator;

 public:
  OrderedMap() = default;
  OrderedMap(const OrderedMap& o) { *this = o; }
  OrderedMap(OrderedMap&& o) { *this = std::move(o); }

  OrderedMap& operator=(const OrderedMap& o) {
    clear();
    for (auto& [key, value] : o) {
      emplace(key, value);
    }
    return *this;
  }
  OrderedMap& operator=(OrderedMap&& o) {
    order_ = std::move(o.order_);
    used_ = std::move(o.used_);
    return *this;
  }
  bool operator==(const OrderedMap& o) const {
    for (auto& [key, a] : used_) {
      auto b = o.find(key);
      if (b == o.end()) {
        return false;
      }
      if (a->second != b->second) {
        return false;
      }
    }
    return size() == o.size();
  }

  iterator begin() { return order_.begin(); }
  const_iterator begin() const { return order_.begin(); }
  iterator end() { return order_.end(); }
  const_iterator end() const { return order_.end(); }
  bool empty() const { return order_.empty(); }
  size_type size() const { return order_.size(); }
  value_type& front() { return order_.front(); }
  const value_type& front() const { return order_.front(); }
  value_type& back() { return order_.back(); }
  const value_type& back() const { return order_.back(); }
  void clear() { used_.clear(), order_.clear(); }

  iterator find(key_type& key) {
    auto it = used_.find(key);
    if (it == used_.end()) {
      return end();
    }
    return it->second;
  }
  const_iterator find(key_type& key) const {
    auto it = used_.find(key);
    if (it == used_.end()) {
      return end();
    }
    return it->second;
  }

  size_type erase(key_type& key) {
    auto it = used_.find(key);
    if (it != used_.end()) {
      erase(it->second);
      return 1;
    }
    return 0;
  }
  iterator erase(const_iterator it) {
    used_.erase(it->first);
    return order_.erase(it);
  }

  iterator erase(const_iterator first, const_iterator last) {
    iterator ret = end();
    for (auto it = first; it != last; ++it) {
      used_.erase(it->first);
      ret = order_.erase(it);
    }
    return ret;
  }

  template <class T>
  std::pair<iterator, bool> emplace(key_type& key, T&& value) {
    auto [it, succ] = used_.emplace(key, iterator{});
    if (succ) {
      order_.emplace_back(key, std::forward<T>(value));
      it->second = --order_.end();
    }
    return std::make_pair(it->second, succ);
  }

  template <class T>
  std::pair<iterator, bool> try_emplace(key_type& key, T&& value) {
    auto [it, succ] = used_.try_emplace(key, iterator{});
    if (succ) {
      order_.emplace_back(key, std::forward<T>(value));
      it->second = --order_.end();
    }
    return std::make_pair(it->second, succ);
  }

  mapped_type& operator[](key_type& key) {
    return emplace(key, mapped_type{}).first->second;
  }

  void reserve(size_type capacity) { used_.reserve(capacity); }

 private:
  list_type order_;
  F14NodeMap<key_type, iterator, H, E> used_;
};

} // namespace folly
