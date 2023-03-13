//#undef FORCE_NO_SINGLE_VISITOR
//#undef FORCE_SINGLE_VISITOR_GENERATION
#include <iostream>

// Very diry and undocumented... scroll down for example

// Proposal for a general Visitor implementation using visitor pattern.
// Usable for configuration or metadata with mixed type and nested containers.
//
// Visitor pattern is useful for efficient interfacing using dynamic polymorphism.
//
// The example uses templates to generate any Visitors and Visitables with different virtual methods.
// A similar approach can be used to describe an Editor for making changes on data.
//
// Basically a Visitor supports different overloads of a function `handle(TYPE)`. 
// Via templates the type can be customized to a specific set of types that should be supported dynamically at run time.
//
// An disadvantage of the visitor pattern is, that in application a lot of overloads have to be defined. However,
// by forming appropriate subgroups and using virtual inheritance. It is easy to provide a lot of more flexible options.
// By doing so, the implementing *Visitable* class has to provide more than one `visit(Visitor)`, 
// i.e. `visit(NumericValueVisitor)`, `visit(IntegralValueVisitor)`, `visit(FloatingValueVisitor)`
// 
// Although this is a lot of more effort in the first instance, it simplifies usage. 
// Usually the `Visitable` is implemented once but accessing with different `Visitors` is happening much more often. 
// Hence it should be worth the afford and it makes the Visitor much more usable.
//
// ## Alternatives & performance
// 
// > Note: Most optimal visitors would be using templates and lambdas to statically compile down optimized code... 
// >       however thats no option for interfaces.
//
// The opposite of using a visitor would be providing a lot of getters and a function returning a type tag. 
// Using a switch statement, the appropriate code may be called. 
// 
// ```
// // Example...
// switch(data.getType()) {
//  case Types::Int: 
//     return data.getInt();
//  case Types::Float: 
//     return data.getFloat();
// ...
// }
// ```
// This gives a lot of flexibilty. Compared to a visitor, it is easy to skip types you are not iterested in...
// From a performance point of handle it should perform similar to a visitor. Depending on the implementation,
// in all cases usually two function calls (getType() and get...()) and a switch needs to be performed.
// For visitors a double-dispatch is performed, first when calling `visit(Visitor)`.  
// Then the viewable may perform a switch to determine the type of the value - or if implemented properly the `visit(Visitor)` is already specialized, 
// and at last the virtual member on the Visitor is called `visitor.handle(value)`.
//
// Concerning the memory layout, explicit implementations with getters usually result in a large vtable. 
// In contrast with visitors, the viewable needs to provide just one `visit(visitor)` and might support more 
// specilized visitores for convenience. Yet the resulting memory used to store the virtual information is smaller, 
// because a lot of the logic is moved to the seperated Visitor type.
//
// Even when using a lot of specialized visitores with virtual inheritance, 
// the vtable should be smaller as with getters and might only have the same 
// size when there are as many visitores as types/getters (then the visitores serve as complicated getters).
//
// C++11 and C++17 verision provided. C++17 version is simpler and less code...



// https://stackoverflow.com/questions/81870/is-it-possible-to-print-a-variables-type-in-standard-c
#include <type_traits>
#include <typeinfo>
#ifndef _MSC_VER
#   include <cxxabi.h>
#endif
#include <memory>
#include <string>
#include <cstdlib>

// Helper do print out typeinformation...found somewhere on stackoverflow
template <class T>
std::string
type_name()
{
    typedef typename std::remove_reference<T>::type TR;
    std::unique_ptr<char, void(*)(void*)> own
           (
#ifndef _MSC_VER
                abi::__cxa_demangle(typeid(TR).name(), nullptr,
                                           nullptr, nullptr),
#else
                nullptr,
#endif
                std::free
           );
    std::string r = own != nullptr ? own.get() : typeid(TR).name();
    if (std::is_const<TR>::value)
        r += " const";
    if (std::is_volatile<TR>::value)
        r += " volatile";
    if (std::is_lvalue_reference<T>::value)
        r += "&";
    else if (std::is_rvalue_reference<T>::value)
        r += "&&";
    return r;
}




#if __cplusplus >= 201703L
template<typename T>
using decay_t = typename std::decay_t<T>;
template<bool T, typename A, typename B>
using conditional_t = typename std::conditional_t<T,A,B>;
#else
template<typename T>
using decay_t = typename std::decay<T>::type;
template<bool T, typename A, typename B>
using conditional_t = typename std::conditional<T,A,B>::type;
#endif


#if __cplusplus >= 201703L
template<template<typename P1, typename P2> class Predicate, typename A, typename ... TS>
struct is_one_of { static const bool value = (Predicate<A,TS>::value || ...); };

template<template<typename P1, typename P2> class Predicate, typename A, typename ... TS>
inline constexpr bool is_one_of_v = is_one_of<Predicate, A,TS...>::value;
#else
template<typename A, typename B>
struct is_same {
    using type = std::false_type;
    static const bool value = false; 
};

template<typename A>
struct is_same<A,A> {
    using type = std::true_type;
    static const bool value = true; 
};

template<template<typename P1, typename P2> class Predicate, typename A, typename ... TS>
struct is_one_of;

template<template<typename P1, typename P2> class Predicate, typename A, typename B>
struct is_one_of<Predicate, A, B> { static const bool value = Predicate<A,B>::value; };

template<template<typename P1, typename P2> class Predicate, typename A, typename B, typename C, typename ... TS>
struct is_one_of<Predicate, A, B, C, TS...> { static const bool value = Predicate<A,B>::value || is_one_of<Predicate,A,C,TS...>::value; };

template<template<typename P1, typename P2> class Predicate, typename A, typename ... TS>
inline constexpr bool is_one_of_v = is_one_of<Predicate, A,TS...>::value;
#endif

// ----------------------------------------------------------------------------
// type_list and helper template to concat type_lists, union and intersect
// ----------------------------------------------------------------------------
template<typename ...T>
struct type_list {};


// Filter a typelist (TList) to contain only unique values. The second argument is a recursion helper.
template<typename TList, typename Acc = type_list<>>
struct unique_type_list;

template<typename T, typename Acc = type_list<>>
using unique_type_list_t = typename unique_type_list<T, Acc>::type;

// End of recursion, return the filtered type_list
template<typename ...Filtered>
struct unique_type_list<type_list<>, type_list<Filtered...>>  {
    using type = type_list<Filtered...>;
};

// Recursion worker... check if T1 is already contained in Filtered and either ignore or append
template<typename T1, typename ...Ts, typename ...Filtered>
struct unique_type_list<type_list<T1, Ts...>, type_list<Filtered...>>  {
    using type = unique_type_list_t<type_list<Ts...>, conditional_t<is_one_of_v<std::is_same, T1, Filtered...>,type_list<Filtered...>, type_list<Filtered..., T1>>>;
};



template<typename ...T>
struct concat_type_list;

template<typename ...T>
using concat_type_list_t = typename concat_type_list<T...>::type;


// End of concatenation
template<typename ...T1>
struct concat_type_list<type_list<T1...>>  {
    using type = type_list<T1...>;
};

// Concat two lists
template<typename ...T1, typename ...T2 , typename ...T>
struct concat_type_list<type_list<T1...>, type_list<T2...>, T...>  {
    using type = concat_type_list_t<type_list<T1..., T2...>, T...>;
};

// Concat two list with arbitrary type
template<typename ...T1, typename T2 , typename ...T>
struct concat_type_list<type_list<T1...>, T2, T...>  {
    using type = concat_type_list_t<type_list<T1..., T2>, T...>;
};

// Start: Shift any type to
template<typename T1, typename ...T>
struct concat_type_list<T1, T...>  {
    using type = concat_type_list_t<type_list<T1>, T...>;
};

template<typename ...T>
using union_type_list_t = unique_type_list_t<concat_type_list_t<T...>>;



template<typename ...T>
struct intersect_type_list;

template<typename ...T>
using intersect_type_list_t = typename intersect_type_list<T...>::type;

// End of intersection
template<typename ...T1>
struct intersect_type_list<type_list<T1...>>  {
    using type = type_list<T1...>;
};

// Intersect two lists
template<typename ...T1, typename ...T2 , typename ...T>
struct intersect_type_list<type_list<T1...>, type_list<T2...>, T...>  {
    using type = intersect_type_list_t<union_type_list_t<conditional_t<is_one_of_v<std::is_same, T1, T2...>, type_list<T1>, type_list<>>...>, T...>;
};


// ----------------------------------------------------------------------------
// Teplate definitions for an arbitrary Visitor and Visitable
// ----------------------------------------------------------------------------

// SingleVisitor has been used to generate abstract virtual methods. 
// However they leave a footprint as for each of them a vtable pointer will be inserted into the final object...
// As they will be grouped anyway, an explicity template specialization for a visitor must be given


#ifndef FORCE_NO_SINGLE_VISITOR
// Defining an (indexed) visitor for a single type
//
// Indexed version - used for containers and iteration. 
// When used in iterate, the return value tells whether iteration 
// should be continued (return true) or stopped (return false);
template <typename IndexType_, typename T>
class SingleVisitor {
public:
    using IndexType = IndexType_;
    using TypeList = type_list<T>;
    
    virtual bool handle(IndexType, T) =0;
    
};

// Flat version
template <typename T>
class SingleVisitor<void, T> {
public:
    using IndexType = void;
    using TypeList = type_list<T>;
    
    virtual void handle(T) =0;
};
#endif // FORCE_NO_SINGLE_VISITOR

// Forward declaration... to be explicitly specialized with templates instead of using "SingleVisitors"
template <typename IndexType_, typename... Ts>
class Visitor;


#ifndef FORCE_NO_SINGLE_VISITOR
#if __cplusplus >= 201703L
template <typename IndexType_, typename... Ts>
class Visitor : private SingleVisitor<IndexType_, Ts>... {
public:
    using SingleVisitor<IndexType_, Ts>::handle...;
    
    using IndexType = IndexType_;
    using TypeList = type_list<Ts...>;
    
    virtual ~Visitor() =default;
};
#else
template <typename IndexType, typename... Ts>
class Visitor;

template <typename IndexType_, typename T, typename Ts1, typename... Ts>
class Visitor<IndexType_, T, Ts1, Ts...>: private SingleVisitor<IndexType_, T>, public Visitor<IndexType_, Ts1, Ts...> {
public:
    using SingleVisitor<IndexType_, T>::handle;
    using Visitor<IndexType_, Ts1, Ts...>::handle;
    
    using IndexType = IndexType_;
    using TypeList = type_list<T, Ts1, Ts...>;
    
    virtual ~Visitor() =default;
};
template <typename IndexType_, typename T>
class Visitor<IndexType_, T> : private SingleVisitor<IndexType_, T> {
public:
    using SingleVisitor<IndexType_, T>::handle;
    
    using IndexType = IndexType_;
    using TypeList = type_list<T>;
    
    virtual ~Visitor() =default;
};
#endif
#endif // FORCE_NO_SINGLE_VISITOR


#if __cplusplus >= 201703L
template <typename TVisitor1, typename... TVisitors>
class VisitorGroup : virtual public TVisitor1, virtual public TVisitors... {
public:
    using TVisitor1::handle;
    using TVisitors::handle...;
    
    using IndexType = typename TVisitor1::IndexType;
    using TypeList = concat_type_list_t<typename TVisitor1::TypeList, typename TVisitors::TypeList...>;
    
