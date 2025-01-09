#include <gst/gst.h>
#include <iostream>

int main(int argc, char *argv[]) {
    gst_init(&argc, &argv);  // Initialize GStreamer

    // Create GStreamer elements
    GstElement *pipeline = gst_pipeline_new("video-pipeline");
    GstElement *source = gst_element_factory_make("v4l2src", "source");
    GstElement *capsfilter = gst_element_factory_make("capsfilter", "capsfilter");
    GstElement *jpegdec = gst_element_factory_make("jpegdec", "jpegdec");
    GstElement *videoconvert = gst_element_factory_make("videoconvert", "videoconvert");
    GstElement *sink = gst_element_factory_make("autovideosink", "sink");

    // Check if all elements were created successfully
    if (!pipeline || !source || !capsfilter || !jpegdec || !videoconvert || !sink) {
        std::cerr << "Failed to create GStreamer elements." << std::endl;
        return -1;
    }

    // Set properties for the source (camera)
    g_object_set(source, "device", "/dev/video0", NULL);  // Adjust the device path if needed

    // Set capsfilter properties for MJPEG format and resolution
    GstCaps *caps = gst_caps_from_string("format=MJPEG,width=1920,height=1080");
    g_object_set(capsfilter, "caps", caps, NULL);
    gst_caps_unref(caps);

    // Build the pipeline
    gst_bin_add_many(GST_BIN(pipeline), source, capsfilter, jpegdec, videoconvert, sink, NULL);

    // Link elements together
    if (!gst_element_link_many(source, capsfilter, jpegdec, videoconvert, sink, NULL)) {
        std::cerr << "Failed to link GStreamer elements." << std::endl;
        gst_object_unref(pipeline);
        return -1;
    }

    // Set pipeline to PLAYING state
    GstStateChangeReturn ret = gst_element_set_state(pipeline, GST_STATE_PLAYING);
    if (ret == GST_STATE_CHANGE_FAILURE) {
        std::cerr << "Failed to set pipeline to PLAYING state." << std::endl;
        gst_object_unref(pipeline);
        return -1;
    }

    // Wait for the pipeline to finish (or for user interruption)
    std::cout << "Streaming video..." << std::endl;
    gst_bus_poll(gst_pipeline_get_bus(GST_PIPELINE(pipeline)), GST_MESSAGE_EOS, -1);

    // Clean up and stop the pipeline
    gst_element_set_state(pipeline, GST_STATE_NULL);
    gst_object_unref(pipeline);

    return 0;
}
