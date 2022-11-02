#ifndef LINKEDLIST_HPP
#define LINKEDLIST_HPP

#include <iostream>
#include <functional>
#include <utility>
#include <fmt/format.h>

enum Ord {
  Asc,
  Des
};

template<typename T>
struct Node {
  T value;
  Node * next;
  Node * prev;

  explicit Node(T value)
      : value(std::move(value))
      , next(nullptr)
      , prev(nullptr)
  {}
};

template<typename T>
struct ListBase {
protected:
  size_t size;
  Node<T> * head;
  Node<T> * tail;
public:

  friend std::ostream & operator<<(std::ostream & os, const ListBase<T> & list) {
    Node<T> * current = list.head;
    os << "[ ";
    while (current != nullptr) {
      os << current->value << " ";
      current = current->next;
    }
    os << "]";
    return os;
  }

  Node<T> * for_each_node(std::function<bool(Node<T> *)> closure) {
    Node<T> * current = head;
    while (current != nullptr) {
      if (closure(current)) {
        return current;
      }
      current = current->next;
    }
    return nullptr;
  }

  Node<T> * for_each(std::function<bool(T)> closure) {
    Node<T> * current = head;
    while (current != nullptr) {
      if (closure(current->value)) {
        return current;
      }
      current = current->next;
    }
    return nullptr;
  }

  size_t len() {
    return size;
  }
  
  
  Node<T> * for_each_c(const std::function<bool(T)> closure) const {
    Node<T> * current = head;
    while (current != nullptr) {
      if (closure(current->value)) {
        return current;
      }
      current = current->next;
    }
    return nullptr;
  }

  // Function that receives a lambda that specifies a value to use for comparison
  template <typename V>
  void push_ord(T value, Ord ord, std::function<V(T)> get_value_from_node) {
    auto * node = new Node<T>(value);
    if (head == nullptr) {
      head = node;
      tail = node;
    } else {
      Node<T> * current = head;
      while (current != nullptr) {
        if (
            (Ord::Asc == ord && get_value_from_node(current->value) > get_value_from_node(value)) ||
            (Ord::Des == ord && get_value_from_node(current->value) < get_value_from_node(value))
            ) {
          push_before(node, current);
          if (current == head) {
            head = node;
          }
          size++;
          return;
        }
        current = current->next;
      }
      push_after(node, tail);
      tail = node;
    }
    size++;
  }

  ListBase()
      : head(nullptr)
      , tail(nullptr)
      , size(0)
  {}

  // Construct list from initializer list
  ListBase(std::initializer_list<T> list)
      : head(nullptr)
      , tail(nullptr)
      , size(0)
  {
    for (auto & value : list) {
      push_bk(value);
    }
  }

  // Copy constructor
  ListBase(const ListBase<T> & other)
      : head(nullptr)
      , tail(nullptr)
      , size(0)
  {
    Node<T> * current = other.head;
    while (current != nullptr) {
      auto next_current = current->next;
      push_bk(current->value);
      current = next_current;
    }
  }

  // Move constructor
  ListBase(ListBase<T> && other) noexcept
      : head(other.head)
      , tail(other.tail)
      , size(other.size)
  {
    other.head = nullptr;
    other.tail = nullptr;
    other.size = 0;
  }

  ~ListBase() {
    if (size > 0) {
      Node<T> * current = head;
      while (current != nullptr) {
        auto next_current = current->next;
        delete current;
        current = next_current;
      }
    }
  }

protected:

  void delete_node(Node<T> * node) {
    if (node == head) {
      head = node->next;
    }
    if (node == tail) {
      tail = node->prev;
    }
    if (node->prev != nullptr) {
      node->prev->next = node->next;
    }
    if (node->next != nullptr) {
      node->next->prev = node->prev;
    }
    delete node;
    size--;
  }

  void push_before(Node<T> * node, Node<T> * at) {
    if (at->prev != nullptr) {
      at->prev->next = node;
    }
    node->prev = at->prev;
    node->next = at;
    at->prev = node;
  }

  void push_after(Node<T> * node, Node<T> * at) {
    if (at->next != nullptr) {
      at->next->prev = node;
    }
    node->next = at->next;
    node->prev = at;
    at->next = node;
  }

