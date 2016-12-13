// RUN: %bad-visibility-finder %s -- | %FileCheck %s
template <class T> class c {
  template <class U> U f();
  template <class U> U g();
};

template <class T> template <class U> U c<T>::f() { return U(); }

// CHECK: c::f (from specialization at
extern template class __attribute__((__visibility__("default"))) c<int>;

// CHECK: c::g (from specialization at
template <class T> template <class U> U c<T>::g() { return U(); }

// we don't want to print the same methods multiple times
// CHECK-NOT: c::f
// CHECK-NOT: c::g
extern template class __attribute__((__visibility__("default"))) c<char>;
