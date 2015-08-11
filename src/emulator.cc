// Copyright (c) 2012-2015 Dano Pernis
// See LICENSE for details

#include <gtk/gtk.h>
#include <glib.h>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <cassert>
#include "CPU.h"

namespace hcc {

struct ROM : public IROM {
    unsigned short *data;
    static const unsigned int size = 0x8000;
    ROM() {
        data = new unsigned short[size];
    }
    virtual ~ROM() {
        delete[] data;
    }

    bool load(const char *filename) {
        std::ifstream input(filename);
        std::string line;
        unsigned int counter = 0;
        while (input.good() && counter < size) {
            getline(input, line);
            if (line.size() == 0)
                continue;
            if (line.size() != 16)
                return false;

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
            data[counter++] = instruction;
        }
        // clear the rest
        while (counter < size) {
            data[counter++] = 0;
        }

        return true;
    }
    virtual unsigned short get(unsigned int address) const {
        if (address < size) {
            return data[address];
        } else {
            std::cerr << "requested memory at " << address << '\n';
            throw std::runtime_error("Memory::get");
        }
    }
};

} // end namespace

struct GUIEmulatorRAM : public hcc::IRAM {
    static const unsigned int CHANNELS = 3;
    static const unsigned int SCREEN_WIDTH = 512;
    static const unsigned int SCREEN_HEIGHT = 256;
    static const unsigned int size = 0x6001;

    unsigned short *data;
    unsigned char *vram;
    GdkPixbuf *pixbuf;
    GtkWidget *screen;

    void putpixel(unsigned short x, unsigned short y, bool black) {
        unsigned int offset = CHANNELS*(SCREEN_WIDTH*y + x);
        for (unsigned int channel = 0; channel<CHANNELS; ++channel) {
            vram[offset++] = black ? 0x00 : 0xff;
        }
    }
public:
    GUIEmulatorRAM() {
        data = new unsigned short[size];
        vram = new unsigned char[CHANNELS*SCREEN_WIDTH*SCREEN_HEIGHT];
        pixbuf = gdk_pixbuf_new_from_data(vram, GDK_COLORSPACE_RGB, FALSE, 8, SCREEN_WIDTH, SCREEN_HEIGHT, CHANNELS*SCREEN_WIDTH, NULL, NULL);
        screen = gtk_image_new_from_pixbuf(pixbuf);
    }
    virtual ~GUIEmulatorRAM() {
        delete[] data;
        delete[] vram;
    }
    void keyboard(unsigned short value) {
        data[0x6000] = value;
    }
    GtkWidget* getScreenWidget() {
        return screen;
    }
    virtual void set(unsigned int address, unsigned short value) {
        if (address >= size) {
            throw std::runtime_error("RAM::set");
        }

        data[address] = value;

        // check if we are writing to video RAM
        if (0x4000 <= address && address <0x6000) {
            address -= 0x4000;

            unsigned short y = address / 32;
            unsigned short x = 16*(address % 32);
            for (int bit = 0; bit<16; ++bit) {
                putpixel(x + bit, y, value & 1);
                value = value >> 1;
            }
            gdk_threads_enter();
            gtk_widget_queue_draw(screen);
            gdk_threads_leave();
        }
    }
    virtual unsigned short get(unsigned int address) const {
        if (address >= size) {
            throw std::runtime_error("RAM::get");
        }

        return data[address];
    }
};

struct emulator {
    emulator();
    void load_clicked();
    void run_clicked();
    void pause_clicked();
    gboolean keyboard_callback(GdkEventKey* event);
    void run_thread();
    GtkToolItem* create_button(const gchar* stock_id, const gchar* text, GCallback callback);

    hcc::ROM rom;
    GUIEmulatorRAM ram;
    hcc::CPU cpu;

    bool running = false;

    GtkWidget* window;
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

gpointer c_run_thread(gpointer user_data)
{
    reinterpret_cast<emulator*>(user_data)->run_thread();
    return NULL;
}

// Translate special keys. See Figure 5.6 in TECS book.
unsigned short translate(guint keyval)
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
    bool loaded = false;

    GtkWidget *dialog = gtk_file_chooser_dialog_new(
        "Load ROM", GTK_WINDOW(window), GTK_FILE_CHOOSER_ACTION_OPEN,
        "gtk-cancel", GTK_RESPONSE_CANCEL,
        "gtk-open", GTK_RESPONSE_ACCEPT,
        NULL);
    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
        char *filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
        loaded = rom.load(filename);
        g_free(filename);
    }
    gtk_widget_destroy(dialog);

    if (!loaded) {
        GtkWidget *dialog = gtk_message_dialog_new(GTK_WINDOW(window),
            GTK_DIALOG_MODAL, GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE,
            "Error loading program");
        gtk_dialog_run(GTK_DIALOG(dialog));
        gtk_widget_destroy(dialog);
    } else {
        cpu.reset();
        gtk_widget_set_sensitive(GTK_WIDGET(button_run), TRUE);
    }
}

void emulator::run_clicked()
{
    assert(!running);

    running = true;
    gtk_widget_set_sensitive(GTK_WIDGET(button_run), FALSE);
    gtk_widget_set_visible(GTK_WIDGET(button_run), FALSE);
    gtk_widget_set_sensitive(GTK_WIDGET(button_pause), TRUE);
    gtk_widget_set_visible(GTK_WIDGET(button_pause), TRUE);

    g_thread_create(c_run_thread, this, FALSE, NULL);
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

void emulator::run_thread()
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

int main(int argc, char *argv[])
{
    gtk_init(&argc, &argv);
    emulator e;
    gtk_main();
    return 0;
}
