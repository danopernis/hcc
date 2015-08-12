// Copyright (c) 2012-2015 Dano Pernis
// See LICENSE for details

#include <gtk/gtk.h>
#include <glib.h>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <cassert>
#include <vector>
#include "CPU.h"

struct ROM : public hcc::IROM {
    ROM() : data(0x8000, 0) { }

    bool load(const char *filename)
    {
        std::ifstream input(filename);
        std::string line;
        auto it = begin(data);
        while (input.good() && it != end(data)) {
            getline(input, line);
            if (line.size() == 0) {
                continue;
            }
            if (line.size() != 16) {
                return false;
            }

            unsigned int instruction = 0;
            for (unsigned int i = 0; i<16; ++i) {
                instruction <<= 1;
                switch (line[i]) {
                case '0':
                    break;
                case '1':
                    instruction |= 1;
                    break;
                default:
                    return false;
                }
            }
            *it++ = instruction;
        }

        // clear the rest
        while (it != end(data)) {
            *it++ = 0;
        }

        return true;
    }

    uint16_t get(unsigned int address) const override
    { return data.at(address); }

private:
    std::vector<uint16_t> data;
};

gboolean on_draw(GtkWidget*, cairo_t* cr, gpointer data)
{
    GdkPixbuf* pixbuf = reinterpret_cast<GdkPixbuf*>(data);
    gdk_cairo_set_source_pixbuf(cr, pixbuf, 0, 0);
    cairo_paint(cr);
    return FALSE;
}

gboolean queue_redraw(gpointer widget)
{
    gtk_widget_queue_draw(GTK_WIDGET(widget));
    return FALSE;
}

struct RAM : public hcc::IRAM {
    static const unsigned int SCREEN_WIDTH = 512;
    static const unsigned int SCREEN_HEIGHT = 256;

    std::vector<uint16_t> data;
    GdkPixbuf *pixbuf;
    GtkWidget *screen;

public:
    RAM() : data(0x6001, 0)
    {
        pixbuf = gdk_pixbuf_new(GDK_COLORSPACE_RGB, FALSE, 8, SCREEN_WIDTH, SCREEN_HEIGHT);
        gdk_pixbuf_fill(pixbuf, 0x0000000);

        screen = gtk_drawing_area_new();
        gtk_widget_set_size_request(screen, SCREEN_WIDTH, SCREEN_HEIGHT);
        g_signal_connect(screen, "draw", G_CALLBACK(on_draw), pixbuf);
    }

    void keyboard(uint16_t value) {
        data[0x6000] = value;
    }

    GtkWidget* getScreenWidget() {
        return screen;
    }

    void set(unsigned int address, uint16_t value) override
    {
        data.at(address) = value;
    }

    void redraw()
    {
        const auto rowstride = gdk_pixbuf_get_rowstride(pixbuf);
        auto first = begin(data) + 0x4000;
        auto last  = begin(data) + 0x6000;
        guchar* row = gdk_pixbuf_get_pixels(pixbuf);
        uint16_t value = 0;

        for (unsigned int y = 0; y < SCREEN_HEIGHT; ++y) {
            guchar* p = row;

            for (unsigned int x = 0; x < SCREEN_WIDTH; ++x) {
                if ((x % 16) == 0) {
                    assert (first != last);
                    value = *first++;
                }

                const auto color = (value & 1) ? 0x00 : 0xff;
                value = value >> 1;

                *p++ = color;
                *p++ = color;
                *p++ = color;
            }
            row += rowstride;
        }
    }

    uint16_t get(unsigned int address) const override
    { return data.at(address); }
};

struct emulator {
    emulator();
    void load_clicked();
    void run_clicked();
    void pause_clicked();
    gboolean keyboard_callback(GdkEventKey* event);
    void cpu_thread();
    void screen_thread();
    GtkToolItem* create_button(const gchar* stock_id, const gchar* text, GCallback callback);

    ROM rom;
    RAM ram;
    hcc::CPU cpu;

    bool running = false;

    GtkWidget* window;
    GtkWidget* load_dialog;
    GtkWidget* error_dialog;
    GtkToolItem* button_load;
    GtkToolItem* button_run;
    GtkToolItem* button_pause;
};

