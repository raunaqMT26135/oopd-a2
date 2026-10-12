// MT26135 - Raunaq Soneja
#pragma once
// Q3b: Thesis — university, degree, supervisor.
// Theses are usually free of cost (unitPrice defaults to zero).
// Pricing: inherited (unitPrice × quantity).

#include <string>

#include "bookmgmt/Resource.h"

namespace bookmgmt {

class Thesis : public Resource {
public:
    Thesis(std::string id, std::string title,
           std::string author, std::string supervisor,
           std::string university, std::string degree,
           std::string publisher, int year,
           Money unitPrice = Money{});  // free by default

    const std::string& author()     const { return author_; }
    const std::string& supervisor() const { return supervisor_; }
    const std::string& university() const { return university_; }
    const std::string& degree()     const { return degree_; }

    ResourceCategory category() const override { return ResourceCategory::Thesis; }

protected:
    void printDetails(std::ostream& os) const override;

private:
    std::string author_;
    std::string supervisor_;
    std::string university_;
    std::string degree_;
};

}  // namespace bookmgmt