    virtual ~VisitorGroup() =default;
};
#else
template <typename... TVisitors>
class VisitorGroup;

template <typename TVisitor, typename TVisitor2, typename... TVisitors>
class VisitorGroup<TVisitor, TVisitor2, TVisitors...> : virtual public TVisitor, public VisitorGroup<TVisitor2, TVisitors...> {
public:
    using TVisitor::handle;
    using VisitorGroup<TVisitor2, TVisitors...>::handle;
    
    using IndexType = typename TVisitor::IndexType;
    using TypeList = concat_type_list_t<typename TVisitor::TypeList, typename VisitorGroup<TVisitor2, TVisitors...>::TypeList>;
    
    virtual ~VisitorGroup() =default;
};
template <typename TVisitor>
class VisitorGroup<TVisitor> : virtual public TVisitor {
public:
    using TVisitor::handle;
    
    using IndexType = typename TVisitor::IndexType;
    using TypeList = typename TVisitor::TypeList;
    
    virtual ~VisitorGroup() =default;
};
#endif


template <bool CONST, typename IndexType, class TVisitor>
class Visitable; 

// For scalar values
template <class TVisitor>
class Visitable<true, void, TVisitor> {
public:
    virtual void visit(TVisitor&) const = 0;
    
    void visit(TVisitor&& v) const {
        visit(v);
    };
    
    virtual ~Visitable() =default;
};
template <class TVisitor>
class Visitable<false, void, TVisitor> {
public:
    virtual void visit(TVisitor&) = 0;
    
    void visit(TVisitor&& v) {
        visit(v);
    };
    
    virtual ~Visitable() =default;
};

// For containers
class VisitableContainerBase {
public:
    // We assume to only have fixed size containers. Each container has as many indices as indicated by size().
    virtual size_t size() const = 0;
};

template <typename IndexType, class TIVisitor>
class Visitable<true, IndexType, TIVisitor>: public virtual VisitableContainerBase {
public:
    using VisitableContainerBase::size;
    
    virtual void visit(IndexType, TIVisitor&) const = 0;
    virtual void iterate(TIVisitor&)          const = 0;

    void visit(IndexType ind, TIVisitor&& visitor) const { visit(ind, visitor); };
    void iterate(TIVisitor&& visitor)              const { iterate(visitor); };
    
    virtual ~Visitable() =default;
};
template <typename IndexType, class TIVisitor>
class Visitable<false, IndexType, TIVisitor>: public virtual VisitableContainerBase {
public:
    using VisitableContainerBase::size;
    
    virtual void visit(IndexType, TIVisitor&) = 0;
    virtual void iterate(TIVisitor&)          = 0;

    void visit(IndexType ind, TIVisitor&& visitor) { visit(ind, visitor); };
    void iterate(TIVisitor&& visitor)              { iterate(visitor); };
    
    virtual ~Visitable() =default;
};


#if __cplusplus >= 201703L
template <typename... TVisitors>
class Visitable<true, void, VisitorGroup<TVisitors...>> : public virtual Visitable<true, void, TVisitors>... {
public:
    using Visitable<true, void, TVisitors>::visit...;

    virtual void visit(VisitorGroup<TVisitors...>&) const =0;
    void visit(VisitorGroup<TVisitors...>&& v) const {
        visit(v);
    };
    
    virtual ~Visitable() =default;
};
template <typename... TVisitors>
class Visitable<false, void, VisitorGroup<TVisitors...>> : public virtual Visitable<false, void, TVisitors>... {
public:
    using Visitable<false, void, TVisitors>::visit...;

    virtual void visit(VisitorGroup<TVisitors...>&) =0;
    void visit(VisitorGroup<TVisitors...>&& v) {
        visit(v);
    };
    
    virtual ~Visitable() =default;
};

template <typename IndexType, typename... TIVisitors>
class Visitable<true, IndexType, VisitorGroup<TIVisitors...>> : public virtual Visitable<true, IndexType, TIVisitors>... {
public:
    using VisitableContainerBase::size;
    using Visitable<true, IndexType, TIVisitors>::visit...;
    using Visitable<true, IndexType, TIVisitors>::iterate...;

    virtual void visit(IndexType, VisitorGroup<TIVisitors...>&) const = 0;
    virtual void iterate(VisitorGroup<TIVisitors...>&)          const = 0;
    
    void visit(IndexType ind, VisitorGroup<TIVisitors...>&& visitor) const { visit(ind, visitor); };
    void iterate(VisitorGroup<TIVisitors...>&& visitor)              const { iterate(visitor); };
    
    virtual ~Visitable() =default;
};
template <typename IndexType, typename... TIVisitors>
class Visitable<false, IndexType, VisitorGroup<TIVisitors...>> : public virtual Visitable<false, IndexType, TIVisitors>... {
public:
    using VisitableContainerBase::size;
    using Visitable<false, IndexType, TIVisitors>::visit...;
    using Visitable<false, IndexType, TIVisitors>::iterate...;

    virtual void visit(IndexType, VisitorGroup<TIVisitors...>&) = 0;
    virtual void iterate(VisitorGroup<TIVisitors...>&)          = 0;
    
    void visit(IndexType ind, VisitorGroup<TIVisitors...>&& visitor) { visit(ind, visitor); };
    void iterate(VisitorGroup<TIVisitors...>&& visitor)              { iterate(visitor); };
    
    virtual ~Visitable() =default;
};
#else
template <bool CONST, typename IndexType, typename... TVisitors>
class VisitableGroupHelper;

template <bool CONST, typename TVisitor, typename TVisitor2, typename... TVisitors>
class VisitableGroupHelper<CONST, void, TVisitor, TVisitor2, TVisitors...>: public virtual Visitable<CONST, void, TVisitor>, public VisitableGroupHelper<CONST, void, TVisitor2, TVisitors...> {
public:
    using Visitable<CONST, void, TVisitor>::visit;
    using VisitableGroupHelper<CONST, void, TVisitor2, TVisitors...>::visit;
};
template <bool CONST, typename TVisitor>
class VisitableGroupHelper<CONST, void, TVisitor>: public Visitable<CONST, void, TVisitor> {
public:
    using Visitable<CONST, void, TVisitor>::visit;
};

template <bool CONST, typename IndexType, typename TVisitor, typename TVisitor2, typename... TVisitors>
class VisitableGroupHelper<CONST, IndexType, TVisitor, TVisitor2, TVisitors...>: public virtual Visitable<CONST, IndexType, TVisitor>, public VisitableGroupHelper<CONST, IndexType, TVisitor2, TVisitors...> {
public:
    using VisitableContainerBase::size;
    using Visitable<CONST, IndexType, TVisitor>::visit;
    using Visitable<CONST, IndexType, TVisitor>::iterate;
    using VisitableGroupHelper<CONST, IndexType, TVisitor2, TVisitors...>::visit;
    using VisitableGroupHelper<CONST, IndexType, TVisitor2, TVisitors...>::iterate;
};
template <bool CONST, typename IndexType, typename TVisitor>
class VisitableGroupHelper<CONST, IndexType, TVisitor>: public virtual Visitable<CONST, IndexType, TVisitor> {
public:
    using VisitableContainerBase::size;
    using Visitable<CONST, IndexType, TVisitor>::visit;
    using Visitable<CONST, IndexType, TVisitor>::iterate;
};

template <typename... TVisitors>
class Visitable<true, void, VisitorGroup<TVisitors...>> : public VisitableGroupHelper<true, void, TVisitors...> {
public:
    using VisitableGroupHelper<true, void, TVisitors...>::visit;

    virtual void visit(VisitorGroup<TVisitors...>&) const =0;
    void visit(VisitorGroup<TVisitors...>&& v) const {
        visit(v);
    };
    
    virtual ~Visitable() =default;
};
template <typename... TVisitors>
class Visitable<false, void, VisitorGroup<TVisitors...>> : public VisitableGroupHelper<false, void, TVisitors...> {
public:
    using VisitableGroupHelper<false, void, TVisitors...>::visit;

    virtual void visit(VisitorGroup<TVisitors...>&) =0;
    void visit(VisitorGroup<TVisitors...>&& v) {
        visit(v);
    };
    
    virtual ~Visitable() =default;
};

template <typename IndexType, typename... TIVisitors>
class Visitable<true, IndexType, VisitorGroup<TIVisitors...>>: public VisitableGroupHelper<true, IndexType, TIVisitors...> {
public:
    using VisitableContainerBase::size;
    using VisitableGroupHelper<true, IndexType, TIVisitors...>::visit;
    using VisitableGroupHelper<true, IndexType, TIVisitors...>::iterate;

    virtual void visit(IndexType, VisitorGroup<TIVisitors...>&) const = 0;
    virtual void iterate(VisitorGroup<TIVisitors...>&)           const = 0;
    
    void visit(IndexType ind, VisitorGroup<TIVisitors...>&& visitor) const { visit(ind, visitor); };
    void iterate(VisitorGroup<TIVisitors...>&& visitor)              const { iterate(visitor); };
    
    virtual ~Visitable() =default;
};
template <typename IndexType, typename... TIVisitors>
class Visitable<false, IndexType, VisitorGroup<TIVisitors...>>: public VisitableGroupHelper<false, IndexType, TIVisitors...> {
public:
    using VisitableContainerBase::size;
    using VisitableGroupHelper<false, IndexType, TIVisitors...>::visit;
    using VisitableGroupHelper<false, IndexType, TIVisitors...>::iterate;

    virtual void visit(IndexType, VisitorGroup<TIVisitors...>&) = 0;
    virtual void iterate(VisitorGroup<TIVisitors...>&)           = 0;
    
    void visit(IndexType ind, VisitorGroup<TIVisitors...>&& visitor) { visit(ind, visitor); };
    void iterate(VisitorGroup<TIVisitors...>&& visitor)               { iterate(visitor); };
    
    virtual ~Visitable() =default;
};
#endif // Below 17


using MapIndexType = const std::string&;
using ListIndexType = long;



template <typename IndexType, class TVisitor>
using Viewable = Visitable<true, IndexType, TVisitor>; 
template <typename IndexType, class TVisitor>
using Editable = Visitable<false, IndexType, TVisitor>; 


template <class TVisitor>
using ViewableScalar = Viewable<void, TVisitor>; 
template <class TVisitor>
using EditableScalar = Editable<void, TVisitor>; 

template <class TVisitor>
using ViewableMap = Viewable<MapIndexType, TVisitor>; 
template <class TVisitor>
using EditableMap = Editable<MapIndexType, TVisitor>; 

template <class TVisitor>
using ViewableList = Viewable<ListIndexType, TVisitor>; 
template <class TVisitor>
using EditableList = Editable<ListIndexType, TVisitor>; 



// ----------------------------------------------------------------------------
// Teplate definitions for an arbitrary IndexedVisitor and VisitableContainer
// ----------------------------------------------------------------------------

// // Defining a Visitor for a single type
// template <typename IndexType, typename T>
// class SingleIndexedVisitor {
// public:
//     virtual void handle(IndexType, T) =0;
// };

// #if __cplusplus >= 201703L
// template <typename IndexType, typename... Ts>
// class IndexedVisitor : public SingleIndexedVisitor<IndexType, Ts>... {
// public:
//     using SingleIndexedVisitor<IndexType, Ts>::handle...;
// };
// #else
// template <typename IndexType, typename... Ts>
// class IndexedVisitor;