gboolean c_keyboard_callback(GtkWidget*, GdkEventKey *event, gpointer user_data)
{ return reinterpret_cast<emulator*>(user_data)->keyboard_callback(event); }

void c_load_clicked(GtkButton*, gpointer user_data)
{ reinterpret_cast<emulator*>(user_data)->load_clicked(); }

void c_run_clicked(GtkButton*, gpointer user_data)
{ reinterpret_cast<emulator*>(user_data)->run_clicked(); }

void c_pause_clicked(GtkButton*, gpointer user_data)
{ reinterpret_cast<emulator*>(user_data)->pause_clicked(); }

gpointer c_cpu_thread(gpointer user_data)
{
    reinterpret_cast<emulator*>(user_data)->cpu_thread();
    return NULL;
}

gpointer c_screen_thread(gpointer user_data)
{
    reinterpret_cast<emulator*>(user_data)->screen_thread();
    return NULL;
}

// Translate special keys. See Figure 5.6 in TECS book.
uint16_t translate(guint keyval)
{
    switch (keyval) {
    case GDK_KEY_Return:    return 128;
    case GDK_KEY_BackSpace: return 129;
    case GDK_KEY_Left:      return 130;
    case GDK_KEY_Up:        return 131;
    case GDK_KEY_Right:     return 132;
    case GDK_KEY_Down:      return 133;
    case GDK_KEY_Home:      return 134;
    case GDK_KEY_End:       return 135;
    case GDK_KEY_Page_Up:   return 136;
    case GDK_KEY_Page_Down: return 137;
    case GDK_KEY_Insert:    return 138;
    case GDK_KEY_Delete:    return 139;
    case GDK_KEY_Escape:    return 140;
    case GDK_KEY_F1:        return 141;
    case GDK_KEY_F2:        return 142;
    case GDK_KEY_F3:        return 143;
    case GDK_KEY_F4:        return 144;
    case GDK_KEY_F5:        return 145;
    case GDK_KEY_F6:        return 146;
    case GDK_KEY_F7:        return 147;
    case GDK_KEY_F8:        return 148;
    case GDK_KEY_F9:        return 149;
    case GDK_KEY_F10:       return 150;
    case GDK_KEY_F11:       return 151;
    case GDK_KEY_F12:       return 152;
    }
    return keyval;
}

emulator::emulator()
{
    /* toolbar buttons */
    button_load    = create_button("document-open",        "Load...", G_CALLBACK(c_load_clicked));
    button_run     = create_button("media-playback-start", "Run",     G_CALLBACK(c_run_clicked));
    button_pause   = create_button("media-playback-pause", "Pause",   G_CALLBACK(c_pause_clicked));

    GtkToolItem *separator1 = gtk_separator_tool_item_new();

    gtk_widget_set_sensitive(GTK_WIDGET(button_run), FALSE);
    gtk_widget_set_sensitive(GTK_WIDGET(button_pause), FALSE);

    /* toolbar itself */
    GtkWidget *toolbar = gtk_toolbar_new();
    gtk_widget_set_hexpand(toolbar, TRUE);
    gtk_toolbar_set_style(GTK_TOOLBAR(toolbar), GTK_TOOLBAR_ICONS);
    gtk_toolbar_insert(GTK_TOOLBAR(toolbar), button_load,  -1);
    gtk_toolbar_insert(GTK_TOOLBAR(toolbar), separator1,   -1);
    gtk_toolbar_insert(GTK_TOOLBAR(toolbar), button_run,   -1);
    gtk_toolbar_insert(GTK_TOOLBAR(toolbar), button_pause, -1);

    /* keyboard */
    GtkWidget *keyboard = gtk_toggle_button_new_with_label("Grab keyboard focus");
    gtk_widget_add_events(keyboard, GDK_KEY_PRESS_MASK | GDK_KEY_RELEASE_MASK);
    g_signal_connect(keyboard, "key-press-event",   G_CALLBACK(c_keyboard_callback), this);
    g_signal_connect(keyboard, "key-release-event", G_CALLBACK(c_keyboard_callback), this);

    /* main layout */
    GtkWidget *grid = gtk_grid_new();
    gtk_grid_attach(GTK_GRID(grid), toolbar, 0, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), ram.getScreenWidget(), 0, 1, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), keyboard, 0, 2, 1, 1);

    /* main window */
    window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(window), "HACK emulator");
    gtk_window_set_resizable(GTK_WINDOW(window), FALSE);
    gtk_window_set_focus(GTK_WINDOW(window), NULL);
    g_signal_connect(window, "destroy", G_CALLBACK (gtk_main_quit), NULL);
    gtk_container_add(GTK_CONTAINER(window), grid);
    gtk_widget_show_all(window);
    gtk_widget_set_visible(GTK_WIDGET(button_pause), FALSE);

    load_dialog = gtk_file_chooser_dialog_new(
        "Load ROM", GTK_WINDOW(window), GTK_FILE_CHOOSER_ACTION_OPEN,
        "gtk-cancel", GTK_RESPONSE_CANCEL,
        "gtk-open", GTK_RESPONSE_ACCEPT,
        NULL);

    error_dialog = gtk_message_dialog_new(GTK_WINDOW(window),
        GTK_DIALOG_MODAL, GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE,
        "Error loading program");
}

