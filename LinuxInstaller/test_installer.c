#define RERIFF_INSTALLER_TEST
#include "installer.c"
static Setup test;
static char *game,*invalid;
static int stage,ticks,failed;
#define VERIFY(c) do{if(!(c)){fprintf(stderr,"FAIL %d: %s\n",__LINE__,#c);failed=1;ui.gtk_main_quit();return FALSE;}}while(0)
static gboolean exercise(gpointer unused){
    (void)unused;
    VERIFY(++ticks<160);
    char *selected=ui.gtk_file_chooser_get_filename((GtkFileChooser *)test.folder);
    if(stage==0){
        if(!selected||strcmp(selected,invalid)){ui.g_free(selected);return TRUE;}
        install_clicked(NULL,&test);VERIFY(!test.child&&!test.log_path);
        ui.gtk_file_chooser_set_filename((GtkFileChooser *)test.folder,game);stage=1;
    }else if(stage==1){
        if(!selected||strcmp(selected,game)){ui.g_free(selected);return TRUE;}
        VERIFY(!test.child&&!test.finished); /* Choosing a folder is read-only. */
        VERIFY(ui.gtk_toggle_button_get_active((GtkToggleButton *)test.cjk_check)); /* Programmatic folder selection restores the prototype choice. */
        ui.gtk_toggle_button_set_active((GtkToggleButton *)test.cjk_check,FALSE);
        install_clicked(NULL,&test);VERIFY(test.child>0&&test.log_path);
        pid_t child=test.child;install_clicked(NULL,&test);VERIFY(test.child==child);
        VERIFY(close_window(NULL,NULL,&test));stage=2;
    }else if(stage==2&&test.finished){
        VERIFY(test.exit_code==7&&!test.child); /* First fixture run fails. */
        VERIFY(access(test.log_path,R_OK)==0);
        ui.gtk_toggle_button_set_active((GtkToggleButton *)test.cjk_check,TRUE);
        ui.gtk_toggle_button_set_active((GtkToggleButton *)test.music_check,TRUE);
        ui.gtk_entry_set_text((GtkEntry *)test.music_path,game);
        install_clicked(NULL,&test);VERIFY(test.child>0);stage=3;
    }else if(stage==3&&test.finished){
        VERIFY(!test.exit_code&&!test.child&&access(test.log_path,R_OK)==0);
        FILE *f=fopen(test.log_path,"r");VERIFY(f);char out[1024]={0};fread(out,1,sizeof(out)-1,f);fclose(f);
        VERIFY(strstr(out,"GUI install succeeded")&&strstr(out,game));
        ui.g_free(selected);ui.gtk_widget_destroy(test.window);return FALSE;
    }
    ui.g_free(selected);return TRUE;
}
int main(int argc,char **argv){
    if(argc!=4||!load_gtk()||!ui.gtk_init_check(&argc,&argv))return 1;
    test.bundle=argv[1];game=argv[2];invalid=argv[3];
    create_window(&test);ui.gtk_toggle_button_set_active((GtkToggleButton *)test.music_check,FALSE);ui.gtk_widget_show_all(test.window);
    ui.gtk_file_chooser_set_filename((GtkFileChooser *)test.folder,invalid);
    ui.g_timeout_add(50,exercise,NULL);ui.gtk_main();
    if(!failed&&stage==3)puts("PASS: GTK folder selection, invalid path, explicit install, close guard, child error, retry, success and persistent log");
    else failed=1;
    ui.g_free(test.log_path);return failed;
}
