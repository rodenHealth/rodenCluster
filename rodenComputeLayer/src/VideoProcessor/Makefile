CXX = g++
CXXFLAGS = -O2 -g -Wall -fmessage-length=0 
CPPFLAGS = -I/usr/local/Cellar/opencv/3.3.1/include

OBJS = VideoProcessor.o

LDFLAGS = -L/usr/local/Cellar/opencv/3.3.1/lib
LDLIBS =  -lopencv_core -lopencv_imgproc -lopencv_calib3d -lopencv_video \
	-lopencv_features2d -lopencv_ml -lopencv_highgui -lopencv_objdetect -lopencv_videoio -lopencv_imgcodecs

TARGET =  VideoProcessor

.PHONY: all
all: $(TARGET)
$(TARGET):  $(OBJS)
	$(CXX) $(LDFLAGS) $^ $(LDLIBS) -o $@

.PHONY: clean
clean:
	rm -f $(OBJS) $(TARGET) frame_* *.jpg tmp/*.jpg