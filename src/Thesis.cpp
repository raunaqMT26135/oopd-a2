// MT26135 - Raunaq Soneja
// Q3b: Thesis implementation
#include "bookmgmt/Thesis.h"

#include <ostream>

namespace bookmgmt {

Thesis::Thesis(std::string id, std::string title,
               std::string author, std::string supervisor,
               std::string university, std::string degree,
               std::string publisher, int year, Money unitPrice)
    : Resource(std::move(id), std::move(title), std::move(publisher), year, unitPrice),
      author_(std::move(author)),
      supervisor_(std::move(supervisor)),
      university_(std::move(university)),
      degree_(std::move(degree)) {}

void Thesis::printDetails(std::ostream& os) const {
    os << "  author: "     << author_     << "\n"
       << "  supervisor: " << supervisor_ << "\n"
       << "  university: " << university_ << "\n"
       << "  degree: "     << degree_     << "\n";
}

}  // namespace bookmgmt
