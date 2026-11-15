using System;
using System.Collections.Generic;
using System.Text;

namespace ReRiff {
    // A snapshot of picker names, not an owner of native entries or UI rows.
    // Results retain source indices so duplicate names never change identity.
    internal sealed class PickerSearchIndex {
        private readonly string[] names, literals, keys;
        internal int Count { get { return names.Length; } }

        internal PickerSearchIndex(string[] source) {
            if(source==null)throw new ArgumentNullException("source");
            names=(string[])source.Clone();
            literals=new string[names.Length];keys=new string[names.Length];
            for(int i=0;i<names.Length;i++) {
                if(names[i]==null)continue;
                literals[i]=Normalize(names[i]);keys[i]=SearchKey(literals[i]);
            }
        }

        // Windows names may contain unpaired UTF-16 surrogates. Keep them
        // searchable literally instead of allowing Normalize to abort a scan.
        private static string Normalize(string text) {
            try{return text.Normalize(NormalizationForm.FormKC);}
            catch(ArgumentException){return text;}
        }
        private static string SearchKey(string text) {
            try{return KanaSearch.Key(text);}
            catch(ArgumentException){return text;}
        }
        internal string Name(int sourceIndex) { return names[sourceIndex]; }

        internal int[] Find(string query) {
            if(query==null)throw new ArgumentNullException("query");
            string literal=Normalize(query),key=SearchKey(literal);
            var matches=new List<int>();
            for(int i=0;i<names.Length;i++) {
                if(names[i]!=null&&(query.Length==0
                    ||literals[i].IndexOf(literal,StringComparison.OrdinalIgnoreCase)>=0
                    ||keys[i].IndexOf(key,StringComparison.OrdinalIgnoreCase)>=0))matches.Add(i);
            }
            return matches.ToArray();
        }
    }
}