// template <typename IndexType, typename T, typename Ts1, typename... Ts>
// class IndexedVisitor<IndexType, T, Ts1, Ts...>: public SingleIndexedVisitor<IndexType, T>, public IndexedVisitor<IndexType, Ts1, Ts...> {
// public:
//     using SingleIndexedVisitor<IndexType, T>::handle;
//     using IndexedVisitor<IndexType, Ts1, Ts...>::handle;
// };
// template <typename IndexType, typename T>
// class IndexedVisitor<IndexType, T> : public SingleIndexedVisitor<IndexType, T> {
// public:
//     using SingleIndexedVisitor<IndexType, T>::handle;
// };
// #endif


// #if __cplusplus >= 201703L
// template <typename IndexType, typename... TVisitors>
// class IndexedVisitorGroup : virtual public TVisitors... {
// public:
//     using TVisitors::handle...;
// };
// #else
// template <typename IndexType, typename... TVisitors>
// class IndexedVisitorGroup;

// template <typename IndexType, typename TVisitor, typename TVisitor2, typename... TVisitors>
// class IndexedVisitorGroup<IndexType, TVisitor, TVisitor2, TVisitors...> : virtual public TVisitor, public IndexedVisitorGroup<IndexType, TVisitor2, TVisitors...> {
// public:
//     using TVisitor::handle;
//     using IndexedVisitorGroup<IndexType, TVisitor2, TVisitors...>::handle;
// };
// template <typename IndexType, typename TVisitor>
// class IndexedVisitorGroup<IndexType, TVisitor> : virtual public TVisitor {
// public:
//     using TVisitor::handle;
// };
// #endif

// template <bool FixedType>
// class VisitableContainerBase {
// public:
//     // All std containers have a fixed type. However a configuration might have mixed and nested types.
//     // When accessing a container, the user might want to have guarantees that a container is of a
//     // fixed type and only specific overloads will get called in a row.
//     static inline const bool hasFixedType() { return FixedType; };

//     // We assume to only have fixed size containers. Each container has as many indices as indicated by size().
//     virtual size_t size() const;
// };

// template <bool CONST, bool FixedType, typename IndexType, class TIVisitor>
// class VisitableContainer;

// template <bool FixedType, typename IndexType, class TIVisitor>
// class VisitableContainer<true, FixedType, IndexType, TIVisitor>: public virtual VisitableContainerBase<FixedType> {
// public:
//     virtual void handle(IndexType, TIVisitor&) const = 0;
//     virtual void iterate(TIVisitor&)           const = 0;

//     void handle(IndexType ind, TIVisitor&& visitor) const { handle(ind, visitor); };
//     void iterate(TIVisitor&& visitor)               const { iterate(visitor); };
// };
// template <bool FixedType, typename IndexType, class TIVisitor>
// class VisitableContainer<false, FixedType, IndexType, TIVisitor>: public virtual VisitableContainerBase<FixedType> {
// public:
//     virtual void handle(IndexType, TIVisitor&) = 0;
//     virtual void iterate(TIVisitor&)           = 0;

//     void handle(IndexType ind, TIVisitor&& visitor) { handle(ind, visitor); };
//     void iterate(TIVisitor&& visitor)               { iterate(visitor); };
// };


// #if __cplusplus >= 201703L
// template <bool FixedType, typename IndexType, typename... TIVisitors>
// class VisitableContainer<true, FixedType, IndexType, IndexedVisitorGroup<TIVisitors...>> : public VisitableContainer<true, FixedType, IndexType, TIVisitors>... {
// public:
//     using VisitableContainer<true, FixedType, IndexType, TIVisitors>::handle...;
//     using VisitableContainer<true, FixedType, IndexType, TIVisitors>::iterate...;

//     virtual void handle(IndexType, IndexedVisitorGroup<TIVisitors...>&) const = 0;
//     virtual void iterate(IndexedVisitorGroup<TIVisitors...>&)           const = 0;
    
//     void handle(IndexType ind, IndexedVisitorGroup<TIVisitors...>&& visitor) const { handle(ind, visitor); };
//     void iterate(IndexedVisitorGroup<TIVisitors...>&& visitor)               const { iterate(visitor); };
// };
// template <bool FixedType, typename IndexType, typename... TIVisitors>
// class VisitableContainer<false, FixedType, IndexType, IndexedVisitorGroup<TIVisitors...>> : public VisitableContainer<false, FixedType, IndexType, TIVisitors>... {
// public:
//     using VisitableContainer<false, FixedType, IndexType, TIVisitors>::handle...;
//     using VisitableContainer<false, FixedType, IndexType, TIVisitors>::iterate...;

//     virtual void handle(IndexType, IndexedVisitorGroup<TIVisitors...>&) = 0;
//     virtual void iterate(IndexedVisitorGroup<TIVisitors...>&)           = 0;
    
//     void handle(IndexType ind, IndexedVisitorGroup<TIVisitors...>&& visitor) { handle(ind, visitor); };
//     void iterate(IndexedVisitorGroup<TIVisitors...>&& visitor)               { iterate(visitor); };
// };
// #else
// template <bool CONST, bool FixedType, typename IndexType, typename... TIVisitors>
// class VisitableContainerGroupHelper;

// template <bool CONST, bool FixedType, typename IndexType, typename TIVisitor, typename TIVisitor2, typename... TIVisitors>
// class VisitableContainerGroupHelper<CONST, FixedType, IndexType, TIVisitor, TIVisitor2, TIVisitors...>: public VisitableContainer<CONST, FixedType, IndexType, TIVisitor>, public VisitableContainerGroupHelper<CONST, FixedType, IndexType, TIVisitor2, TIVisitors...> {
// public:
//     using VisitableContainer<CONST, FixedType, IndexType, TIVisitor>::handle;
//     using VisitableContainer<CONST, FixedType, IndexType, TIVisitor>::iterate;
//     using VisitableContainerGroupHelper<CONST, FixedType, IndexType, TIVisitor2, TIVisitors...>::handle;
//     using VisitableContainerGroupHelper<CONST, FixedType, IndexType, TIVisitor2, TIVisitors...>::iterate;
// };
// template <bool CONST, bool FixedType, typename IndexType, typename TIVisitor>
// class VisitableContainerGroupHelper<CONST, FixedType, IndexType, TIVisitor>: public VisitableContainer<CONST, FixedType, IndexType, TIVisitor> {
// public:
//     using VisitableContainer<CONST, FixedType, IndexType, TIVisitor>::handle;
//     using VisitableContainer<CONST, FixedType, IndexType, TIVisitor>::interate;
// };

// template <bool FixedType, typename IndexType, typename... TIVisitors>
// class VisitableContainer<true, FixedType, IndexType, IndexedVisitorGroup<IndexType, TIVisitors...>>: public VisitableContainerGroupHelper<true, FixedType, IndexType, TIVisitors...> {
// public:
//     using VisitableContainerGroupHelper<true, FixedType, IndexType, TIVisitors...>::handle;
//     using VisitableContainerGroupHelper<true, FixedType, IndexType, TIVisitors...>::iterate;

//     virtual void handle(IndexType, IndexedVisitorGroup<TIVisitors...>&) const = 0;
//     virtual void iterate(IndexedVisitorGroup<TIVisitors...>&)           const = 0;
    
//     void handle(IndexType ind, IndexedVisitorGroup<TIVisitors...>&& visitor) const { handle(ind, visitor); };
//     void iterate(IndexedVisitorGroup<TIVisitors...>&& visitor)               const { iterate(visitor); };
// };
// template <bool FixedType, typename IndexType, typename... TIVisitors>
// class VisitableContainer<false, FixedType, IndexType, IndexedVisitorGroup<IndexType, TIVisitors...>>: public VisitableContainerGroupHelper<false, FixedType, IndexType, TIVisitors...> {
// public:
//     using VisitableContainerGroupHelper<false, FixedType, IndexType, TIVisitors...>::handle;
//     using VisitableContainerGroupHelper<false, FixedType, IndexType, TIVisitors...>::iterate;

//     virtual void handle(IndexType, IndexedVisitorGroup<TIVisitors...>&) = 0;
//     virtual void iterate(IndexedVisitorGroup<TIVisitors...>&)           = 0;
    
//     void handle(IndexType ind, IndexedVisitorGroup<TIVisitors...>&& visitor) { handle(ind, visitor); };
//     void iterate(IndexedVisitorGroup<TIVisitors...>&& visitor)               { iterate(visitor); };
// };
// #endif // Below 17

// template <bool CONST, class TIVisitorGroup>
// using VisitableMixedMap = VisitableContainer<CONST, false, const std::string&, TIVisitorGroup>;
// template <bool CONST, class TIVisitorGroup>
// using VisitableMixedList = VisitableContainer<CONST, false, long, TIVisitorGroup>;

// template <bool CONST, class TIVisitorGroup>
// using VisitableFixedMap = VisitableContainer<CONST, true, const std::string&, TIVisitorGroup>;
// template <bool CONST, class TIVisitorGroup>
// using VisitableFixedList = VisitableContainer<CONST, true, long, TIVisitorGroup>;


// template <class TIVisitorGroup>
// using ViewableMixedMap = VisitableMixedMap<true, TIVisitorGroup>;
// template <class TIVisitorGroup>
// using ViewableMixedList = VisitableMixedList<true, TIVisitorGroup>;

// template <class TIVisitorGroup>
// using ViewableFixedMap = VisitableFixedMap<true, TIVisitorGroup>;
// template <class TIVisitorGroup>
// using ViewableFixedList = VisitableFixedList<true, TIVisitorGroup>;



// template <class TIVisitorGroup>
// using EditableMixedMap = VisitableMixedMap<false, TIVisitorGroup>;
// template <class TIVisitorGroup>
// using EditableMixedList = VisitableMixedList<false, TIVisitorGroup>;

// template <class TIVisitorGroup>
// using EditableFixedMap = VisitableFixedMap<false, TIVisitorGroup>;
// template <class TIVisitorGroup>
// using EditableFixedList = VisitableFixedList<false, TIVisitorGroup>;




// ----------------------------------------------------------------------------
// Utilities for any arbitrary Visitor and Visitable
// FreeVisitor: Allow building Visitors from lambda functions
// ComposedVisitor: Allow building higher-ordered visitores from sub visitores
// ----------------------------------------------------------------------------

template <typename Derived, typename TVisitor, typename IndexType, typename T>
class SingleFreeVisitor;

template <typename Derived, typename TVisitor,typename T>
class SingleFreeVisitor<Derived, TVisitor, void, T> : virtual public TVisitor {
public:
    void handle(T v) override {
        static_cast<Derived*>(this)->operator()(v);
    }
};

template <typename Derived, typename TVisitor, typename IndexType, typename T>
class SingleFreeVisitor: virtual public TVisitor {
public:
    bool handle(IndexType i, T v) override {
        return static_cast<Derived*>(this)->operator()(i, v);
    }
};



#if __cplusplus >= 201703L
template <typename Derived, typename TVisitor>
class FreeVisitorBuilder;

template <typename Derived, typename IndexType, typename... Ts>
class FreeVisitorBuilder<Derived, Visitor<IndexType, Ts...>> : public SingleFreeVisitor<Derived, Visitor<IndexType, Ts...>, IndexType, Ts>... {
public:
    using SingleFreeVisitor<Derived, Visitor<IndexType, Ts...>, IndexType, Ts>::handle...;
};

