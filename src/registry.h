// An implementation of the Registry pattern, which allows derived classes of a
// common super class to be constructed by their string class name. All
// subclasses must share a common constructor in order to be constructed by this
// implementation of the registry pattern.
//
// This file provided templated Registry classes that can be used for base
// classes with 0-4 constructor parameters. If more parameters are needed,
// follow the pattern to make a new REGISTER_SUBCLASS# macro.
//
// Example base class:
//
// class Base {
//  public:
//   explicit Base(std::ostream& os) : os_(os) {}
//   virtual void Print() = 0;
//  protected:
//   std::ostream& os_;
// };
// 
// Then, derived subclasses should call the REGISTER_SUBCLASS macro after their
// class definition. For example:
//
// class Derived1 : public Base {
//  public:
//   explicit Derived1(std::ostream& os) : Base(os) {}
//   virtual void Print() { os_ << "Derived1" << std::endl; }
// };
// REGISTER_SUBCLASS(Derived1, Base, std::ostream&)
//
// Then, when an instance of one of the subclasses is desired, it can be
// constructed as:
//
// Registry<Base, std::ostream&>::Create("Derived1", std::cout);
//
// If instead, Derived1 should be constructed when the string "foo" is passed to
// Create(), then the REGISTER_SUBCLASS_W_IDENTIFIER macro should be called. For
// example,
//
// REGISTER_SUBCLASS_W_IDENTIFIER(Derived1, Base, foo, std::ostream&)
//
// NOTE: If the base and subclasses are going to be declared and linked in a
// library (not directly with the executable), then the linker must be forced
// to include all of the subclasses, since their headers will never be
// explicitly included. On Linux this is done with:
//
// -Wl,--whole-archive -lmylib -Wl,--no-whole-archive
//
// On Max OS X, this is done with
//
// -Wl,-force_load -lmylib
//
// See CMakeLists.txt in examples/ for a CMake example.
//
// Author: Philip Salvaggio

#ifndef REGISTRY_H
#define REGISTRY_H

#include <functional>
#include <iostream>
#include <string>
#include <unordered_map>

namespace ofxMTVideoInput {

// Utility macros
#define REGISTRY_SPACE
#define REGISTRY_CONCAT(...) __VA_ARGS__
#define REGISTRY_GET_MACRO(_0, _1, _2, _3, _4, NAME, ...) NAME

#define REGISTER_SUBCLASS_W_IDENTIFIER(Base, Derived, Identifier, ...) \
    REGISTRY_GET_MACRO(_0, ##__VA_ARGS__, \
        REGISTER_SUBCLASS4, \
        REGISTER_SUBCLASS3, \
        REGISTER_SUBCLASS2, \
        REGISTER_SUBCLASS1, \
        REGISTER_SUBCLASS0)(Base, Derived, #Identifier, ##__VA_ARGS__)

#define REGISTER_SUBCLASS(Base, Derived, ...) \
    REGISTER_SUBCLASS_W_IDENTIFIER(Base, Derived, Derived, ##__VA_ARGS__)

#define REGISTRY_CTOR(Base, Derived, params, param_names) \
  [](params) -> Base* { return new Derived(param_names); }
    
#define REGISTER_SUBCLASS0(Base, Derived, Identifier) \
  static bool _registered_##Derived = \
      ofxMTVideoInput::Registry<Base>::Register(Identifier, \
          REGISTRY_CTOR(Base, Derived,,));

#define REGISTER_SUBCLASS1(Base, Derived, Identifier, dtype1) \
  static bool _registered_##Derived = \
      ofxMTVideoInput::Registry<Base, dtype1>::Register(Identifier, \
          REGISTRY_CTOR(Base, Derived, \
              dtype1 REGISTRY_SPACE arg1, arg1));

#define REGISTER_SUBCLASS2(Base, Derived, Identifier, dtype1, dtype2) \
  static bool _registered_##Derived = \
      ofxMTVideoInput::Registry<Base, dtype1, dtype2>::Register(Identifier, \
          REGISTRY_CTOR(Base, Derived, \
              REGISTRY_CONCAT(dtype1 REGISTRY_SPACE arg1, \
                              dtype2 REGISTRY_SPACE arg2), \
              REGISTRY_CONCAT(arg1, arg2)));

#define REGISTER_SUBCLASS3(Base, Derived, Identifier, dtype1, dtype2, dtype3) \
  static bool _registered_##Derived = \
      ofxMTVideoInput::Registry<Base, dtype1, dtype2, dtype3>::Register(Identifier, \
          REGISTRY_CTOR(Base, Derived, \
              REGISTRY_CONCAT(dtype1 REGISTRY_SPACE arg1, \
                              dtype2 REGISTRY_SPACE arg2, \
                              dtype3 REGISTRY_SPACE arg3), \
              REGISTRY_CONCAT(arg1, arg2, arg3)));

#define REGISTER_SUBCLASS4(Base, Derived, Identifier, dtype1, dtype2, dtype3, \
                           dtype4) \
  static bool _registered_##Derived = \
      ofxMTVideoInput::Registry<Base, dtype1, dtype2, dtype3, dtype4>::Register( \
          Identifier, \
          REGISTRY_CTOR(Base, Derived, \
              REGISTRY_CONCAT(dtype1 REGISTRY_SPACE arg1, \
                              dtype2 REGISTRY_SPACE arg2, \
                              dtype3 REGISTRY_SPACE arg3, \
                              dtype4 REGISTRY_SPACE arg4), \
              REGISTRY_CONCAT(arg1, arg2, arg3, arg4)));

template<typename T, typename ... Pack>
class Registry {
 public:
  using ctor_t = std::function<T*(Pack...)>;
  using map_t = std::unordered_map<std::string, ctor_t>;

  template<typename ... Args>
  static T* Create(const std::string& class_name, Args && ... pack) {
    if (ctors().count(class_name) == 1) {
      return ctors()[class_name](std::forward<Args>(pack)...);
    }
    std::cerr << "Class " << class_name << " not registered." << std::endl;
    return nullptr;
  }

  static bool Register(const std::string& class_name, const ctor_t& ctor) {
    ctors()[class_name] = ctor;
    return true;
  }

  static bool IsRegistered(const std::string& class_name) {
    return ctors().count(class_name) == 1;
  }

  static void Unregister(const std::string& class_name) {
    ctors().erase(class_name);
  }

 private:
  static map_t& ctors() {
    static map_t ctor_map;
    return ctor_map;
  }

  Registry();
  Registry(const Registry& other);
  Registry& operator=(const Registry& other);
};

}

#endif  // REGISTRY_H
