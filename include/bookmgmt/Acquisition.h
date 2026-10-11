#pragma once
// AcquisitionManager: turns purchase requests into orders, enforcing the
// Budget's quotas, updating Catalog holdings and keeping an order history.
//
// Q7:  Optional TaxPolicy; records store pre-tax cost and tax separately.
// Q8:  Budget tracks number of distinct titles bought per category.
// Q9:  cancelOrder() refunds budget and reduces holdings.
// Q10: Optional per-department Budget+TaxPolicy support.
// Q12: Emits warnings when a category crosses 80% of its quota.
// Q13: processBatch supports all-or-nothing mode.
// Q14: PurchaseRequest carries a priority field.

#include <iosfwd>
#include <optional>
#include <string>
#include <vector>

#include "bookmgmt/Budget.h"
#include "bookmgmt/Catalog.h"
#include "bookmgmt/TaxPolicy.h"

namespace bookmgmt {

// Q14: Priority — higher number = processed first.
struct PurchaseRequest {
    std::string resourceId;
    int quantity;
    int priority = 0;
    std::string department; // Q10
};

struct PurchaseRecord {
    int orderNo;
    std::string resourceId;
    std::string title;
    ResourceCategory category;
    int quantity;
    Money cost;
    Money tax;
    Money totalCost;
    bool approved;
    std::string reason;
    bool cancelled = false;
    int cancelledByOrder = -1;
    std::string department; // Q10
};

class AcquisitionManager {
public:
    AcquisitionManager(Catalog& catalog, Budget& budget,
                       TaxPolicy* taxPolicy = nullptr);

    // Q10: Add a department's budget
    void addDepartment(const std::string& name, Budget& budget, TaxPolicy* taxPolicy = nullptr);

    Money quote(const std::string& id, int quantity) const;

    bool canPurchase(const std::string& id, int quantity,
                     std::string* reason = nullptr,
                     const std::string& department = "") const;

    const PurchaseRecord& purchase(const std::string& id, int quantity,
                                   const std::string& department = "");

    std::vector<PurchaseRecord> processBatch(
        const std::vector<PurchaseRequest>& reqs,
        bool allOrNothing   = false,
        bool sortByPriority = false);

    const PurchaseRecord& cancelOrder(int orderNo);

    const std::vector<PurchaseRecord>& history() const { return history_; }
    Money totalSpent() const;
    Money totalTax()   const;

    void printReport(std::ostream& os) const;

private:
    struct DeptInfo {
        Budget* budget;
        TaxPolicy* taxPolicy;
    };
    DeptInfo getDept(const std::string& dept) const;

    PurchaseRecord& record(const Resource* r, const std::string& id, int qty,
                           Money cost, Money tax, bool approved, std::string reason,
                           const std::string& department);

    void checkQuotaWarning(const std::string& dept, ResourceCategory c) const;

    Catalog& catalog_;
    std::map<std::string, DeptInfo> departments_;
    std::vector<PurchaseRecord> history_;
    int nextOrderNo_ = 1;
};

}  // namespace bookmgmt