  void push_ft(T value) {
    auto * node = new Node<T>(value);
    if (head == nullptr) {
      head = node;
      tail = node;
    } else {
      push_before(node, head);
      head = node;
    }
    size++;
  }

  void push_bk(T value) {
    auto * node = new Node<T>(value);
    if (head == nullptr) {
      head = node;
      tail = node;
    } else {
      push_after(node, tail);
      tail = node;
    }
    size++;
  }

};

template<typename T>
struct List: ListBase<T> {

  void push_front(T value) {
    ListBase<T>::push_ft(value);
  }

  void push_back(T value) {
    ListBase<T>::push_bk(value);
  }

  void pop_back() {
    ListBase<T>::delete_node(ListBase<T>::tail);
  }

  void pop_front() {
    ListBase<T>::delete_node(ListBase<T>::tail);
  }

  List() : ListBase<T>() {}

  // Construct list from initializer list
  List(std::initializer_list<T> list)
      : ListBase<T>(list)
  {}
};

template<typename T>
struct FreValue {
  T value;
  size_t fre;

  friend std::ostream & operator<<(std::ostream & os, const FreValue<T> & fre_value) {
    os << "{ " << fre_value.value << " : " << fre_value.fre << " }";
    return os;
  }
};

template<typename T>
struct ListFre : ListBase<FreValue<T>> {
protected:
  using ListBase<FreValue<T>>::ListBase;
  using ListBase<FreValue<T>>::push_ft;
  using ListBase<FreValue<T>>::push_bk;
  using ListBase<FreValue<T>>::head;
  using ListBase<FreValue<T>>::tail;
  using ListBase<FreValue<T>>::size;

public:
  void push(T value) {
    if (head == nullptr) {
      push_ft({value, 1});
      return;
    }

    Node<FreValue<T>> * node = this->for_each([&value](FreValue<T> fre_value) {
      return fre_value.value == value;
    });

    if (node != nullptr) {
      node->value.fre++;
    } else {
      push_bk({value, 1});
    }
  };

  void pop(T value) {
    Node<FreValue<T>> * node = this->for_each([&value](FreValue<T> fre_value) {
      return fre_value.value == value;
    });

    if (node != nullptr) {
      if (node->value.fre > 1) {
        node->value.fre--;
      } else {
        this->delete_node(node);
      }
    }
  }

  ListFre() : ListBase<FreValue<T>>() {}

  ListFre(std::initializer_list<FreValue<T>> list)
      : ListBase<FreValue<T>>()
  {
    for (auto & value : list) {
      push_bk(value);
    }
  }

  ListFre(std::initializer_list<T> list)
      : ListBase<FreValue<T>>()
  {
    for (auto & value : list) {
      push(value);
    }
  }
};

template<typename K, typename V>
struct KeyValue {
  K key;
  V value;

  friend std::ostream & operator<<(std::ostream & os, const KeyValue<K, V> & fre_value) {
    os << "{ " << fre_value.key << " : " << fre_value.value << " }";
    return os;
  }
};

template<typename K, typename V>
struct KeyValueListBase : ListBase<KeyValue<K, V>> {
  using ListBase<KeyValue<K, V>>::push_bk;
  using ListBase<KeyValue<K, V>>::for_each_node;

protected:
  bool pop_k(K key) {
    Node<KeyValue<K, V>> * node = for_each_node([&](Node<KeyValue<K, V>> * key_value) {
      return key_value->value.key == key;
    });

    if (node != nullptr) {
      this->delete_node(node);
      return true;
    }

    return false;
  }

  V * g(K key) {
    Node<KeyValue<K, V>> * node = for_each_node([&](Node<KeyValue<K, V>> * value){
      if (value->value.key == key) {
        return true;
      }
      return false;
    });

    if (node == nullptr) {
      return nullptr;
    } else {
      return &node->value.value;
    }
  }

  bool i_u(K key, V value) {
    Node<KeyValue<K, V>> * node = for_each_node([&](Node<KeyValue<K, V>> * value){
      if (value->value.key == key) {
        return true;
      }
      return false;
    });

    if (node == nullptr) {
      push_bk({key, value});
      return true;
    } else {
      node->value.value = value;
      return false;
    }
  }

