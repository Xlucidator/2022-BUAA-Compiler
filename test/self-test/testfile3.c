#include "selfio.h"
/*=========== testfile3 : Level B ===========*/
/*
 * try to cover Decl
 */
const int a = 1;
const int b = 2, c =3, d = 4;
const int aa[1] = {1}, ba[1+1] = {2, 3}, ca[4-1] = {1,2,3};
const int aaa[1][1] = {{2}}, baa[2*1][2%3] = {{1,(9+1)/2},{2,8/2}}; 

int va;
int vb, vc=3;
int vaa[2];
int vab[3], vac[1] = {1}, vad[3] = {0, 1%4, 2};
int vaaa[3][2], vaab[2][2] = {{0, 1}, {2, 3}};

void receive_d1(int a[], int n) {
    printf("a[0]:%d, n:%d\n", a[0], n);
}
void receive_d2(int a[][2], int n) {
    printf("a[1][1]:%d, n:%d\n", a[1][1], n);
}

int main() {
    printf("20373585\n");
    va = a;
    vaa[0] = c - a*2;   // 1
    vaa[1] = 2;
    vab[0] = vaa[0]; vab[1] = vab[1]; vab[2] = 2;
    vaaa[0][0+1] = 0*0+1;
    vaaa[1][1] = ba[0] + ca[2]; // 2 + 3 = 5
    receive_d1(vab, 3);
    receive_d1(vaab[1], 2); // vaab[1][0] = 2
    receive_d2(vaaa, 3);    // vaaa[1][1] = 5
    return 0;
}
