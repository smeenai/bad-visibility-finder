// RUN: %bad-visibility-finder %s -- | %FileCheck %s
class __attribute__((__visibility__("default"))) c {
  template <class T> T m();
  void n();
  template <class T> __attribute__((__visibility__("hidden"))) T h();
  template <class T> T g();
  template <class T> __attribute__((__visibility__("default"))) T d();
  template <class T> T i();
};

// CHECK: c::m
template <class T> T c::m() { return T(); }

// CHECK-NOT: c::n
void c::n() {}

// CHECK-NOT: c::h
template <class T> T c::h() { return T(); }

// CHECK-NOT: c::i
template <class T> __attribute__((__visibility__("hidden"))) T c::g() {
  return T();
}

// CHECK-NOT: c::d
template <class T> __attribute__((__visibility__("default"))) T c::d() {
  return T();
}

// CHECK-NOT: t::i
template <class T> inline T c::i() { return T(); }

class __attribute__((__visibility__("hidden"))) hc {
  template <class T> T m();
};

// CHECK-NOT: hc::m
template <class T> T hc::m() { return T(); }

class __attribute__((__type_visibility__("default"))) tdc {
  template <class T> T m();
};

// CHECK-NOT: tdc::m
template <class T> T tdc::m() { return T(); }
