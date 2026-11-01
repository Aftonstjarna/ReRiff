// Native News/version policy; Python and managed code only adapt transport/I/O.
#include <array>
#include <optional>

static std::optional<std::array<unsigned,3>> news_version(const std::string& text) {
    std::array<unsigned,3> result{};
    size_t pos=0;
    for (size_t i=0;i<3;i++) {
        const size_t start=pos;
        while (pos<text.size() && text[pos]>='0' && text[pos]<='9') {
            if (pos-start>=4) return std::nullopt;
            result[i]=result[i]*10+static_cast<unsigned>(text[pos++]-'0');
        }
        if (start==pos) return std::nullopt;
        if (i<2 && (pos==text.size() || text[pos++]!='.')) return std::nullopt;
    }
    return pos==text.size()?std::optional(result):std::nullopt;
}

extern "C" EXPORT size_t reriff_news(const char* input,char* output,size_t capacity) noexcept {
    std::string result;
    try {
        const auto request=json::parse(input);
        const auto server=request.value("server_version",std::string());
        const auto client=request.value("client_version",std::string());
        const auto server_number=news_version(server), client_number=news_version(client);
        const bool update=server_number && client_number && *server_number>*client_number;
        auto entries=json::array();
        for (const auto& entry:request.at("news")) {
            const auto id=entry.value("objectId",std::string());
            if (id!="RiffApiLocalNews" && id!="ReRiffServerStatus") entries.push_back(entry);
        }
        std::string title=update?"Exit to update ReRiff":"ReRiff server";
        std::string message;
        if (update) message="This server is running ReRiff "+server+". Your client is "+client+
            ".\nExit the game and update ReRiff, then reconnect. Do not update while the game is running.";
        else if (server_number && client_number) message="Server: ReRiff "+server+". Your client: "+client+
            ".\nThe server is not newer than your client.";
        else if (server_number) message="Server: ReRiff "+server+
            ".\nYour client version was not reported or is invalid; update status is unknown.";
        else message="Server version is unavailable or invalid; update status is unknown.";
        json notice={{"objectId","ReRiffServerStatus"},{"ID",0},{"Pinned",true},{"Status","Active"},
            {"createdAt","2000-01-01T00:00:00.000Z"},{"updatedAt","2000-01-01T00:00:00.000Z"}};
        for (const auto* language:{"en","de","ru","es","it","fr"}) {
            notice[language]=message; notice[std::string(language)+"Title"]=title;
        }
        entries.insert(entries.begin(),notice);
        result=json({{"news",entries},{"update_available",update}}).dump();
    } catch (const std::exception& e) { result=json({{"error",e.what()}}).dump(); }
      catch (...) { result="{\"error\":\"News policy failure\"}"; }
    const size_t required=result.size()+1;
    if (output && capacity>=required) std::memcpy(output,result.c_str(),required);
    return required;
}
