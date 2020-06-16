#include <iostream>
#include <vector>
#include <string>
#include "XExporter.h"
#include "XFileProducer.h"
#include "XFFProducer.h"
#include "XTimeCounter.h"

void testExport() {
    std::string outPath = "/Users/andy/export.mp4";
    int width = 720;
    int height = 1280;
    int fps = 25;
    long duration = 10 * 1000;
    auto exporter = std::make_unique<XExporter>(outPath, width, height, fps, duration);
    exporter->setExportResultCallback([](ExportResult result) {
        switch (result) {
            case EXPORT_RESULT_SUCCEED: {
                std::cout << "合成结果: EXPORT_RESULT_SUCCEED" << std::endl;
            }
                break;
            case EXPORT_RESULT_CANCELED: {
                std::cout << "合成结果: EXPORT_RESULT_CANCELED" << std::endl;
            }
                break;
            case EXPORT_RESULT_FAILED: {
                std::cout << "合成结果: EXPORT_RESULT_FAILED" << std::endl;
            }
                break;
            default:
                break;
        }
    });
    exporter->setAudioDisable(true);
    exporter->start();

    auto fileProducer = std::make_unique<XFileProducer>();
    try {
        fileProducer->setInput("/Users/andy/Movies/jieqian_720x1280.rgba");
    } catch (std::exception& e) {
        std::cout << e.what() << std::endl;
        exit(0);
    }

    long delay = static_cast<long>(1000.0 / fps);
    int encodeCount = 0;
    for (long clock = 0; clock < duration; clock += delay) {
        auto image = fileProducer->getImage(clock);
        exporter->encodeFrame(image->pixels, image->width, image->height);
        encodeCount++;
    }
    exporter->stop();
    exporter->debug();

    std::cout << "delay: " << delay
              << "\nencodeCount: " << encodeCount
              << "\noutpath: " << outPath
              << std::endl;

    fileProducer.reset();
    exporter.reset();
}

void testProducerOpen() {
    std::vector<std::string> filenames = {
        "/Users/andy/Movies/1553566650589.mp4",
        "/Users/andy/Movies/2.5D水墨Pre.mp4",
        "/Users/andy/Movies/8k.mp4",
        "/Users/andy/Movies/2vuy.mov",
        "/Users/andy/Movies/720.mp4",
        "/Users/andy/Movies/MMVideo_1006741768.mp4",
        "/Users/andy/Movies/_duanshipin.mp4",
        "/Users/andy/Movies/_output.mp4",
        "/Users/andy/Movies/backgroundVideo.mp4",
        "/Users/andy/Movies/douyin_1.mp4",
        "/Users/andy/Movies/douyin_700x1240.mp4",
        "/Users/andy/Movies/hepingjingying.mp4",
        "/Users/andy/Movies/jieqian_4s_720x1280.mp4",
        "/Users/andy/Movies/jieqian_720x1280.mp4",
        "/Users/andy/Movies/lianche.mp4",
        "/Users/andy/Movies/shangxianping_5e267085d8593_1579577477.mp4",
        "/Users/andy/Movies/suoping.mp4",
        "/Users/andy/Movies/yangzhiganlu.mp4",
        "/Users/andy/Movies/zayin.mp4",
        "/Users/andy/Movies/背景视频.mp4",
        "/Users/andy/Movies/预览视频.mp4"
    };
    
    std::vector<std::shared_ptr<XFFProducer>> producerList;

    int size = filenames.size();
    XTimeCounter openCounter;
    int index = 0;
    for (int i = 0; i < size; ++i) {
        openCounter.markStart();
        index = i;
        auto producer = std::make_shared<XFFProducer>();
        try {
            producer->setInput(filenames.at(index));
            producerList.emplace_back(producer);
            producer->start();
        } catch (std::exception& e) {
            std::cout << "[Application] set input failed: " << e.what() << std::endl;
        }
        openCounter.markEnd();
        std::cout << "[Application] [" << openCounter.getRunDuration() << " ms]: " << filenames.at(index) << std::endl;
    }
}

void testProducerReadPacket() {
    auto producer = std::make_shared<XFFProducer>();
    try {
        producer->setInput("/Users/andy/Movies/jieqian_720x1280.mp4");
        producer->start();
    } catch (std::exception& e) {
        std::cout << "[Application] set input failed: " << e.what() << std::endl;
    }
    getchar();
}

int main() {
    testProducerReadPacket();
    return 0;
}