template <typename Derived, typename... Ts>
class FreeVisitorBuilder<Derived, VisitorGroup<Ts...>> : virtual public VisitorGroup<Ts...>, public FreeVisitorBuilder<Derived, Ts>... {
public:
    using FreeVisitorBuilder<Derived, Ts>::handle...;
};


template <typename TVisitor, typename... TFunctors>
class FreeVisitor : public TFunctors..., public FreeVisitorBuilder<FreeVisitor<TVisitor, TFunctors...>, TVisitor> {
public:
    using TFunctors::operator()...;

    using IndexType = typename TVisitor::IndexType;
    using TypeList = typename TVisitor::TypeList;

    template<typename ...Ts>
    FreeVisitor(Ts&& ... ts): decay_t<Ts>(std::forward<Ts>(ts))... {}
        
    // Only use handle, hide all operator()(...)
    using FreeVisitorBuilder<FreeVisitor<TVisitor, TFunctors...>, TVisitor>::handle;
};
#else
template <typename Derived, typename TVisitor, typename ... Ts>
class FreeVisitorBuilder;


// Recurse on using...
template <typename Derived, typename IndexType, typename ...TVs, typename T1, typename T2, typename... Ts>
class FreeVisitorBuilder<Derived, Visitor<IndexType, TVs...>, T1, T2, Ts...> : public SingleFreeVisitor<Derived, Visitor<IndexType, TVs...>, IndexType, T1>, public FreeVisitorBuilder<Derived, Visitor<IndexType, TVs...>, T2, Ts...> {
public:
    using SingleFreeVisitor<Derived, Visitor<IndexType, TVs...>, IndexType, T1>::handle;
    using FreeVisitorBuilder<Derived, Visitor<IndexType, TVs...>, T2, Ts...>::handle;
};

// End recursion
template <typename Derived, typename IndexType, typename ...TVs, typename T1>
class FreeVisitorBuilder<Derived, Visitor<IndexType, TVs...>, T1> : public SingleFreeVisitor<Derived, Visitor<IndexType, TVs...>, IndexType, T1> {
public:
    using SingleFreeVisitor<Derived, Visitor<IndexType, TVs...>, IndexType, T1>::handle;
};

// Start  recursion
template <typename Derived, typename IndexType, typename ...Ts>
class FreeVisitorBuilder<Derived, Visitor<IndexType, Ts...>> : public FreeVisitorBuilder<Derived, Visitor<IndexType, Ts...>, Ts...> {
public:
    using FreeVisitorBuilder<Derived, Visitor<IndexType, Ts...>, Ts...>::handle;
};


// Recurse
template <typename Derived, typename... TVs, typename T1, typename T2, typename ...Ts>
class FreeVisitorBuilder<Derived, VisitorGroup<TVs...>, T1, T2, Ts...> : public FreeVisitorBuilder<Derived, T1>, public FreeVisitorBuilder<Derived, VisitorGroup<Ts...>, T2, Ts...> {
public:
    using FreeVisitorBuilder<Derived, T1>::handle;
    using FreeVisitorBuilder<Derived, VisitorGroup<Ts...>, T2, Ts...>::handle;
};
// End Recursion
template <typename Derived, typename... TVs, typename T1>
class FreeVisitorBuilder<Derived, VisitorGroup<TVs...>, T1> : public FreeVisitorBuilder<Derived, T1> {
public:
    using FreeVisitorBuilder<Derived, T1>::handle;
};
// Start recursion
template <typename Derived, typename... Ts>
class FreeVisitorBuilder<Derived, VisitorGroup<Ts...>> : virtual public VisitorGroup<Ts...>, public FreeVisitorBuilder<Derived, VisitorGroup<Ts...>, Ts...> {
public:
    using  FreeVisitorBuilder<Derived, VisitorGroup<Ts...>, Ts...>::handle;
};


template<typename ...Ts>
class FreeVisitorOperators;

template<typename T1, typename T2, typename ...Ts>
class FreeVisitorOperators<T1, T2, Ts...>: public T1, public FreeVisitorOperators<T2, Ts...> {
public:
    template<typename F1, typename ...F>
    FreeVisitorOperators(F1&& f1, F&& ... f): T1{std::forward<F1>(f1)}, FreeVisitorOperators<T2, Ts...>(std::forward<F>(f)...) {}

    using T1::operator();
    using FreeVisitorOperators<T2, Ts...>::operator();
};
template<typename T1>
class FreeVisitorOperators<T1>: public T1 {
public:
    template<typename F1>
    FreeVisitorOperators(F1&& f1): T1{std::forward<F1>(f1)} {}
    
    using T1::operator();
};

template <typename TVisitor, typename... TFunctors>
class FreeVisitor : public FreeVisitorOperators<TFunctors...>, public FreeVisitorBuilder<FreeVisitor<TVisitor, TFunctors...>, TVisitor> {
public:
    using FreeVisitorOperators<TFunctors...>::operator();
    
    using IndexType = typename TVisitor::IndexType;
    using TypeList = typename TVisitor::TypeList;

    template<typename ...Ts>
    FreeVisitor(Ts&& ... ts): FreeVisitorOperators<TFunctors...>(std::forward<Ts>(ts)...) {}
        
    // Only use handle, hide all operator()(...)
    using FreeVisitorBuilder<FreeVisitor<TVisitor, TFunctors...>, TVisitor>::handle;
};
#endif // Below 17

template <typename TVisitor, typename... Ts>
auto freeVisitor(Ts&&... ts) -> decltype(FreeVisitor<TVisitor, Ts...>{std::forward<Ts>(ts)...}) { // decltype can be removed in C++17
    return FreeVisitor<TVisitor, Ts...>{std::forward<Ts>(ts)...};
}




#if __cplusplus >= 201703L
template<typename TVisitorGroup, typename ...TVisitors>
class ComposedVisitor : public TVisitors..., virtual public TVisitorGroup {
public:
    template<typename ...Ts>
    ComposedVisitor(Ts&& ... ts): decay_t<Ts>(std::forward<Ts>(ts))... {}
    
    using IndexType = typename TVisitorGroup::IndexType;
    using TypeList = typename TVisitorGroup::TypeList;
    
    using TVisitors::handle...;
};
#else

template<typename ... Ts>
class ComposedVisitorHelper;

template<typename T1, typename T2, typename ...Ts>
class ComposedVisitorHelper<T1, T2, Ts...>: public T1, public ComposedVisitorHelper<T2, Ts...> {
public:
    using T1::handle;
    using ComposedVisitorHelper<T2, Ts...>::handle;
    
    template<typename F1, typename ...F>
    ComposedVisitorHelper(F1&& f1, F&& ... f): T1(std::forward<F1>(f1)), ComposedVisitorHelper<T2, Ts...>(std::forward<F>(f)...) {}
};

template<typename T1>
class ComposedVisitorHelper<T1>: public T1 {
public:
    using T1::handle;
    
    template<typename F1>
    ComposedVisitorHelper(F1&& f1): T1(std::forward<F1>(f1)) {}
};

template<typename TVisitorGroup, typename ...TVisitors>
class ComposedVisitor : public ComposedVisitorHelper<TVisitors...>, virtual public TVisitorGroup {
public:
    template<typename ...Ts>
    ComposedVisitor(Ts&& ... ts): ComposedVisitorHelper<TVisitors...>(std::forward<Ts>(ts)...) {}
    
    using IndexType = typename TVisitorGroup::IndexType;
    using TypeList = typename TVisitorGroup::TypeList;
    
    using ComposedVisitorHelper<TVisitors...>::handle;
};
#endif // Below 17


template<typename  TVisitorGroup, typename... Ts>
auto composedVisitor(Ts&&... ts) -> decltype(ComposedVisitor<TVisitorGroup, Ts...>{std::forward<Ts>(ts)...}) { // decltype can be removed in C++17
    return ComposedVisitor<TVisitorGroup, Ts...>{std::forward<Ts>(ts)...};
}


// ----------------------------------------------------------------------------
// Templated visitor with CRTP pattern (Curiously Recurring Template Pattern)
// to use visitors with full type knowledge for use when types are known (i.e. inside a project)
//
// This class is meant to be used to generate all the visit overloads on a container class by calling one 
// templated visit function with a type list.
//
// For types like std::variant (which evaluate a tag) this is straight forward by calling `std::visit` (i.e. switching on the type tag)
// and then filter on the type_list. This can be very efficient because the tag evaluation 
// can be efficiently compressed without performing a function call.
//
// For purerly dynamic value types (i.e. eckit::Value) this is different. 
// Here dynamic polymorphism is used to achieve specialization with virtual function calls. Instead of an additional switch on a tag,
// a vtable lookup and function call must be called.
// ----------------------------------------------------------------------------

template <class TVisitable, typename Derived>
class CRTPVisitable; 


#if __cplusplus >= 201703L
// Scalar values - group recursion
template <typename Derived, typename... TVisitors>
class CRTPVisitable<Visitable<true, void, VisitorGroup<TVisitors...>>, Derived> : 
    public virtual Visitable<true, void, VisitorGroup<TVisitors...>>, 
    public CRTPVisitable<Visitable<true, void, TVisitors>, Derived>... {
public:
    using Visitable<true, void, VisitorGroup<TVisitors...>>::visit;
    using CRTPVisitable<Visitable<true, void, TVisitors>, Derived>::visit...;

    void visit(VisitorGroup<TVisitors...>& v) const override {
        static_cast<const Derived&>(*this).Derived::visit(v, typename VisitorGroup<TVisitors...>::TypeList());
    };

    virtual ~CRTPVisitable() =default;
};
template <typename Derived, typename... TVisitors>
class CRTPVisitable<Visitable<false, void, VisitorGroup<TVisitors...>>, Derived> : 
    public virtual Visitable<false, void, VisitorGroup<TVisitors...>>,
    public CRTPVisitable<Visitable<false, void, TVisitors>, Derived>... {
public:
    using Visitable<false, void, VisitorGroup<TVisitors...>>::visit;
    using CRTPVisitable<Visitable<false, void, TVisitors>, Derived>::visit...;

    void visit(VisitorGroup<TVisitors...>& v) override {
        static_cast<Derived&>(*this).Derived::visit(v, typename VisitorGroup<TVisitors...>::TypeList());
    };

    virtual ~CRTPVisitable() =default;
};

