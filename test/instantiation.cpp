// RUN: %bad-visibility-finder %s -- | %FileCheck %s
template <class T> class c;
template <class T> class c {
  template <class U> U f();
  template <class U> __attribute__((__visibility__("hidden"))) U g();
  template <class U> U h();
  template <class U> U i();
};

template <class T> template <class U> U c<T>::f() { return U(); }

template <class T> template <class U> U c<T>::g() { return U(); }

// CHECK: c::f at [[PATH:.*]]instantiation.cpp:[[@LINE-4]] (declaration at [[PATH]]instantiation.cpp:4) (from specialization at [[PATH]]instantiation.cpp:[[@LINE+2]])
// CHECK-NOT: c::g
extern template class __attribute__((__visibility__("default"))) c<int>;

// CHECK: c::h at [[PATH]]instantiation.cpp:[[@LINE+1]] (declaration at [[PATH]]instantiation.cpp:6) (from specialization at [[PATH]]instantiation.cpp:[[@LINE-2]])
template <class T> template <class U> U c<T>::h() { return U(); }

// CHECK: c::i
template <class T>
template <class U>
__attribute__((__visibility__("default"))) U c<T>::i() {
  return T();
}

// we don't want to print the same methods multiple times
// CHECK-NOT: c::f
// CHECK-NOT: c::h
extern template class __attribute__((__visibility__("default"))) c<char>;

template <class T> class __attribute__((__type_visibility__("default"))) d {
  template <class U> U f();
};

// CHECK-NOT: d::f
extern template class __attribute__((__visibility__("hidden"))) d<int>;

// CHECK-NOT: d::f
extern template class d<char>;
