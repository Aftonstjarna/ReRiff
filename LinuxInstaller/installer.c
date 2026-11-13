/* Native GTK setup. GTK is resolved at runtime so command-line installs never
 * depend on a desktop toolkit. All game changes use the shared installer. */
#define _GNU_SOURCE
#include <gtk/gtk.h>
#include <dlfcn.h>
#undef g_free
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <spawn.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>
#ifndef RERIFF_VERSION
#define RERIFF_VERSION "development"
#endif
/* Only APIs available on GTK 3.10 / GLib 2.52 or earlier. No GTK link dependency. */
#define GTK_API(X) \
 X(gtk_init_check) X(gtk_window_new) X(gtk_window_set_title) X(gtk_window_set_default_size) \
 X(gtk_container_set_border_width) X(gtk_box_new) X(gtk_container_add) X(gtk_box_pack_start) \
 X(gtk_label_new) X(gtk_label_set_text) X(gtk_label_set_line_wrap) X(gtk_misc_set_alignment) \
 X(gtk_file_chooser_button_new) X(gtk_file_chooser_set_filename) X(gtk_file_chooser_get_filename) \
 X(gtk_check_button_new_with_label) X(gtk_toggle_button_set_active) X(gtk_toggle_button_get_active) \
 X(gtk_entry_new) X(gtk_entry_set_text) X(gtk_entry_get_text) \
 X(gtk_link_button_new_with_label) X(gtk_button_new_with_label) X(gtk_button_set_label) X(gtk_widget_set_sensitive) \
 X(gtk_progress_bar_new) X(gtk_progress_bar_pulse) X(gtk_progress_bar_set_fraction) \
 X(gtk_scrolled_window_new) X(gtk_text_view_new) X(gtk_text_view_set_editable) \
 X(gtk_text_view_set_cursor_visible) X(gtk_text_view_get_buffer) X(gtk_text_buffer_set_text) \
 X(gtk_widget_show_all) X(gtk_widget_destroy) X(gtk_main) X(gtk_main_quit) \
 X(g_signal_connect_data) X(g_timeout_add) X(g_free) X(g_error_free) X(g_build_filename) \
 X(g_spawn_sync) X(g_mkdir_with_parents) X(g_get_home_dir) X(g_utf8_make_valid) X(g_file_get_contents) X(g_convert)
