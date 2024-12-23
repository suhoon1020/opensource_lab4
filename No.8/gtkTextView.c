#include <gtk/gtk.h>

typedef struct
{
    GtkWidget *entry, *textview;
} Widgets;
/* GtkEntry 로부터 GtkTextView 로 텍스트 삽입 */
static void insert_text(GtkButton *button, Widgets *w)
{
    GtkTextBuffer *buffer;
    GtkTextMark *mark;
    GtkTextIter iter;
    const gchar *text;
    buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(w->textview));
    text = gtk_entry_get_text(GTK_ENTRY(w->entry));
    mark = gtk_text_buffer_get_insert(buffer);
    gtk_text_buffer_get_iter_at_mark(buffer, &iter,
                                     mark);
    /* 버퍼에 텍스트가 있으면 뉴라인 삽입 */
    if (gtk_text_buffer_get_char_count(buffer))
        gtk_text_buffer_insert(buffer, &iter, "\n", 1);
    gtk_text_buffer_insert(buffer, &iter, text, -1);
    mark = gtk_text_buffer_get_insert(buffer);
    gtk_text_view_scroll_mark_onscreen(GTK_TEXT_VIEW(w->textview), mark);
}
int main(int argc, char *argv[])
{
    GtkWidget *window, *scrolled_win, *hbox, *vbox,
        *insert;
    Widgets *w = g_slice_new(Widgets);
    gtk_init(&argc, &argv);
    window =
        gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(window), "텍스트뷰");
    gtk_container_set_border_width(GTK_CONTAINER(window), 10);
    gtk_widget_set_size_request(window, -1, 200);
    w->textview = gtk_text_view_new();
    w->entry = gtk_entry_new();
    insert = gtk_button_new_with_label("Insert Text");
    g_signal_connect(insert, "clicked",
                     G_CALLBACK(insert_text), (gpointer)w);
    scrolled_win = gtk_scrolled_window_new(NULL,
                                           NULL);
    gtk_container_add(GTK_CONTAINER(scrolled_win),
                      w->textview);
    hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    gtk_box_pack_start(GTK_BOX(hbox), w->entry, FALSE,
                       FALSE, 5);
    gtk_box_pack_start(GTK_BOX(hbox), insert, FALSE,
                       FALSE, 5);
    vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_box_pack_start(GTK_BOX(vbox), scrolled_win,
                       TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE,
                       TRUE, 0);
    gtk_container_add(GTK_CONTAINER(window), vbox);
    gtk_widget_show_all(window);
    gtk_main();
    return 0;
}
