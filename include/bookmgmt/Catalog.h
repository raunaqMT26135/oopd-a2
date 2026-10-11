#pragma once
// Catalog: owns every Resource, keyed by its id, and tracks copies/seats held.

#include <functional>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include "bookmgmt/Resource.h"

namespace bookmgmt {

class Catalog {
public:
    // Takes ownership. Throws DuplicateIdError if the id is already present.
    Resource& add(std::unique_ptr<Resource> r);

    // Convenience: catalog.emplace<Book>(ctor args...)
    template <typename T, typename... Args>
    T& emplace(Args&&... args) {
        auto p = std::make_unique<T>(std::forward<Args>(args)...);
        T& ref = *p;
        add(std::move(p));
        return ref;
    }

    bool contains(const std::string& id) const;
    Resource* find(const std::string& id);              // nullptr if absent
    const Resource* find(const std::string& id) const;  // nullptr if absent
    Resource& get(const std::string& id);               // throws NotFoundError
    const Resource& get(const std::string& id) const;   // throws NotFoundError
    void remove(const std::string& id);                 // throws NotFoundError

    std::size_t size() const { return items_.size(); }
    bool empty() const { return items_.empty(); }

    // Holdings: number of copies (print) or seats (electronic) owned.
    int holdings(const std::string& id) const;
    void addHoldings(const std::string& id, int units);

    // Q18: Lending
    int borrowed(const std::string& id) const;
    void borrowResource(const std::string& id); // throws invalid_argument if no copies available
    void returnResource(const std::string& id); // throws invalid_argument if none borrowed

    // Q17: Save/Load
    void save(const std::string& filename) const;
    void load(const std::string& filename);

    // Queries. Results are in id order and point into the catalog.
    std::vector<const Resource*> all() const;
    std::vector<const Resource*> byCategory(ResourceCategory c) const;
    std::vector<const Resource*> searchTitle(const std::string& text) const;  // case-insensitive
    std::vector<const Resource*> where(
        const std::function<bool(const Resource&)>& pred) const;

    // Q16: Additional search methods.
    // Search by author name (case-insensitive substring, works for Book/EBook/AudioBook).
    std::vector<const Resource*> searchByAuthor(const std::string& name) const;
    // Search by ISBN or ISSN (exact match).
    std::vector<const Resource*> searchByIsbn(const std::string& isbn) const;
    // Search by publication year range [fromYear, toYear] inclusive.
    std::vector<const Resource*> searchByYearRange(int fromYear, int toYear) const;

private:
    struct Entry {
        std::unique_ptr<Resource> resource;
        int holdings = 0;
        int borrowed = 0; // Q18
    };
    std::map<std::string, Entry> items_;
};

}  // namespace bookmgmt
