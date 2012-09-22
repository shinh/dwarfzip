#include <dwarf.h>

#include <vector>

using namespace std;

static vector<const char*> g_dw_at_str;
static vector<const char*> g_dw_form_str;

#define DEFINE_DW_AT(x) do {                    \
    if (g_dw_at_str.size() <= x)                \
      g_dw_at_str.resize(x + 1);                \
    g_dw_at_str[x] = #x;                        \
  } while (0)

#define DEFINE_DW_AT_GNU(x,v) do {              \
    if (g_dw_at_str.size() <= v)                \
      g_dw_at_str.resize(v + 1);                \
    g_dw_at_str[v] = #x;                        \
  } while (0)

#define DEFINE_DW_FORM(x) do {                    \
    if (g_dw_form_str.size() <= x)                \
      g_dw_form_str.resize(x + 1);                \
    g_dw_form_str[x] = #x;                        \
  } while (0)

void initDwarfStr() {
#include "dwarf.tab"
}

const char* DW_AT_STR(int value) {
  if (value < 0 || value >= (int)g_dw_at_str.size())
    return "***ERROR***";
  return g_dw_at_str[value];
}

const char* DW_FORM_STR(int value) {
  if (value < 0 || value >= (int)g_dw_form_str.size())
    return "***ERROR***";
  return g_dw_form_str[value];
}
