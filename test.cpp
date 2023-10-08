#include <iostream>
#include <cstring>

using namespace std;
 
int main() {
  string res;
  char buffer[64] = "hello world";
  copy(buffer, buffer + strlen(buffer), back_insert_iterator(res));
  cout << res << '\n';

  return 0;
}