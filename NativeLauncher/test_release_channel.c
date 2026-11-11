/* Portable tests for the launcher's actual release-selection and metadata code. */
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
typedef int BOOL;
#define TRUE 1
#define FALSE 0
#include "release_channel.h"
#define CHECK(c) do {if(!(c)){printf("FAIL %d: %s\n",__LINE__,#c);return 1;}}while(0)
#define RELEASE(tag,pre,draft,date) "{\"tag_name\":\"" tag "\",\"prerelease\":" pre ",\"draft\":" draft ",\"published_at\":\"" date "\"}"

int main(int argc,char **argv){
    GithubRelease selected={0};unsigned count=0;
    const char *releases="["
        RELEASE("v99.0.0-10","true","false","2026-10-01T01:00:00Z") ","
        RELEASE("v1.2.3","false","false","2026-11-02T01:00:00Z") ","
        RELEASE("v1.2.4-beta.1","true","false","2026-11-03T01:00:00Z") ","
        RELEASE("next-preview","true","true","2026-11-05T01:00:00Z") ","
        "{\"tag_name\":\"display-test/rc\",\"prerelease\":true,\"draft\":false,\"published_at\":\"2026-11-04T01:00:00Z\","
        "\"created_at\":\"2025-01-01T00:00:00Z\",\"body\":\"prerelease false\","
        "\"assets\":[{\"prerelease\":false,\"draft\":true,\"published_at\":\"2030-01-01T00:00:00Z\"}]},"
        RELEASE("experimental","true","false","2026-09-01T01:00:00Z") "]";
    CHECK(select_github_release(releases,TRUE,&selected,&count)&&count==6&&selected.flags);
    CHECK(!strcmp(selected.tag,"display-test/rc"));
    /* A later page can contain a more recently published release. */
    CHECK(select_github_release("[" RELEASE("candidate-c","true","false","2026-11-06T01:00:00Z") "]",TRUE,&selected,&count));
    CHECK(count==1&&!strcmp(selected.tag,"candidate-c"));
    CHECK(select_github_release("[]",TRUE,&selected,&count)&&count==0&&!strcmp(selected.tag,"candidate-c"));
    selected=(GithubRelease){0};
    CHECK(select_github_release(releases,FALSE,&selected,&count)&&!strcmp(selected.tag,"v1.2.3"));
    selected=(GithubRelease){0};
    CHECK(select_github_release(RELEASE("release-latest","false","false","2026-11-02T01:00:00Z"),FALSE,&selected,&count));
    CHECK(count==1&&!strcmp(selected.tag,"release-latest"));
    selected=(GithubRelease){0};
    CHECK(select_github_release("[{\"tag_name\":\"draft\",\"prerelease\":true,\"draft\":true,\"published_at\":null},"
        "{\"tag_name\":\"missing-flags\",\"published_at\":\"2026-11-01T00:00:00Z\"},"
        "{\"tag_name\":\"unpublished\",\"prerelease\":true,\"draft\":false}]",TRUE,&selected,&count)&&!selected.flags);
    const char *invalid[]={"", "[", "[{\"tag_name\":\"broken\"}", "[{},]", "{\"draft\":true,}", "{} garbage", "[{\"bad\":invalid}]"};
    for(size_t i=0;i<sizeof(invalid)/sizeof(*invalid);i++)CHECK(!select_github_release(invalid[i],TRUE,&selected,&count));
    char encoded[768];
    CHECK(release_tag_path("preview/rc+1?build#x%20",encoded,sizeof(encoded))&&!strcmp(encoded,"preview%2Frc%2B1%3Fbuild%23x%2520"));
    CHECK(!release_tag_path("..",encoded,sizeof(encoded))&&!release_tag_path("",encoded,sizeof(encoded)));
    CHECK(!release_tag_path("large",encoded,2));
    CHECK(select_github_release(RELEASE("test-\\u03b2-\\ud83d\\ude80","true","false","2026-11-06T01:00:00Z"),TRUE,&selected,&count));
    CHECK(release_tag_path(selected.tag,encoded,sizeof(encoded))&&!strcmp(encoded,"test-%CE%B2-%F0%9F%9A%80"));
    CHECK(!select_github_release(RELEASE("test-\\ud83d","true","false","2026-11-06T01:00:00Z"),TRUE,&selected,&count));
    ReleasePackage package;
    const char *metadata="{\"version\":\"preview-next+candidate\",\"artifacts\":[\"ReRiff-source.tar.gz\",\"client-test-linux.run\",\"client-test-windows.zip\"],\"commit\":\"abc\"}";
    CHECK(release_package(metadata,TRUE,&package)&&!strcmp(package.version,"preview-next+candidate")&&!strcmp(package.filename,"client-test-linux.run"));
    CHECK(release_package(metadata,FALSE,&package)&&!strcmp(package.filename,"client-test-windows.zip"));
    CHECK(!release_needs_install(package.version,package.version,TRUE,TRUE,FALSE));
    CHECK(release_needs_install(package.version,package.version,FALSE,TRUE,FALSE));
    CHECK(release_needs_install(package.version,package.version,TRUE,TRUE,TRUE));
    CHECK(release_needs_install(package.version,"1.2.3",TRUE,FALSE,FALSE));
    CHECK(release_needs_install(package.version,"different-preview",TRUE,TRUE,FALSE));
    const char *bad_packages[]={
        "{\"version\":\"next\",\"artifacts\":[\"../client-linux.run\"]}",
        "{\"version\":\"next\",\"artifacts\":[\"C:client-linux.run\"]}",
        "{\"version\":\"next\",\"artifacts\":[\"client-linux.run?query\"]}",
        "{\"version\":\"next\",\"artifacts\":[\"a-linux.run\",\"b-linux.run\"]}",
        "{\"version\":\"next\",\"artifacts\":[\"client-windows.zip\"]}",
        "{\"version\":\"next\",\"artifacts\":[\"client-linux.run\",]}",
        "{\"version\":\"next\",\"artifacts\":[\"client-linux.run\"],}",
        "{\"version\":\"next\\u0000ignored\",\"artifacts\":[\"client-linux.run\"]}",
        "{\"version\":\"next\\nline\",\"artifacts\":[\"client-linux.run\"]}",
        "{\"version\":\"next\",\"version\":\"other\",\"artifacts\":[\"client-linux.run\"]}",
        "{\"body\":{\"version\":\"next\",\"artifacts\":[\"client-linux.run\"]}}"
    };
    for(size_t i=0;i<sizeof(bad_packages)/sizeof(*bad_packages);i++)CHECK(!release_package(bad_packages[i],TRUE,&package));
    /* Optional captured GitHub responses reproduce real-world selection. */
    if(argc==2){
        FILE *file=fopen(argv[1],"rb");CHECK(file);CHECK(!fseek(file,0,SEEK_END));long size=ftell(file);CHECK(size>=0);rewind(file);
        char *json=calloc((size_t)size+1,1);CHECK(json);CHECK(fread(json,1,(size_t)size,file)==(size_t)size);fclose(file);
        selected=(GithubRelease){0};CHECK(select_github_release(json,TRUE,&selected,&count)&&selected.flags);
        printf("Selected prerelease: %s (%s), from %u releases\n",selected.tag,selected.published,count);free(json);
    }
    puts("PASS: publication ordering, arbitrary tags, pagination, channel isolation, metadata and repeat-update suppression");return 0;
}
