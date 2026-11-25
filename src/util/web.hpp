#pragma once

#include "Comments.hpp"

#include <Geode/Geode.hpp>

namespace cw::modcmmts {
    namespace srv {
        struct PageData final {
            uint64_t page = 1;
            uint64_t pageSize = 15;
            uint64_t totalItems = 0;
            uint64_t totalPages = 0;
        };

        template <class T>
        struct ListResponse final {
            std::vector<T> items;
            PageData pagination;
        };

        using CommentListResponse = ListResponse<Comment>;
    };

    namespace request {
        geode::utils::web::WebRequest withAuth(std::string token);
        arc::Future<geode::utils::web::WebRequest> withAuthCo(std::string token);
    };
};

template <>
struct matjson::Serialize<cw::modcmmts::srv::PageData> final {
    static geode::Result<cw::modcmmts::srv::PageData> fromJson(matjson::Value const& value);
    static matjson::Value toJson(cw::modcmmts::srv::PageData const& value);
};

template <typename T>
struct matjson::Serialize<cw::modcmmts::srv::ListResponse<T>> final {
    static geode::Result<cw::modcmmts::srv::ListResponse<T>> fromJson(matjson::Value const& value) {
        cw::modcmmts::srv::ListResponse<T> out{};

        GEODE_UNWRAP_INTO(auto const _items, value["items"].asArray());
        out.items.reserve(_items.size());

        for (auto const& [k, value] : _items) {
            GEODE_UNWRAP_INTO(auto dev, value.as<T>());
            out.items.push_back(std::move(dev));
        };

        GEODE_UNWRAP_INTO(out.pagination, value["pagination"].as<cw::modcmmts::srv::PageData>());

        return geode::Ok(std::move(out));
    }

    static matjson::Value toJson(cw::modcmmts::srv::ListResponse<T> const& value) {
        matjson::Value obj;
        obj["items"] = value.items;
        obj["pagination"] = value.pagination;

        return obj;
    }
};