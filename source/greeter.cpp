// #include <fmt/format.h>
#include <netlistx/greeter.h>

using namespace netlistx;

NetlistX::NetlistX(std::string _name) : name(std::move(_name)) {}

// std::string NetlistX::greet(LanguageCode lang) const {
//   switch (lang) {
//     default:
//     case LanguageCode::EN:
//       return fmt::format("Hello, {}!", name);
//     case LanguageCode::DE:
//       return fmt::format("Hallo {}!", name);
//     case LanguageCode::ES:
//       return fmt::format("¡Hola {}!", name);
//     case LanguageCode::FR:
//       return fmt::format("Bonjour {}!", name);
//   }
// }