// Indexed values - group recursion
template <typename Derived, typename IndexType, typename... TIVisitors>
class CRTPVisitable<Visitable<true, IndexType, VisitorGroup<TIVisitors...>>, Derived> : 
    public virtual Visitable<true, IndexType, VisitorGroup<TIVisitors...>>,
    public CRTPVisitable<Visitable<true, IndexType, TIVisitors>, Derived>... {
public:
    using Visitable<true, IndexType, VisitorGroup<TIVisitors...>>::visit;
    using Visitable<true, IndexType, VisitorGroup<TIVisitors...>>::iterate;
    using CRTPVisitable<Visitable<true, IndexType, TIVisitors>, Derived>::visit...;
    using CRTPVisitable<Visitable<true, IndexType, TIVisitors>, Derived>::iterate...;
    using VisitableContainerBase::size;
    
    void visit(IndexType i, VisitorGroup<TIVisitors...>& v) const override {
        static_cast<const Derived&>(*this).Derived::visit(i, v, typename VisitorGroup<TIVisitors...>::TypeList());
    };
    void iterate(VisitorGroup<TIVisitors...>& v) const override {
        static_cast<const Derived&>(*this).Derived::iterate(v, typename VisitorGroup<TIVisitors...>::TypeList());
    };

    virtual ~CRTPVisitable() =default;
};
template <typename Derived, typename IndexType, typename... TIVisitors>
class CRTPVisitable<Visitable<false, IndexType, VisitorGroup<TIVisitors...>>, Derived> : 
    public virtual Visitable<false, IndexType, VisitorGroup<TIVisitors...>>,
    public CRTPVisitable<Visitable<false, IndexType, TIVisitors>, Derived>... {
public:
    using Visitable<false, IndexType, VisitorGroup<TIVisitors...>>::visit;
    using Visitable<false, IndexType, VisitorGroup<TIVisitors...>>::iterate;
    using CRTPVisitable<Visitable<false, IndexType, TIVisitors>, Derived>::visit...;
    using CRTPVisitable<Visitable<false, IndexType, TIVisitors>, Derived>::iterate...;
    using VisitableContainerBase::size;
    
    void visit(IndexType i, VisitorGroup<TIVisitors...>& v) override {
        static_cast<Derived&>(*this).Derived::visit(i, v, VisitorGroup<TIVisitors...>::TypeList());
    };
    void iterate(VisitorGroup<TIVisitors...>& v) override {
        static_cast<Derived&>(*this).Derived::iterate(v, VisitorGroup<TIVisitors...>::TypeList());
    };

    virtual ~CRTPVisitable() =default;
};
#else
// TBD C11/14 version without pack expansion....
#endif

// For scalar values
template <class TVisitor, typename Derived>
class CRTPVisitable<Visitable<true, void, TVisitor>, Derived>: public virtual Visitable<true, void, TVisitor> {
public:
    using Visitable<true, void, TVisitor>::visit;
    void visit(TVisitor& v) const override {
        static_cast<const Derived&>(*this).Derived::visit(v, typename TVisitor::TypeList());
    };

    virtual ~CRTPVisitable() =default;
};
template <class TVisitor, typename Derived>
class CRTPVisitable<Visitable<false, void, TVisitor>, Derived>: public virtual Visitable<false, void, TVisitor> {
public:
    using Visitable<false, void, TVisitor>::visit;
    void visit(TVisitor& v) override {
        static_cast<Derived&>(*this).Derived::visit(v, typename TVisitor::TypeList());
    };

    virtual ~CRTPVisitable() =default;
};

// For indexed values
template <typename IndexType, class TIVisitor, typename Derived>
class CRTPVisitable<Visitable<true, IndexType, TIVisitor>, Derived>: public virtual Visitable<true, IndexType, TIVisitor> {
public:
    using Visitable<true, IndexType, TIVisitor>::visit;
    using Visitable<true, IndexType, TIVisitor>::iterate;
    using VisitableContainerBase::size;
    
    void visit(IndexType i, TIVisitor& v) const override {
        static_cast<const Derived&>(*this).Derived::visit(i, v, typename TIVisitor::TypeList());
    };
    
    void iterate(TIVisitor& v) const override {
        static_cast<const Derived&>(*this).Derived::iterate(v, typename TIVisitor::TypeList());
    };

    virtual ~CRTPVisitable() =default;
};
template <typename IndexType, class TIVisitor, typename Derived>
class CRTPVisitable<Visitable<false, IndexType, TIVisitor>, Derived>: public virtual Visitable<false, IndexType, TIVisitor> {
public:
    using Visitable<false, IndexType, TIVisitor>::visit;
    using Visitable<false, IndexType, TIVisitor>::iterate;
    using VisitableContainerBase::size;
    
    void visit(IndexType i, TIVisitor& v) override {
        static_cast<Derived&>(*this).Derived::visit(i, v, typename TIVisitor::TypeList());
    };
    
    void iterate(TIVisitor& v) override {
        static_cast<Derived&>(*this).Derived::iterate(v, typename TIVisitor::TypeList());
    };

    virtual ~CRTPVisitable() =default;
};



// ----------------------------------------------------------------------------
// Example
//
// Concrete definitions for a type ValueViewer supporting flat scalar values 
// as well as nested structures
// ----------------------------------------------------------------------------

// Wrapper to hold an array with its size, similar to std::span in C++20
template<typename T>
struct ContiguousDataView {
    const T* data;
    const size_t size;
    
    using type = T;
};

// Converter 
// To be specialized...
template<typename T>
class ToContiguousDataView {
public:
    using type = void;
};

template<typename T, typename Allocator>
class ToContiguousDataView<std::vector<T, Allocator>> {
public:
    using type = ContiguousDataView<T>;
    
    inline type operator()(const std::vector<T, Allocator>& v) const noexcept(true) {
        return {v.data(), v.size()};
    }
};

#if __cplusplus >= 201703L
template <typename From>
auto toContiguousDataView(From&& from) noexcept(true) {
    return ToContiguousDataView<decay_t<From>>()(std::forward<From>(from));
}
#endif



// Explicit template specialization in favor of generation with SingleVisitor base classes...
#ifndef FORCE_SINGLE_VISITOR_GENERATION
// Integral types
template <typename IndexType_>
class Visitor<IndexType_, long, size_t, int, bool> {
public:
    using IndexType = IndexType_;
    using TypeList = type_list<long, size_t, int, bool>;
    
    virtual bool handle(IndexType, long)   = 0;
    virtual bool handle(IndexType, size_t) = 0;
    virtual bool handle(IndexType, int)    = 0;
    virtual bool handle(IndexType, bool)   = 0;
 
    virtual ~Visitor() =default;
};

template<>
class Visitor<void, long, size_t, int, bool> {
public:
    using IndexType = void;
    using TypeList = type_list<long, size_t, int, bool>;
    
    virtual void handle(long)   = 0;
    virtual void handle(size_t) = 0;
    virtual void handle(int)    = 0;
    virtual void handle(bool)   = 0;
 
    virtual ~Visitor() =default;
};

// Floating types
template <typename IndexType_>
class Visitor<IndexType_, float, double> {
public:
    using IndexType = IndexType_;
    using TypeList = type_list<float, double>;
    
    virtual bool handle(IndexType, float)   = 0;
    virtual bool handle(IndexType, double) = 0;
 
    virtual ~Visitor() =default;
};

template<>
class Visitor<void, float, double> {
public:
    using IndexType = void;
    using TypeList = type_list<float, double>;
    
    virtual void handle(float)   = 0;
    virtual void handle(double) = 0;
 
    virtual ~Visitor() =default;
};

// String types
template <typename IndexType_>
class Visitor<IndexType_, const std::string&, const char*> {
public:
    using IndexType = IndexType_;
    using TypeList = type_list<const std::string&, const char*>;
    
    virtual bool handle(IndexType, const std::string&)   = 0;
    virtual bool handle(IndexType, const char*) = 0;
 
    virtual ~Visitor() =default;
};

template<>
class Visitor<void, const std::string&, const char*> {
public:
    using IndexType = void;
    using TypeList = type_list<const std::string&, const char*>;
    
    virtual void handle(const std::string&)   = 0;
    virtual void handle(const char*) = 0;
 
    virtual ~Visitor() =default;
};

// Integral contiguous types...
template <typename IndexType_>
class Visitor<IndexType_, ContiguousDataView<long>, ContiguousDataView<size_t>, ContiguousDataView<int>, ContiguousDataView<bool>> {
public:
    using IndexType = IndexType_;
    using TypeList = type_list<ContiguousDataView<long>, ContiguousDataView<size_t>, ContiguousDataView<int>, ContiguousDataView<bool>>;
    
    virtual bool handle(IndexType, ContiguousDataView<long>)     = 0;
    virtual bool handle(IndexType, ContiguousDataView<size_t>)   = 0;
    virtual bool handle(IndexType, ContiguousDataView<int>)      = 0;
    virtual bool handle(IndexType, ContiguousDataView<bool>)     = 0;
 
    virtual ~Visitor() =default;
};

template<>
class Visitor<void, ContiguousDataView<long>, ContiguousDataView<size_t>, ContiguousDataView<int>, ContiguousDataView<bool>> {
public:
    using IndexType = void;
    using TypeList = type_list<ContiguousDataView<long>, ContiguousDataView<size_t>, ContiguousDataView<int>, ContiguousDataView<bool>>;
    
    virtual void handle(ContiguousDataView<long>)     = 0;
    virtual void handle(ContiguousDataView<size_t>)   = 0;
    virtual void handle(ContiguousDataView<int>)      = 0;
    virtual void handle(ContiguousDataView<bool>)     = 0;
 
    virtual ~Visitor() =default;
};

// Floating contiguous types...
template <typename IndexType_>
class Visitor<IndexType_, ContiguousDataView<float>, ContiguousDataView<double>> {
public:
    using IndexType = IndexType_;
    using TypeList = type_list<ContiguousDataView<float>, ContiguousDataView<double>>;
    
    virtual bool handle(IndexType, ContiguousDataView<float>)     = 0;
    virtual bool handle(IndexType, ContiguousDataView<double>)    = 0;
 
    virtual ~Visitor() =default;
};

template<>
class Visitor<void, ContiguousDataView<float>, ContiguousDataView<double>> {
public:
    using IndexType = void;
    using TypeList = type_list<ContiguousDataView<float>, ContiguousDataView<double>>;
    
    virtual void handle(ContiguousDataView<float>)     = 0;
    virtual void handle(ContiguousDataView<double>)    = 0;
 
    virtual ~Visitor() =default;
};

// Binary contiguous types...
template <typename IndexType_>
class Visitor<IndexType_, ContiguousDataView<unsigned char>> {
public:
    using IndexType = IndexType_;
    using TypeList = type_list<ContiguousDataView<unsigned char>>;
    
    virtual bool handle(IndexType, ContiguousDataView<unsigned char>)     = 0;
 
    virtual ~Visitor() =default;
};

template<>
class Visitor<void, ContiguousDataView<unsigned char>> {
public:
    using IndexType = void;
    using TypeList = type_list<ContiguousDataView<unsigned char>>;
    
    virtual void handle(ContiguousDataView<unsigned char>)     = 0;
 
    virtual ~Visitor() =default;
};
#endif // FORCE_SINGLE_VISITOR_GENERATION


// --- Scala Values ---
template<typename IndexType>
using IntegralValueViewer = Visitor<IndexType, long, size_t, int, bool>;
template<typename IndexType>
using FloatingValueViewer = Visitor<IndexType, float, double>;
template<typename IndexType>
using NumericValueViewer  = VisitorGroup<IntegralValueViewer<IndexType>, FloatingValueViewer<IndexType>>;

template<typename IndexType>
using StringValueViewer = Visitor<IndexType, const std::string&, const char*>;

template<typename IndexType>
using ScalarValueViewer = VisitorGroup<NumericValueViewer<IndexType>, StringValueViewer<IndexType>>;


// --- Contiguous Values ---

template<typename IndexType>
using IntegralContiguousValueViewer = Visitor<IndexType, ContiguousDataView<long>, ContiguousDataView<size_t>, ContiguousDataView<int>, ContiguousDataView<bool>>; // bool may be omitted here. Arrays of bool are usually not reasonable. Moreover containers like std::vector<bool> are specialized
template<typename IndexType>
using FloatingContiguousValueViewer = Visitor<IndexType, ContiguousDataView<float>, ContiguousDataView<double>>;
template<typename IndexType>
using NumericContiguousValueViewer  = VisitorGroup<IntegralContiguousValueViewer<IndexType>, FloatingContiguousValueViewer<IndexType>>;

