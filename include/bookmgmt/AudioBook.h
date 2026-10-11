#pragma once
// Q3a: AudioBook — narrator and duration in minutes.
// Pricing: inherited from Resource (unitPrice × quantity).

#include <string>
#include <vector>

#include "bookmgmt/Resource.h"

namespace bookmgmt {

class AudioBook : public Resource {
public:
    AudioBook(std::string id, std::string title,
              std::vector<std::string> authors,
              std::string narrator,
              int durationMinutes,
              std::string publisher, int year, Money unitPrice);

    const std::vector<std::string>& authors()         const { return authors_; }
    const std::string&              narrator()         const { return narrator_; }
    int                             durationMinutes()  const { return durationMinutes_; }

    ResourceCategory category() const override { return ResourceCategory::AudioBook; }

protected:
    void printDetails(std::ostream& os) const override;

private:
    std::vector<std::string> authors_;
    std::string narrator_;
    int durationMinutes_;
};

}  // namespace bookmgmt
