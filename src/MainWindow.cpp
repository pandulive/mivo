#include "MainWindow.h"
#include "KeyPad.h"


MainWindow::MainWindow(): m_VBox(Gtk::ORIENTATION_VERTICAL),
        m_ButtonBox(Gtk::ORIENTATION_HORIZONTAL) {
        
        set_title("Mivonix");
        set_default_size(1300, 800);
        // Layout: Main box
        add(m_VBox);

        m_DrawingArea.set_size_request(1280, 720);
        m_VBox.pack_start(m_DrawingArea, Gtk::PACK_EXPAND_WIDGET);

        
        // Buttons
        m_ButtonBox.set_spacing(10);
        add_button(m_Button1, "Start/Stop", 1);
        add_button(m_Button2, "Pause", 2);
        add_button(m_Button3, "Zoom +/-", 3);
        add_button(m_Button4, "AWB", 4);

        m_VBox.pack_start(m_ButtonBox, Gtk::PACK_SHRINK);

            // Initialize FTDI GPIO handler
            try {
                gpio_handler = new FT232HHandler([this](int button) {
                    Glib::signal_idle().connect_once([this, button]() {
                        handle_button_press(button);
                    });
                });
                gpio_handler->initialize();
                gpio_handler->start();
                
            } catch (const std::runtime_error& e) {
                std::cout<< "Error: " + std::string(e.what())<<std::endl;
            }
            // End of Keypad syncing

                // Connect button signals
            // m_Button1.signal_clicked().connect(sigc::mem_fun(*this, &MainWindow::on_play));
            // m_Button2.signal_clicked().connect(sigc::mem_fun(*this, &MainWindow::on_pause));
            // m_Button3.signal_clicked().connect(sigc::mem_fun(*this, &MainWindow::on_zoom));
            // m_Button4.signal_clicked().connect(sigc::mem_fun(*this, &MainWindow::on_awb));
            m_DrawingArea.signal_realize().connect(sigc::mem_fun(*this, &MainWindow::on_drawing_area_realized));

            // Start of Camera syncing using Gstreamer

    // Initialize GStreamer
    gst_init(nullptr, nullptr);

    // Create GStreamer elements
    pipeline = gst_pipeline_new("video-pipeline");
    source = gst_element_factory_make("v4l2src", "source");
    capsfilter = gst_element_factory_make("capsfilter", "capsfilter");
    // jpegdec = gst_element_factory_make("jpegdec", "jpegdec");
    crop = gst_element_factory_make("videocrop", "crop");
    convert = gst_element_factory_make("videoconvert", "convert");
    sink = gst_element_factory_make("glimagesink", "sink");

    // Below is for Sony usb
    if (!pipeline || !source || !capsfilter || !crop || !convert || !sink) {
        std::cerr << "Failed to create GStreamer elements." << std::endl;
        return;
    }

    // Below is for Sonymulti
    // if (!pipeline || !source || !capsfilter || !jpegdec || !crop || !convert || !sink) {
    //     std::cerr << "Failed to create GStreamer elements." << std::endl;
    //     return;
    // }

    // Set camera device
    g_object_set(source, "device", "/dev/video0", nullptr);
    // g_object_set(source, "buffer-size", 1048576, NULL);
    // g_object_set(source, "latency", 200, NULL);

        // Set capsfilter properties
    // GstCaps *caps = gst_caps_from_string("format=MJPEG,width=1280,height=720,framerate=40/1");
    // g_object_set(capsfilter, "caps", caps, NULL);
    // gst_caps_unref(caps); // Free the caps structure after setting it

    // Set default resolution to 1280x720 or 1920*1080
    change_resolution(1280, 720);

    // Add and link elements for SonyUSB
    gst_bin_add_many(GST_BIN(pipeline), source, capsfilter, crop, convert, sink, nullptr);
    if (!gst_element_link_many(source, capsfilter, crop, convert, sink, nullptr)) {
        std::cerr << "Failed to link GStreamer elements." << std::endl;
    }

    // Add and link elements for Sonymulti
    // gst_bin_add_many(GST_BIN(pipeline), source, capsfilter, jpegdec, crop, convert, sink, nullptr);
    // if (!gst_element_link_many(source, capsfilter, jpegdec, crop, convert, sink, nullptr)) {
    //     std::cerr << "Failed to link GStreamer elements." << std::endl;
    // }

    // Start with Video Play
    gst_element_set_state(pipeline, GST_STATE_PLAYING);
    std::cout << "Initialise with Streaming..." << std::endl;

    show_all_children();
    
}

MainWindow::~MainWindow() { 
    gpio_handler->reverse();

    if (pipeline) {
        gst_element_set_state(pipeline, GST_STATE_NULL);
        gst_object_unref(pipeline);
    }
}

