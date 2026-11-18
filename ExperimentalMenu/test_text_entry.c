#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <wchar.h>
#include "text_entry_model.h"
#define CHECK(c) do{if(!(c)){fprintf(stderr,"FAIL %d: %s\n",__LINE__,#c);return 1;}}while(0)
int main(void){
    TextEntryModel m;text_model_init(&m,L"https://example.com",2047);text_model_key(&m,VK_HOME,FALSE);text_model_key(&m,VK_RIGHT,TRUE);CHECK(m.anchor==0&&m.cursor==1);text_model_insert(&m,L"H",1);CHECK(!wcscmp(m.value,L"Https://example.com"));
    m.anchor=0;m.cursor=(unsigned)wcslen(m.value);text_model_insert(&m,L"くちづけ",4);CHECK(!wcscmp(m.value,L"くちづけ"));text_model_key(&m,VK_BACK,FALSE);CHECK(!wcscmp(m.value,L"くちづ"));
    text_model_key(&m,VK_HOME,FALSE);text_model_key(&m,VK_DELETE,FALSE);CHECK(!wcscmp(m.value,L"ちづ"));
    const WCHAR emoji[]={0xd83d,0xde00,0};text_model_init(&m,L"",4);text_model_insert(&m,emoji,2);CHECK(m.cursor==2);text_model_key(&m,VK_LEFT,FALSE);CHECK(!m.cursor);text_model_key(&m,VK_DELETE,FALSE);CHECK(!*m.value);
    text_model_insert(&m,emoji,2);text_model_key(&m,VK_BACK,FALSE);CHECK(!*m.value);
    text_model_init(&m,L"abc",3);text_model_insert(&m,L"d",1);CHECK(!wcscmp(m.value,L"abc"));m.anchor=0;text_model_insert(&m,L"xy",2);CHECK(!wcscmp(m.value,L"xy"));
    text_model_init(&m,L"",1);text_model_insert(&m,emoji,2);CHECK(!*m.value);text_model_insert(&m,L"\n",1);CHECK(!*m.value);
    text_model_init(&m,L"abcd",4);m.anchor=1;m.cursor=3;text_model_key(&m,VK_LEFT,FALSE);CHECK(m.cursor==1&&m.anchor==1);m.anchor=1;m.cursor=3;text_model_key(&m,VK_RIGHT,FALSE);CHECK(m.cursor==3&&m.anchor==3);
    static TextEntryHistory history;TextEntryModel before;
    text_model_init(&m,L"",2047);text_history_init(&history,&m);
    before=m;text_model_insert(&m,L"a",1);text_history_record(&history,&before,&m,FALSE);
    before=m;text_model_insert(&m,L"b",1);text_history_record(&history,&before,&m,TRUE);
    CHECK(!wcscmp(m.value,L"ab")&&history.count==2);CHECK(text_history_step(&history,&m,FALSE)&&!*m.value);CHECK(!text_history_step(&history,&m,FALSE));
    CHECK(text_history_step(&history,&m,TRUE)&&!wcscmp(m.value,L"ab"));CHECK(!text_history_step(&history,&m,TRUE));
    m.anchor=0;before=m;text_model_insert(&m,L"くちづけ",4);text_history_record(&history,&before,&m,FALSE);
    CHECK(text_history_step(&history,&m,FALSE)&&!wcscmp(m.value,L"ab")&&m.anchor==0&&m.cursor==2);
    CHECK(text_history_step(&history,&m,TRUE)&&!wcscmp(m.value,L"くちづけ"));CHECK(text_history_step(&history,&m,FALSE));
    before=m;text_model_insert(&m,L"x",1);text_history_record(&history,&before,&m,FALSE);CHECK(!text_history_step(&history,&m,TRUE)&&!wcscmp(m.value,L"x"));
    for(unsigned i=0;i<100;i++){before=m;text_model_insert(&m,L"x",1);text_history_record(&history,&before,&m,FALSE);}
    CHECK(history.count==TEXT_ENTRY_HISTORY);unsigned undos=0;while(text_history_step(&history,&m,FALSE))undos++;CHECK(undos==TEXT_ENTRY_HISTORY-1);
    text_model_init(&m,L"new field",2047);text_history_init(&history,&m);CHECK(!text_history_step(&history,&m,FALSE)&&!text_history_step(&history,&m,TRUE));
    puts("PASS: text insertion/replacement, selection, caret, home/end, deletion, single-line bounds, UTF-16 integrity, grouped undo/redo, branch replacement and bounded independent history");return 0;
}
