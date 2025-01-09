#include <gtk/gtk.h>
#include <gst/gst.h>
#include <gst/app/gstappsink.h>
#include <opencv2/opencv.hpp>
#include <iostream>

static GstElement *pipeline, *appsink;
static GtkWidget *window, *video_area, *capture_button;

// Callback function to capture the image from the video stream
static GstFlowReturn new_sample(GstAppSink *appsink, gpointer user_data) {
    GstSample *sample = gst_app_sink_pull_sample(appsink);
    if (!sample) {
        return GST_FLOW_ERROR;
    }

    GstBuffer *buffer = gst_sample_get_buffer(sample);
    GstMapInfo map;
    gst_buffer_map(buffer, &map, GST_MAP_READ);

    // Decode the buffer into OpenCV Mat format
    cv::Mat img = cv::imdecode(cv::Mat(1, map.size, CV_8UC1, map.data), cv::IMREAD_COLOR);

    // Save the captured image
    if (!img.empty()) {
        cv::imwrite("captured_image.jpg", img);
        std::cout << "Captured image saved as captured_image.jpg" << std::endl;
    }

    gst_buffer_unmap(buffer, &map);
    gst_sample_unref(sample);

    return GST_FLOW_OK;
}

// Button click handler to capture image
static void on_capture_button_clicked(GtkButton *button, gpointer user_data) {
    // Trigger image capture when the button is clicked
    std::cout << "Capture button clicked!" << std::endl;
    new_sample(appsink, nullptr);  // Capture a frame from appsink
}

// Initialize the GStreamer pipeline for video streaming
static void create_pipeline() {
    // Set up GStreamer pipeline
    pipeline = gst_parse_launch("v4l2src ! videoconvert ! appsink name=sink", nullptr);

    // Get the appsink element for pulling the frames
    appsink = gst_bin_get_by_name(GST_BIN(pipeline), "sink");
    g_signal_connect(appsink, "new-sample", G_CALLBACK(new_sample), nullptr);

    gst_element_set_state(pipeline, GST_STATE_PLAYING);
}

// This function is called when the GTK window's video area is realized
static void on_realize(GtkWidget *widget, gpointer data) {
    create_pipeline();
}

int main(int argc, char *argv[]) {
    gst_init(&argc, &argv);  // Initialize GStreamer
    gtk_init(&argc, &argv);  // Initialize GTK

    // Create GTK Window
    window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(window), "GStreamer Camera Stream");
    gtk_window_set_default_size(GTK_WINDOW(window), 640, 480);
    g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);

    // Create video area (for the camera stream)
    video_area = gtk_drawing_area_new();
    gtk_container_add(GTK_CONTAINER(window), video_area);
    g_signal_connect(video_area, "realize", G_CALLBACK(on_realize), NULL);

    // Create Capture Image button
    capture_button = gtk_button_new_with_label("Capture Image");
    g_signal_connect(capture_button, "clicked", G_CALLBACK(on_capture_button_clicked), NULL);

    // Create a vertical box to layout the video and button
    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_box_pack_start(GTK_BOX(box), video_area, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(box), capture_button, FALSE, FALSE, 0);

    // Add the box to the window
    gtk_container_add(GTK_CONTAINER(window), box);

    // Show all elements
    gtk_widget_show_all(window);

    // Start the GTK main loop
    gtk_main();

    return 0;
}
