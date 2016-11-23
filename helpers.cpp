#include "helpers.h"

int pow(int x, int p)
{
  if (p == 0) return 1;
  if (p == 1) return x;

  int tmp = pow(x, p/2);
  if (p%2 == 0) return tmp * tmp;
  else return x * tmp * tmp;
}
