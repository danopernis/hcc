// Copyright (c) 2012-2015 Dano Pernis
// See LICENSE for details

#include "CPU.h"
#include <cassert>
#include <fstream>
#include <gtkmm.h>
#include <iostream>
#include <mutex>
#include <stdexcept>
#include <thread>
#include <vector>


// sigc workaround
namespace sigc {
SIGC_FUNCTORS_DEDUCE_RESULT_TYPE_WITH_DECLTYPE
}


namespace {


const unsigned int SCREEN_WIDTH = 512;
const unsigned int SCREEN_HEIGHT = 256;


struct ROM : public hcc::IROM {
    ROM() : data(0x8000, 0) { }

    uint16_t get(unsigned int address) const override
    { return data.at(address); }

    bool load(const char* filename);

    std::vector<uint16_t> data;
};


struct RAM : public hcc::IRAM {
    RAM() : data(0x6001, 0) { }

    uint16_t get(unsigned int address) const override
    { return data.at(address); }

    void set(unsigned int address, uint16_t value) override
    { data.at(address) = value; }

    void keyboard(uint16_t value) { set(0x6000, value); };

    std::vector<uint16_t> data;
};


struct screen_widget : Gtk::DrawingArea {
    screen_widget(RAM& ram);

    bool draw(const Cairo::RefPtr<Cairo::Context>& cr);

private:
    Glib::RefPtr<Gdk::Pixbuf> pixbuf;
    RAM& ram;
};


struct emulator {
    emulator();
    ~emulator() { pause(); }
    void load_clicked();
    void run_clicked();
    void pause_clicked();
    bool keyboard_callback(GdkEventKey* event);
    void cpu_thread();
    void screen_thread();
    void setup_button(Gtk::ToolButton& button, const gchar* stock_id, const gchar* text);

    ROM rom;
    RAM ram;
    hcc::CPU cpu;

private:
    std::thread thread_cpu;
    std::thread thread_screen;
    std::mutex running_mutex;
    bool running = false;
public:
    void run()
    {
        std::lock_guard<std::mutex> lock(running_mutex);
        if (running) {
            return;
        }
        running = true;
        thread_cpu = std::thread([&] { cpu_thread(); });
        thread_screen = std::thread([&] { screen_thread(); });
    }
    void pause()
    {
         std::lock_guard<std::mutex> lock(running_mutex);
         if (!running) {
             return;
         }
         running = false;
         thread_cpu.join();
         thread_screen.join();
    }

