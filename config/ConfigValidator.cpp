#include "ConfigValidator.hpp"

//   1. Chaque server block a un listen (port != kPortNotSet)
//   2. Port dans [1-65535]
//   3. Host valide (format IP ou "localhost")
//   4. Pas de doublons host:port entre server blocks
//   5. Codes error_page valides (quelle range exactement ? 4xx-5xx ou autre
//   chose ?)

ConfigValidator::ConfigValidator() {
}

ConfigValidator::~ConfigValidator() {
}
