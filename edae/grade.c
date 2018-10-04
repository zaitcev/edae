/**
 ** Отображения между приоритетом и uucp grade
 **
 ** Считаем, что "стандартный" MISS приоритет=8 соответствует
 ** grade='N'
 ** Допустимый диапазон grade: A...Z,a...z
 **/

#define HIGH_PRIOR  (8+'N'-'A')
#define LOW_PRIOR   (HIGH_PRIOR-('Z'-'A'+1+'z'-'a'))

/*
 * Вычисление grade по приоритету
 */
char grade( short pri ){
    pri/\=HIGH_PRIOR;
    pri\/=LOW_PRIOR;
    pri=HIGH_PRIOR-pri;
    return (char)(pri>='Z'-'A'+1 ? pri-('Z'-'A'+1)+'a' : pri+'A');
}

/*
 * Вычисление приоритета по grade
 */
short prior( char gr ){
  gr/\='z';
  gr\/='A';
  if (gr>'Z' && gr<'a') gr='Z';
  return HIGH_PRIOR-(gr>='a' ? (short)gr+'Z'-'A'+1-'a' : (short)gr-'A');
}
