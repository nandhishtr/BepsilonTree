#include <iostream>

#include "BTree.h"
#include "NoCache.h"
#include "glog/logging.h"
#include <type_traits>

template<typename T = X>
class D {
public:
    D(T* t) : ptr(t) {}

    T* ptr;

    void print() 
    { 
        std::cout << "D" << std::endl; 
        std::cout << "..." << ptr->print() << std::endl;
    }
};

class X {
public:
    void print() { std::cout << "X" << std::endl; }
};

class C : public X {
public:
    void print() { std::cout << "C" << std::endl; }
};

class B : public X {
public:
    void print() { std::cout << "B" << std::endl; }
};

class A {
public:
    std::vector<std::shared_ptr<void>> vt;

    template <typename T>
    void add()
    {
        vt.push_back(std::make_shared<T>());
    }

    void get()
    {
        return vt.pop_back();
    }
};


#include <variant>
#include <typeinfo>

template <typename... Types>
std::pair<std::string, std::variant<Types...>> getCurrentValueAndType(const std::variant<Types...>& v) {
    std::string typeString;
    std::visit([&typeString](const auto& value) {
        typeString = typeid(value).name();
        }, v);

    return { typeString, v };
}

template <
    typename KeyType,
    template <typename, typename> typename Storage,
    template <typename, typename, typename...> typename ValueType, typename ValueCoreTypeA, typename ValueCoreTypeB, typename... ValueCoreTypes>
class _A {

public:
    Storage<KeyType, ValueType<ValueCoreTypeA, ValueCoreTypeB, ValueCoreTypes...>> _a;

    void add() {
        _a[0] = 1;
        _a[0] = '1';
    }
};

template <
    template <typename, template <typename, typename> typename, typename, typename> typename Storage, typename KeyType,
    template <typename, typename> typename ValueType, typename ValueCoreTypeA, typename ValueCoreTypeB, typename... ValueCoreTypesZ
>
class _B {
    Storage<KeyType, ValueType, ValueCoreTypeA, ValueCoreTypeB> _a;
};

template<typename T, typename... TT>
class CC {
public:
    void print()
    {
        std::cout << "CC" << std::endl;
    }
};

template<typename T>
class CC1 {
public:
    void print()
    {
        std::cout << "CC" << std::endl;
    }
};

template<typename T, typename TT>
class CC2 {
public:
    void print()
    {
        std::cout << "CC" << std::endl;
    }
};

template <size_t N, typename... Types>
struct NthType;

// Specialization for N == 0 (base case)
template <typename FirstType, typename... RemainingTypes>
struct NthType<0, FirstType, RemainingTypes...> {
    using type = FirstType;
};

// Specialization for N > 0 (recursive case)
template <size_t N, typename FirstType, typename... RemainingTypes>
struct NthType<N, FirstType, RemainingTypes...> {
    using type = typename NthType<N - 1, RemainingTypes...>::type;
};

template<typename T,
    template <typename...> typename ValueType, typename... ValueCoreTypeB>
class DD {
public:
    DD(T* t) : ptr(t) {}

    using type = typename NthType<0, ValueCoreTypeB...>::type;
    using type2 = typename NthType<1, ValueCoreTypeB...>::type;

    type d;
    type2 c;
    ValueType<ValueCoreTypeB...> x;
    std::variant<ValueCoreTypeB...> _;

    T* ptr;

    void print()
    {
        d = 1;
        c = 'p';
        _ = 1;
        _ = '1';
        _ = 1.0f;
        std::cout << "D" << std::endl;
    }
};


int main(int argc, char* argv[]) {

    DD<int, CC, int, char, float> xyz(NULL);
    xyz.print();
    //_B<DD, int, CC, int, int> __b;
    //NVRAMVolatileStorageEx<uintptr_t, NVRAMCacheObjectEx, int, float>* _aa = new NVRAMVolatileStorageEx<uintptr_t, NVRAMCacheObjectEx, int, float>(1);
    NVRAMLRUCache<NVRAMVolatileStorage, uintptr_t, NVRAMCacheObjectEx, int, float> __b(1);

    //BTree<int, int, NVRAMLRUCache<uintptr_t, NVRAMVolatileStorageEx<, NVRAMCacheObjectEx, int, float, char>>* m_ptrTree
    //    = new BTree<int, int, NVRAMLRUCache<NVRAMVolatileStorage, uintptr_t, INVRAMCacheObject, NVRAMCacheObject2, int>>(5, 10);

    _A<int, std::map, std::variant, float, int, char, double>* a = new _A<int, std::map, std::variant, float, int, char, double>();
    a->add();

    FLAGS_logtostderr = true;
    //fLS::FLAGS_log_dir = "D:/Study/OVGU/RA/code/haldindb";
    google::InitGoogleLogging(argv[0]);

    LOG(INFO) << "Sandbox Started.";

    //BTree<int, int, NoCache<std::shared_ptr<INVRAMCacheObject>>>* m_ptrTree = new BTree<int, int, NoCache<std::shared_ptr<INVRAMCacheObject>>>(5);
    BTree<int, int, NVRAMLRUCache<NVRAMVolatileStorage, uintptr_t, NVRAMCacheObject2, INVRAMCacheObject, int>>* m_ptrTree 
        = new BTree<int, int, NVRAMLRUCache<NVRAMVolatileStorage, uintptr_t, NVRAMCacheObject2, INVRAMCacheObject, int>>(5, 10);

    //auto a = new A();
    //a->add<B>();
    //a->add<C>();

    std::vector<std::variant<B*, C*, D<B>*, D<C>*>> vt;
    vt.push_back(new B());
    vt.push_back(new C());
    vt.push_back(new D<B>(new B()));
    vt.push_back(new D<C>(new C()));
    vt.push_back(new B());
    vt.push_back(new C());
    vt.push_back(new D<B>(new B()));
    vt.push_back(new D<C>(new C()));

    for (auto i = vt.begin(); i != vt.end(); i++)
    {
        //std::string typeString;
        //std::visit([&typeString](const auto& value) {
        //    typeString = typeid(value).name();
        //    std::get<B*>(value);
        //    }, *i);

        //std::get<std::string>(*i);
        //*i->print();

        //D d(new X());

        //if (std::holds_alternative<typename std::decay<B*>::type>(*i)) {
        //    std::get<typename std::decay<B*>::type>(*i)->print();
        //    //d = D<B>(std::get<typename std::decay<B*>::type>(*i));
        //}
        //else if (std::holds_alternative<typename std::decay<C*>::type>(*i)) {
        //    std::get<typename std::decay<C*>::type>(*i)->print();
        //}
        //else if (std::holds_alternative<typename std::decay<D<B>*>::type>(*i)) {
        //    std::get<typename std::decay<D<B>*>::type>(*i)->print();
        //}
        //else if (std::holds_alternative<typename std::decay<D<C>*>::type>(*i)) {
        //    std::get<typename std::decay<D<C>*>::type>(*i)->print();
        //}

        //d->print();
    }

    //obj _ = a->get();



    for (size_t nCntr = 0; nCntr < 1000; nCntr++)
    {
        m_ptrTree->insert(nCntr, nCntr);
    }

    for (size_t nCntr = 0; nCntr < 1000; nCntr++)
    {
        int nValue = 0;
        ErrorCode code = m_ptrTree->search(nCntr, nValue);

        std::cout << "K: " << nCntr << ", V: " << nValue << std::endl;
    }

    return 0;
}
