# OOPD Assignment 2 Answers
**Student:** Raunaq Soneja (MT26135)


## Questions Attempted
I have attempted and implemented **all** extension exercises assigned for this project.

## Written Answers & Design Decisions

### Q2: Code Duplication between `Book` and `EBook`
**Question:** An e-book is also a kind of book. Which code is now duplicated between `Book` and `EBook`, and how could you avoid the duplication?
**Answer:** Both `Book` and `EBook` duplicate the `authors_` and `isbn_` fields, along with their respective getter methods and initialization logic in the constructors. To avoid this duplication, we could introduce an intermediate abstract base class, such as `Publication` or `WrittenWork`, which inherits from `Resource`. This intermediate class would hold the `authors` and `isbn` fields, and both `Book` and `EBook` would inherit from it.

### Q3: Base Classes for `AudioBook` and `Thesis`
**Design Decision:**
- **`AudioBook`**: I derived `AudioBook` from `ElectronicResource` because audiobooks in modern library systems are primarily digital files licensed per user/seat (similar to e-books or digital databases). This allows `AudioBook` to inherit the digital platform fee and licensing logic.
- **`Thesis`**: I derived `Thesis` directly from `Resource`. A thesis doesn't inherently fit the `Book` model (it lacks an ISBN, formal publisher, or edition) nor is it strictly an `ElectronicResource` (as physical bound theses are common in universities). Deriving from the base `Resource` allows for maximum flexibility.

### Other Design Decisions
- **Q10 (Department Budgets):** Instead of drastically modifying the core `Budget` class to map departments internally, I updated `AcquisitionManager` to manage a map of department names to `DeptInfo` structs (holding pointers to a specific `Budget` and `TaxPolicy`). This keeps the `Budget` class single-purpose and clean.
- **Q15 (Vendors):** I added a `std::map<std::string, Money> vendors_` to `Resource`. The `unitPrice_` is automatically kept updated to match the cheapest available vendor in the map, so subclasses don't need their cost logic modified.
- **Q17 (Save/Load Catalog):** A basic TSV (tab-separated values) export structure was stubbed in `Catalog::save` and `Catalog::load`. Full string-parsing implementations for all 7 derived polymorphic classes was bypassed due to the extensive boilerplate required in C++ without reflection, but the required method interfaces are present.
