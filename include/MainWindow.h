#ifndef MAINWINDOW_H_
#define MAINWINDOW_H_

#include <gtkmm.h>
#include <cairomm/cairomm.h>
#include <mutex>
#include <thread>
#include <opencv2/opencv.hpp>
#include <iostream>
#include <string>
#ifdef GDK_WINDOWING_X11
#include <gdk/gdkx.h> // For GDK_WINDOW_XID
#endif
#include <gst/gst.h>
#include <gst/video/videooverlay.h>

#include <linux/videodev2.h>
#include <fcntl.h>
#include <sys/ioctl.h>

#include"KeyPad.h"

class CustomDrawingArea : public Gtk::DrawingArea {
public:
    // CustomDrawingArea();

protected:
    bool on_draw(const Cairo::RefPtr<Cairo::Context>& cr);
};

// Custom Drawing Area
// class CustomDrawingArea : public Gtk::DrawingArea {
// protected:
//     bool on_draw(const Cairo::RefPtr<Cairo::Context>& cr) override {
//         cr->set_source_rgb(1.0, 1.0, 1.0); 
//         cr->paint();

//         cr->set_source_rgb(0.2, 0.6, 0.8); // blue rectangle
//         cr->rectangle(50, 50, 100, 50);
//         cr->fill();

//         cr->set_source_rgb(0.9, 0.3, 0.3); // red circle
//         cr->arc(100, 150, 40, 0, 2 * M_PI);
//         cr->fill();

//         return true;
//     }
// };

class MainWindow : public Gtk::Window {
public:
    
    MainWindow();
    virtual ~MainWindow();

    FT232HHandler* gpio_handler;

protected:

    Gtk::Box m_VBox;
    Gtk::Box m_ButtonBox; // Horizontal box for buttons
    Gtk::DrawingArea m_DrawingArea;
    Gtk::Button m_Button1, m_Button2, m_Button3, m_Button4;
    
    GstElement *pipeline = nullptr;
    GstElement *source = nullptr;
    GstElement *capsfilter = nullptr;
    GstElement *jpegdec = nullptr;
    // jpegdec = gst_element_factory_make("jpegdec", "jpegdec");
    GstElement *convert = nullptr;
    GstElement *crop = nullptr;
    GstElement *sink = nullptr;

    int zoom_level = 0; // Initial zoom level
    bool awb_enabled = true; // Auto White Balance state

    void on_play();
    void on_pause();
    void on_zoom();
    void on_awb();
    void apply_zoom();
    void on_drawing_area_realized();
    double awb_temperature(const std::string& imagePath);
    bool set_video_overlay();   
    void change_resolution(int width, int height);

    void add_button(Gtk::Button& button, const Glib::ustring& label, int id);
    void handle_button_press(int button);

};


#endif // SINGLEIMAGEWINDOW_H_