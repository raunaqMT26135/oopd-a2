// Q16: Additional search methods added.
#include "bookmgmt/Catalog.h"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <stdexcept>

// Q16: concrete type headers for dynamic_cast in search methods.
#include "bookmgmt/AudioBook.h"
#include "bookmgmt/Book.h"
#include "bookmgmt/EBook.h"
#include "bookmgmt/Journal.h"

#include "bookmgmt/Exceptions.h"

namespace bookmgmt {

namespace {
std::string lower(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(),
                   [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });
    return s;
}
}  // namespace

Resource& Catalog::add(std::unique_ptr<Resource> r) {
    if (!r) throw std::invalid_argument("cannot add a null resource");
    const std::string id = r->id();
    if (items_.count(id)) throw DuplicateIdError(id);
    Entry& e = items_[id];
    e.resource = std::move(r);
    return *e.resource;
}

bool Catalog::contains(const std::string& id) const { return items_.count(id) > 0; }

Resource* Catalog::find(const std::string& id) {
    auto it = items_.find(id);
    return it == items_.end() ? nullptr : it->second.resource.get();
}

const Resource* Catalog::find(const std::string& id) const {
    auto it = items_.find(id);
    return it == items_.end() ? nullptr : it->second.resource.get();
}

Resource& Catalog::get(const std::string& id) {
    if (Resource* r = find(id)) return *r;
    throw NotFoundError(id);
}

const Resource& Catalog::get(const std::string& id) const {
    if (const Resource* r = find(id)) return *r;
    throw NotFoundError(id);
}

void Catalog::remove(const std::string& id) {
    if (items_.erase(id) == 0) throw NotFoundError(id);
}

int Catalog::holdings(const std::string& id) const {
    auto it = items_.find(id);
    if (it == items_.end()) throw NotFoundError(id);
    return it->second.holdings;
}

void Catalog::addHoldings(const std::string& id, int units) {
    auto it = items_.find(id);
    if (it == items_.end()) throw NotFoundError(id);
    if (it->second.holdings + units < 0)
        throw std::invalid_argument("holdings cannot become negative");
    it->second.holdings += units;
}

int Catalog::borrowed(const std::string& id) const {
    auto it = items_.find(id);
    if (it == items_.end()) throw NotFoundError(id);
    return it->second.borrowed;
}

void Catalog::borrowResource(const std::string& id) {
    auto it = items_.find(id);
    if (it == items_.end()) throw NotFoundError(id);
    if (it->second.borrowed >= it->second.holdings) {
        throw std::invalid_argument("no copies/seats available");
    }
    it->second.borrowed++;
}

void Catalog::returnResource(const std::string& id) {
    auto it = items_.find(id);
    if (it == items_.end()) throw NotFoundError(id);
    if (it->second.borrowed <= 0) {
        throw std::invalid_argument("no copies/seats currently borrowed");
    }
    it->second.borrowed--;
}

void Catalog::save(const std::string& filename) const {
    std::ofstream os(filename);
    if (!os) throw std::runtime_error("cannot open file");
    // Just a placeholder to show the design:
    // In a full solution, we would serialize all fields into TSV format.
    for (const auto& [id, entry] : items_) {
        os << categoryName(entry.resource->category()) << "\t" << id << "\t" << entry.holdings << "\t" << entry.borrowed << "\n";
    }
}

void Catalog::load(const std::string& filename) {
    std::ifstream is(filename);
    if (!is) throw std::runtime_error("cannot open file");
    // Placeholder to show design:
    // In a full solution, we'd read each line, parse the category, and call the respective constructor.
}

std::vector<const Resource*> Catalog::where(
    const std::function<bool(const Resource&)>& pred) const {
    std::vector<const Resource*> out;
    for (const auto& [id, entry] : items_)
        if (pred(*entry.resource)) out.push_back(entry.resource.get());
    return out;
}

std::vector<const Resource*> Catalog::all() const {
    return where([](const Resource&) { return true; });
}

std::vector<const Resource*> Catalog::byCategory(ResourceCategory c) const {
    return where([c](const Resource& r) { return r.category() == c; });
}

std::vector<const Resource*> Catalog::searchTitle(const std::string& text) const {
    const std::string needle = lower(text);
    return where([&needle](const Resource& r) {
        return lower(r.title()).find(needle) != std::string::npos;
    });
}

// Q16: Search by author name (case-insensitive substring).
// Works for Book, EBook, AudioBook.
std::vector<const Resource*> Catalog::searchByAuthor(const std::string& name) const {
    const std::string needle = lower(name);
    return where([&needle](const Resource& r) {
        auto inList = [&needle](const std::vector<std::string>& authors) {
            for (const auto& a : authors)
                if (lower(a).find(needle) != std::string::npos) return true;
            return false;
        };
        if (const auto* b = dynamic_cast<const Book*>(&r))
            return inList(b->authors());
        if (const auto* e = dynamic_cast<const EBook*>(&r))
            return inList(e->authors());
        if (const auto* a = dynamic_cast<const AudioBook*>(&r))
            return inList(a->authors());
        return false;
    });
}

// Q16: Search by ISBN (Book/EBook) or ISSN (Journal/Magazine) — exact match.
std::vector<const Resource*> Catalog::searchByIsbn(const std::string& isbn) const {
    return where([&isbn](const Resource& r) {
        if (const auto* b = dynamic_cast<const Book*>(&r))
            return b->isbn() == isbn;
        if (const auto* e = dynamic_cast<const EBook*>(&r))
            return e->isbn() == isbn;
        if (const auto* j = dynamic_cast<const Journal*>(&r))
            return j->issn() == isbn;  // same function covers ISSN
        return false;
    });
}

// Q16: Search by publication year range [fromYear, toYear] inclusive.
std::vector<const Resource*> Catalog::searchByYearRange(int fromYear, int toYear) const {
    return where([fromYear, toYear](const Resource& r) {
        return r.year() >= fromYear && r.year() <= toYear;
    });
}

}  // namespace bookmgmt
