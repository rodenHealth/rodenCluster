#include <stdio.h>
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <iostream>
#include <stdlib.h>
#include <string.h>

using namespace cv;

void splitVideo(String filename, String outputDirectory, int precision)
{
    printf("* Filename: %s\n", filename.c_str());

    CvCapture *capture = cvCaptureFromAVI(filename.c_str());
    if (!capture)
    {
        // TODO: Good error handling framework
        printf("Error reading file\n");
        return;
    }

    // Calculations
    int frameNumbers = (int)cvGetCaptureProperty(capture, CV_CAP_PROP_FRAME_COUNT);
    int fps = (int)cvGetCaptureProperty(capture, CV_CAP_PROP_FPS);
    int frameSkip = (fps / precision);
    int videoLength = (frameNumbers / fps) * precision;

    // FPS
    printf("* FPS: %d\n", fps);

    // Setup for read
    IplImage *frame = NULL;
    int frame_number = 1;

    for (int n = 0; n < videoLength; n++)
    {
        // Read frame
        frame = cvQueryFrame(capture);
        if (!frame)
        {
            // TODO: Handle this
            printf("Error, no frame\n");
            break;
        }

        // Build filename
        char filename[100];
        sprintf(filename, "%s", outputDirectory.c_str());

        if (filename[strlen(outputDirectory.c_str())] != '\\')
        {
            filename[strlen(outputDirectory.c_str()) + 1] = '\\';
        }

        strcat(filename, "frame_");

        // Frame Id
        char frame_id[30];
        sprintf(frame_id, "%d", frame_number);
        strcat(filename, frame_id);
        strcat(filename, ".jpg");

        // Debug print
        printf("* Saving: %s\n", filename);

        // Save frame
        if (!cvSaveImage(filename, frame))
        {
            // TODO: Handle
            printf("Error saving image\n");
            break;
        }

        // For the file name
        frame_number++;

        // Read N files
        for (int i = 0; i < frameSkip; i++)
        {
            // get frame
            frame = cvQueryFrame(capture);
            if (!frame)
            {
                printf("!!! cvQueryFrame failed: no frame\n");
                break;
            }
        }
    }

    // free resources
    cvReleaseCapture(&capture);
}

int main(int argc, char **argv)
{
    // Sample usage
    splitVideo("expressions.mp4", "./tmp/", 2);

    return 0;
}