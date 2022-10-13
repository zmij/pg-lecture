// src/hello.hpp
#pragma once

#include <string>
#include <string_view>

#include <userver/components/component_list.hpp>

namespace pg_lecture {

enum class UserType {
  kFirstTime,
  kKnown,
};

struct User {
  std::string name;
  UserType type;
};

std::string SayHelloTo(std::string_view name, UserType type);

void AppendHello(userver::components::ComponentList& component_list);

}  // namespace pg_lecture
