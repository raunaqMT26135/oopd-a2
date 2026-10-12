// MT26135 - Raunaq Soneja
#include "bookmgmt/Acquisition.h"

#include <algorithm>
#include <iomanip>
#include <iostream>
#include <numeric>
#include <ostream>
#include <stdexcept>

#include "bookmgmt/Exceptions.h"

namespace bookmgmt {

AcquisitionManager::AcquisitionManager(Catalog& catalog, Budget& budget,
                                       TaxPolicy* taxPolicy)
    : catalog_(catalog) {
    departments_[""] = {&budget, taxPolicy};
}

void AcquisitionManager::addDepartment(const std::string& name, Budget& budget, TaxPolicy* taxPolicy) {
    departments_[name] = {&budget, taxPolicy};
}

AcquisitionManager::DeptInfo AcquisitionManager::getDept(const std::string& dept) const {
    auto it = departments_.find(dept);
    if (it == departments_.end()) {
        throw std::invalid_argument("unknown department: " + dept);
    }
    return it->second;
}

Money AcquisitionManager::quote(const std::string& id, int quantity) const {
    return catalog_.get(id).costFor(quantity);
}

bool AcquisitionManager::canPurchase(const std::string& id, int quantity,
                                     std::string* reason, const std::string& department) const {
    std::string why;
    DeptInfo d;
    try {
        d = getDept(department);
    } catch (const std::exception& e) {
        if (reason) *reason = e.what();
        return false;
    }
    
    if (const Resource* r = catalog_.find(id)) {
        if (quantity <= 0)
            why = "quantity must be positive";
        else
            why = d.budget->check(r->category(), quantity, r->costFor(quantity), id);
    } else {
        why = "resource not found: " + id;
    }
    if (reason) *reason = why;
    return why.empty();
}

PurchaseRecord& AcquisitionManager::record(const Resource* r, const std::string& id,
                                           int qty, Money cost, Money tax,
                                           bool approved, std::string reason,
                                           const std::string& department) {
    history_.push_back(PurchaseRecord{
        nextOrderNo_++, id, r ? r->title() : std::string("(unknown)"),
        r ? r->category() : ResourceCategory::Book, qty,
        cost, tax, cost + tax, approved, std::move(reason), false, -1, department});
    return history_.back();
}

void AcquisitionManager::checkQuotaWarning(const std::string& dept, ResourceCategory c) const {
    DeptInfo d = getDept(dept);
    auto q = d.budget->quotaFor(c);
    if (!q) return;
    Usage u = d.budget->usageFor(c);

    std::string prefix = "[Q12 WARNING]";
    if (!dept.empty()) prefix += " [" + dept + "]";

    if (q->maxUnits > 0) {
        int pct = u.units * 100 / q->maxUnits;
        if (pct >= 80)
            std::cerr << prefix << " " << categoryName(c)
                      << " unit quota at " << pct << "% ("
                      << u.units << "/" << q->maxUnits << ")\n";
    }
    if (!q->maxSpend.isZero()) {
        std::int64_t pct = u.spent.minorUnits() * 100 / q->maxSpend.minorUnits();
        if (pct >= 80)
            std::cerr << prefix << " " << categoryName(c)
                      << " spend quota at " << pct << "% ("
                      << u.spent << "/" << q->maxSpend << ")\n";
    }
}

const PurchaseRecord& AcquisitionManager::purchase(const std::string& id, int quantity, const std::string& department) {
    DeptInfo d = getDept(department);
    const Resource& r = catalog_.get(id);          
    const Money cost = r.costFor(quantity);         
    d.budget->commit(r.category(), quantity, cost, id); 
    catalog_.addHoldings(id, quantity);
    
    Money tax;
    if (d.taxPolicy) tax = d.taxPolicy->taxOn(r.category(), cost);
    checkQuotaWarning(department, r.category());
    
    return record(&r, id, quantity, cost, tax, true, {}, department);
}

std::vector<PurchaseRecord> AcquisitionManager::processBatch(
    const std::vector<PurchaseRequest>& reqs,
    bool allOrNothing,
    bool sortByPriority) {

    std::vector<std::size_t> order(reqs.size());
    std::iota(order.begin(), order.end(), 0);
    if (sortByPriority)
        std::stable_sort(order.begin(), order.end(),
                         [&reqs](std::size_t a, std::size_t b) {
                             return reqs[a].priority > reqs[b].priority;
                         });

    if (allOrNothing) {
        std::map<std::string, Budget> tempBudgets;
        for (const auto& [name, d] : departments_) {
            tempBudgets.insert({name, *d.budget});
        }
        
        for (std::size_t i : order) {
            const auto& req = reqs[i];
            const Resource* r = catalog_.find(req.resourceId);
            if (!r || req.quantity <= 0) goto rollback;
            
            auto it = tempBudgets.find(req.department);
            if (it == tempBudgets.end()) goto rollback;
            
            try {
                it->second.commit(r->category(), req.quantity, r->costFor(req.quantity), req.resourceId);
            } catch (const std::exception&) {
                goto rollback;
            }
        }
        goto process;
        
    rollback:
        std::vector<PurchaseRecord> results;
        results.reserve(reqs.size());
        for (std::size_t i : order) {
            const auto& req = reqs[i];
            const Resource* r = catalog_.find(req.resourceId);
            results.push_back(record(r, req.resourceId, req.quantity, Money{}, Money{},
                                     false, "batch rejected (all-or-nothing)", req.department));
        }
        return results;
    }

process:
    std::vector<PurchaseRecord> results;
    results.reserve(reqs.size());
    for (std::size_t i : order) {
        const auto& req = reqs[i];
        const Resource* r = catalog_.find(req.resourceId);
        Money cost;
        std::string why;
        DeptInfo d;
        
        try {
            d = getDept(req.department);
        } catch (const std::exception& e) {
            why = e.what();
        }
        
        if (why.empty()) {
            if (!r) {
                why = "resource not found: " + req.resourceId;
            } else if (req.quantity <= 0) {
                why = "quantity must be positive";
            } else {
                cost = r->costFor(req.quantity);
                why = d.budget->check(r->category(), req.quantity, cost, req.resourceId);
            }
        }

        if (why.empty()) {
            results.push_back(purchase(req.resourceId, req.quantity, req.department));
        } else {
            results.push_back(record(r, req.resourceId, req.quantity, cost, Money{},
                                     false, why, req.department));
        }
    }
    return results;
}

const PurchaseRecord& AcquisitionManager::cancelOrder(int orderNo) {
    PurchaseRecord* orig = nullptr;
    for (auto& rec : history_) {
        if (rec.orderNo == orderNo) { orig = &rec; break; }
    }
    if (!orig) throw NotFoundError("order #" + std::to_string(orderNo));
    if (!orig->approved)
        throw std::invalid_argument("order #" + std::to_string(orderNo) + " was not approved");
    if (orig->cancelled)
        throw std::invalid_argument("order #" + std::to_string(orderNo) + " is already cancelled");

    DeptInfo d = getDept(orig->department);
    d.budget->refund(orig->category, orig->quantity, orig->cost, orig->resourceId);
    
    if (catalog_.find(orig->resourceId))
        catalog_.addHoldings(orig->resourceId, -orig->quantity);

    int cancelNo = nextOrderNo_;
    orig->cancelled        = true;
    orig->cancelledByOrder = cancelNo;

    return record(nullptr, orig->resourceId, orig->quantity, orig->cost, orig->tax,
                  false, "cancellation of order #" + std::to_string(orderNo), orig->department);
}

Money AcquisitionManager::totalSpent() const {
    Money sum;
    for (const auto& rec : history_)
        if (rec.approved && !rec.cancelled) sum += rec.cost;
    return sum;
}

Money AcquisitionManager::totalTax() const {
    Money sum;
    for (const auto& rec : history_)
        if (rec.approved && !rec.cancelled) sum += rec.tax;
    return sum;
}

void AcquisitionManager::printReport(std::ostream& os) const {
    os << "Order history (" << history_.size() << " orders)\n";
    for (const auto& rec : history_) {
        std::string status = rec.approved ? "APPROVED" : "REJECTED";
        if (rec.cancelled) status = "CANCELLED";
        os << "  #" << std::setw(3) << std::left << rec.orderNo << " "
           << status << "  " << std::setw(6) << rec.resourceId
           << " x" << std::setw(3) << rec.quantity << " "
           << std::setw(12) << std::right << rec.cost.toString() << std::left << "  "
           << rec.title;
        if (!rec.department.empty())
            os << "  [Dept: " << rec.department << "]";
        if (!rec.tax.isZero())
            os << "  (tax: " << rec.tax << ", total: " << rec.totalCost << ")";
        if (!rec.approved || rec.cancelled)
            os << "\n        reason: " << rec.reason;
        os << "\n";
    }
    os << "Total spent (pre-tax): " << totalSpent() << "\n";
    // If ANY department has tax policy, we can print total tax. Let's just print it if it's > 0.
    if (!totalTax().isZero())
        os << "Total tax:             " << totalTax() << "\n"
           << "Total (incl. tax):     " << (totalSpent() + totalTax()) << "\n";
}

}  // namespace bookmgmt
