using System;
using ReRiff;
class KanaSearchSmoke {
    static int count;
    static void Check(bool value,string name){count++;if(!value)throw new Exception(name);}
    static void Main(){
        foreach(string name in new[]{"つ","ツ","ﾂ"})Check(KanaSearch.Matches(name,"tsu"),"tsu matches all kana forms");
        Check(KanaSearch.Matches("BUCK‐TICK - くちづけ.mp3","kuchizuke"),"Mixed filename search");
        Check(KanaSearch.Matches("ミスターフィクサー","misutaafikusaa"),"Foreign sounds and prolonged vowels");
        Check(KanaSearch.Matches("カタカナ","かたかな")&&KanaSearch.Matches("ひらがな","ヒラガナ"),"Cross-script search");
        Check(KanaSearch.Matches("がくせい","か\u3099く")&&KanaSearch.Matches("ｶﾞｸｾｲ","gaku"),"Composed/decomposed/halfwidth normalization");
        Check(KanaSearch.Key("きゃぎゅしょじゃちゅにゃひょびゃぴゅみょりゃ")=="kyagyushojachunyahyobyapyumyorya","Digraph table");
        Check(KanaSearch.Key("まっちゃ")=="matcha"&&KanaSearch.Key("がっこう")=="gakkou","Small tsu doubling");
        Check(KanaSearch.Key("しんよう")=="shin'you"&&KanaSearch.Key("しにょう")=="shinyou","Moraic n boundary");
        Check(KanaSearch.Matches("DAWID PODSIADŁO.flac","podsiadło"),"Existing Latin case-insensitive search");
        Check(!KanaSearch.Matches("月下麗人","gekka")&&KanaSearch.Matches("月下麗人","月下"),"Kanji literal search only");
        Check(!KanaSearch.Matches("つ","shi")&&KanaSearch.Matches("anything","")&&!KanaSearch.Matches(null,"a"),"No-match and empty/null handling");
        string original="BUCK‐TICK - くちづけ.mp3";KanaSearch.Matches(original,"kuchizuke");Check(original=="BUCK‐TICK - くちづけ.mp3","Original filename preserved");
        Console.WriteLine("PASS: {0} kana/romaji matching, normalization, mixed scripts and original-name preservation checks",count);
    }
}
