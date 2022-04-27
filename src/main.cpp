#include <cstdint>
#include <iostream>
#include <string>
#include <vector>

#include "router.h"

int main() {

  // printf("%d\n", argc);
  // for (int i = 0; i < argc; i++) {
  //   printf("%s\n", argv[i]);
  // }

  Router router;

  router.loop();

  return 0;
}
