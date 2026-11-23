// SPDX-License-Identifier: GPL-3.0-or-later
#include <unity.h>

#include <cstring>

#include "since_core.h"

void setUp() {}
void tearDown() {}

static void test_core_version_is_set() {
  TEST_ASSERT_NOT_NULL(since::coreVersion());
  TEST_ASSERT_TRUE(std::strlen(since::coreVersion()) > 0);
}

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_core_version_is_set);
  return UNITY_END();
}