template<typename IndexType>
using BinaryContiguousValueViewer = Visitor<IndexType, ContiguousDataView<unsigned char>>;
template<typename IndexType>
using ContiguousValueViewer       = VisitorGroup<NumericContiguousValueViewer<IndexType>, BinaryContiguousValueViewer<IndexType>>;

// --- Container Values ---
// First forward define nested Viewables
class ViewableMapValue;
class ViewableListValue;


// Explicit template specialization in favor of generation with SingleVisitor base classes...
#ifndef FORCE_SINGLE_VISITOR_GENERATION
// Map container types...
template <typename IndexType_>
class Visitor<IndexType_, const ViewableMapValue&> {
public:
    using IndexType = IndexType_;
    using TypeList = type_list<const ViewableMapValue&>;
    
    virtual bool handle(IndexType, const ViewableMapValue&)     = 0;
 
    virtual ~Visitor() =default;
};

template<>
class Visitor<void, const ViewableMapValue&> {
public:
    using IndexType = void;
    using TypeList = type_list<const ViewableMapValue&>;
    
    virtual void handle(const ViewableMapValue&)     = 0;
 
    virtual ~Visitor() =default;
};

// List container types...
template <typename IndexType_>
class Visitor<IndexType_, const ViewableListValue&> {
public:
    using IndexType = IndexType_;
    using TypeList = type_list<const ViewableListValue&>;
    
    virtual bool handle(IndexType, const ViewableListValue&)     = 0;
 
    virtual ~Visitor() =default;
};

template<>
class Visitor<void, const ViewableListValue&> {
public:
    using IndexType = void;
    using TypeList = type_list<const ViewableListValue&>;
    
    virtual void handle(const ViewableListValue&)     = 0;
 
    virtual ~Visitor() =default;
};
#endif // FORCE_SINGLE_VISITOR_GENERATION


// Now we can define Container Viewers
template<typename IndexType>
using MapValueViewer  = Visitor<IndexType, const ViewableMapValue&>;
template<typename IndexType>
using ListValueViewer = Visitor<IndexType, const ViewableListValue&>;

// General Container value viewer
// By adding all the overlapping group, a Viewable supporting this viewer also needs to add an overload for all the other combinations
template<typename IndexType>
using ContainerValueViewer = VisitorGroup<ListValueViewer<IndexType>, MapValueViewer<IndexType>>;


// --- General ---
// using ValueViewer = VisitorGroup<ScalarValueViewer, ContiguousValueViewer, ContainerValueViewer>;
template<typename IndexType>
using ValueViewer = VisitorGroup<ScalarValueViewer<IndexType>, ContiguousValueViewer<IndexType>, ContainerValueViewer<IndexType>>;


// Very general values... have to implement a lot
using ViewableValue = Viewable<void, ValueViewer<void>>;

// Nested container involve recursion, hence they have to be referred to by reference/pointer and need an explicit virtual destructor
class ViewableMapValue : public virtual ViewableMap<ValueViewer<MapIndexType>> {
public:
    using ViewableMap<ValueViewer<MapIndexType>>::visit;
    using ViewableMap<ValueViewer<MapIndexType>>::iterate;
    using VisitableContainerBase::size;
    
    virtual ~ViewableMapValue() =default;
};
class ViewableListValue : public virtual ViewableList<ValueViewer<ListIndexType>> {
public:
    using ViewableList<ValueViewer<ListIndexType>>::visit;
    using ViewableList<ValueViewer<ListIndexType>>::iterate; using VisitableContainerBase::size;
    
    virtual ~ViewableListValue() =default;
};



// class TestViewableMap: public ViewableMapValue

class TestViewable: public ViewableValue {
public:
    using ViewableValue::visit; // Loading rvalue overloads
    void visit(ValueViewer<void>& visitor) const override {
        std::cout << "TestViewable::visit(ValueViewer<void>& visitor) const" << std::endl;
        // Now calling all handles for demonstration. Usually one handle is called depending the value it represents or contains.
        TestViewable::visit((ScalarValueViewer<void>&) visitor);
        TestViewable::visit((ContiguousValueViewer<void>&) visitor);
        TestViewable::visit((ContainerValueViewer<void>&) visitor);
        std::cout << std::endl;
    }
    
    void visit(ContiguousValueViewer<void>& visitor) const override {
        std::cout << "TestViewable::visit(ContiguousValueViewer<void>& visitor) const" << std::endl;
        TestViewable::visit((NumericContiguousValueViewer<void>&) visitor);
        TestViewable::visit((BinaryContiguousValueViewer<void>&) visitor);
        std::cout << std::endl;
    }
    void visit(NumericContiguousValueViewer<void>& visitor) const override {
        std::cout << "TestViewable::visit(NumericContiguousValueViewer<void>& visitor) const" << std::endl;
        TestViewable::visit((IntegralContiguousValueViewer<void>&) visitor);
        TestViewable::visit((FloatingContiguousValueViewer<void>&) visitor);
        std::cout << std::endl;
    }
    void visit(IntegralContiguousValueViewer<void>& visitor) const override {
        std::cout << "TestViewable::visit(IntegralContiguousValueViewer<void>& visitor) const" << std::endl;
        visitor.handle(ContiguousDataView<ListIndexType>{nullptr, 0});
        visitor.handle(ContiguousDataView<size_t>{nullptr, 0});
        visitor.handle(ContiguousDataView<int>{nullptr, 0});
        visitor.handle(ContiguousDataView<bool>{nullptr, 0});
        std::cout << std::endl;
    }
    void visit(FloatingContiguousValueViewer<void>& visitor) const override {
        std::cout << "TestViewable::visit(FloatingContiguousValueViewer<void>& visitor) const" << std::endl;
        visitor.handle(ContiguousDataView<float>{nullptr, 0});
        visitor.handle(ContiguousDataView<double>{nullptr, 0});
        std::cout << std::endl;
    }
    void visit(BinaryContiguousValueViewer<void>& visitor) const override {
        std::cout << "TestViewable::visit(BinaryContiguousValueViewer<void>& visitor) const" << std::endl;
        visitor.handle(ContiguousDataView<unsigned char>{nullptr, 0});
        std::cout << std::endl;
    }
    
    void visit(ScalarValueViewer<void>& visitor) const override {
        std::cout << "TestViewable::visit(ScalarValueViewer<void>& visitor) const" << std::endl;
        TestViewable::visit((NumericValueViewer<void>&)visitor);
        TestViewable::visit((StringValueViewer<void>&)visitor);
        std::cout << std::endl;
    }
    void visit(NumericValueViewer<void>& visitor) const override {
        std::cout << "TestViewable::visit(NumericValueViewer<void>& visitor) const" << std::endl;
        TestViewable::visit((IntegralValueViewer<void>&)visitor);
        TestViewable::visit((FloatingValueViewer<void>&)visitor);
        std::cout << std::endl;
    }
    void visit(IntegralValueViewer<void>& visitor) const override {
        std::cout << "TestViewable::visit(IntegralValueViewer<void>& visitor) const" << std::endl;
        visitor.handle((int)2);
        visitor.handle((size_t)3);
        visitor.handle((long)4);
        visitor.handle(true);
        std::cout << std::endl;
    }
    void visit(FloatingValueViewer<void>& visitor) const override {
        std::cout << "TestViewable::visit(FloatingValueViewer<void>& visitor) const" << std::endl;
        visitor.handle((float)0.5);
        visitor.handle((double)0.25);
        std::cout << std::endl;
    }
    void visit(StringValueViewer<void>& visitor) const override {
        std::cout << "TestViewable::visit(StringValueViewer<void>& visitor) const" << std::endl;
        visitor.handle(std::string("String"));
        visitor.handle("CString");
        std::cout << std::endl;
    }
    
    void visit(ContainerValueViewer<void>& visitor) const override {
        std::cout << "TestViewable::visit(ContainerValueViewer<void>& visitor) const" << std::endl;
        TestViewable::visit((ListValueViewer<void>&) visitor);
        TestViewable::visit((MapValueViewer<void>&) visitor);
        std::cout << std::endl;
    }
    void visit(ListValueViewer<void>& visitor) const override {
        std::cout << "TestViewable::visit(ListValueViewer<void>& visitor) const" << std::endl;
        // visitor.handle("CString");
        std::cout << std::endl;
    }
    void visit(MapValueViewer<void>& visitor) const override {
        std::cout << "TestViewable::visit(MapValueViewer<void>& visitor) const" << std::endl;
        // visitor.handle("CString");
        std::cout << std::endl;
    }
}; 

class TestViewer: public ValueViewer<void> {
    void handle(long v) override { 
        std::cout << "TestViewer::handle(long v)" << std::endl;
    }
    void handle(size_t v) override { 
        std::cout << "TestViewer::handle(size_t v)" << std::endl;
    }
    void handle(int v) override { 
        std::cout << "TestViewer::handle(int v)" << std::endl;
    }
    void handle(bool v) override { 
        std::cout << "TestViewer::handle(bool v)" << std::endl;
    }
    
    void handle(float v) override { 
        std::cout << "TestViewer::handle(float v)" << std::endl;
    }
    void handle(double v) override { 
        std::cout << "TestViewer::handle(double v)" << std::endl;
    }
    
    void handle(MapIndexType) override { 
        std::cout << "TestViewer::handle(const std::string&)" << std::endl;
    }
    void handle(const char*) override { 
        std::cout << "TestViewer::handle(const char*)" << std::endl;
    }
    
    void handle(ContiguousDataView<ListIndexType> v) override { 
        std::cout << "TestViewer::handle(ContiguousDataView<ListIndexType> v)" << std::endl;
    }
    void handle(ContiguousDataView<size_t> v) override { 
        std::cout << "TestViewer::handle(ContiguousDataView<size_t> v)" << std::endl;
    }
    void handle(ContiguousDataView<int> v) override { 
        std::cout << "TestViewer::handle(ContiguousDataView<int> v)" << std::endl;
    }
    void handle(ContiguousDataView<bool> v) override { 
        std::cout << "TestViewer::handle(ContiguousDataView<bool> v)" << std::endl;
    }
    
    void handle(ContiguousDataView<float> v) override { 
        std::cout << "TestViewer::handle(ContiguousDataView<float> v)" << std::endl;
    }
    void handle(ContiguousDataView<double> v) override { 
        std::cout << "TestViewer::handle(ContiguousDataView<double> v)" << std::endl;
    }
    
    void handle(ContiguousDataView<unsigned char> v) override { 
        std::cout << "TestViewer::handle(ContiguousDataView<unsigned char> v)" << std::endl;
    }
    
