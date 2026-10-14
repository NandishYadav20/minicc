int main() {
  int i = 0;
  int s = 0;
  while (i < 10) {
    i = i + 1;
    s = s + i;
  }
  return s;
} // expect: 55
