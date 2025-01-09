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
    GstElement *tee = gst_element_factory_make("tee", "tee");
    GstElement *queue_display = gst_element_factory_make("queue", "queue_display");
    GstElement *sink_display = gst_element_factory_make("glimagesink", "sink_display");
    GstElement *queue_capture = gst_element_factory_make("queue", "queue_capture");
    GstElement *convert_capture = gst_element_factory_make("videoconvert", "convert_capture");

    // Encoding elements
    GstElement *encoder = gst_element_factory_make("x264enc", "encoder");
    GstElement *muxer = gst_element_factory_make("mp4mux", "muxer");
    GstElement *filesink = gst_element_factory_make("filesink", "filesink");

    if (!pipeline || !source || !capsfilter || !tee || !queue_display || !sink_display || !queue_capture ||
        !convert_capture || !encoder || !muxer || !filesink) {
        std::cerr << "Failed to create GStreamer elements." << std::endl;
        return -1;
    }

    // Set properties for the source (camera)
    g_object_set(source, "device", "/dev/video0", NULL); // Set your camera device

    // Set capsfilter properties (set video resolution and format)
    GstCaps *caps = gst_caps_from_string("video/x-raw,width=1280,height=720");
    g_object_set(capsfilter, "caps", caps, NULL);
    gst_caps_unref(caps);

    // Set properties for filesink (saving the video to output.mp4)
    g_object_set(filesink, "location", "output.mp4", NULL); // Save video to "output.mp4"

    // Set properties for encoder (x264enc)
    g_object_set(encoder, "tune", 4, "bitrate", 1000, NULL);  // Tune for quality and set bitrate (adjust as needed)

    // Set properties for glimagesink
    g_object_set(sink_display, "sync", TRUE, NULL);

    // Build the pipeline
    gst_bin_add_many(GST_BIN(pipeline), source, capsfilter, tee, queue_display, sink_display, queue_capture,
                     convert_capture, encoder, muxer, filesink, NULL);

    // Link elements for display branch (showing video in a window using glimagesink)
    if (!gst_element_link_many(source, capsfilter, tee, queue_display, sink_display, NULL)) {
        std::cerr << "Failed to link display elements." << std::endl;
        gst_object_unref(pipeline);
        return -1;
    }

    // Link elements for the capture and save branch (recording video to a file using filesink)
    if (!gst_element_link_many(tee, queue_capture, convert_capture, encoder, muxer, filesink, NULL)) {
        std::cerr << "Failed to link capture elements." << std::endl;
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

    std::cout << "Streaming video and recording to output.mp4..." << std::endl;

    // Wait for 10 seconds while recording video
    std::this_thread::sleep_for(std::chrono::seconds(10));

    // Stop the pipeline and allow it to finalize the recording
    gst_element_send_event(pipeline, gst_event_new_eos());  // Send EOS (End of Stream) event to flush buffers
    std::this_thread::sleep_for(std::chrono::seconds(2));    // Wait for finalization

    // Stop the pipeline
    gst_element_set_state(pipeline, GST_STATE_NULL);
    gst_object_unref(pipeline);

    std::cout << "Recording complete." << std::endl;

    return 0;
}