struct GtkApi {
#define DECLARE(name) __typeof__(&name) name;
 GTK_API(DECLARE)
#undef DECLARE
};
static struct GtkApi ui;
static int load_gtk(void) {
    void *library=dlopen("libgtk-3.so.0",RTLD_NOW|RTLD_LOCAL);
    if(!library)return 0;
#define LOAD(name) *(void **)(&ui.name)=dlsym(library,#name);if(!ui.name)return 0;
    GTK_API(LOAD)
#undef LOAD
    return 1;
}
typedef struct Setup {
    GtkWidget *window,*folder,*button,*close,*status,*progress,*details,*log_label,*music_check,*music_path,*cjk_check,*latin_check,*logging_check;
    char *bundle,*log_path;
    pid_t child;
    int finished,exit_code,temporary_log;
} Setup;
static void set_status(Setup *s,const char *text){ui.gtk_label_set_text((GtkLabel *)s->status,text);}
static void refresh_log(Setup *s) {
    if(!s->log_path)return;
    FILE *f=fopen(s->log_path,"rb");if(!f)return;
    /* Bounded display; the complete log stays on disk. */
    if(fseek(f,0,SEEK_END)){fclose(f);return;}
    long size=ftell(f);if(size<0){fclose(f);return;}
    long start=size>65536?size-65536:0;
    if(fseek(f,start,SEEK_SET)){fclose(f);return;}
    char data[65537];size_t n=fread(data,1,sizeof(data)-1,f);fclose(f);data[n]=0;
    /* Installer paths can contain arbitrary filesystem bytes: GTK needs UTF-8.
     * Client-helper diagnostics are UTF-8 on supported Linux environments. */
    for(size_t i=0;i<n;i++)if(data[i]==0)data[i]=' ';
    char *valid=ui.g_utf8_make_valid(data,(gssize)n);
    ui.gtk_text_buffer_set_text(ui.gtk_text_view_get_buffer((GtkTextView *)s->details),valid,-1);ui.g_free(valid);
}
static gboolean child_tick(gpointer data) {
    Setup *s=data;int status;pid_t result=waitpid(s->child,&status,WNOHANG);
    refresh_log(s);
    if(!result){ui.gtk_progress_bar_pulse((GtkProgressBar *)s->progress);return TRUE;}
    if(result<0&&errno==EINTR)return TRUE;
    s->exit_code=result>0&&WIFEXITED(status)?WEXITSTATUS(status):1;
    s->child=0;s->finished=1;
    ui.gtk_progress_bar_set_fraction((GtkProgressBar *)s->progress,s->exit_code?0.0:1.0);
    ui.gtk_widget_set_sensitive(s->folder,TRUE);
    ui.gtk_widget_set_sensitive(s->cjk_check,TRUE);ui.gtk_widget_set_sensitive(s->latin_check,TRUE);ui.gtk_widget_set_sensitive(s->logging_check,TRUE);
    ui.gtk_widget_set_sensitive(s->music_check,TRUE);
    ui.gtk_widget_set_sensitive(s->music_path,ui.gtk_toggle_button_get_active((GtkToggleButton *)s->music_check));
    ui.gtk_widget_set_sensitive(s->button,s->exit_code!=0);
    ui.gtk_widget_set_sensitive(s->close,TRUE);
    ui.gtk_button_set_label((GtkButton *)s->button,s->exit_code?"Retry installation":"Installed");
    set_status(s,s->exit_code?
        "Installation failed. Read the details below or open the installer log. Your saved data is retained.":
        "ReRiff installed. Launch ReRiff-Launcher.exe through Steam/Proton. Your settings and saved server data were preserved.");
    return FALSE;
}
extern char **environ;
static void install_clicked(GtkWidget *widget,gpointer data) {
    (void)widget;Setup *s=data;
    if(s->child)return;
    char *game=ui.gtk_file_chooser_get_filename((GtkFileChooser *)s->folder);
    if(!game){set_status(s,"Choose the Riff Racer folder containing Game.exe.");return;}
    char *exe=ui.g_build_filename(game,"Game.exe",NULL);struct stat st;
    if(stat(exe,&st)||!S_ISREG(st.st_mode)){
        set_status(s,"Game.exe was not found in that folder. Select the installed Riff Racer folder.");
        ui.g_free(exe);ui.g_free(game);return;
    }
    ui.g_free(exe);
    gboolean logging=ui.gtk_toggle_button_get_active((GtkToggleButton *)s->logging_check);
    char *logs=logging?ui.g_build_filename(game,"logs",NULL):ui.g_build_filename("/tmp",NULL);
    if(logging&&ui.g_mkdir_with_parents(logs,0700)){
        set_status(s,"Could not create the installer log folder. Check the game-folder permissions.");
        ui.g_free(logs);ui.g_free(game);return;
    }
    if(s->temporary_log&&s->log_path)unlink(s->log_path);s->temporary_log=!logging;
    ui.g_free(s->log_path);s->log_path=ui.g_build_filename(logs,"installer-XXXXXX.log",NULL);ui.g_free(logs);
    int fd=mkstemps(s->log_path,4);
    if(fd<0){set_status(s,"Could not create the installer log. No game files were changed.");ui.g_free(game);return;}
    const gboolean link_music=ui.gtk_toggle_button_get_active((GtkToggleButton *)s->music_check);
    const char *music=ui.gtk_entry_get_text((GtkEntry *)s->music_path);
    if(link_music&&(!music||!*music)){
        close(fd);ui.g_free(game);set_status(s,"Enter a Linux music folder, or uncheck Link Linux music folder.");return;
    }

    char *runtime=ui.g_build_filename(s->bundle,"payload","localserver","ReRiff-ClientHost",NULL);
    char *argv[]={runtime,"--client-host","install","--bundle",s->bundle,"--game-dir",game,logging?"--logging":"--no-logging",ui.gtk_toggle_button_get_active((GtkToggleButton *)s->cjk_check)?"--cjk":"--no-cjk",ui.gtk_toggle_button_get_active((GtkToggleButton *)s->latin_check)?"--latin":"--no-latin",
        link_music?"--music-dir":"--no-music-link",link_music?(char *)music:NULL,NULL};
    posix_spawn_file_actions_t actions;int error=posix_spawn_file_actions_init(&actions);
    if(!error){
        error=posix_spawn_file_actions_adddup2(&actions,fd,STDOUT_FILENO);
        if(!error)error=posix_spawn_file_actions_adddup2(&actions,fd,STDERR_FILENO);
        if(!error)error=posix_spawn_file_actions_addclose(&actions,fd);
        if(!error)error=posix_spawn_file_actions_addopen(&actions,STDIN_FILENO,"/dev/null",O_RDONLY,0);
        if(!error)error=posix_spawn(&s->child,runtime,&actions,NULL,argv,environ);
        posix_spawn_file_actions_destroy(&actions);
    }
    close(fd);ui.g_free(runtime);ui.g_free(game);
    ui.gtk_label_set_text((GtkLabel *)s->log_label,logging?s->log_path:"Setup details are temporary while logging is disabled.");
    if(error){s->child=0;set_status(s,"Could not start the bundled Linux runtime. No game files were changed.");return;}
    s->finished=0;s->exit_code=0;
    ui.gtk_widget_set_sensitive(s->cjk_check,FALSE);ui.gtk_widget_set_sensitive(s->latin_check,FALSE);ui.gtk_widget_set_sensitive(s->logging_check,FALSE);
    ui.gtk_widget_set_sensitive(s->music_check,FALSE);ui.gtk_widget_set_sensitive(s->music_path,FALSE);
    ui.gtk_widget_set_sensitive(s->folder,FALSE);ui.gtk_widget_set_sensitive(s->button,FALSE);ui.gtk_widget_set_sensitive(s->close,FALSE);
    set_status(s,"Installing ReRiff… Please keep setup open until it finishes.");
    ui.g_timeout_add(150,child_tick,s);
}
static gboolean close_window(GtkWidget *widget,GdkEvent *event,gpointer data) {
    (void)widget;(void)event;Setup *s=data;
    if(s->child){set_status(s,"Installation is still running. Setup will let you close when it finishes.");return TRUE;}
    ui.gtk_main_quit();return FALSE;
}
static void window_destroyed(GtkWidget *widget,gpointer data){(void)widget;(void)data;ui.gtk_main_quit();}
static void close_clicked(GtkWidget *widget,gpointer data){(void)widget;Setup *s=data;if(!s->child)ui.gtk_widget_destroy(s->window);}
static GtkWidget *label(const char *text){
    GtkWidget *w=ui.gtk_label_new(text);ui.gtk_label_set_line_wrap((GtkLabel *)w,TRUE);
    ui.gtk_misc_set_alignment((GtkMisc *)w,0,0.5f);return w;
}
static void connect_signal(void *object,const char *signal,GCallback callback,Setup *s){ui.g_signal_connect_data(object,signal,callback,s,NULL,0);}
static void music_toggled(GtkWidget *widget,gpointer data){
    (void)widget;Setup *s=data;
    ui.gtk_widget_set_sensitive(s->music_path,ui.gtk_toggle_button_get_active((GtkToggleButton *)s->music_check)&&!s->child);
}
static void language_folder_changed(GtkWidget *widget,gpointer data){
    (void)widget;Setup *s=data;if(s->child)return;
    char *game=ui.gtk_file_chooser_get_filename((GtkFileChooser *)s->folder);if(!game)return;
    char *path=ui.g_build_filename(game,"server.ini",NULL),*contents=NULL;gsize length=0;GError *error=NULL;
    FILE *file=NULL;
    if(ui.g_file_get_contents(path,&contents,&length,&error)) {
        if(length>=2&&((unsigned char)contents[0]==255||(unsigned char)contents[0]==254)) {
            gsize written=0;char *utf8=ui.g_convert(contents,(gssize)length,"UTF-8","UTF-16",NULL,&written,NULL);ui.g_free(contents);contents=utf8;length=written;
        }
        if(contents){unsigned skip=length>=3&&!memcmp(contents,"\xef\xbb\xbf",3)?3:0;file=fmemopen(contents+skip,length-skip,"r");}
    }
    if(error)ui.g_error_free(error);
    int saved=-1,latin_saved=0,legacy=0,section=0,log_saved=-1;
    if(file){char line[1024];while(fgets(line,sizeof(line),file)){char *p=line;while(*p==' '||*p=='\t')p++;
        if(*p=='['){section=!strncasecmp(p,"[languages]",11)?1:!strncasecmp(p,"[diagnostics]",13)?2:0;continue;}
        char *eq=strchr(p,'=');if(!eq)continue;char *end=eq;while(end>p&&(end[-1]==' '||end[-1]=='\t'))end--;
        char *value=eq+1;while(*value==' '||*value=='\t')value++;int enabled=!strncasecmp(value,"true",4)||*value=='1';
        if(section==1&&end-p==11&&!strncasecmp(p,"cjk_enabled",11))saved=enabled;
        if(section==1&&end-p==13&&!strncasecmp(p,"latin_enabled",13))latin_saved=enabled;
        if(section==2&&end-p==15&&!strncasecmp(p,"logging_enabled",15))log_saved=enabled;
        if(section==2&&end-p==16&&!strncasecmp(p,"cjk_render_probe",16))legacy=enabled;
    }fclose(file);}ui.gtk_toggle_button_set_active((GtkToggleButton *)s->latin_check,latin_saved);ui.gtk_toggle_button_set_active((GtkToggleButton *)s->cjk_check,saved>=0?saved:legacy);ui.gtk_toggle_button_set_active((GtkToggleButton *)s->logging_check,log_saved>=0?log_saved:(strchr(RERIFF_VERSION,'-')!=NULL||!strcmp(RERIFF_VERSION,"development")));ui.g_free(contents);ui.g_free(path);ui.g_free(game);
}
static void create_window(Setup *s) {
    s->window=ui.gtk_window_new(GTK_WINDOW_TOPLEVEL);
    ui.gtk_window_set_title((GtkWindow *)s->window,"ReRiff " RERIFF_VERSION " — Setup");
    ui.gtk_window_set_default_size((GtkWindow *)s->window,640,690);
    ui.gtk_container_set_border_width((GtkContainer *)s->window,20);
    GtkWidget *outer=ui.gtk_scrolled_window_new(NULL,NULL);ui.gtk_container_add((GtkContainer *)s->window,outer);
    GtkWidget *box=ui.gtk_box_new(GTK_ORIENTATION_VERTICAL,12);ui.gtk_container_add((GtkContainer *)outer,box);
    ui.gtk_box_pack_start((GtkBox *)box,label("Install ReRiff for Linux / Proton"),FALSE,FALSE,0);
    ui.gtk_box_pack_start((GtkBox *)box,label("Close Riff Racer and its launcher, then choose the game folder containing Game.exe. Settings, scores, and server data are kept. Original DLL backups are verified and stored in the game's .bak folder."),FALSE,FALSE,0);
    s->folder=ui.gtk_file_chooser_button_new("Choose the Riff Racer game folder",GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER);
    ui.gtk_box_pack_start((GtkBox *)box,s->folder,FALSE,FALSE,0);
    s->cjk_check=ui.gtk_check_button_new_with_label("Install and enable CJK fonts (Chinese, Japanese, Korean; about 105 MiB)");
    ui.gtk_box_pack_start((GtkBox *)box,s->cjk_check,FALSE,FALSE,0);
    s->latin_check=ui.gtk_check_button_new_with_label("Install and enable Latin/European fonts (including Polish)");
    ui.gtk_box_pack_start((GtkBox *)box,s->latin_check,FALSE,FALSE,0);
    s->logging_check=ui.gtk_check_button_new_with_label("Enable diagnostic logging (defaults on for beta/development builds)");
    ui.gtk_toggle_button_set_active((GtkToggleButton *)s->logging_check,strchr(RERIFF_VERSION,'-')!=NULL||!strcmp(RERIFF_VERSION,"development"));
    ui.gtk_box_pack_start((GtkBox *)box,s->logging_check,FALSE,FALSE,0);
    s->music_check=ui.gtk_check_button_new_with_label("Link Linux music folder into the Proton prefix");
    ui.gtk_toggle_button_set_active((GtkToggleButton *)s->music_check,TRUE);
    ui.gtk_box_pack_start((GtkBox *)box,s->music_check,FALSE,FALSE,0);
    s->music_path=ui.gtk_entry_new();
    char *music=ui.g_build_filename(ui.g_get_home_dir(),"Music",NULL);
    ui.gtk_entry_set_text((GtkEntry *)s->music_path,music);ui.g_free(music);
    ui.gtk_box_pack_start((GtkBox *)box,s->music_path,FALSE,FALSE,0);
    ui.gtk_box_pack_start((GtkBox *)box,ui.gtk_link_button_new_with_label("https://github.com/Aftonstjarna/ReRiff","ReRiff on GitHub — releases, help and source"),FALSE,FALSE,0);
    s->status=label("Choose the game folder, then click Install.");ui.gtk_box_pack_start((GtkBox *)box,s->status,FALSE,FALSE,0);
    s->progress=ui.gtk_progress_bar_new();ui.gtk_box_pack_start((GtkBox *)box,s->progress,FALSE,FALSE,0);
    GtkWidget *scroll=ui.gtk_scrolled_window_new(NULL,NULL);s->details=ui.gtk_text_view_new();
    ui.gtk_text_view_set_editable((GtkTextView *)s->details,FALSE);ui.gtk_text_view_set_cursor_visible((GtkTextView *)s->details,FALSE);
    ui.gtk_container_add((GtkContainer *)scroll,s->details);ui.gtk_box_pack_start((GtkBox *)box,scroll,TRUE,TRUE,0);
    s->log_label=label("Enabled logging saves reports in the game's logs folder. Existing logs are kept when disabled.");ui.gtk_box_pack_start((GtkBox *)box,s->log_label,FALSE,FALSE,0);
    GtkWidget *buttons=ui.gtk_box_new(GTK_ORIENTATION_HORIZONTAL,12);
    s->button=ui.gtk_button_new_with_label("Install");s->close=ui.gtk_button_new_with_label("Close");
    ui.gtk_box_pack_start((GtkBox *)buttons,s->button,TRUE,TRUE,0);ui.gtk_box_pack_start((GtkBox *)buttons,s->close,FALSE,FALSE,0);
    ui.gtk_box_pack_start((GtkBox *)box,buttons,FALSE,FALSE,0);
    connect_signal(s->window,"delete-event",G_CALLBACK(close_window),s);
    connect_signal(s->window,"destroy",G_CALLBACK(window_destroyed),s);
    connect_signal(s->folder,"file-set",G_CALLBACK(language_folder_changed),s);
    connect_signal(s->folder,"selection-changed",G_CALLBACK(language_folder_changed),s);
    connect_signal(s->music_check,"toggled",G_CALLBACK(music_toggled),s);
    connect_signal(s->button,"clicked",G_CALLBACK(install_clicked),s);
    connect_signal(s->close,"clicked",G_CALLBACK(close_clicked),s);
}
#ifndef RERIFF_INSTALLER_TEST
int main(int argc,char **argv) {
    if(argc!=1){fprintf(stderr,"Use the .run installer with --help for command-line options.\n");return 2;}
    if(!load_gtk()||!ui.gtk_init_check(&argc,&argv)){
        fprintf(stderr,"The graphical installer needs a desktop session and GTK 3. Use the .run file with --cli --game-dir '/path/to/Riff Racer' instead.\n");return 1;
    }
    char executable[PATH_MAX];ssize_t n=readlink("/proc/self/exe",executable,sizeof(executable)-1);
    if(n<0||n>=(ssize_t)sizeof(executable)-1)return 1;
    executable[n]=0;char *slash=strrchr(executable,'/');if(!slash)return 1;*slash=0;
    Setup s={0};s.bundle=executable;create_window(&s);

    char *runtime=ui.g_build_filename(s.bundle,"payload","localserver","ReRiff-ClientHost",NULL);
    char *detect[]={runtime,"--client-host","print-game-dir",NULL},*out=NULL,*err=NULL;int status=1;GError *error=NULL;
    if(ui.g_spawn_sync(NULL,detect,NULL,G_SPAWN_SEARCH_PATH,NULL,NULL,&out,&err,&status,&error)&&status==0&&out&&*out){
        out[strcspn(out,"\r\n")]=0;ui.gtk_file_chooser_set_filename((GtkFileChooser *)s.folder,out);
    }
    if(error)ui.g_error_free(error);ui.g_free(out);ui.g_free(err);
    out=NULL;err=NULL;error=NULL;detect[2]="print-music-dir";
    if(ui.g_spawn_sync(NULL,detect,NULL,G_SPAWN_SEARCH_PATH,NULL,NULL,&out,&err,&status,&error)&&status==0&&out&&*out){
        out[strcspn(out,"\r\n")]=0;ui.gtk_entry_set_text((GtkEntry *)s.music_path,out);
    }
    if(error)ui.g_error_free(error);ui.g_free(out);ui.g_free(err);ui.g_free(runtime);
    ui.gtk_widget_show_all(s.window);ui.gtk_main();if(s.temporary_log&&s.log_path)unlink(s.log_path);ui.g_free(s.log_path);
    return s.exit_code;
}
#endif
