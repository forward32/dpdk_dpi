#include <rte_config.h>
#include <rte_eal.h>
#include <rte_common.h>

int main(int argc, char *argv[]) {
  auto ret = rte_eal_init(argc, argv);
  if (ret < 0) {
    rte_exit(EXIT_FAILURE, "Invalid EAL parameters\n");
  }

  return 0;
}
