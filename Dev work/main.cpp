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
    Gtk::ScrolledWindow m_ScrolledWindow;
    Gtk::DrawingArea m_DrawingArea;
    Gtk::Box m_ButtonBox;  // To contain buttons below the video
    Gtk::Button m_PlayButton;
    Gtk::Button m_PauseButton;
    Gtk::Button m_StopButton;
    Gtk::Button m_ZoomButton;
    Gtk::Button m_AwbButton;
    
    // Zoom work
    std::vector<std::pair<int, int>> zoom_levels; // Store width and height for zoom levels
    size_t current_zoom_index; // Index to track current zoom level

    GstElement *pipeline = nullptr;
    GstElement *source = nullptr;
    GstElement *convert = nullptr;
    GstElement *capsfilter = nullptr;
    GstElement *sink = nullptr;

    bool set_video_overlay();
    void on_mapped();
    void change_resolution(int width, int height);
    
    void on_play();
    void on_pause();
    void on_stop();
    void on_zoom();
    void on_awb();
};

CameraWindow::CameraWindow() : m_VBox(Gtk::ORIENTATION_VERTICAL), 
    m_PlayButton("Play"), m_PauseButton("Pause"), m_StopButton("Stop"), m_ZoomButton("Zoom +/-"), m_AwbButton("AWB"), current_zoom_index(0) {

    set_title("USB Camera with GTKmm & GStreamer");
    set_default_size(1280, 720);

    add(m_VBox);

    // Add the ScrolledWindow with DrawingArea to the VBox
    m_ScrolledWindow.set_policy(Gtk::POLICY_AUTOMATIC, Gtk::POLICY_AUTOMATIC);
    m_ScrolledWindow.add(m_DrawingArea);
    m_VBox.pack_start(m_ScrolledWindow, Gtk::PACK_EXPAND_WIDGET);

    // Create a Box for the buttons and add to the VBox
    m_ButtonBox.set_orientation(Gtk::ORIENTATION_HORIZONTAL);
    m_ButtonBox.pack_start(m_PlayButton, Gtk::PACK_SHRINK);
    m_ButtonBox.pack_start(m_PauseButton, Gtk::PACK_SHRINK);
    m_ButtonBox.pack_start(m_StopButton, Gtk::PACK_SHRINK);
    m_ButtonBox.pack_start(m_ZoomButton, Gtk::PACK_SHRINK);
    m_ButtonBox.pack_start(m_AwbButton, Gtk::PACK_SHRINK);

    m_VBox.pack_start(m_ButtonBox, Gtk::PACK_SHRINK);

    // Define zoom levels
    zoom_levels = {
        {1280, 720},  // Default zoom
        {640, 480},  // Zoom+
        {1920, 1080},   // Zoom++
        {1280, 720}   // Back to default
    };

    // Initialize GStreamer
    gst_init(nullptr, nullptr);

    // Create GStreamer elements
    pipeline = gst_pipeline_new("usb-camera-pipeline");
    source = gst_element_factory_make("v4l2src", "source");
    capsfilter = gst_element_factory_make("capsfilter", "capsfilter");
    convert = gst_element_factory_make("videoconvert", "convert");
    sink = gst_element_factory_make("glimagesink", "sink"); // Use gtksink for GTK integration

    if (!pipeline || !source || !convert || !capsfilter || !sink) {
        std::cerr << "Failed to create GStreamer elements." << std::endl;
        return;
    }

    // Set the USB camera device (adjust /dev/video0 if needed)
    g_object_set(source, "device", "/dev/video0", NULL);

    // Set default resolution to 1920x1080
    change_resolution(1280, 720);

    // Build the pipeline
    gst_bin_add_many(GST_BIN(pipeline), source, capsfilter, convert, sink, NULL);
    gst_element_link(source, capsfilter);
    gst_element_link(capsfilter, convert);
    gst_element_link(convert, sink);

    // Connect signals
    m_DrawingArea.signal_map().connect(sigc::mem_fun(*this, &CameraWindow::on_mapped));
    show_all_children();

    // Connect button signals
    m_PlayButton.signal_clicked().connect(sigc::mem_fun(*this, &CameraWindow::on_play));
    m_PauseButton.signal_clicked().connect(sigc::mem_fun(*this, &CameraWindow::on_pause));
    m_StopButton.signal_clicked().connect(sigc::mem_fun(*this, &CameraWindow::on_stop));
    m_ZoomButton.signal_clicked().connect(sigc::mem_fun(*this, &CameraWindow::on_zoom));
    m_AwbButton.signal_clicked().connect(sigc::mem_fun(*this, &CameraWindow::on_awb));
}

