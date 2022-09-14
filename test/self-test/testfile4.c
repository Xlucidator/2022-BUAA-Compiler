#include "selfio.h"
/*=========== testfile4 : Level C ===========*/
/*
 * write for check & fun & Stmt
 */
int a = 1;

void p(int i) {
    printf("%d\n", i);
    return;
}

void pa() { p(a); }

int main() {
    printf("20373585\n");

    p(a);
    int a = 2;
    p(a);
    {
        int a = 3;
        p(a);
        {
            int a = 4;
            p(a);
            {
                int a = 5;
                p(a);
                {

                }
                {;}
                p(a);
            }
            p(a);
        }
        p(a);
    }
    p(a);
    pa();
    while (1) {
        int i = 1;
        if (i == 2) continue;
        break;
    }
    return 0;
}
