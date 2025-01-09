#include <gtkmm.h>
#include <gst/gst.h>
#include <gst/app/app.h>
#include <gst/app/gstappsink.h>
#include <opencv2/opencv.hpp>
#include <iostream>
#include <thread>
#include <chrono>

class VideoCaptureApp : public Gtk::Window {
public:
    VideoCaptureApp() {
        gst_init(nullptr, nullptr);

        set_title("Video Capture App");
        set_default_size(800, 600);

        // Create buttons
        button_record.set_label("Start Video Processing");
        button_record.signal_clicked().connect(sigc::mem_fun(*this, &VideoCaptureApp::on_record_video));

        // Layout
        vbox.pack_start(button_record);
        add(vbox);

        show_all_children();
    }

    virtual ~VideoCaptureApp() {
        gst_deinit();
    }

protected:
    void on_record_video() {
        process_video();
    }

private:
    Gtk::Box vbox{Gtk::ORIENTATION_VERTICAL, 10};
    Gtk::Button button_record;

    void process_video() {
        // GStreamer elements for capturing video
        GstElement *pipeline = gst_pipeline_new("video-pipeline");
        GstElement *source = gst_element_factory_make("v4l2src", "source");
        GstElement *capsfilter = gst_element_factory_make("capsfilter", "capsfilter");
        GstElement *tee = gst_element_factory_make("tee", "tee");
        GstElement *queue_display = gst_element_factory_make("queue", "queue_display");
        GstElement *sink_display = gst_element_factory_make("glimagesink", "sink_display");
        GstElement *queue_process = gst_element_factory_make("queue", "queue_process");
        GstElement *convert_process = gst_element_factory_make("videoconvert", "convert_process");
        GstElement *appsink = gst_element_factory_make("appsink", "appsink");

        if (!pipeline || !source || !capsfilter || !tee || !queue_display || !sink_display ||
            !queue_process || !convert_process || !appsink) {
            std::cerr << "Failed to create GStreamer elements." << std::endl;
            return;
        }

        g_object_set(source, "device", "/dev/video0", NULL);  // Set camera device
        GstCaps *caps = gst_caps_from_string("video/x-raw,width=1280,height=720");
        g_object_set(capsfilter, "caps", caps, NULL);
        gst_caps_unref(caps);

        // Set properties for appsink (for OpenCV frame capture)
        g_object_set(appsink, "emit-signals", TRUE, "sync", FALSE, NULL);
        g_signal_connect(appsink, "new-sample", G_CALLBACK(on_new_sample), this);

        // Build the pipeline
        gst_bin_add_many(GST_BIN(pipeline), source, capsfilter, tee, queue_display, sink_display,
                         queue_process, convert_process, appsink, NULL);

        // Link elements for display branch (using glimagesink)
        if (!gst_element_link_many(source, capsfilter, tee, queue_display, sink_display, NULL)) {
            std::cerr << "Failed to link display elements." << std::endl;
            gst_object_unref(pipeline);
            return;
        }

        // Link elements for OpenCV processing branch
        if (!gst_element_link_many(tee, queue_process, convert_process, appsink, NULL)) {
            std::cerr << "Failed to link processing elements." << std::endl;
            gst_object_unref(pipeline);
            return;
        }

        // Start the pipeline
        gst_element_set_state(pipeline, GST_STATE_PLAYING);

        std::cout << "Processing video and displaying using glimagesink..." << std::endl;

        // Run the GTK main loop
        Gtk::Main::run(*this);

        // Stop the pipeline
        gst_element_set_state(pipeline, GST_STATE_NULL);
        gst_object_unref(pipeline);
    }

    static GstFlowReturn on_new_sample(GstElement *sink, VideoCaptureApp *app) {
        GstSample *sample;
        GstBuffer *buffer;
        GstCaps *caps;
        GstStructure *str;

        // Get the sample from appsink
        sample = gst_app_sink_pull_sample(GST_APP_SINK(sink));
        if (!sample) {
            return GST_FLOW_ERROR;
        }

        buffer = gst_sample_get_buffer(sample);
        caps = gst_sample_get_caps(sample);
        str = gst_caps_get_structure(caps, 0);

        // Convert buffer to OpenCV Mat
        GstMapInfo info;
        if (gst_buffer_map(buffer, &info, GST_MAP_READ)) {
            // Get the raw data from buffer and create OpenCV Mat
            cv::Mat frame(cv::Size(1280, 720), CV_8UC3, (char *)info.data);

            // Process the frame with OpenCV (e.g., draw a tree or do other processing)
            cv::Mat processed_frame;
            cv::cvtColor(frame, processed_frame, cv::COLOR_BGR2GRAY);  // Example: Convert to grayscale

            // Show the frame using OpenCV (just for demonstration)
            cv::imshow("Processed Frame", processed_frame);
            cv::waitKey(1);  // Display the frame for 1 ms

            gst_buffer_unmap(buffer, &info);
        }

        gst_sample_unref(sample);
        return GST_FLOW_OK;
    }
};

int main(int argc, char *argv[]) {
    Gtk::Main kit(argc, argv);
    VideoCaptureApp app;
    Gtk::Main::run(app);
    return 0;
}
