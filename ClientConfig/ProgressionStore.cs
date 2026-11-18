using System;
using System.IO;
using System.Security.Cryptography;
using System.Text;

namespace ReRiff {
    // Local singleplayer storage only. This is not multiplayer authentication.
    // Native serialization remains opaque; the manifest commits a complete
    // snapshot and its permanent progression counters as one generation.
    internal sealed class ProgressionStore {
        internal sealed class State {
            internal readonly ulong Generation, MainXp;
            internal readonly int CycleXp;
            internal readonly uint PrestigeCount;
            internal readonly string BalanceRevision;
            internal State(ulong generation,ulong mainXp,int cycleXp,uint prestigeCount,string revision) {
                if(cycleXp<0||cycleXp>2000000000)throw new ArgumentOutOfRangeException("cycleXp");
                ParseHash(revision);
                Generation=generation;MainXp=mainXp;CycleXp=cycleXp;PrestigeCount=prestigeCount;BalanceRevision=revision;
            }
        }
        internal sealed class Checkpoint {
            internal readonly State Progress;
            internal readonly byte[] NativeSave;
            internal Checkpoint(State progress,byte[] bytes){Progress=progress;NativeSave=bytes;}
        }
        private static readonly byte[] Magic=Encoding.ASCII.GetBytes("RRPGST01");
        private readonly string directory,manifest;
        private readonly byte[] domain;
        // Deterministic crash injection for checkpoint regressions. Production
        // construction leaves this null; it is never configured from a file.
        internal Action<string> BeforeAtomicCommit {get;set;}
        internal ProgressionStore(string directory,string domainIdentity) {
            if(String.IsNullOrEmpty(domainIdentity))throw new ArgumentException("Domain identity required");
            this.directory=Path.GetFullPath(directory);manifest=Path.Combine(this.directory,"state.bin");
            domain=Hash(Encoding.UTF8.GetBytes(domainIdentity));
        }
        private static byte[] Hash(byte[] bytes){using(var hash=SHA256.Create())return hash.ComputeHash(bytes);}
        private static bool Equal(byte[] left,byte[] right){
            if(left.Length!=right.Length)return false;
            int difference=0;for(int i=0;i<left.Length;i++)difference|=left[i]^right[i];return difference==0;
        }
        private static byte[] ParseHash(string value){
            if(value==null||value.Length!=64)throw new ArgumentException("SHA-256 revision required");
            var bytes=new byte[32];for(int i=0;i<32;i++){
                char a=value[i*2],b=value[i*2+1];
                if(!((a>='0'&&a<='9')||(a>='a'&&a<='f'))||!((b>='0'&&b<='9')||(b>='a'&&b<='f')))throw new ArgumentException("Lowercase SHA-256 revision required");
                bytes[i]=Convert.ToByte(value.Substring(i*2,2),16);
            }return bytes;
        }
        private static string Hex(byte[] bytes){return BitConverter.ToString(bytes).Replace("-","").ToLowerInvariant();}
        private string Slot(int index){return Path.Combine(directory,"save-"+index+".bin");}
        private FileStream Lock(){
            Directory.CreateDirectory(directory);
            return new FileStream(Path.Combine(directory,"transaction.lock"),FileMode.OpenOrCreate,FileAccess.ReadWrite,FileShare.None);
        }
        private byte[] Encode(State value,int slot,byte[] snapshot){
            using(var buffer=new MemoryStream()){
                using(var writer=new BinaryWriter(buffer,Encoding.UTF8)){
                    writer.Write(Magic);writer.Write(domain);writer.Write(value.Generation);writer.Write(slot);
                    writer.Write(value.MainXp);writer.Write(value.CycleXp);writer.Write(value.PrestigeCount);
                    writer.Write(ParseHash(value.BalanceRevision));writer.Write(Hash(snapshot));writer.Flush();
                    byte[] body=buffer.ToArray();writer.Write(Hash(body));writer.Flush();return buffer.ToArray();
                }
            }
        }
        private Checkpoint ReadUnlocked(out int slot){
            slot=-1;if(!File.Exists(manifest)){
                if(File.Exists(Slot(0))||File.Exists(Slot(1)))throw new IOException("Progression manifest is missing; preserve snapshots for recovery");
                return null;
            }
            byte[] bytes=File.ReadAllBytes(manifest);if(bytes.Length!=164)throw new IOException("Invalid progression manifest size");
            byte[] body=new byte[132],checksum=new byte[32];Array.Copy(bytes,body,132);Array.Copy(bytes,132,checksum,0,32);
            if(!Equal(Hash(body),checksum))throw new IOException("Progression manifest checksum mismatch");
            using(var reader=new BinaryReader(new MemoryStream(body),Encoding.UTF8)){
                if(!Equal(reader.ReadBytes(8),Magic)||!Equal(reader.ReadBytes(32),domain))throw new IOException("Progression manifest version or domain mismatch");
                ulong generation=reader.ReadUInt64();slot=reader.ReadInt32();
                if(generation==0||(slot!=0&&slot!=1))throw new IOException("Invalid progression generation or slot");
                ulong mainXp=reader.ReadUInt64();int cycleXp=reader.ReadInt32();uint prestige=reader.ReadUInt32();
                string revision=Hex(reader.ReadBytes(32));byte[] expected=reader.ReadBytes(32);
                var state=new State(generation,mainXp,cycleXp,prestige,revision);
                byte[] snapshot=File.ReadAllBytes(Slot(slot));
                if(snapshot.Length==0||!Equal(Hash(snapshot),expected))throw new IOException("Committed native save checksum mismatch");
                return new Checkpoint(state,snapshot);
            }
        }
        internal Checkpoint Read(){
            if(!Directory.Exists(directory))return null;
            using(Lock()){int slot;return ReadUnlocked(out slot);}
        }
        private void Atomic(string path,byte[] bytes,string stage){
            string temporary=path+"."+Guid.NewGuid().ToString("N")+".tmp";
            try {
                using(var stream=new FileStream(temporary,FileMode.CreateNew,FileAccess.Write,FileShare.None)){
                    stream.Write(bytes,0,bytes.Length);stream.Flush(true);
                }
                if(BeforeAtomicCommit!=null)BeforeAtomicCommit(stage);
                if(File.Exists(path))File.Replace(temporary,path,null);else File.Move(temporary,path);
            } finally {if(File.Exists(temporary))File.Delete(temporary);}
        }
        internal Checkpoint Initialize(byte[] nativeSave,ulong mainXp,int cycleXp,string revision){
            if(nativeSave==null||nativeSave.Length==0)throw new ArgumentException("Native save required");
            using(Lock()){
                int slot;var existing=ReadUnlocked(out slot);if(existing!=null)return existing;
                var state=new State(1,mainXp,cycleXp,0,revision);
                Atomic(Slot(0),nativeSave,"snapshot");Atomic(manifest,Encode(state,0,nativeSave),"manifest");
                return new Checkpoint(state,(byte[])nativeSave.Clone());
            }
        }
        internal Checkpoint Commit(byte[] nativeSave,State proposal,ulong expectedGeneration){
            if(nativeSave==null||nativeSave.Length==0)throw new ArgumentException("Native save required");
            if(proposal==null)throw new ArgumentNullException("proposal");
            using(Lock()){
                int active;var previous=ReadUnlocked(out active);
                if(previous==null||previous.Progress.Generation!=expectedGeneration||proposal.Generation!=checked(expectedGeneration+1))throw new IOException("Progression generation changed; reload before saving");
                var old=previous.Progress;
                if(proposal.MainXp<old.MainXp||proposal.PrestigeCount<old.PrestigeCount
                    ||(ulong)proposal.PrestigeCount>(ulong)old.PrestigeCount+1)throw new IOException("Invalid permanent progression transition");
                if(proposal.PrestigeCount!=old.PrestigeCount&&proposal.CycleXp!=0)throw new IOException("A prestige checkpoint must restart cycle XP at zero");
                if(proposal.PrestigeCount==old.PrestigeCount&&proposal.CycleXp<old.CycleXp)throw new IOException("Cycle XP cannot decrease without prestige");
                int next=1-active;
                Atomic(Slot(next),nativeSave,"snapshot");
                // Until this atomic replacement succeeds, the old manifest
                // still points to the untouched old active slot and counters.
                Atomic(manifest,Encode(proposal,next,nativeSave),"manifest");
                return new Checkpoint(proposal,(byte[])nativeSave.Clone());
            }
        }
    }
}