  bool u(K key, V value) {
    Node<KeyValue<K, V>> * node = for_each_node([&](Node<KeyValue<K, V>> * value){
      if (value->value.key == key) {
        return true;
      }
      return false;
    });

    if (node == nullptr) {
      return false;
    } else {
      node->value.value = value;
      return true;
    }
  }

  bool i(K key, V value) {
    Node<KeyValue<K, V>> * node = for_each_node([&](Node<KeyValue<K, V>> * value){
      if (value->value.key == key) {
        return true;
      }
      return false;
    });

    if (node == nullptr) {
      push_bk({key, value});
      return true;
    } else {
      return false;
    }
  }

  KeyValueListBase() : ListBase<KeyValue<K, V>>() {}

  KeyValueListBase(std::initializer_list<KeyValue<K, V>> list)
      : ListBase<KeyValue<K, V>>()
  {
    for (auto & value : list) {
      push_bk(value);
    }
  }
};

template<typename K, typename V>
struct KeyValueList : KeyValueListBase<K, V> {
  using ListBase<KeyValue<K, V>>::push_bk;
  using ListBase<KeyValue<K, V>>::for_each_node;

  V * get(K key) {
    return KeyValueListBase<K, V>::g(key);
  }

  bool delete_key(K key) {
    return KeyValueListBase<K, V>::pop_k(key);
  }

  bool insert_update(K key, V value) {
    return KeyValueListBase<K, V>::i_u(key, value);
  }

  bool update(K key, V value) {
    return KeyValueListBase<K, V>::u(key, value);
  }

  bool insert(K key, V value) {
    return KeyValueListBase<K, V>::i(key, value);
  }

  // Copy assignment operator
  KeyValueList & operator=(const KeyValueList & other) {
    if (this != &other) {

      // Reset this
      this->head = nullptr;
      this->tail = nullptr;
      this->size = 0;

      for_each_node([&](Node<KeyValue<K, V>> * node) {
        this->push_bk(node->value);
        return false;
      });
    }
    return *this;
  }

  
  // Compare operator
  bool operator==(const KeyValueList<K, V> & other) const {
    if (this->size != other.size) return false;
    
    Node<KeyValue<K, V>> * current_other = other.head;
    Node<KeyValue<K, V>> * current = this->head;
    
    for(int i=0; i < this->size; i++){
      if (current_other->value.key != current->value.key || 
        current_other->value.value != current->value.value) {
        return false;
      }
      current_other = current_other->next;
      current = current->next;
    };
    
    return true;
  }


  KeyValueList() : KeyValueListBase<K, V>() {}

  KeyValueList(std::initializer_list<KeyValue<K, V>> list)
      : KeyValueListBase<K, V>(list)
  {}
};

template<typename K, typename V>
struct Cortina: KeyValueListBase<K, List<V>> {
  using KeyValueListBase<K, List<V>>::g;
  using KeyValueListBase<K, List<V>>::i_u;
  using KeyValueListBase<K, List<V>>::u;
  using KeyValueListBase<K, List<V>>::i;

  void insert_append(K key, V value) {
    List<V> * list = g(key);
    if (list == nullptr) {
      i(key, {value});
    } else {
      list->push_back(value);
    }
  }

  bool delete_key(K key) {
    return KeyValueListBase<K, List<V>>::pop_k(key);
  }

  void insert(K key, V value) {
    List<V> * list = g(key);
    if (list == nullptr) {
      i(key, {value});
    } else {
      list->push_back(value);
    }
  }

  void append(K key, V value) {
    List<V> * list = g(key);
    if (list == nullptr) {
      i(key, {value});
    } else {
      list->push_back(value);
    }
  }

  Cortina() : KeyValueListBase<K, List<V>>() {}

  Cortina(std::initializer_list<KeyValue<K, List<V>>> list)
      : KeyValueListBase<K, List<V>>()
  {
    for (auto & value : list) {
      i(value.key, value.value);
    }
  }
};

#endif //LINKEDLIST_HPP
