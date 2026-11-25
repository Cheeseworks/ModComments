#include "../web.hpp"

#include <cw.h>

#include <rhatt/prelude.h>

#include <Geode/Geode.hpp>

using namespace geode::prelude;
using namespace cw::modcmmts;

Result<srv::PageData> matjson::Serialize<srv::PageData>::fromJson(matjson::Value const& value) {
    srv::PageData out;

    GEODE_UNWRAP_INTO(out.page, value["page"].asUInt());
    GEODE_UNWRAP_INTO(out.pageSize, value["page_size"].asUInt());
    GEODE_UNWRAP_INTO(out.totalItems, value["total_items"].asUInt());
    GEODE_UNWRAP_INTO(out.totalPages, value["total_pages"].asUInt());

    return Ok(out);
}

matjson::Value matjson::Serialize<srv::PageData>::toJson(srv::PageData const& value) {
    Value out;
    out["page"] = value.page;
    out["page_size"] = value.pageSize;
    out["total_items"] = value.totalItems;
    out["total_pages"] = value.totalPages;

    return out;
}

web::WebRequest request::withAuth(std::string token) {
    auto acc = argon::getGameAccountData();

    return cw::request::base()
        .param("account_id", acc.accountId)
        .param("authtoken", std::move(token))
        .param("user_id", acc.userId)
        .param("username", std::move(acc.username));
}

arc::Future<web::WebRequest> request::withAuthCo(std::string token) {
    co_return *co_await async::waitForMainThread<web::WebRequest>([t = std::move(token)]() mutable {
        return withAuth(std::move(t));
    });
}