void MainWindow::change_resolution(int width, int height) {
    GstCaps *caps = gst_caps_new_simple(
        "video/x-raw",
        "width", G_TYPE_INT, width,
        "height", G_TYPE_INT, height,
        // "framerate", GST_TYPE_FRACTION, 60, 1,
        NULL);

    if (caps) {
        g_object_set(capsfilter, "caps", caps, NULL);
        gst_caps_unref(caps);
        std::cout << "Resolution set to " << width << "x" << height << std::endl;
    } else {
        std::cerr << "Failed to set resolution." << std::endl;
    }
}


    void MainWindow::add_button(Gtk::Button& button, const Glib::ustring& label, int id) {
        button.set_label(label);
        button.signal_clicked().connect([this, id]() { handle_button_press(id); });
        m_ButtonBox.pack_start(button, Gtk::PACK_SHRINK);
        }

    void MainWindow::handle_button_press(int button) {
        std::cout<< "Button " + std::to_string(button) + " pressed!" << std::endl;
        
        if(button == 1){
            on_play();
        }
        if(button == 2){
        on_pause();
        }
        if(button == 3){
        on_zoom();
        }
        if(button == 4){
        on_awb();
        }
        
    }


void MainWindow::on_play() {
    gst_element_set_state(pipeline, GST_STATE_PLAYING);
    std::cout << "Pipeline playing..." << std::endl;
}

void MainWindow::on_pause() {
    gst_element_set_state(pipeline, GST_STATE_PAUSED);
    std::cout << "Pipeline paused." << std::endl;
}

void MainWindow::on_zoom() {
    // Increment zoom level
    zoom_level = (zoom_level + 1) % 4; // Cycle through 4 zoom levels (0-3)
    apply_zoom();
    std::cout << "Zoom level: " << zoom_level << std::endl;
}

    int set_v4l2_control(const char *device, int control_id, int value) {
    int fd = open(device, O_RDWR);
    if (fd < 0) {
        perror("Failed to open video device");
        return -1;
    }

    struct v4l2_control control;
    control.id = control_id;
    control.value = value;

    if (ioctl(fd, VIDIOC_S_CTRL, &control) < 0) {
        perror("Failed to set control");
        close(fd);
        return -1;
    }

    close(fd);
    return 0;
}


void MainWindow::on_awb() {
    // Toggle AWB and set white balance temperature to 4600K when AWB is disabled
    awb_enabled = !awb_enabled;
    gst_element_set_state(pipeline, GST_STATE_NULL);
    
    gst_element_set_state(pipeline, GST_STATE_PLAYING);
    const char *cam = "/dev/video0";
    if (awb_enabled) {
        set_v4l2_control(cam, V4L2_CID_AUTO_WHITE_BALANCE, 1); 
        std::cout << "AWB enabled." << std::endl;
        gst_element_set_state(pipeline, GST_STATE_NULL);
        const char *command = "ffmpeg -f v4l2 -i /dev/video0 -framerate 30 -vframes 1 output_image.jpg";
        
            std::cout << "Capturing Image" << std::endl;
            int ret = system(command);

            if (ret == 0) {
                printf("Image captured successfully and saved as 'captured_frame.jpg'.\n");
            } else {
                std::cout << "Failed to capture image." << std::endl;
            }
        
        std::cout << V4L2_CID_WHITE_BALANCE_TEMPERATURE << std::endl;
        
    } else {

        std::string imagePath = "output_image.jpg"; // Path to the image
        double temperature = awb_temperature(imagePath);

        if (temperature < 0) {
            std::cerr << "Failed to calculate color temperature." << std::endl;
            // return 1; // Exit with error code
        }

        std::cout << "Estimated Color Temperature: " << static_cast<int>(temperature) << "K" << std::endl;
   
        // double temperature = awb_temperature("output_image.jpg");
        set_v4l2_control(cam, V4L2_CID_AUTO_WHITE_BALANCE, 0);  // Disable auto white balance
        set_v4l2_control(cam, V4L2_CID_WHITE_BALANCE_TEMPERATURE, static_cast<int>(temperature));
        std::cout << "AWB disabled. White balance temperature set to " << temperature<< std::endl;
        system("rm -rf output_image.jpg");
    }
}


void MainWindow::apply_zoom() {
    // Crop values for each zoom level
    int crop_values[4][4] = {
        {0, 0, 0, 0},       // No crop (zoom level 0)
        {50, 50, 50, 50}, // Moderate zoom (zoom level 1)
        {100, 100, 100, 100}, // Higher zoom (zoom level 2)
        {150, 150, 150, 150}  // Maximum zoom (zoom level 3)
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


void MainWindow::on_drawing_area_realized() {
    Glib::signal_idle().connect(sigc::mem_fun(*this, &MainWindow::set_video_overlay));
}

bool MainWindow::set_video_overlay() {
    // Retrieve the GDK window for the drawing area
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

double MainWindow::awb_temperature(const std::string& imagePath) {
// Load the image
    cv::Mat image = cv::imread(imagePath);
    if (image.empty()) {
        std::cerr << "Failed to load image." << std::endl;
        return -1; // Return a negative value to indicate failure
    }

    // Convert to LAB color space
    cv::Mat labImage;
    cv::cvtColor(image, labImage, cv::COLOR_BGR2Lab);

    // Split LAB image into channels
    std::vector<cv::Mat> labChannels(3);
    cv::split(labImage, labChannels);
    cv::Mat a = labChannels[1];
    cv::Mat b = labChannels[2];

    // Calculate the average 'a' and 'b' channel values
    double avgA = cv::mean(a)[0];
    double avgB = cv::mean(b)[0];

    // Approximate color temperature using a heuristic formula
    double colorTemperature = 5000 + (avgA - avgB) * 100;
    colorTemperature = std::max(1000.0, std::min(colorTemperature, 10000.0)); // Clamp between 1000K and 10000K

    return colorTemperature;

}
