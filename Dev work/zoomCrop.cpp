#include <gtkmm.h>
#include <gst/gst.h>
#include <gst/video/videooverlay.h>
#include <iostream>

#ifdef GDK_WINDOWING_X11
#include <gdk/gdkx.h> // For GDK_WINDOW_XID
#endif

class CameraWindow : public Gtk::Window {
public:
    CameraWindow();
    virtual ~CameraWindow();

private:
    Gtk::Box m_VBox;
    Gtk::Box m_ButtonBox; // Horizontal box for buttons
    Gtk::DrawingArea m_DrawingArea;
    Gtk::Button m_PlayButton, m_PauseButton, m_ZoomButton, m_AwbButton;

    GstElement *pipeline = nullptr;
    GstElement *source = nullptr;
    GstElement *convert = nullptr;
    GstElement *crop = nullptr;
    GstElement *capsfilter = nullptr;
    GstElement *sink = nullptr;

    int zoom_level = 0; // Initial zoom level
    void on_play();
    void on_pause();
    void on_zoom();
    void on_awb();
    void apply_zoom();
    void on_drawing_area_realized();
    bool set_video_overlay();
};

CameraWindow::CameraWindow()
    : m_VBox(Gtk::ORIENTATION_VERTICAL),
      m_ButtonBox(Gtk::ORIENTATION_HORIZONTAL),
      m_PlayButton("Play"),
      m_PauseButton("Pause"),
      m_ZoomButton("Zoom +/-"),
      m_AwbButton("AWB") {

    set_title("GStreamer Video Zoom");
    set_default_size(1920, 1080);

    add(m_VBox);

    m_DrawingArea.set_size_request(1280, 720);
    m_VBox.pack_start(m_DrawingArea, Gtk::PACK_EXPAND_WIDGET);

    // Add buttons to the horizontal box
    m_ButtonBox.pack_start(m_PlayButton, Gtk::PACK_SHRINK);
    m_ButtonBox.pack_start(m_PauseButton, Gtk::PACK_SHRINK);
    m_ButtonBox.pack_start(m_ZoomButton, Gtk::PACK_SHRINK);
    m_ButtonBox.pack_start(m_AwbButton, Gtk::PACK_SHRINK);

    // Add the button box to the main vertical box
    m_VBox.pack_start(m_ButtonBox, Gtk::PACK_SHRINK);

    // Connect button signals
    m_PlayButton.signal_clicked().connect(sigc::mem_fun(*this, &CameraWindow::on_play));
    m_PauseButton.signal_clicked().connect(sigc::mem_fun(*this, &CameraWindow::on_pause));
    m_ZoomButton.signal_clicked().connect(sigc::mem_fun(*this, &CameraWindow::on_zoom));
    m_AwbButton.signal_clicked().connect(sigc::mem_fun(*this, &CameraWindow::on_awb));
    m_DrawingArea.signal_realize().connect(sigc::mem_fun(*this, &CameraWindow::on_drawing_area_realized));

    // Initialize GStreamer
    gst_init(nullptr, nullptr);

    // Create GStreamer elements
    pipeline = gst_pipeline_new("video-pipeline");
    source = gst_element_factory_make("v4l2src", "source");
    crop = gst_element_factory_make("videocrop", "crop");
    convert = gst_element_factory_make("videoconvert", "convert");
    sink = gst_element_factory_make("glimagesink", "sink");

    if (!pipeline || !source || !crop || !convert || !sink) {
        std::cerr << "Failed to create GStreamer elements." << std::endl;
        return;
    }

    // Set camera device
    g_object_set(source, "device", "/dev/video0", nullptr);

    // Add and link elements
    gst_bin_add_many(GST_BIN(pipeline), source, crop, convert, sink, nullptr);
    if (!gst_element_link_many(source, crop, convert, sink, nullptr)) {
        std::cerr << "Failed to link GStreamer elements." << std::endl;
    }

    show_all_children();
}

CameraWindow::~CameraWindow() {
    if (pipeline) {
        gst_element_set_state(pipeline, GST_STATE_NULL);
        gst_object_unref(pipeline);
    }
}

void CameraWindow::on_play() {
    gst_element_set_state(pipeline, GST_STATE_PLAYING);
    std::cout << "Pipeline playing..." << std::endl;
}

void CameraWindow::on_pause() {
    gst_element_set_state(pipeline, GST_STATE_PAUSED);
    std::cout << "Pipeline paused." << std::endl;
}

void CameraWindow::on_zoom() {
    // Increment zoom level
    zoom_level = (zoom_level + 1) % 4; // Cycle through 4 zoom levels (0-3)
    apply_zoom();
    std::cout << "Zoom level: " << zoom_level << std::endl;
}

void CameraWindow::on_awb() {
    // Simulate AWB button functionality
    std::cout << "AWB button clicked (functionality not implemented)." << std::endl;
}

void CameraWindow::apply_zoom() {
    // Crop values for each zoom level
    int crop_values[4][4] = {
        {0, 0, 0, 0},       // No crop (zoom level 0)
        {160, 160, 160, 160}, // Moderate zoom (zoom level 1)
        {320, 320, 320, 320}, // Higher zoom (zoom level 2)
        {480, 480, 480, 480}  // Maximum zoom (zoom level 3)
    };

    // Set crop properties based on the zoom level
    g_object_set(crop,
                 "left", crop_values[zoom_level][0],
                 "right", crop_values[zoom_level][1],
                 "top", crop_values[zoom_level][2],
                 "bottom", crop_values[zoom_level][3],
                 nullptr);

    // Restart pipeline to apply changes
    gst_element_set_state(pipeline, GST_STATE_NULL);
    gst_element_set_state(pipeline, GST_STATE_PLAYING);
}

void CameraWindow::on_drawing_area_realized() {
    Glib::signal_idle().connect(sigc::mem_fun(*this, &CameraWindow::set_video_overlay));
}

bool CameraWindow::set_video_overlay() {
    auto gdk_window = m_DrawingArea.get_window();
    if (!gdk_window) {
        std::cerr << "Drawing area window not available!" << std::endl;
        return false;
    }

#ifdef GDK_WINDOWING_X11
    if (GDK_IS_X11_WINDOW(gdk_window->gobj())) {
        auto xid = GDK_WINDOW_XID(gdk_window->gobj());
        gst_video_overlay_set_window_handle(GST_VIDEO_OVERLAY(sink), xid);
        std::cout << "Video overlay set to GTK DrawingArea." << std::endl;
    }
#endif

    return false;
}

int main(int argc, char *argv[]) {
    auto app = Gtk::Application::create(argc, argv, "org.gtkmm.example");

    CameraWindow window;
    return app->run(window);
}
