#include "pcrt.h"
//    #define __PLACEMENT_NEW_INLINE
#include <utility>


primal::string funcret1()
{
    return primal::string("Return World");
}

primal::string funcret2()
{
    primal::string s = primal::string("Return World");
    return s;
}

void func1(primal::string p)
{
    printv("func1 pass-by-value ");
    printv("primal::string ", p.cz());
}

void func2(primal::string &p)
{
    printv("func1 pass-by-ref: ");
    printv("primal::string ", p.cz());
}

void func3(const primal::string &p)
{
    printv("func1 pass-by-const-ref: ");
    printv("primal::string ", p.cz());
}
void testString()
{
    primal::string A;
    primal::string B("Hello world");
    primal::string C(B);

    primal::string D = B;
    A = C;

    func1(B);
    func2(B);
    func3(B);
    func1(std::move(B));
    func1(primal::string("New World"));
    func1("Hi there");
    func1(funcret1());
    func1(funcret2());

    primal::SsoBuffer<20> a;
    auto b = std::move(a);
}

void pcrtmain()
{
    int32 i = 9001010;
    // $2021, $i
    printv("Hello", "world\n", " another\n", "and another\n", _D(0xdeadbeef, 16), " ", _D(47));
    printv("Hello", "world", _D(2021));
    printv(_D(-2021), _D(i));
    printv(_D(0xFFF0F0F0FFF0F0F0, 2));
    // $(2021, rightPad(10), upper)
    //print(rightPad(_D(2021), 10))

#ifdef _DEBUG
    printv("Allocs:", _D(primal::sAllocCnt), " Frees: ", _D(primal::sFreeCnt));
#endif

}
