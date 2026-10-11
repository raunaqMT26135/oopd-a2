// Q3a: AudioBook implementation
#include "bookmgmt/AudioBook.h"

#include <ostream>
#include <stdexcept>

#include "bookmgmt/Book.h"  // joinAuthors

namespace bookmgmt {

AudioBook::AudioBook(std::string id, std::string title,
                     std::vector<std::string> authors,
                     std::string narrator,
                     int durationMinutes,
                     std::string publisher, int year, Money unitPrice)
    : Resource(std::move(id), std::move(title), std::move(publisher), year, unitPrice),
      authors_(std::move(authors)),
      narrator_(std::move(narrator)),
      durationMinutes_(durationMinutes) {
    if (durationMinutes_ < 1)
        throw std::invalid_argument("durationMinutes must be >= 1");
}

void AudioBook::printDetails(std::ostream& os) const {
    os << "  authors: "           << joinAuthors(authors_) << "\n"
       << "  narrator: "          << narrator_             << "\n"
       << "  duration (minutes): " << durationMinutes_     << "\n";
}

}  // namespace bookmgmt
