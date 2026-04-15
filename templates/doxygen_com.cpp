
// note, not all fields are mandatory ! use what you need, keep it simple
// doxygen documents the interface contract (what, why, for the caller)
// not the implementation choices (how) prefer the use of inline comments inside
// the function body for that

/**
 * @brief One-line summary (mandatory, always first).
 *
 * Optional longer description. Can span multiple lines.
 *
 * @param name Description of the parameter
 * @param[in] name Input-only parameter
 * @param[out] name Output-only parameter - rarely used in C++98, we use refs
 * @param[in,out] name Both read and modified - rarely used in C++98, we use
 * refs
 *
 * @return Description of the return value
 *
 * @throws std::runtime_error If some condition is met
 *
 * @note Something worth knowing but not critical
 * @warning Something that could cause unexpected behavior
 *
 * @see OtherClass::otherMethod()
 */
