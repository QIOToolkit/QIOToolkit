
#pragma once

#include <assert.h>

#include <map>
#include <string>
#include <unordered_map>
#include <vector>

#include "utils/exception.h"
#include "utils/log.h"

namespace utils
{
class Structure
{
 public:
  enum Type
  {
    UNKNOWN_TYPE,
    BOOL,
    INT32,
    UINT32,
    INT64,
    UINT64,
    DOUBLE,
    STRING,
    ARRAY,
    OBJECT
  };

  Structure() : uint64_(0), type_(UNKNOWN_TYPE) {}
  Structure(Type type) : Structure() { type_ = type; }
  ~Structure() { clear(); }
  void remove(const std::string& key);

  // Constructor
  Structure(const Structure& other);
  Structure& operator=(const Structure& other);

  template <class T>
  Structure(const T& value) : Structure()
  {
    this->set_value(value);
  }

  void check_reset(Structure::Type type)
  {
    if (type_ != type)
    {
      clear();
      type_ = type;
    }
  }

  void reset(Structure::Type type)
  {
    clear();
    type_ = type;
  }

  // Assignment operator
  template <class T>
  Structure& operator=(const T& value)
  {
    clear();
    this->set_value(value);
    return *this;
  }

  // traversal
  Structure& operator[](size_t index)
  {
    check_reset(ARRAY);
    if (array_ == nullptr)
    {
      array_ = new std::vector<Structure>();
    }
    if (array_->size() <= index)
    {
      array_->resize(index + 1);
    }
    return (*array_)[index];
  }
  Structure& operator[](int32_t index)
  {
    if (index >= 0) return operator[]((size_t)index);
    THROW(utils::IndexOutOfRangeException, "Accessing out of bounds ", index,
          " of [0..", array_ == nullptr ? 0 : array_->size(), ").");
  }
  const Structure& operator[](size_t index) const
  {
    check_array();
    if (array_ != nullptr && index < array_->size()) return (*array_)[index];
    THROW(utils::IndexOutOfRangeException, "Accessing out of bounds ", index,
          " of [0..", array_ == nullptr ? 0 : array_->size(), ").");
  }
  void check_array() const
  {
    if (type_ != ARRAY)
    {
      THROW(utils::AccessAsIncorrectTypeException,
            "Access a non array object by index");
    }
  }
  const Structure& operator[](int32_t index) const
  {
    check_array();
    if (index >= 0) return operator[]((size_t)index);
    THROW(utils::IndexOutOfRangeException, "Accessing out of bounds ", index,
          " of [0..", array_ == nullptr ? 0 : array_->size(), ").");
  }

  Structure& operator[](const std::string& key)
  {
    check_reset(OBJECT);
    if (object_ == nullptr)
    {
      object_ = new std::map<std::string, Structure>();
    }
    if (object_->find(key) == object_->end())
    {
      (*object_)[key] = Structure();
    }
    return (*object_)[key];
  }

  void check_object() const
  {
    if (type_ != OBJECT)
    {
      THROW(utils::AccessAsIncorrectTypeException,
            "Access a non OBJECT object by key");
    }
  }

  const Structure& operator[](const std::string& key) const
  {
    check_object();
    if (object_ != nullptr)
    {
      auto found = object_->find(key);
      if (found != object_->end()) return found->second;
    }

    THROW(utils::KeyDoesNotExistException, "Accessing unset key ", key);
  }

  Structure& get_extension(const std::string& name)
  {
    const Structure* found = find_extension(name);
    if (found == nullptr)
    {
      Structure extension;
      extension["name"] = name;
      extension["value"] = Structure();
      auto& extensions = operator[]("extensions");
      extensions.push_back(extension);
      found = &extensions[extensions.get_array_size() - 1];
    }
    return (*const_cast<Structure*>(found))["value"];
  }

  // introspection
  Type get_type() const { return type_; }

  bool is_empty() const
  {
    return type_ == UNKNOWN_TYPE ||
           (type_ == ARRAY && (array_ == nullptr || array_->empty())) ||
           (type_ == OBJECT && (object_ == nullptr || object_->empty()));
  }

  bool has_key(const std::string& key) const
  {
    if (type_ != OBJECT)
    {
      return false;
    }
    return object_ != nullptr && object_->find(key) != object_->end();
  }

  bool has_extension(const std::string& name) const
  {
    return find_extension(name) != nullptr;
  }

  size_t get_array_size() const
  {
    check_array();
    if (array_ == nullptr)
    {
      return 0;
    }
    return array_->size();
  }

  std::vector<std::string> get_object_keys() const
  {
    check_object();
    std::vector<std::string> keys;
    if (object_ == nullptr)
    {
      return keys;
    }
    for (const auto& it : *object_)
    {
      keys.push_back(it.first);
    }
    return keys;
  }

  std::vector<std::string> get_extension_names() const
  {
    std::vector<std::string> names;
    check_object();
    if (object_ == nullptr)
    {
      return names;
    }
    const auto extensions = object_->find("extensions");
    if (extensions != object_->end())
    {
      for (const auto& extension : *(extensions->second.array_))
      {
        if (!extension.has_key("name"))
        {
          LOG(WARN, "extension has no name: " + extension.to_string());
          continue;
        }
        const auto& name_field = extension.object_->at("name");
        if (name_field.type_ != STRING)
        {
          LOG(WARN, "extension name is not a string: " + extension.to_string());
          continue;
        }
        names.push_back(*name_field.string_);
      }
    }
    return names;
  }

  // content access
  template <class T>
  T get() const
  {
    THROW(utils::NotImplementedException,
          "unimplemented: ", __FUNCTION_SIGNATURE__);
  }