    void handle(const ViewableMapValue& v) override { 
        std::cout << "TestViewer::handle(ViewableMapValue v)" << std::endl;
    }
    void handle(const ViewableListValue& v) override { 
        std::cout << "TestViewer::handle(ViewableListValue v)" << std::endl;
    }
};



 
int testExample1()
{   
  TestViewable viewable;
  TestViewer viewer;
  
  // Sizeof viewable is 72 - 9 Pointers to different vtable of Visitables
  // (clang 14, vvtable for classes, vftable for 8 visit overloads + virtual destructors and vbase_offset, vcall_offset, offset_to_top,...)
  std::cout << "Sizeof(viewable): " << sizeof(TestViewable) << std::endl;
  // Sizeof viewer is 160 (168 for C++11) - 20 Pointers to different vtables of Visitors and Visitor groups - as well as single visitors... these swallow memory
  // (clang 14, vvtable for 8 classes, vftable for 16 handle overloads + virtual destructuors and baseoffsets, 
  // unfortunately the SingleVistor also get one vtable per each with 3 entries (offset_to_top, RTTI, handle() pointer).
  std::cout << "Sizeof(viewer): " << sizeof(TestViewer) << std::endl;
  
  // Call some specialized visit to demonstrate specialization in output.
  viewable.visit(viewer);
  // viewable.visit((ScalarValueViewer<void>&)viewer);
  // viewable.visit((NumericValueViewer<void>&)viewer);
  // viewable.visit((StringValueViewer<void>&)viewer);
  // viewable.visit((IntegralValueViewer<void>&)viewer);
  // viewable.visit((FloatingValueViewer<void>&)viewer);
  
  // viewable.visit((ContiguousValueViewer<void>&)viewer);
  // viewable.visit((NumericContiguousValueViewer<void>&)viewer);
  // viewable.visit((BinaryContiguousValueViewer<void>&)viewer);
  
#if __cplusplus >= 201703L
  viewable.visit(freeVisitor<NumericValueViewer<void>>(
    // Read https://en.cppreference.com/w/cpp/utility/variant/visit to see why we should use auto here
    // Auto lambda only possible in c++14 or higher
    [](auto) -> void{
        std::cout << "freeVisitor<NumericValueViewer<void>> ... conversion to int" << std::endl;
    },
    [](double) -> void{
        std::cout << "freeVisitor<NumericValueViewer<void>> ... conversion to double" << std::endl;
    },
    [](float) -> void{
        std::cout << "freeVisitor<NumericValueViewer<void>> ... conversion to double" << std::endl;
    }
  )); 
#endif
  // Implicit conversion forces us to define long and size_t because conversion to double is possible. Auto would solve that but that's C++17
  viewable.visit(freeVisitor<NumericValueViewer<void>>(
    [](long) -> void{
        std::cout << "freeVisitor<NumericValueViewer<void>> ... conversion to long" << std::endl;
    },
    [](size_t) -> void{
        std::cout << "freeVisitor<NumericValueViewer<void>> ... conversion to size_t" << std::endl;
    },
    [](int) -> void{
        std::cout << "freeVisitor<NumericValueViewer<void>> ... conversion to int" << std::endl;
    },
    [](double) -> void{
        std::cout << "freeVisitor<NumericValueViewer<void>> ... conversion to double" << std::endl;
    },
    [](float) -> void{
        std::cout << "freeVisitor<NumericValueViewer<void>> ... conversion to double" << std::endl;
    }
  )); 

  
  // By composing properly structured viewers we don't have this problem
  viewable.visit(
    composedVisitor<NumericValueViewer<void>>(
        freeVisitor<IntegralValueViewer<void>>(
            // or use auto here...
            [](int) -> void{
                std::cout << "freeVisitor<IntegralValueViewer<void>> ... conversion to int" << std::endl;
            }),
        freeVisitor<FloatingValueViewer<void>>(
            // or use auto here...
            [](double) -> void{
                std::cout << "freeVisitor<IntegralValueViewer<void>> ... conversion to double" << std::endl;
            })
    )); 
  return 0;
}



// More complex C++17
#if __cplusplus >= 201703L
// Test example
#include <type_traits>
#include <variant>
#include <unordered_map>
#include <vector>

using GenericValueHolder = std::variant<long, size_t, int, bool, double, float, std::string, const char*, std::vector<ListIndexType>, std::vector<size_t>, std::vector<int>, std::vector<float>, std::vector<double>, std::vector<unsigned char>, std::shared_ptr<ViewableListValue>, std::shared_ptr<ViewableMapValue>>;


template<class SmartPointer>
struct get_smart_pointer_type { using type = void; };

template<typename T>
struct get_smart_pointer_type<std::shared_ptr<T>> { using type = T;};
template<typename T>
struct get_smart_pointer_type<std::unique_ptr<T>> { using type = T;};

template<class SmartPointer>
using get_smart_pointer_type_t = typename get_smart_pointer_type<SmartPointer>::type;


template<class ContainerType>
struct get_contiguous_data_type { using type = void; };

template<typename T, typename Allocator>
struct get_contiguous_data_type<std::vector<T, Allocator>> { using type = T;};
template<typename T>
struct get_contiguous_data_type<ContiguousDataView<T>> { using type = T;};

template<class ContainerType>
using get_contiguous_data_type_t = typename get_contiguous_data_type<ContainerType>::type;


template<template<typename A, typename B> class Predicate>
struct predicate_template {};

template<typename A, typename B>
using test_some_or_convertible = std::integral_constant<bool,
        (std::is_trivial_v<decay_t<A>> 
            ? std::is_same_v<decay_t<A>, decay_t<B>>  
            : ( 
                std::is_convertible_v<A, B> || 
                std::is_base_of_v<B, A>
            ) 
        )>;
        
        
template<typename V>
void callFirstOfType(V&&) {
    // Nothing matched, do nothing...
}

template<
    typename V, 
    template<typename A, typename B> class Predicate,
    typename ... TS, 
    typename Func, 
    typename ... MoreFuncs, 
    std::enable_if_t<is_one_of_v<Predicate, V, TS...>, bool> = true>
void callFirstOfType(V&& v, predicate_template<Predicate>, type_list<TS...>, Func&& func, MoreFuncs&& ...args) {
    std::forward<Func>(func)(std::forward<V>(v));
}

template<
    typename V, 
    template<typename A, typename B> class Predicate,
    typename ... TS, 
    typename Func, 
    typename ... MoreFuncs, 
    std::enable_if_t<!is_one_of_v<Predicate, V, TS...>, bool> = true>
void callFirstOfType(V&& v, predicate_template<Predicate>, type_list<TS...>, Func&& func, MoreFuncs&& ...args) {
    callFirstOfType(std::forward<V>(v), std::forward<MoreFuncs>(args)...);
}

// Add default predicate
template<typename V, typename ... TS, typename ... Args>
void callFirstOfType(V&& v, type_list<TS...>, Args&& ...args) {
    callFirstOfType(std::forward<V>(v), predicate_template<test_some_or_convertible>{}, type_list<TS...>{}, std::forward<Args>(args)...);
}




template<typename A, typename B>
using test_for_smartpointer = std::is_same<get_smart_pointer_type_t<decay_t<A>>, decay_t<B>>; // For smart pointers, we check if the unpacked types equal...
template<typename A, typename B>
using test_for_contiguous_data = std::is_same<typename ToContiguousDataView<decay_t<A>>::type, decay_t<B>>; // For smart pointers, we check if the unpacked types equal...
    

template<typename VariantType = GenericValueHolder>
class Value: public CRTPVisitable<ViewableValue, Value<VariantType>> {
private:
    VariantType val_;
    
public:
    template<typename T>
    Value(T&& v): val_(std::forward<T>(v)) {}
    
    using CRTPVisitable<ViewableValue, Value<VariantType>>::visit;
    
    template<typename TViewer, typename TLIST = typename TViewer::TypeList>
    void visit(TViewer& visitor, TLIST = TLIST{}) const {
        std::visit([&visitor](const auto& v){
            callFirstOfType(v,
                // Check if v is a scalar value
                intersect_type_list_t<TLIST, ScalarValueViewer<void>::TypeList>{}, [&visitor](const auto& scalar){
                    visitor.handle(scalar);
                },
                // if not, check if is v contiguous array
                predicate_template<test_for_contiguous_data>{}, intersect_type_list_t<TLIST, ContiguousValueViewer<void>::TypeList>{}, [&visitor](const auto& vector){
                    using VT = decay_t<decltype(vector[0])>;
                    visitor.handle(ContiguousDataView<VT>{vector.data(), vector.size()});
                },
                // if not, check if is v is one of the container classes wrapped in a smart pointer.
                predicate_template<test_for_smartpointer>{}, intersect_type_list_t<TLIST, ContainerValueViewer<void>::TypeList>{}, [&visitor](const auto& upointer){
                    visitor.handle(*upointer.get());
                }
            );
        }, val_);
    }
};


template<typename VariantType = GenericValueHolder>
class Map: public CRTPVisitable<ViewableMap<ValueViewer<MapIndexType>>, Map<VariantType>>, public ViewableMapValue {
private:
    std::unordered_map<std::string, VariantType> val_;
    
public:
    virtual ~Map() =default;
    
    Map(std::initializer_list<std::pair<const std::string, VariantType>> l): val_{l} {}
    
    // Load rvalue overloads
    // using ViewableMapValue::visit;
    // using ViewableMapValue::iterate;
    // using ViewableMapValue::size;
    
    using CRTPVisitable<ViewableMap<ValueViewer<MapIndexType>>, Map<VariantType>>::visit;
    using CRTPVisitable<ViewableMap<ValueViewer<MapIndexType>>, Map<VariantType>>::iterate;
    using CRTPVisitable<ViewableMap<ValueViewer<MapIndexType>>, Map<VariantType>>::size;
    
    virtual size_t size() const override{
        return val_.size();
    };
    
    template<typename TViewer, typename TLIST = typename TViewer::TypeList>
    void visit(MapIndexType k, TViewer& visitor, TLIST = TLIST{}) const {
        auto it = val_.find(k);
        if(it == val_.end()) return;
        std::visit([&k, &visitor](const auto& v){
            callFirstOfType(v,
                // Check if v is a scalar value
                intersect_type_list_t<TLIST, ScalarValueViewer<MapIndexType>::TypeList>{}, [&k, &visitor](const auto& scalar){
                    visitor.handle(k, scalar);
                },
                // if not, check if is v contiguous array
                predicate_template<test_for_contiguous_data>{}, intersect_type_list_t<TLIST, ContiguousValueViewer<MapIndexType>::TypeList>{}, [&k, &visitor](const auto& vector){
                    using VT = decay_t<decltype(vector[0])>;
                    visitor.handle(k, ContiguousDataView<VT>{vector.data(), vector.size()});
                },
                // if not, check if is v is one of the container classes wrapped in a smart pointer.
                predicate_template<test_for_smartpointer>{}, intersect_type_list_t<TLIST, ContainerValueViewer<MapIndexType>::TypeList>{}, [&k, &visitor](const auto& upointer){
                    visitor.handle(k, *upointer.get());
                }
            );
        }, it->second);
    }
    
    template<typename TViewer, typename TLIST = typename TViewer::TypeList> 
    void iterate(TViewer& visitor, TLIST = TLIST{}) const {
        for(const auto& p: val_) {
            bool cont = true;
            std::visit([&cont, &k = p.first, &visitor](auto&& v){
                // std::cout << "Iterate " << type_name<TLIST>() << " for " << k << "(" << type_name<decltype(v)>() << ")" << std::endl;
                // std::cout << "intersect container: " << type_name<intersect_type_list_t<TLIST, ContainerValueViewer<MapIndexType>::TypeList>>() << std::endl;
                callFirstOfType(v,
                    // Check if v is a scalar value
                    intersect_type_list_t<TLIST, ScalarValueViewer<MapIndexType>::TypeList>{}, [&cont, &k, &visitor](const auto& scalar){
                        cont = visitor.handle(k, scalar);
                    },
                    // if not, check if is v contiguous array
                    predicate_template<test_for_contiguous_data>{}, intersect_type_list_t<TLIST, ContiguousValueViewer<MapIndexType>::TypeList>{}, [&cont, &k, &visitor](const auto& vector){
                        using VT = decay_t<decltype(vector[0])>;
                        cont = visitor.handle(k, ContiguousDataView<VT>{vector.data(), vector.size()});
                    },
                    // if not, check if is v is one of the container classes wrapped in a smart pointer.
                    predicate_template<test_for_smartpointer>{}, intersect_type_list_t<TLIST, ContainerValueViewer<MapIndexType>::TypeList>{}, [&cont, &k, &visitor](const auto& upointer){
                        cont = visitor.handle(k, *upointer.get());
                    }
                );
            }, p.second);

            if(!cont) return;
        }
    }
};


