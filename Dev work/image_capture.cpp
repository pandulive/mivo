#include <gst/gst.h>
#include <iostream>
#include <thread>
#include <chrono>

int main(int argc, char *argv[]) {
    gst_init(&argc, &argv);

    // Create GStreamer elements
    GstElement *pipeline = gst_pipeline_new("video-pipeline");
    GstElement *source = gst_element_factory_make("v4l2src", "source");
    GstElement *capsfilter = gst_element_factory_make("capsfilter", "capsfilter");
    GstElement *jpegenc = gst_element_factory_make("jpegenc", "jpegenc");
    GstElement *filesink = gst_element_factory_make("filesink", "filesink");

    if (!pipeline || !source || !capsfilter || !jpegenc || !filesink) {
        std::cerr << "Failed to create GStreamer elements." << std::endl;
        return -1;
    }

    // Set properties for the source
    g_object_set(source, "device", "/dev/video0", NULL); // Set your camera device

    // Set capsfilter properties
    GstCaps *caps = gst_caps_from_string("video/x-raw,width=1280,height=720");
    g_object_set(capsfilter, "caps", caps, NULL);
    gst_caps_unref(caps);

    // Set properties for filesink
    g_object_set(filesink, "location", "captured_frame.jpg", NULL);

    // Build the pipeline
    gst_bin_add_many(GST_BIN(pipeline), source, capsfilter, jpegenc, filesink, NULL);

    // Link elements
    if (!gst_element_link_many(source, capsfilter, jpegenc, filesink, NULL)) {
        std::cerr << "Failed to link elements." << std::endl;
        gst_object_unref(pipeline);
        return -1;
    }

    // Start the pipeline
    GstStateChangeReturn ret = gst_element_set_state(pipeline, GST_STATE_PLAYING);
    if (ret == GST_STATE_CHANGE_FAILURE) {
        std::cerr << "Failed to set pipeline to PLAYING state." << std::endl;
        gst_object_unref(pipeline);
        return -1;
    }

    std::cout << "Streaming video... Capturing frame after 5 seconds." << std::endl;

    // Wait for 5 seconds to ensure the pipeline is running
    std::this_thread::sleep_for(std::chrono::seconds(5));

    // Stop the pipeline after capturing the frame
    gst_element_set_state(pipeline, GST_STATE_NULL);
    gst_object_unref(pipeline);

    std::cout << "Frame captured and saved to captured_frame.jpg." << std::endl;

    return 0;
}
