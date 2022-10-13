// src/hello.cpp

#include "hello.hpp"

#include <fmt/format.h>
#include <sstream>

#include <userver/clients/dns/component.hpp>
#include <userver/server/handlers/http_handler_base.hpp>
#include <userver/storages/postgres/cluster.hpp>
#include <userver/storages/postgres/component.hpp>
#include <userver/storages/postgres/io/enum_types.hpp>
#include <userver/utils/assert.hpp>

namespace userver::storages::postgres::io {

template <>
struct CppToUserPg<pg_lecture::UserType>
    : EnumMappingBase<pg_lecture::UserType> {
  static constexpr DBTypeName postgres_name = "hello_schema.user_type";
  static constexpr Enumerator enumerators[]{
      {EnumType::kFirstTime, "first_time"}, {EnumType::kKnown, "known"}};
};

template <>
struct CppToUserPg<pg_lecture::User> {
  static constexpr DBTypeName postgres_name = "hello_schema.user";
};

}  // namespace userver::storages::postgres::io

namespace pg_lecture {

namespace {

const std::string kSelectTop10 = R"~(
SELECT name, count
FROM hello_schema.users
ORDER by count DESC, name ASC
LIMIT 10
)~";

const std::string kInsertUser = R"~(
INSERT INTO hello_schema.users(name, count) VALUES($1, 1)
ON CONFLICT (name)
DO UPDATE SET count = users.count + 1
RETURNING users.count
)~";

const std::string kInsertUserV2 = R"~(
INSERT INTO hello_schema.users(name, count) VALUES($1, 1)
ON CONFLICT (name)
DO UPDATE SET count = users.count + 1
RETURNING CASE 
    WHEN users.count > 1 THEN 'known'::hello_schema.user_type
	ELSE 'first_time'::hello_schema.user_type
END CASE AS 'type' 
)~";

struct UserData {
  std::string name;
  std::int32_t count;
};

namespace pg = userver::storages::postgres;

class Top10 final : public userver::server::handlers::HttpHandlerBase {
 public:
  static constexpr std::string_view kName = "handler-top";

  Top10(const userver::components::ComponentConfig& config,
        const userver::components::ComponentContext& component_context)
      : HttpHandlerBase(config, component_context),
        pg_cluster_{
            component_context
                .FindComponent<userver::components::Postgres>("pg-lecture")
                .GetCluster()} {}

  std::string HandleRequestThrow(
      const userver::server::http::HttpRequest&,
      userver::server::request::RequestContext&) const override {
    std::ostringstream res;
    auto result =
        pg_cluster_->Execute(pg::ClusterHostType::kSlave, kSelectTop10);

    for (auto user : result.AsSetOf<UserData>(pg::kRowTag)) {
      res << user.name << " " << user.count << "\n";
    }
    return res.str();
  }

  // ...
 private:
  userver::storages::postgres::ClusterPtr pg_cluster_;
};

class Hello final : public userver::server::handlers::HttpHandlerBase {
 public:
  static constexpr std::string_view kName = "handler-hello";

  Hello(const userver::components::ComponentConfig& config,
        const userver::components::ComponentContext& component_context)
      : HttpHandlerBase(config, component_context),
        pg_cluster_(
            component_context
                .FindComponent<userver::components::Postgres>("pg-lecture")
                .GetCluster()) {}

  std::string HandleRequestThrow(
      const userver::server::http::HttpRequest& request,
      userver::server::request::RequestContext&) const override {
    const auto& name = request.GetArg("name");

    auto user_type = UserType::kFirstTime;
    if (!name.empty()) {
      auto tran = pg_cluster_->Begin(pg::Transaction::RW);
      auto result = tran.Execute(kInsertUser, name);

      if (result.AsSingleRow<std::int32_t>() > 1) {
        user_type = UserType::kKnown;
      }
      tran.Commit();
    }

    return pg_lecture::SayHelloTo(name, user_type);
  }

 private:
  userver::storages::postgres::ClusterPtr pg_cluster_;
};

class HelloV2 final : public userver::server::handlers::HttpHandlerBase {
 public:
  static constexpr std::string_view kName = "handler-hello-v2";

  HelloV2(const userver::components::ComponentConfig& config,
          const userver::components::ComponentContext& component_context)
      : HttpHandlerBase(config, component_context),
        pg_cluster_(
            component_context
                .FindComponent<userver::components::Postgres>("pg-lecture")
                .GetCluster()) {}

  std::string HandleRequestThrow(
      const userver::server::http::HttpRequest& request,
      userver::server::request::RequestContext&) const override {
    const auto& name = request.GetArg("name");

    auto user_type = UserType::kFirstTime;
    if (!name.empty()) {
      auto result = pg_cluster_->Execute(pg::ClusterHostType::kMaster,
                                         kInsertUserV2, name);
      user_type = result.AsSingleRow<UserType>();
    }

    return pg_lecture::SayHelloTo(name, user_type);
  }

 private:
  userver::storages::postgres::ClusterPtr pg_cluster_;
};

}  // namespace

std::string SayHelloTo(std::string_view name, UserType type) {
  if (name.empty()) {
    name = "unknown user";
  }

  switch (type) {
    case UserType::kFirstTime:
      return fmt::format("Hello, {}!\n", name);
    case UserType::kKnown:
      return fmt::format("Hi again, {}!\n", name);
  }

  UASSERT(false);
}

//! @file hello.cpp
void AppendHello(userver::components::ComponentList& component_list) {
  component_list.Append<Top10>();
  component_list.Append<Hello>();
  component_list.Append<HelloV2>();
  component_list.Append<userver::clients::dns::Component>();
  component_list.Append<userver::components::Postgres>("pg-lecture");
}

}  // namespace pg_lecture
