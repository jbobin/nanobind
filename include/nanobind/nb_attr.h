NAMESPACE_BEGIN(NB_NAMESPACE)

struct scope {
    PyObject *value;
    NB_INLINE scope(handle value) : value(value.ptr()) { }
};

struct name {
    const char *value;
    NB_INLINE name(const char *value) : value(value) { }
};

struct arg_v;
struct arg {
    NB_INLINE constexpr explicit arg(const char *name = nullptr) : name(name), convert_(true), none_(false) { }
    template <typename T> NB_INLINE arg_v operator=(T &&value) const;
    NB_INLINE arg &noconvert(bool value = true) { convert_ = !value; return *this; }
    NB_INLINE arg &none(bool value = true) { none_ = value; return *this; }

    const char *name;
    bool convert_;
    bool none_;
};

struct arg_v : arg {
    object value;
    NB_INLINE arg_v(const arg &base, object &&value) : arg(base), value(std::move(value)) { }
};

struct is_method { };

NAMESPACE_BEGIN(literals)
constexpr arg operator"" _a(const char *name, size_t) { return arg(name); }
NAMESPACE_END(literals)

NAMESPACE_BEGIN(detail)

enum class func_flags : uint16_t {
    /* Low 3 bits reserved for return value policy */

    /// Did the user specify a name for this function, or is it anonymous?
    has_name       = (1 << 4),
    /// Did the user specify a scope where this function should be installed?
    has_scope      = (1 << 5),
    /// Did the user specify a docstring?
    has_doc        = (1 << 6),
    /// Did the user specify nb::arg/arg_v annotations for all arguments?
    has_args       = (1 << 7),
    /// Does the function signature contain an *args-style argument?
    has_var_args   = (1 << 8),
    /// Does the function signature contain an *kwargs-style argument?
    has_var_kwargs = (1 << 9),
    /// Is this function a class method?
    is_method      = (1 << 10),
    /// Is this function a method called __init__? (automatically generated)
    is_constructor = (1 << 11),
    /// When the function is GCed, do we need to call func_data::free?
    has_free       = (1 << 12),
    /// Should the func_new() call return a new reference?
    return_ref     = (1 << 13)
};

struct arg_data {
    const char *name;
    PyObject *name_py;
    PyObject *value;
    bool convert;
    bool none;
};

template <size_t Size> struct func_data {
    // A small amount of space to capture data used by the function/closure
    void *capture[3];

    // Callback to clean up the 'capture' field
    void (*free)(void *);

    /// Implementation of the function call
    PyObject *(*impl)(void *, PyObject **, bool *, rv_policy, PyObject *);

    /// Function signature description
    const char *descr;

    /// C++ types referenced by 'descr'
    const std::type_info **descr_types;

    /// Total number of function call arguments
    uint16_t nargs;

    /// Supplementary flags
    uint16_t flags;

    // ------- Extra fields -------

    const char *name;
    const char *doc;
    PyObject *scope;
    arg_data args[Size];
};

template <typename F>
NB_INLINE void func_extra_apply(F &f, const name &name, size_t &) {
    f.name = name.value;
    f.flags |= (uint16_t) func_flags::has_name;
}

template <typename F>
NB_INLINE void func_extra_apply(F &f, const scope &scope, size_t &) {
    f.scope = scope.value;
    f.flags |= (uint16_t) func_flags::has_scope;
}

template <typename F>
NB_INLINE void func_extra_apply(F &f, const char *doc, size_t &) {
    f.doc = doc;
    f.flags |= (uint16_t) func_flags::has_doc;
}

template <typename F> NB_INLINE void func_extra_apply(F &f, is_method, size_t &) {
    f.flags |= (uint16_t) func_flags::is_method;
}

template <typename F> NB_INLINE void func_extra_apply(F &f, rv_policy pol, size_t &) {
    f.flags = (f.flags & ~0b11) | (uint16_t) pol;
}

template <typename F> NB_INLINE void func_extra_apply(F &f, const arg &a, size_t &index) {
    arg_data &arg = f.args[index++];
    arg.name = a.name;
    arg.value = nullptr;
    arg.convert = a.convert_;
    arg.none = a.none_;
}

template <typename F> NB_INLINE void func_extra_apply(F &f, const arg_v &a, size_t &index) {
    arg_data &arg = f.args[index++];
    arg.name = a.name;
    arg.value = a.value.ptr();
    arg.convert = a.convert_;
    arg.none = a.none_;
}

NAMESPACE_END(detail)
NAMESPACE_END(NB_NAMESPACE)