    Gtk::SeparatorToolItem separator;
    Gtk::Toolbar toolbar;
    Gtk::ToggleButton keyboard;
    Gtk::Grid grid;
    Gtk::Window window;
    Gtk::FileChooserDialog load_dialog;
    Gtk::MessageDialog error_dialog;
    Gtk::ToolButton button_load;
    Gtk::ToolButton button_run;
    Gtk::ToolButton button_pause;
    screen_widget screen;
};


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


bool ROM::load(const char* filename)
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


screen_widget::screen_widget(RAM& ram)
    : pixbuf(
        Gdk::Pixbuf::create(Gdk::Colorspace::COLORSPACE_RGB, false, 8, SCREEN_WIDTH, SCREEN_HEIGHT))
    , ram(ram)
{
    set_size_request(SCREEN_WIDTH, SCREEN_HEIGHT);
    signal_draw().connect([&] (const Cairo::RefPtr<Cairo::Context>& cr) { return draw(cr); });
}


bool screen_widget::draw(const Cairo::RefPtr<Cairo::Context>& cr)
{
    const auto rowstride = pixbuf->get_rowstride();
    auto first = begin(ram.data) + 0x4000;
    auto last  = begin(ram.data) + 0x6000;
    guchar* row = pixbuf->get_pixels();
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

    Gdk::Cairo::set_source_pixbuf(cr, pixbuf, 0, 0);
    cr->paint();
    return false;
}


emulator::emulator()
    : keyboard("Grab keyboard focus")
    , load_dialog(window, "Load ROM")
    , error_dialog(window, "Error loading program", false, Gtk::MessageType::MESSAGE_ERROR,
        Gtk::ButtonsType::BUTTONS_CLOSE, true)
    , screen(ram)
{
    /* toolbar buttons */
    setup_button(button_load,  "document-open",        "Load...");
    setup_button(button_run,   "media-playback-start", "Run");
    setup_button(button_pause, "media-playback-pause", "Pause");
    button_load.signal_clicked().connect([&] () { load_clicked(); });
    button_run.signal_clicked().connect([&] () { run_clicked(); });
    button_pause.signal_clicked().connect([&] () { pause_clicked(); });

    button_run.set_sensitive(false);
    button_pause.set_sensitive(false);

    /* toolbar itself */
    toolbar.set_hexpand(true);
    toolbar.set_toolbar_style(Gtk::ToolbarStyle::TOOLBAR_ICONS);
    toolbar.insert(button_load,  -1);
    toolbar.insert(separator,    -1);
    toolbar.insert(button_run,   -1);
    toolbar.insert(button_pause, -1);

    /* keyboard */
    keyboard.add_events(Gdk::EventMask::KEY_PRESS_MASK | Gdk::EventMask::KEY_RELEASE_MASK);
    keyboard.signal_key_press_event().connect([&] (GdkEventKey* event) {
        return keyboard_callback(event);
    });
    keyboard.signal_key_release_event().connect([&] (GdkEventKey* event) {
        return keyboard_callback(event);
    });

    /* main layout */
    grid.attach(toolbar, 0, 0, 1, 1);
    grid.attach(screen, 0, 1, 1, 1);
    grid.attach(keyboard, 0, 2, 1, 1);

    /* main window */
    window.set_title("HACK emulator");
    window.set_resizable(false);
    window.add(grid);
    window.show_all();
    button_pause.set_visible(false);

    load_dialog.add_button("gtk-cancel", GTK_RESPONSE_CANCEL);
    load_dialog.add_button("gtk-open", GTK_RESPONSE_ACCEPT);
}


void emulator::setup_button(Gtk::ToolButton& button, const gchar* stock_id, const gchar* text)
{
    button.set_label(text);
    button.set_tooltip_text(text);
    button.set_icon_name(stock_id);
}


void emulator::load_clicked()
{
    const gint result = load_dialog.run();
    load_dialog.hide();

    if (result != GTK_RESPONSE_ACCEPT) {
        return;
    }

    auto filename = load_dialog.get_filename();
    const bool loaded = rom.load(filename.c_str());

    if (!loaded) {
        error_dialog.run();
        error_dialog.hide();
        return;
    }

    cpu.reset();
    button_run.set_sensitive(true);
}


void emulator::run_clicked()
{
    run();

    button_run.set_sensitive(false);
    button_run.set_visible(false);
    button_pause.set_sensitive(true);
    button_pause.set_visible(true);
}


void emulator::pause_clicked()
{
    pause();

    button_pause.set_sensitive(false);
    button_pause.set_visible(false);
    button_run.set_sensitive(true);
    button_run.set_visible(true);
}


bool emulator::keyboard_callback(GdkEventKey* event)
{
    if (event->type == GDK_KEY_RELEASE) {
        ram.keyboard(0);
    } else {
        ram.keyboard(translate(event->keyval));
    }
    return true;
}


void emulator::cpu_thread()
{
    int steps = 0;
    while (running) {
        cpu.step(&rom, &ram);
        if (steps>100) {
            std::this_thread::sleep_for(std::chrono::microseconds(10));
            steps = 0;
        }
        ++steps;
    }
}


void emulator::screen_thread()
{
    while (running) {
        Glib::signal_idle().connect_once([&] () { screen.queue_draw(); });
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}


} // anonymous namespace


int main(int argc, char *argv[])
{
    Glib::RefPtr<Gtk::Application> app = Gtk::Application::create(
        argc, argv, "hcc.emulator");
    emulator e;
    return app->run(e.window);
}
