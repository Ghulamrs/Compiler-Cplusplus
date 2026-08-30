// Six features this subset leaves out that used to be rejected by whatever
// generic rule failed first.  Each was reported as trouble with a bracket or a
// semicolon, which describes where the parser stopped rather than what the
// program was reaching for -- and `a ? 1 : 2` came back as "unknown token",
// which reads as a broken compiler rather than a smaller language.
//
// Every one now earns the sentence the other exclusions get.  TOK_QUESTION,
// TOK_PIPE and TOK_CARET exist for the same reason TOK_RESERVED does: a token
// the parser can name.
int one(int n) { return n; }

int main() {
  int a = 1, b = 2;             // several names on one type
  int t = a ? 1 : 2;            // conditional operator
  int m = a & b;                // bitwise and
  int o = a | b;                // bitwise or
  int x = a ^ b;                // bitwise xor
  int c = (a, b);               // comma operator
  return t + m + o + x + c;
}