template<typename VariantType = GenericValueHolder>
class List: public CRTPVisitable<ViewableList<ValueViewer<ListIndexType>>, List<VariantType>>, public ViewableListValue {
private:
    std::vector<VariantType> val_;
    
public:
    virtual ~List() =default;
    
    // template<typename ...TS>
    // List(TS&& ...vs): val_(std::forward<TS>(vs)...) {}
    List(std::initializer_list<VariantType> l): val_{l} {}
    
    // Load rvalue overloads
    using ViewableListValue::visit;
    using ViewableListValue::iterate;
    using ViewableListValue::size;
    
    virtual size_t size() const override {
        return val_.size();
    };
    
    template<typename TViewer, typename TLIST = typename TViewer::TypeList>
    void visit(ListIndexType i, TViewer& visitor, TLIST = TLIST{}) const {
        if(i < 0 || i >= val_.size()) return;
        std::visit([&i, &visitor](const auto& v){
            callFirstOfType(v,
                // Check if v is a scalar value
                intersect_type_list_t<TLIST, ScalarValueViewer<MapIndexType>::TypeList>{}, [&i, &visitor](const auto& scalar){
                    visitor.handle(i, scalar);
                },
                // if not, check if is v contiguous array
                predicate_template<test_for_contiguous_data>{}, intersect_type_list_t<TLIST, ContiguousValueViewer<MapIndexType>::TypeList>{}, [&i, &visitor](const auto& vector){
                    using VT = decay_t<decltype(vector[0])>;
                    visitor.handle(i, ContiguousDataView<VT>{vector.data(), vector.size()});
                },
                // if not, check if is v is one of the container classes wrapped in a smart pointer.
                predicate_template<test_for_smartpointer>{}, intersect_type_list_t<TLIST, ContainerValueViewer<MapIndexType>::TypeList>{}, [&i, &visitor](const auto& upointer){
                    visitor.handle(i, *upointer.get());
                }
            );
        }, val_[i]);
    }
    
    template<typename TViewer, typename TLIST = typename TViewer::TypeList> 
    void iterate(TViewer& visitor, TLIST = TLIST{}) const {
        for(long i = 0; i < val_.size(); ++i) {
            bool cont = true;
            std::visit([&cont, &i, &visitor](auto&& v){
                callFirstOfType(v,
                    // Check if v is a scalar value
                    intersect_type_list_t<TLIST, ScalarValueViewer<MapIndexType>::TypeList>{}, [&cont, &i, &visitor](const auto& scalar){
                        cont = visitor.handle(i, scalar);
                    },
                    // if not, check if is v contiguous array
                    predicate_template<test_for_contiguous_data>{}, intersect_type_list_t<TLIST, ContiguousValueViewer<MapIndexType>::TypeList>{}, [&cont, &i, &visitor](const auto& vector){
                        using VT = decay_t<decltype(vector[0])>;
                        cont = visitor.handle(i, ContiguousDataView<VT>{vector.data(), vector.size()});
                    },
                    // if not, check if is v is one of the container classes wrapped in a smart pointer.
                    predicate_template<test_for_smartpointer>{}, intersect_type_list_t<TLIST, ContainerValueViewer<MapIndexType>::TypeList>{}, [&cont, &i, &visitor](const auto& upointer){
                        cont = visitor.handle(i, *upointer.get());
                    }
                );
            }, val_[i]);
            if(!cont) return;
        }
    }
};

    
    
    
    
    
//-----------------------------------------------------------------------------


inline std::string depthString(int depth) {
    return std::string(depth*2, ' ');
}

# include <iomanip>
inline void printData(std::ostream& out, const unsigned char* data, size_t len) {
    for(int i=0; i<len; ++i) { 
        out << std::setw(2) << std::setfill('0') << std::hex << (int) data[i] << " ";
    }
}
                    
int testGenericValue() {
    std::function<std::unique_ptr<ValueViewer<MapIndexType>>(int)> makeMapViewer;
    std::function<std::unique_ptr<ValueViewer<ListIndexType>>(int)> makeListViewer;
    
    makeMapViewer = [&makeMapViewer, &makeListViewer](int depth) {
        auto lambda = composedVisitor<ValueViewer<MapIndexType>>(
            freeVisitor<ScalarValueViewer<MapIndexType>>(
                [depth](MapIndexType k, auto v) -> bool{
                    std::cout << depthString(depth)  << k << ": " << v << std::endl;
                    return true;
                }),
            freeVisitor<ContiguousValueViewer<MapIndexType>>(
                [depth](MapIndexType k, auto v) -> bool{
                    std::cout << depthString(depth) << k << ": ";
                    printData(std::cout, reinterpret_cast<const unsigned char*>(v.data), v.size * sizeof(typename decltype(v)::type));
                    std::cout << std::endl;
                    return true;
                }),
            composedVisitor<ContainerValueViewer<MapIndexType>>(
                freeVisitor<MapValueViewer<MapIndexType>>(
                    [depth, &makeMapViewer](MapIndexType k, auto& v) -> bool{
                        std::cout << depthString(depth) << k << ": {" << std::endl;
                        auto nestedMapViewer = makeMapViewer(depth+1);
                        v.iterate(*nestedMapViewer.get());
                        std::cout << depthString(depth) << "}" << std::endl;
                        return true;
                    }),
                freeVisitor<ListValueViewer<MapIndexType>>(
                    [depth, &makeListViewer](MapIndexType k, auto& v) -> bool{
                        std::cout << depthString(depth) << k << ": [" << std::endl;
                        auto nestedListViewer = makeListViewer(depth+1);
                        v.iterate(*nestedListViewer.get());
                        std::cout << depthString(depth) << "]" << std::endl;
                        return true;
                })
            )
        );
        return std::make_unique<decay_t<decltype(lambda)>>(std::move(lambda));
    };
    
    makeListViewer = [&makeMapViewer, &makeListViewer](int depth) {
        auto lambda = composedVisitor<ValueViewer<ListIndexType>>(
            freeVisitor<ScalarValueViewer<ListIndexType>>(
                [depth](ListIndexType i, auto v) -> bool{
                    std::cout << depthString(depth) << "#" << i << ": " << v << std::endl;
                    return true;
                }),
            freeVisitor<ContiguousValueViewer<ListIndexType>>(
                [depth](ListIndexType i, auto v) -> bool{
                    std::cout << depthString(depth) << "#" << i << ": ";
                    printData(std::cout, reinterpret_cast<const unsigned char*>(v.data), v.size * sizeof(typename decltype(v)::type));
                    std::cout << std::endl;
                    return true;
                }),
            composedVisitor<ContainerValueViewer<ListIndexType>>(
                freeVisitor<MapValueViewer<ListIndexType>>(
                    [depth, &makeMapViewer](ListIndexType i, auto& v) -> bool {
                        std::cout << depthString(depth) << "#" << i << ": {" << std::endl;
                        auto nestedMapViewer = makeMapViewer(depth+1);
                        v.iterate(*nestedMapViewer.get());
                        std::cout << depthString(depth) << "}" << std::endl;
                        return true;
                    }),
                freeVisitor<ListValueViewer<ListIndexType>>(
                    [depth, &makeListViewer](ListIndexType i, auto& v) -> bool {
                        std::cout << depthString(depth) << "#" << i << ": [" << std::endl;
                        auto nestedListViewer = makeListViewer(depth+1);
                        v.iterate(*nestedListViewer.get());
                        std::cout << depthString(depth) << "]" << std::endl;
                        return true;
                    })
            )
        );
        return std::make_unique<decay_t<decltype(lambda)>>(std::move(lambda));
    };
    
    // Map & ListViewer are stored as abstract ValueViewer, using the dynamic interface to break cyclic dependency.
    // Using the fully templated handle calls is possible by creating explict functions
    auto mapViewer = makeMapViewer(0);
    auto listViewer = makeListViewer(0);
    
    Map m{ 
        { {"a", 0.0f}  
        , {"b", 1L}  
        , {"c", "c-string"}  
        , {"d", std::string("c++ string")} 
        , {"e", std::make_shared<List<>>(List<>{
            { "XX"
            , 2
            , "3"
            , std::make_shared<Map<>>(Map<>{
                { {"deep", "nested"}
                , {"a", true}
              }})
            , std::make_shared<List<>>(List<>{
                { 1
                , "2"
                , "three"
                , std::make_shared<Map<>>(Map<>{
                    { {"very deep", "nested"}
                    , {"and so", "on"}
                    , {"some data", std::vector<unsigned char>{1,2,3,4}}
                  }})
              }})
            }})
          }
    }};
    // Call templated interface, templated iterate call is hiding virtual methods of ViewableMapValue
    m.iterate(*mapViewer.get());
    
    // Explicitly call dynamic interafces 
    std::cout << std::endl << "Explicitly call dynamic interafces" << std::endl;
    const ViewableMapValue& abstractM = (const ViewableMapValue&) m;
    abstractM.iterate(*mapViewer.get());
    
    
    // Lambad as visitor
    auto valueVisitor = composedVisitor<ValueViewer<void>>(
            freeVisitor<ScalarValueViewer<void>>(
                [](auto v) -> bool{
                    std::cout << v << std::endl;
                    return true;
                }),
            freeVisitor<ContiguousValueViewer<void>>(
                [](auto v) -> bool{
                    printData(std::cout, reinterpret_cast<const unsigned char*>(v.data), v.size * sizeof(typename decltype(v)::type));
                    std::cout << std::endl;
                    return true;
                }),
            composedVisitor<ContainerValueViewer<void>>(
                freeVisitor<MapValueViewer<void>>(
                    [&makeMapViewer](auto& v) -> bool {
                        auto nestedMapViewer = makeMapViewer(0);
                        v.iterate(*nestedMapViewer.get());
                        std::cout << std::endl;
                        return true;
                    }),
                freeVisitor<ListValueViewer<void>>(
                    [&makeListViewer](auto& v) -> bool {
                        auto nestedListViewer = makeListViewer(0);
                        v.iterate(*nestedListViewer.get());
                        std::cout << std::endl;
                        return true;
                    })
            )
        );
    // View a value
    std::cout << std::endl << "Print some values..." << std::endl;
    Value v = std::string("test value");
    // Real templated call
    v.visit(valueVisitor);
    
    v = 0.1;
    v.visit(valueVisitor);
    
    v = "c-string";
    v.visit(valueVisitor);
    
    v = true;
    v.visit(valueVisitor);
    
    v = 1234;
    v.visit(valueVisitor);
    
    v = std::vector<double>{{0.1, 0.2, 0.3}};
    v.visit(valueVisitor);


    return 0;
};

#endif


 
int main()
{   
    testExample1();
#if __cplusplus >= 201703L
    testGenericValue();
#endif
    return 0;
};
