#include <stdio.h>

int main(void)
{
    int    satellites = 8;
    int   *p_sat      = &satellites;

    printf("satellites 變數位於記憶體位址: %p\n", (void *)&satellites);
    printf("p_sat 指標自己的位址:         %p\n", (void *)&p_sat);
    printf("p_sat 內容 (它指向哪裡):       %p\n", (void *)p_sat);
    printf("*p_sat 解參考後拿到的值:        %d\n", *p_sat);

    *p_sat = 12;
    printf("透過指標改寫後, satellites = %d\n", satellites);

    return 0;
}
