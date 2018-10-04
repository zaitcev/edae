#include <FileSystem>
#include <MailSystem>
#include <SysErrors>
#include "UULibDef"
#include "mainDef"

int
rbmail(mto, datalab, misscode)
char **mto;
char datalab;                 /* Для статистики и для "Date: *" */
boolean misscode;             /* FALSE - КОИ-8, TRUE - MISS кодировка */
{
  short             retcode;
  FileItem         *aitem=GetFileItem(datalab);
  static FileItem   irbmail=
    {FMail,FilAccNo,1,0,{pwd=1<<5},{mail=0,0,8},"          ",NullProc};

  mto=mto; misscode=misscode;
  if (irbmail.processor.c[0]==0) {
    static UnpFproc procname={0, InterpreterFlag, '~', "rbmail    "};
    _FPpack(&procname,&irbmail.processor);
  } //if
  /* Перенаправляем на "~'RBMAIL", он создаст нормальные файлы */
  if (aitem==0) {
    retcode=ErrFile;
  } else {
    irbmail.name=aitem->name;
    contim( &irbmail.info.mail.date, &irbmail.info.mail.time );
    retcode=SendMail(&irbmail, datalab);
  } //if
  return retcode ? retcode : 1;         //1 - файл перенаправлен
} //rbmail
