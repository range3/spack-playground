#include <hello_header_only/hello_header_only.hpp>
#include <memory>

class ClassName {};

auto main() -> int {
  hello_header_only::doubleHello("Konnitiwa!");
  return 0;
}