GtkToolItem* emulator::create_button(const gchar* stock_id, const gchar* text, GCallback callback)
{
    GtkToolItem *button = gtk_tool_button_new(NULL, text);
    gtk_tool_button_set_icon_name(GTK_TOOL_BUTTON(button), stock_id);
    gtk_tool_item_set_tooltip_text(button, text);
    g_signal_connect(button, "clicked", callback, this);
    return button;
}

void emulator::load_clicked()
{
    const gint result = gtk_dialog_run(GTK_DIALOG(load_dialog));
    gtk_widget_hide(load_dialog);

    if (result != GTK_RESPONSE_ACCEPT) {
        return;
    }

    char *filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(load_dialog));
    const bool loaded = rom.load(filename);
    g_free(filename);

    if (!loaded) {
        gtk_dialog_run(GTK_DIALOG(error_dialog));
        gtk_widget_hide(error_dialog);
        return;
    }

    cpu.reset();
    gtk_widget_set_sensitive(GTK_WIDGET(button_run), TRUE);
}

void emulator::run_clicked()
{
    assert(!running);

    running = true;
    gtk_widget_set_sensitive(GTK_WIDGET(button_run), FALSE);
    gtk_widget_set_visible(GTK_WIDGET(button_run), FALSE);
    gtk_widget_set_sensitive(GTK_WIDGET(button_pause), TRUE);
    gtk_widget_set_visible(GTK_WIDGET(button_pause), TRUE);

    g_thread_new("c_cpu_thread",    c_cpu_thread,    this);
    g_thread_new("c_screen_thread", c_screen_thread, this);
}

void emulator::pause_clicked()
{
    assert(running);

    running = false;
    gtk_widget_set_sensitive(GTK_WIDGET(button_pause), FALSE);
    gtk_widget_set_visible(GTK_WIDGET(button_pause), FALSE);
    gtk_widget_set_sensitive(GTK_WIDGET(button_run), TRUE);
    gtk_widget_set_visible(GTK_WIDGET(button_run), TRUE);
}

gboolean emulator::keyboard_callback(GdkEventKey* event)
{
    if (event->type == GDK_KEY_RELEASE) {
        ram.keyboard(0);
    } else {
        ram.keyboard(translate(event->keyval));
    }
    return TRUE;
}

void emulator::cpu_thread()
{
    int steps = 0;
    while (running) {
        cpu.step(&rom, &ram);
        if (steps>100) {
            g_usleep(10);
            steps = 0;
        }
        ++steps;
    }
}

void emulator::screen_thread()
{
    while (running) {
        ram.redraw();
        gdk_threads_add_idle(queue_redraw, ram.getScreenWidget());
        g_usleep(10000);
    }
}

int main(int argc, char *argv[])
{
    gtk_init(&argc, &argv);
    emulator e;
    gtk_main();
    return 0;
}