CameraWindow::~CameraWindow() {
    if (pipeline) {
        gst_element_set_state(pipeline, GST_STATE_NULL);
        gst_object_unref(pipeline);
    }
}

void CameraWindow::change_resolution(int width, int height) {
    GstCaps *caps = gst_caps_new_simple(
        "video/x-raw",
        "width", G_TYPE_INT, width,
        "height", G_TYPE_INT, height,
        NULL);

    if (caps) {
        g_object_set(capsfilter, "caps", caps, NULL);
        gst_caps_unref(caps);
        std::cout << "Resolution set to " << width << "x" << height << std::endl;
    } else {
        std::cerr << "Failed to set resolution." << std::endl;
    }
}

bool CameraWindow::set_video_overlay() {
    // Retrieve the GDK window for the drawing area
    auto gdk_window = m_DrawingArea.get_window();
    if (!gdk_window) {
        std::cerr << "Drawing area window is not available!" << std::endl;
        return false;
    }

#ifdef GDK_WINDOWING_X11
    // Ensure we have an X11 window for video overlay
    if (GDK_IS_X11_WINDOW(gdk_window->gobj())) {
        auto xid = GDK_WINDOW_XID(gdk_window->gobj());
        // Set the XID for the video overlay on the gtksink
        gst_video_overlay_set_window_handle(GST_VIDEO_OVERLAY(sink), xid);
    }
#endif

    // Start playing the pipeline
    gst_element_set_state(pipeline, GST_STATE_PLAYING);
    return false;
}

void CameraWindow::on_mapped() {
    // Make sure the video overlay is set after the window is mapped
    Glib::signal_idle().connect(sigc::mem_fun(*this, &CameraWindow::set_video_overlay));
}

void CameraWindow::on_play() {
    gst_element_set_state(pipeline, GST_STATE_PLAYING);
    std::cout << "Play button clicked" << std::endl;
}

void CameraWindow::on_pause() {
    gst_element_set_state(pipeline, GST_STATE_PAUSED);
    std::cout << "Pause button clicked" << std::endl;
}

void CameraWindow::on_stop() {
    gst_element_set_state(pipeline, GST_STATE_NULL);
    std::cout << "Stop button clicked" << std::endl;
}
void CameraWindow::on_zoom() {
    // Cycle to the next zoom level
    current_zoom_index = (current_zoom_index + 1) % zoom_levels.size();

    // Retrieve the new resolution
    auto [width, height] = zoom_levels[current_zoom_index];

    // Update the resolution
    change_resolution(width, height);

    // Restart the pipeline for the new resolution to take effect
    gst_element_set_state(pipeline, GST_STATE_NULL);
    gst_element_set_state(pipeline, GST_STATE_PLAYING);

    std::cout << "Zoom changed to " << width << "x" << height << std::endl;
}
void CameraWindow::on_awb() {
    gst_element_set_state(pipeline, GST_STATE_NULL);
    std::cout << "Awb button clicked" << std::endl;
}

int main(int argc, char *argv[]) {
    auto app = Gtk::Application::create(argc, argv, "org.gtkmm.example");

    CameraWindow window;
    return app->run(window);
}