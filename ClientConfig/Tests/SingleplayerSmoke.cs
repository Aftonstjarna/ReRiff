using System;
using System.IO;
using System.Linq;
using System.Net;
using System.Text;
using Parse;
using System.Reflection;

class SingleplayerSmoke {
    static void Check(bool ok, string name) { if (!ok) throw new Exception(name); Console.WriteLine("PASS: " + name); }
    static void Main() {
        string folder = Path.Combine(Path.GetTempPath(), "reriff-singleplayer-" + Guid.NewGuid().ToString("N"));
        Directory.CreateDirectory(folder);
        try {
            string legacy = Path.Combine(folder, "localserver/server/data/classes/LegacyFixture");
            Directory.CreateDirectory(legacy);
            string legacyObject = Path.Combine(legacy, "keep.json");
            File.WriteAllText(legacyObject, "{\"objectId\":\"keep\",\"value\":42}");
            string id, fileId;
            byte[] ghost = Encoding.UTF8.GetBytes("singleplayer ghost fixture");
            using (var local = new ReRiff.Singleplayer(Path.Combine(folder, "store"))) {
                ParseClient.Initialize(new ParseClient.Configuration { ApplicationId = "test", WindowsKey = "test", Server = local.Endpoint });
                Check(ParseObject.GetQuery("LegacyFixture").GetAsync("keep").Result.Get<int>("value") == 42,
                    "legacy local database copied into singleplayer");
                Check(File.ReadAllText(legacyObject) == "{\"objectId\":\"keep\",\"value\":42}", "legacy database left unchanged");
                var config = ParseObject.GetQuery("Configuration").FirstOrDefaultAsync().Result;
                Check(config.Get<int>("TrackConversionCost") == 0, "original Parse SDK loads singleplayer configuration");
                Check(ParseObject.GetQuery("Version").FirstOrDefaultAsync().Result.Get<int>("Ver") == 100, "original version handshake");
                var user = ParseUser.LogInAsync("singleplayer-fixture", "not-sent-to-a-server").Result;
                Check(user.ObjectId.Length > 0 && ParseUser.CurrentUser != null, "original SDK login and session");
                var score = new ParseObject("Score"); score["SongID"] = "metal-fixture"; score["Owner"] = user.ObjectId; score["Score"] = 1234;
                score.SaveAsync().Wait(); id = score.ObjectId;
                score.Increment("Score", 10); score.SaveAsync().Wait();
                var scores = ParseObject.GetQuery("Score").WhereEqualTo("SongID", "metal-fixture").WhereGreaterThan("Score", 1000).FindAsync().Result.ToList();
                Check(scores.Count == 1 && scores[0].Get<int>("Score") == 1244, "score persistence, increment and filtered queries");
                Check(ParseObject.GetQuery("Score").WhereEqualTo("SongID", "different").FindAsync().Result.Count() == 0, "unrelated song excluded");
                var file = new ParseFile("ghost.dat", ghost, "application/octet-stream"); file.SaveAsync().Wait();
                var record = new ParseObject("ghost_v1"); record["data"] = file; record["song"] = "metal-fixture"; record.SaveAsync().Wait(); fileId = record.ObjectId;
                using (var client = new WebClient()) Check(client.DownloadData(file.Url).SequenceEqual(ghost), "original SDK upload and game-style ghost download");
                // Missing backend ghosts may already exist as native personal bests.
                string cache=Path.Combine(folder,"native-cache");Directory.CreateDirectory(cache);
                typeof(ReRiff.Singleplayer).GetField("NativeGhostDirectory",BindingFlags.Instance|BindingFlags.NonPublic).SetValue(local,cache);
                var profile=new ParseObject("Profile_Steam");profile["SystemID"]="123456789";profile["Username"]="Ghost fixture";profile.SaveAsync().Wait();
                string ghostName="NativeSong-123456789.ghost";byte[] native;
                using(var stream=new MemoryStream())using(var writer=new BinaryWriter(stream)) {
                    writer.Write(18);writer.Write(5);writer.Write(123f);
                    foreach(var part in new[]{"123456789","NativeSong","fab"}) {
                        byte[] text=Encoding.ASCII.GetBytes(part);writer.Write(text.Length);writer.Write(text);
                        if(part=="123456789")writer.Write(500f);
                    }
                    writer.Write(6);writer.Write(0);writer.Write((byte)1);native=stream.ToArray();
                }
                File.WriteAllBytes(Path.Combine(cache,ghostName),native);
                var imported=ParseObject.GetQuery("ghost_v18").WhereEqualTo("song","NativeSong").FindAsync().Result.ToList();
                Check(imported.Count==1&&imported[0].Get<int>("score")==500&&imported[0].Get<string>("user")=="123456789"&&imported[0].Get<string>("car")=="fab"&&imported[0].Get<int>("pack")==6,"native cached ghost metadata imported through normal query");
                var savedFile=ParseObject.GetQuery("FileData").WhereEqualTo("name",ghostName).FirstAsync().Result.Get<ParseFile>("data");
                using(var client=new WebClient())Check(client.DownloadData(savedFile.Url).SequenceEqual(native),"native ghost payload served byte-for-byte");
                imported[0]["score"]=900;imported[0].SaveAsync().Wait();
                imported=ParseObject.GetQuery("ghost_v18").WhereEqualTo("song","NativeSong").FindAsync().Result.ToList();
                Check(imported.Count==1&&imported[0].Get<int>("score")==900,"repeat queries preserve existing ghost record and do not duplicate");
                Check(File.ReadAllBytes(Path.Combine(cache,ghostName)).SequenceEqual(native),"original native cached ghost unchanged");
                var absent = ParseObject.GetQuery("Score").GetAsync("does-not-exist");
                try { absent.Wait(); } catch (AggregateException) { }
                Check(absent.IsFaulted, "missing objects fail through original SDK error handling");
                var a = new ParseObject("BatchFixture"); a["n"] = 10;
                var b = new ParseObject("BatchFixture"); b["n"] = 20;
                ParseObject.SaveAllAsync(new[] { a, b }).Wait();
                var ordered = ParseObject.GetQuery("BatchFixture").OrderByDescending("n").Skip(1).Limit(1).FindAsync().Result.ToList();
                Check(ordered.Count == 1 && ordered[0].Get<int>("n") == 10, "original SDK batch writes, ordering and pagination");
            }
            using (var reopened = new ReRiff.Singleplayer(Path.Combine(folder, "store"))) {
                ParseClient.Initialize(new ParseClient.Configuration { ApplicationId = "test", WindowsKey = "test", Server = reopened.Endpoint });
                Check(ParseObject.GetQuery("Score").GetAsync(id).Result.Get<int>("Score") == 1244, "scores survive process-style restart");
                var record = ParseObject.GetQuery("ghost_v1").GetAsync(fileId).Result;
                var file = record.Get<ParseFile>("data");
                using (var client = new WebClient()) Check(client.DownloadData(file.Url).SequenceEqual(ghost), "saved ghost URLs rebind to the new session");
                ParseObject.GetQuery("Score").GetAsync(id).Result.DeleteAsync().Wait();
                Check(ParseObject.GetQuery("Score").CountAsync().Result == 0, "delete and count use persisted state");
            }
            File.WriteAllText(Path.Combine(folder, "server.ini"), "[server]\nnetworking_enabled=false\nexternal_url=https://retain.example\nfallback_url=https://secondary.example\n");
            string before = File.ReadAllText(Path.Combine(folder, "server.ini"));
            Check(ReRiff.Singleplayer.IsEndpoint(ReRiff.ClientConfig.Select(folder)), "former disabled mode selects functioning singleplayer");
            Check(File.ReadAllText(Path.Combine(folder, "server.ini")) == before, "singleplayer retains both configured server addresses");
            File.WriteAllText(Path.Combine(folder, "server.ini"), "[server]\nexternal_url=http://127.0.0.1:1\nfallback_url=http://127.0.0.1:2\ntimeout_seconds=0.05\n");
            Check(ReRiff.Singleplayer.IsEndpoint(ReRiff.ClientConfig.Select(folder)), "both unreachable servers fall back to singleplayer");
        } finally { Directory.Delete(folder, true); }
    }
}
