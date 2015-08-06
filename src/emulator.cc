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
hcc::ROM rom;
GUIEmulatorRAM *ram;
hcc::CPU cpu;

bool running = false;

GtkWidget *window;
GtkToolItem *button_load, *button_run, *button_pause;

void load_clicked(GtkButton *button, gpointer user_data)
{
	bool loaded = false;

	GtkWidget *dialog = gtk_file_chooser_dialog_new("Load ROM",
		GTK_WINDOW(window), GTK_FILE_CHOOSER_ACTION_OPEN, GTK_STOCK_CANCEL,
		GTK_RESPONSE_CANCEL, GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT, NULL);
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

gpointer run_thread(gpointer user_data)
{
	int steps = 0;
	while (running) {
		cpu.step(&rom, ram);
		if (steps>100) {
			g_usleep(10);
			steps = 0;
		}
		++steps;
	}
	return NULL;
}
void run_clicked()
{
	assert(!running);

	running = true;
	gtk_widget_set_sensitive(GTK_WIDGET(button_run), FALSE);
	gtk_widget_set_visible(GTK_WIDGET(button_run), FALSE);
	gtk_widget_set_sensitive(GTK_WIDGET(button_pause), TRUE);
	gtk_widget_set_visible(GTK_WIDGET(button_pause), TRUE);

	g_thread_create(run_thread, NULL, FALSE, NULL);
}
void pause_clicked()
{
	assert(running);

	running = false;
	gtk_widget_set_sensitive(GTK_WIDGET(button_pause), FALSE);
	gtk_widget_set_visible(GTK_WIDGET(button_pause), FALSE);
	gtk_widget_set_sensitive(GTK_WIDGET(button_run), TRUE);
	gtk_widget_set_visible(GTK_WIDGET(button_run), TRUE);
}

gboolean keyboard_callback(GtkWidget *widget, GdkEventKey *event, gpointer user_data)
{
	if (event->type == GDK_KEY_RELEASE) {
		ram->keyboard(0);
	} else {
		// Translate special keys. See Figure 5.6 in TECS book.
		switch (event->keyval) {
		case GDK_KEY_Return:
			ram->keyboard(128);
			break;
		case GDK_KEY_BackSpace:
			ram->keyboard(129);
			break;
		case GDK_KEY_Left:
			ram->keyboard(130);
			break;
		case GDK_KEY_Up:
			ram->keyboard(131);
			break;
		case GDK_KEY_Right:
			ram->keyboard(132);
			break;
		case GDK_KEY_Down:
			ram->keyboard(133);
			break;
		case GDK_KEY_Home:
			ram->keyboard(134);
			break;
		case GDK_KEY_End:
			ram->keyboard(135);
			break;
		case GDK_KEY_Page_Up:
			ram->keyboard(136);
			break;
		case GDK_KEY_Page_Down:
			ram->keyboard(137);
			break;
		case GDK_KEY_Insert:
			ram->keyboard(138);
			break;
		case GDK_KEY_Delete:
			ram->keyboard(139);
			break;
		case GDK_KEY_Escape:
			ram->keyboard(140);
			break;
		case GDK_KEY_F1:
			ram->keyboard(141);
			break;
		case GDK_KEY_F2:
			ram->keyboard(142);
			break;
		case GDK_KEY_F3:
			ram->keyboard(143);
			break;
		case GDK_KEY_F4:
			ram->keyboard(144);
			break;
		case GDK_KEY_F5:
			ram->keyboard(145);
			break;
		case GDK_KEY_F6:
			ram->keyboard(146);
			break;
		case GDK_KEY_F7:
			ram->keyboard(147);
			break;
		case GDK_KEY_F8:
			ram->keyboard(148);
			break;
		case GDK_KEY_F9:
			ram->keyboard(149);
			break;
		case GDK_KEY_F10:
			ram->keyboard(150);
			break;
		case GDK_KEY_F11:
			ram->keyboard(151);
			break;
		case GDK_KEY_F12:
			ram->keyboard(152);
			break;
		default:
			ram->keyboard(event->keyval);
		}
	}
	return TRUE;
}


GtkToolItem *create_button(const gchar *stock_id, const gchar *text, GCallback callback)
{
	GtkToolItem *button = gtk_tool_button_new_from_stock(stock_id);
	gtk_tool_button_set_label(GTK_TOOL_BUTTON(button), text);
	gtk_tool_item_set_tooltip_text(button, text);
	g_signal_connect(button, "clicked", callback, NULL);
	return button;
}

int main(int argc, char *argv[])
{
	gtk_init(&argc, &argv);
	ram = new GUIEmulatorRAM();

	/* toolbar buttons */
	button_load    = create_button(GTK_STOCK_OPEN,        "Load...", G_CALLBACK(load_clicked));
	button_run     = create_button(GTK_STOCK_MEDIA_PLAY,  "Run",     G_CALLBACK(run_clicked));
	button_pause   = create_button(GTK_STOCK_MEDIA_PAUSE, "Pause",   G_CALLBACK(pause_clicked));

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
	g_signal_connect(keyboard, "key-press-event",   G_CALLBACK(keyboard_callback), NULL);
	g_signal_connect(keyboard, "key-release-event", G_CALLBACK(keyboard_callback), NULL);

	/* main layout */
	GtkWidget *grid = gtk_grid_new();
	gtk_grid_attach(GTK_GRID(grid), toolbar, 0, 0, 1, 1);
	gtk_grid_attach(GTK_GRID(grid), ram->getScreenWidget(), 0, 1, 1, 1);
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

	gtk_main();
	return 0;
}
