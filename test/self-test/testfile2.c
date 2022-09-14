#include "selfio.h"
/*=========== testfile2 : Level B ===========*/
/*
 * try to cover Cond/Exp
 */
const int a=4, b=6, c=7;
int arr[10] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};

void test_cond() {
    if (-+-!arr[0]) printf("test UnaryExp ok\n");
    if (3%5/2*+arr[1]*-0) ; else printf("test MulExp ok\n");
    if (!(arr[2]*4-5*3+7%8)) printf("test AddExp ok\n");
    if (4*2 < arr[3]+5) ; 
        else if (4*2 <=arr[3]+4) ; 
        else if (4*2 > arr[3]+5) ;
        else if (4*2 >=arr[3]+5) printf("test RelExp ok\n");
    if (3%arr[4]*2 != 2) printf("test EqExp ok!\n");
    if (arr[5] == arr[6]-1 && arr[6]+1 != 2+arr[5]) ; else printf("test LAndExp ok\n");
    if (!(arr[9]-9) && -1 || !arr[7]-arr[8]) printf("test LOrExp ok!\n");
}

int main() {
    printf("20373585\n");
    test_cond();
    return 0;
}
