using System;
using System.IO;
using System.Linq;
using System.Net;
using System.Text;
using Parse;

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
