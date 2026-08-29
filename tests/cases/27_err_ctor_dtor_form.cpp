class P {
public:
  int x;
  P() { return 5; }
  virtual P(int a) : x(a) { }
  ~P(int a) { }
  ~P() { }
};