  // content modification
  void clear()
  {
    if (type_ == STRING)
    {
      delete string_;
    }
    else if (type_ == ARRAY)
    {
      delete array_;
    }
    else if (type_ == OBJECT)
    {
      delete object_;
    }
    uint64_ = 0;
    type_ = UNKNOWN_TYPE;
  }

  template <class T>
  void push_back(const T& value)
  {
    check_reset(ARRAY);
    if (array_ == nullptr)
    {
      array_ = new std::vector<Structure>();
    }
    array_->push_back(value);
  }

  template <class T>
  void append(const std::vector<T>& values)
  {
    check_reset(ARRAY);
    if (array_ == nullptr)
    {
      array_ = new std::vector<Structure>();
    }
    for (const T& value : values)
    {
      array_->push_back(value);
    }
  }

  template <class T>
  void prepend(const std::vector<T>& values)
  {
    check_reset(ARRAY);
    if (array_ == nullptr)
    {
      array_ = new std::vector<Structure>();
    }
    array_->insert(array_->begin(), values.begin(), values.end());
  }

  template <class T>
  void set_extension(const std::string& name, const T& value)
  {
    get_extension(name) = value;
  }

  // render as string
  std::string to_string(bool pretty = true) const;

  bool operator<(const Structure& rhs) const;

 protected:
  const Structure* find_extension(const std::string& name) const
  {
    check_object();
    if (object_ == nullptr)
    {
      return nullptr;
    }
    if (!has_key("extensions")) return nullptr;
    const auto& extensions = *(object_->at("extensions").array_);
    for (const auto& extension : extensions)
    {
      if (!extension.has_key("name"))
      {
        LOG(WARN, "extension has no name: " + extension.to_string());
        continue;
      }
      const auto& name_field = extension.object_->at("name");
      if (name_field.type_ != STRING)
      {
        LOG(WARN, "extension name is not a string: " + extension.to_string());
        continue;
      }
      if ((*name_field.string_) == name)
      {
        return &extension;
      }
    }
    return nullptr;
  }

  void set_value(bool value)
  {
    reset(BOOL);
    bool_ = value;
  }

  void set_value(int32_t value)
  {
    reset(INT32);
    int32_ = value;
  }

  void set_value(uint32_t value)
  {
    reset(UINT32);
    uint32_ = value;
  }

  void set_value(int64_t value)
  {
    reset(INT64);
    int64_ = value;
  }

  void set_value(uint64_t value)
  {
    reset(UINT64);
    uint64_ = value;
  }

  void set_value(float value)
  {
    reset(DOUBLE);
    double_ = value;
  }

  void set_value(double value)
  {
    reset(DOUBLE);
    double_ = value;
  }

  void set_value(const char value[])
  {
    reset(STRING);
    assert(string_ == nullptr);
    string_ = new std::string(value);
  }

  void set_value(const std::string& value)
  {
    reset(STRING);
    assert(string_ == nullptr);
    string_ = new std::string(value);
  }

  void set_value(const Structure& value) { *this = value; }

  template <class T>
  void set_value(const std::vector<T>& array)
  {
    reset(ARRAY);
    if (array_ == nullptr)
    {
      array_ = new std::vector<Structure>();
    }
    for (const auto& item : array)
    {
      push_back(Structure(item));
    }
  }

  template <class T>
  void set_value(const std::map<std::string, T>& object)
  {
    reset(OBJECT);
    if (object_ == nullptr)
    {
      object_ = new std::map<std::string, Structure>();
    }
    for (const auto& entry : object)
    {
      (*object_)[entry.first] = entry.second;
    }
  }

  template <class T>
  void set_value(const std::unordered_map<std::string, T>& object)
  {
    reset(OBJECT);
    if (object_ == nullptr)
    {
      object_ = new std::map<std::string, Structure>();
    }
    for (const auto& entry : object)
    {
      (*object_)[entry.first] = entry.second;
    }
  }

  template <class T>
  void set_value(const T& component)
  {
    *this = component.render();
  }

  bool is_simple() const
  {
    if (type_ == OBJECT)
    {
      if (object_ == nullptr) return true;
      if (object_->size() > 3) return false;
      for (const auto& it : *object_)
      {
        if (it.second.type_ == OBJECT || !it.second.is_simple())
        {
          return false;
        }
      }
      return true;
    }
    else if (type_ == ARRAY)
    {
      if (array_ == nullptr) return true;
      if (array_->size() > 10) return false;
      for (const auto& item : *array_)
      {
        if (item.type_ == OBJECT || item.type_ == ARRAY)
        {
          return false;
        }
      }
      return true;
    }
    return true;
  }

  union
  {
    bool bool_;
    int32_t int32_;
    uint32_t uint32_;
    int64_t int64_;
    uint64_t uint64_;
    double double_;
    std::string* string_;
    std::vector<Structure>* array_;
    std::map<std::string, Structure>* object_;
    std::unordered_map<std::string, Structure>* hash_map_object_;
  };

  Type type_;

  static std::string type2string(Type type);
  std::string to_string(bool pretty, int level) const;

 private:
  // Doing deep copy value from other to this
  // this object must has been cleaned.
  void deep_copy(const Structure& other);
};

template <>
bool Structure::get<bool>() const;
template <>
int32_t Structure::get<int32_t>() const;
template <>
uint32_t Structure::get<uint32_t>() const;
template <>
int64_t Structure::get<int64_t>() const;
template <>
uint64_t Structure::get<uint64_t>() const;
template <>
double Structure::get<double>() const;
template <>
std::string Structure::get<std::string>() const;

std::ostream& operator<<(std::ostream& out, const Structure& s);

}  // namespace utils
