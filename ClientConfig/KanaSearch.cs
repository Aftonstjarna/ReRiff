using System;
using System.Collections.Generic;
using System.Text;

namespace ReRiff {
    // Search keys only. Never replace the displayed name, path or metadata.
    internal static class KanaSearch {
        private static readonly Dictionary<char,string> Kana=new Dictionary<char,string>();
        private static readonly Dictionary<string,string> Pairs=new Dictionary<string,string>();
        static KanaSearch(){
            string chars="ぁあぃいぅうぇえぉおかがきぎくぐけげこごさざしじすずせぜそぞただちぢっつづてでとどなにぬねのはばぱひびぴふぶぷへべぺほぼぽまみむめもゃやゅゆょよらりるれろゎわゐゑをんゔゕゖ";
            string[] keys="a a i i u u e e o o ka ga ki gi ku gu ke ge ko go sa za shi ji su zu se ze so zo ta da chi ji * tsu zu te de to do na ni nu ne no ha ba pa hi bi pi fu bu pu he be pe ho bo po ma mi mu me mo ya ya yu yu yo yo ra ri ru re ro wa wa wi we wo n vu ka ke".Split(' ');
            if(chars.Length!=keys.Length)throw new InvalidOperationException("Kana table length differs");
            for(int i=0;i<chars.Length;i++)Kana.Add(chars[i],keys[i]);
            string[] bases={"き","ぎ","し","じ","ち","ぢ","に","ひ","び","ぴ","み","り"};
            string[] stems={"ky","gy","sh","j","ch","j","ny","hy","by","py","my","ry"};
            string small="ゃゅょ",vowels="auo";
            for(int i=0;i<bases.Length;i++)for(int j=0;j<3;j++)Pairs[bases[i]+small[j]]=stems[i]+vowels[j];
            string[] extra={"いぇ:ye","うぃ:wi","うぇ:we","うぉ:wo","しぇ:she","じぇ:je","ちぇ:che","つぁ:tsa","つぃ:tsi","つぇ:tse","つぉ:tso","てぃ:ti","でぃ:di","とぅ:tu","どぅ:du","てゅ:tyu","でゅ:dyu","ふぁ:fa","ふぃ:fi","ふぇ:fe","ふぉ:fo","ふゅ:fyu","ゔぁ:va","ゔぃ:vi","ゔぇ:ve","ゔぉ:vo","ゔゅ:vyu"};
            foreach(string value in extra){string[] pair=value.Split(':');Pairs.Add(pair[0],pair[1]);}
        }
        internal static string Key(string text){
            if(text==null)throw new ArgumentNullException("text");
            text=text.Normalize(NormalizationForm.FormKC).ToLowerInvariant();
            var normalized=new StringBuilder(text.Length);
            foreach(char raw in text){char c=raw>='ァ'&&raw<='ヶ'?(char)(raw-0x60):raw;normalized.Append(c);}
            text=normalized.ToString();var result=new StringBuilder(text.Length);bool doubled=false;
            for(int i=0;i<text.Length;i++){
                char c=text[i];
                if(c=='っ'){doubled=true;continue;}
                if(c=='ー'){
                    if(result.Length>0&&"aeiou".IndexOf(result[result.Length-1])>=0)result.Append(result[result.Length-1]);
                    else result.Append(c);
                    doubled=false;continue;
                }
                string token;
                if(i+1<text.Length&&Pairs.TryGetValue(text.Substring(i,2),out token))i++;
                else if(!Kana.TryGetValue(c,out token))token=c.ToString();
                if(doubled){if(token.Length>0&&"bcdfghjklmpqrstvwxyz".IndexOf(token[0])>=0)result.Append(token.StartsWith("ch",StringComparison.Ordinal)?'t':token[0]);else result.Append('っ');doubled=false;}
                // Preserve syllable boundaries: ん + や differs from にゃ.
                if(c=='ん'&&i+1<text.Length){string next;if(Kana.TryGetValue(text[i+1],out next)&&"aiueoy".IndexOf(next[0])>=0)token="n'";}
                result.Append(token);
            }
            if(doubled)result.Append('っ');return result.ToString();
        }
        internal static bool Matches(string name,string query){
            if(name==null||query==null)return false;
            if(query.Length==0)return true;
            string normalizedName=name.Normalize(NormalizationForm.FormKC),normalizedQuery=query.Normalize(NormalizationForm.FormKC);
            return normalizedName.IndexOf(normalizedQuery,StringComparison.OrdinalIgnoreCase)>=0
                ||Key(normalizedName).IndexOf(Key(normalizedQuery),StringComparison.OrdinalIgnoreCase)>=0;
        }
    }
}
